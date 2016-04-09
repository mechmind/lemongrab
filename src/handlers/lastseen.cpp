#include "lastseen.h"

#include <glog/logging.h>

#include <algorithm>
#include <ctime>
#include <regex>
#include <chrono>

#include "util/stringops.h"

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{
	_lastSeenDB.init("lastseen");
	_nick2jidDB.init("nick2jid");
}

LemonHandler::ProcessingResult LastSeen::HandleMessage(const std::string &from, const std::string &body)
{
	std::string wantedUser;

	if (getCommandArguments(body, "!seenjid", wantedUser))
	{
		auto searchResults = FindSimilar(wantedUser, FindBy::Jid);
		if (searchResults.empty())
			SendMessage(from + ": I have nothing :<");
		else
			SendMessage("Matching JIDs:" + searchResults);
		return ProcessingResult::StopProcessing;
	}

	if (!getCommandArguments(body, "!seen", wantedUser))
		return ProcessingResult::KeepGoing;

	if (!_nick2jidDB.isOK() || !_lastSeenDB.isOK())
	{
		SendMessage("Database connection error");
		return ProcessingResult::KeepGoing;
	}

	std::string lastSeenRecord = "0";
	std::string jidRecord = wantedUser;

	if (!_lastSeenDB.Get(wantedUser, lastSeenRecord))
	{
		if (_nick2jidDB.Get(wantedUser, jidRecord))
		{
			auto similarUsers = FindSimilar(wantedUser, FindBy::User);
			if (similarUsers.empty())
				SendMessage(wantedUser + "? Who's that?");
			else
				SendMessage("Users similar to " + wantedUser + ":" + similarUsers);

			return ProcessingResult::StopProcessing;
		}

		if (!_lastSeenDB.Get(jidRecord, lastSeenRecord))
		{
			SendMessage("Well this is weird, " + wantedUser + " resolved to " + jidRecord + " but I have no record for this jid");
			return ProcessingResult::StopProcessing;
		}
	}

	auto currentNick = _botPtr->GetNickByJid(jidRecord);
	if (!currentNick.empty())
	{
		if (wantedUser != currentNick)
			SendMessage(wantedUser + " (" + jidRecord + ") is still here as " + currentNick);
		else
			SendMessage(wantedUser + " is still here");
		return ProcessingResult::StopProcessing;
	}

	std::chrono::system_clock::duration lastSeenDiff;
	try {
		auto now = std::chrono::system_clock::now();
		auto lastSeenTime = std::chrono::system_clock::from_time_t(std::stol(lastSeenRecord));
		lastSeenDiff = now - lastSeenTime;
	} catch (std::exception &e) {
		SendMessage("Something broke: " + std::string(e.what()));
		return ProcessingResult::StopProcessing;
	}

	SendMessage(wantedUser + " (" + jidRecord + ") last seen " + CustomTimeFormat(lastSeenDiff) + " ago");
	return ProcessingResult::StopProcessing;
}

void LastSeen::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	auto now = std::chrono::system_clock::now();

	if (_nick2jidDB.isOK())
		_nick2jidDB.Set(from, jid);

	if (_lastSeenDB.isOK())
		_lastSeenDB.Set(jid, std::to_string(std::chrono::system_clock::to_time_t(now)));
}

const std::string LastSeen::GetVersion() const
{
	return "0.1";
}

const std::string LastSeen::GetHelp() const
{
	return "Use !seen %nickname% or !seen %jid%; use !seen %regex% or !seenjid %regex% to search users by regex";
}

std::string LastSeen::FindSimilar(std::string input, FindBy searchOptions)
{
	std::regex inputRegex;
	std::string matchingRecords;
	try {
		inputRegex = std::regex(toLower(input));
	} catch (std::regex_error &e) {
		SendMessage("Can't do deep search, regex error: " + std::string(e.what()));
		LOG(WARNING) << "Regex exception: " << e.what();
	}

	int matches = 0;

	_nick2jidDB.ForEach([&](std::pair<std::string, std::string> record)->bool{
		std::smatch regexMatch;
		bool doesMatch = false;
		std::string userId = (searchOptions == FindBy::User) ?
					toLower(record.first) : toLower(record.second);
		try {
			doesMatch = std::regex_search(userId, regexMatch, inputRegex);
		} catch (std::regex_error &e) {
			LOG(WARNING) << "Regex exception: " << e.what();
			return true;
		}

		if (doesMatch)
		{
			if (matches >= 10)
			{
				matchingRecords.append(" ... (too many matches)");
				return false;
			}

			matches++;
			matchingRecords.append(" " + record.first + " (" + record.second + ")");
		}

		return true;
	});

	return matchingRecords;
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
