#include "lastseen.h"

#include <glog/logging.h>

#include <ctime>
#include <chrono>

#include "util/stringops.h"

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{
	_lastSeenDB.init("lastseen");
	_nick2jidDB.init("nick2jid");
	_lastActiveDB.init("lastmsg");
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
		PrintSeenStat();
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

	PrintUserInfo(wantedUser);
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

void LastSeen::PrintSeenStat()
{
	if (!_nick2jidDB.isOK() || !_lastSeenDB.isOK() || !_lastActiveDB.isOK())
	{
		SendMessage("Database connection error");
		return;
	}

	std::string response = "Seen nicks: " + std::to_string(_nick2jidDB.Size()) + " | Seen users: " + std::to_string(_lastSeenDB.Size());

	SendMessage(response);
}

void LastSeen::PrintUserInfo(const std::__cxx11::string &wantedUser)
{
	std::string lastSeenRecord = "0";
	std::string lastActiveRecord = "0";
	std::string jidRecord = wantedUser;

	if (!_lastSeenDB.Get(wantedUser, lastSeenRecord))
	{
		if (!_nick2jidDB.Get(wantedUser, jidRecord))
		{
			auto similarUsers = _nick2jidDB.Find(wantedUser, LevelDBPersistentMap::FindOptions::All);
			std::string message;

			if (similarUsers.empty())
				message = wantedUser + "? Who's that?";
			else if (similarUsers.size() > maxSearchResults)
				message = "Too many matches";
			else
			{
				message = "Similar users:";
				for (const auto &user : similarUsers)
					message += " " + user.first + " (" + user.second + ")";
			}

			SendMessage(message);
			return;
		}

		if (!_lastSeenDB.Get(jidRecord, lastSeenRecord))
		{
			SendMessage("Well this is weird, " + wantedUser + " resolved to " + jidRecord + " but I have no record for this jid");
			return;
		}
	}

	std::string lastActiveMessageResponse;
	std::string lastActiveMessageText;
	{
		if (_lastActiveDB.Get(jidRecord, lastActiveRecord))
		{
			auto data = tokenize(lastActiveRecord, ' ', 2);
			lastActiveRecord = data.at(0);

			if (data.size() > 1)
				lastActiveMessageText = data.at(1);

			std::chrono::system_clock::duration lastActiveDiff;
			try {
				auto now = std::chrono::system_clock::now();
				auto lastActiveTime = std::chrono::system_clock::from_time_t(std::stol(lastActiveRecord));
				lastActiveDiff = now - lastActiveTime;
			} catch (std::exception &e) {
				SendMessage("Something broke: " + std::string(e.what()));
				return;
			}

			lastActiveMessageResponse = "; last active " + CustomTimeFormat(lastActiveDiff) + " ago";
			if (!lastActiveMessageText.empty())
				lastActiveMessageResponse.append(": \"" + lastActiveMessageText + "\"");
		}
	}

	auto currentNick = _botPtr->GetNickByJid(jidRecord);
	if (!currentNick.empty())
	{
		if (wantedUser != currentNick)
			SendMessage(wantedUser + " (" + jidRecord + ") is still here as " + currentNick + lastActiveMessageResponse);
		else
			SendMessage(wantedUser + " is still here" + lastActiveMessageResponse);
		return;
	}

	std::chrono::system_clock::duration lastSeenDiff;
	try {
		auto now = std::chrono::system_clock::now();
		auto lastSeenTime = std::chrono::system_clock::from_time_t(std::stol(lastSeenRecord));
		lastSeenDiff = now - lastSeenTime;
	} catch (std::exception &e) {
		SendMessage("Something broke: " + std::string(e.what()));
		return;
	}

	SendMessage(wantedUser + " (" + jidRecord + ") last seen " + CustomTimeFormat(lastSeenDiff) + " ago" + lastActiveMessageResponse);
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

TEST(LastSeen, DurationFormat)
{
	time_t diff = 93784; // 1 day 2 hours 3 minutes 4 seconds
	std::chrono::system_clock::duration dur = std::chrono::system_clock::from_time_t(diff) - std::chrono::system_clock::from_time_t(0);
	EXPECT_EQ("1d 2h 3m 4s", CustomTimeFormat(dur));
}

#endif // LCOV_EXCL_STOP
