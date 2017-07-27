#include "lastseen.h"

#include <glog/logging.h>

#include <ctime>

#include "util/stringops.h"

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{

}

LemonHandler::ProcessingResult LastSeen::HandleMessage(const ChatMessage &msg)
{
	auto now = std::chrono::system_clock::now();
	auto now_t = std::chrono::system_clock::to_time_t(now);

	if (auto userRecord = getStorage().get_no_throw<DB::UserActivity>(msg._jid)) {
		userRecord->nick = msg._nick;
		userRecord->message = msg._body;
		userRecord->timepoint_message = now_t;
		getStorage().update(*userRecord);
	} else {
		getStorage().replace(DB::UserActivity{msg._jid, msg._nick, msg._body, now_t, now_t});
	}

	if (msg._body == "!seenstat")
	{
		SendMessage(GetStats());
		return ProcessingResult::KeepGoing;
	}

	std::string wantedUser;
	if (!getCommandArguments(msg._body, "!seen", wantedUser))
		return ProcessingResult::KeepGoing;

	SendMessage(GetUserInfo(wantedUser));
	return ProcessingResult::KeepGoing;
}

void LastSeen::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	auto now = std::chrono::system_clock::now();
	auto now_t = std::chrono::system_clock::to_time_t(now);

	using namespace sqlite_orm;
	DB::Nick newNick{ from, jid };
	getStorage().replace(newNick);

	if (auto userRecord = getStorage().get_no_throw<DB::UserActivity>(jid)) {
		userRecord->nick = from;
		userRecord->timepoint_status = now_t;
		getStorage().update(*userRecord);
	} else {
		getStorage().replace(DB::UserActivity{jid, from, "", now_t, now_t});
	}
}

const std::string LastSeen::GetHelp() const
{
	return "Use !seen %nickname% or !seen %jid%; use !seen %regex% or !seenjid %regex% to search users by regex\n"
		   "!seenstat - show statistics";
}

std::string LastSeen::GetStats()
{
	auto nick_count = getStorage().count<DB::Nick>();
	auto user_count = getStorage().count<DB::UserActivity>();
	return "Seen nicks: " + std::to_string(nick_count) + " | Seen users: " + std::to_string(user_count);
}

std::string LastSeen::GetUserInfo(const std::string &wantedUser)
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

	if (auto lastActivity = GetLastActive(lastStatus.jid))
	{
		result.append("; last active " + CustomTimeFormat(lastActivity->when) + " ago");
		if (!lastActivity->what.empty())
			result.append(": \"" + lastActivity->what + "\"");
	}

	return result;
}

LastSeen::LastStatus LastSeen::GetLastStatus(const std::string &name) // FIXME const
{
	using namespace sqlite_orm;
	auto now = std::chrono::system_clock::now();

	std::string lastSeenRecord = "0";

	if (auto userRecord = getStorage().get_no_throw<DB::UserActivity>(name))
	{
		auto lastSeenTime = std::chrono::system_clock::from_time_t(userRecord->timepoint_status);
		return { now - lastSeenTime, name, "" };
	}

	if (auto nick2jid = getStorage().get_no_throw<DB::Nick>(name))
	{
		if (auto userRecord = getStorage().get_no_throw<DB::UserActivity>(nick2jid->uniqueID))
		{
			auto lastSeenTime = std::chrono::system_clock::from_time_t(userRecord->timepoint_status);
			return { now - lastSeenTime, userRecord->uniqueID, "" };
		}

		return { std::chrono::nanoseconds{0}, "", "User activity and nick database mismatch" };
	}

	auto similarUsers = getStorage().get_all<DB::Nick>(where(like(&DB::Nick::nick, "%" + name + "%")), limit(maxSearchResults));
	auto similarUsersByJid = getStorage().get_all<DB::Nick>(where(like(&DB::Nick::uniqueID, "%" + name + "%")), limit(maxSearchResults));
	if (similarUsers.empty())
		return { std::chrono::seconds{0}, "", name + "? Who's that?" };
	else
	{
		std::string similarUsersStr = "Similar nicks:";
		for (const auto &user : similarUsers)
			similarUsersStr.append(" " + user.nick + " (" + user.uniqueID + ")");
		similarUsersStr.append("\n\nSimilar JIDs:");
		for (const auto &user : similarUsersByJid)
			similarUsersStr.append(" " + user.nick + " (" + user.uniqueID + ")");
		return { std::chrono::seconds{0}, "", similarUsersStr };
	}
}

std::optional<LastSeen::LastActivity> LastSeen::GetLastActive(const std::string &jid)
{
	auto now = std::chrono::system_clock::now();
	if (auto userRecord = getStorage().get_no_throw<DB::UserActivity>(jid)) {
		auto lastActiveTime = std::chrono::system_clock::from_time_t(userRecord->timepoint_message);
		return {{ now - lastActiveTime, userRecord->message }};
	} else {
		return { };
	}
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

class LastSeenBot : public LemonBot
{
public:
	LastSeenBot() : LemonBot(":memory:") {
		_storage.sync_schema();
	}

	void SendMessage(const std::string &text)
	{

	}
};

TEST(LastSeen, DurationFormat)
{
	time_t diff = 93784; // 1 day 2 hours 3 minutes 4 seconds
	std::chrono::system_clock::duration dur = std::chrono::system_clock::from_time_t(diff) - std::chrono::system_clock::from_time_t(0);
	EXPECT_EQ("1d 2h 3m 4s", CustomTimeFormat(dur));
}

TEST(LastSeen, GetLastStatus_OnlineOffline)
{
	LastSeenBot tb;
	LastSeen test(&tb);

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
