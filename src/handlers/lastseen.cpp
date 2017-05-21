#include "lastseen.h"

#include <glog/logging.h>

#include <ctime>

#include "util/stringops.h"

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{
	_lastSeenDB.init("lastseen", bot ? bot->GetDBPathPrefix() : "testdb/");
	_nick2jidDB.init("nick2jid", bot ? bot->GetDBPathPrefix() : "testdb/");
	_lastActiveDB.init("lastmsg", bot ? bot->GetDBPathPrefix() : "testdb/");
}

LemonHandler::ProcessingResult LastSeen::HandleMessage(const ChatMessage &msg)
{
	if (_lastActiveDB.isOK())
	{
		auto now = std::chrono::system_clock::now();
		_lastActiveDB.Set(msg._jid, std::to_string(std::chrono::system_clock::to_time_t(now)) + " " + msg._body);
	}

	if (msg._body == "!seenstat")
	{
		SendMessage(GetStats());
		return ProcessingResult::KeepGoing;
	}

	std::string wantedUser;

	if (!getCommandArguments(msg._body, "!seen", wantedUser))
		return ProcessingResult::KeepGoing;

	if (!_nick2jidDB.isOK() || !_lastSeenDB.isOK() || !_lastActiveDB.isOK())
	{
		SendMessage("Database connection error");
		return ProcessingResult::KeepGoing;
	}

	SendMessage(GetUserInfo(wantedUser));
	return ProcessingResult::KeepGoing;
}

void LastSeen::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	auto now = std::chrono::system_clock::now();

	if (_nick2jidDB.isOK())
		_nick2jidDB.Set(from, jid);

	if (_lastSeenDB.isOK())
		_lastSeenDB.Set(jid, std::to_string(std::chrono::system_clock::to_time_t(now)));
}

const std::string LastSeen::GetHelp() const
{
	return "Use !seen %nickname% or !seen %jid%; use !seen %regex% or !seenjid %regex% to search users by regex\n"
		   "!seenstat - show statistics";
}

std::string LastSeen::GetStats() const
{
	if (!_nick2jidDB.isOK() || !_lastSeenDB.isOK() || !_lastActiveDB.isOK())
		return "Database connection error";

	return "Seen nicks: " + std::to_string(_nick2jidDB.Size()) + " | Seen users: " + std::to_string(_lastSeenDB.Size());
}

std::string LastSeen::GetUserInfo(const std::string &wantedUser) const
{
	auto lastStatus = GetLastStatus(wantedUser);
	if (!lastStatus._error.empty())
		return lastStatus._error;

	std::string result;
	auto currentNick = _botPtr->GetNickByJid(lastStatus.jid);
	if (!currentNick.empty())
	{
		result = wantedUser != currentNick
				? wantedUser + " (" + lastStatus.jid + ") is still here as " + currentNick
				: wantedUser + " is still here";
	} else {
		result = wantedUser + " (" + lastStatus.jid + ") last seen " + CustomTimeFormat(lastStatus.when) + " ago";
	}

	auto lastActivity = GetLastActive(lastStatus.jid);
	if (lastActivity._valid)
	{
		result.append("; last active " + CustomTimeFormat(lastActivity.when) + " ago");
		if (!lastActivity.what.empty())
			result.append(": \"" + lastActivity.what + "\"");
	}

	return result;
}

LastSeen::LastStatus LastSeen::GetLastStatus(const std::string &name) const
{
	LastStatus result(name);

	if (!_lastSeenDB.isOK() || !_nick2jidDB.isOK())
	{
		result._error = "Database connection error";
		return result;
	}

	std::string lastSeenRecord = "0";

	if (!_lastSeenDB.Get(name, lastSeenRecord))
	{
		if (!_nick2jidDB.Get(name, result.jid))
		{
			auto similarUsers = _nick2jidDB.Find(name, LevelDBPersistentMap::FindOptions::All);
			if (similarUsers.empty())
				result._error = name + "? Who's that?";
			else if (similarUsers.size() > maxSearchResults)
				result._error = "Too many matches";
			else
			{
				result._error = "Similar users:";
				for (const auto &user : similarUsers)
					result._error += " " + user.first + " (" + user.second + ")";
			}

			return result;
		}

		if (!_lastSeenDB.Get(result.jid, lastSeenRecord))
			result._error = "Well this is weird, " + name + " resolved to " + result.jid + " but I have no record for this jid";
	}

	try {
		auto now = std::chrono::system_clock::now();
		auto lastSeenTime = std::chrono::system_clock::from_time_t(std::stol(lastSeenRecord));
		result.when = now - lastSeenTime;
	} catch (std::exception &e) {
		result._error = "Something broke: " + std::string(e.what());
	}

	return result;
}

LastSeen::LastActivity LastSeen::GetLastActive(const std::string &jid) const
{
	LastSeen::LastActivity result;
	std::string lastActiveRecord = "";
	if (!_lastActiveDB.Get(jid, lastActiveRecord))
		return result;

	auto data = tokenize(lastActiveRecord, ' ', 2);
	lastActiveRecord = data.at(0);

	if (data.size() > 1)
		result.what = data.at(1);

	try {
		auto now = std::chrono::system_clock::now();
		auto lastActiveTime = std::chrono::system_clock::from_time_t(std::stol(lastActiveRecord));
		result.when = now - lastActiveTime;
		result._valid = true;
	} catch (std::exception &e) {
		LOG(WARNING) << "Something broke: " + std::string(e.what());
		return result;
	}

	return result;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

TEST(LastSeen, DurationFormat)
{
	time_t diff = 93784; // 1 day 2 hours 3 minutes 4 seconds
	std::chrono::system_clock::duration dur = std::chrono::system_clock::from_time_t(diff) - std::chrono::system_clock::from_time_t(0);
	EXPECT_EQ("1d 2h 3m 4s", CustomTimeFormat(dur));
}

TEST(LastSeen, GetLastStatus_OnlineOffline)
{
	LastSeen test(nullptr);

	{
		EXPECT_FALSE(test.GetLastStatus("test_user")._error.empty());
		EXPECT_FALSE(test.GetLastStatus("test@test.com")._error.empty());
	}

	{
		test.HandlePresence("test_user", "test@test.com", true);

		EXPECT_TRUE(test.GetLastStatus("test_user")._error.empty());
		EXPECT_EQ("test@test.com", test.GetLastStatus("test_user").jid);

		EXPECT_TRUE(test.GetLastStatus("test@test.com")._error.empty());
		EXPECT_EQ("test@test.com", test.GetLastStatus("test@test.com").jid);
	}

	{
		test.HandlePresence("test_user", "test@test.com", false);

		EXPECT_TRUE(test.GetLastStatus("test_user")._error.empty());
		EXPECT_EQ("test@test.com", test.GetLastStatus("test_user").jid);

		EXPECT_TRUE(test.GetLastStatus("test@test.com")._error.empty());
		EXPECT_EQ("test@test.com", test.GetLastStatus("test@test.com").jid);
	}
}

#endif // LCOV_EXCL_STOP
