#include "lastseen.h"

#include <leveldb/db.h>
#include <leveldb/options.h>

#include <algorithm>
#include <iostream>
#include <ctime>
#include <regex>
#include <chrono>

#include "util/stringops.h"

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{
	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::DB *lastSeenDB;
	leveldb::Status status = leveldb::DB::Open(options, "db/lastseen", &lastSeenDB);
	if (!status.ok())
		std::cerr << status.ToString() << std::endl;

	_lastSeenDB.reset(lastSeenDB);

	leveldb::DB *nick2jidDB;
	status = leveldb::DB::Open(options, "db/nick2jid", &nick2jidDB);
	if (!status.ok())
		std::cerr << status.ToString() << std::endl;

	_nick2jidDB.reset(nick2jidDB);
}

LemonHandler::ProcessingResult LastSeen::HandleMessage(const std::string &from, const std::string &body)
{
	std::string wantedUser;
	if (!getCommandArguments(body, "!seen", wantedUser));
		return ProcessingResult::KeepGoing;

	if (!_nick2jidDB || !_lastSeenDB)
	{
		SendMessage("Database connection error");
		return ProcessingResult::KeepGoing;
	}

	std::string lastSeenRecord = "0";
	std::string jidRecord = wantedUser;
	leveldb::Status getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), wantedUser, &lastSeenRecord);

	if (getLastSeenByJID.IsNotFound())
	{
		leveldb::Status getJIDByNick = _nick2jidDB->Get(leveldb::ReadOptions(), wantedUser, &jidRecord);

		if (getJIDByNick.IsNotFound())
		{
			auto similarUsers = FindSimilar(wantedUser);
			if (similarUsers.empty())
				SendMessage(wantedUser + "? Who's that?");
			else
				SendMessage("Users similar to " + wantedUser + ":" + similarUsers);

			return ProcessingResult::StopProcessing;
		}

		getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), jidRecord, &lastSeenRecord);

		if (getLastSeenByJID.IsNotFound())
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

	if (_nick2jidDB)
		_nick2jidDB->Put(leveldb::WriteOptions(), from, jid);

	if (_lastSeenDB)
		_lastSeenDB->Put(leveldb::WriteOptions(), jid, std::to_string(std::chrono::system_clock::to_time_t(now)));
}

const std::string LastSeen::GetVersion() const
{
	return "0.1";
}

const std::string LastSeen::GetHelp() const
{
	return "Use !seen %nickname% or !seen %jid%";
}

std::string LastSeen::FindSimilar(std::string input)
{
	std::regex inputRegex;
	std::string matchingRecords;
	try {
		inputRegex = std::regex(toLower(input));
	} catch (std::regex_error &e) {
		SendMessage("Can't do deep search, regex error: " + std::string(e.what()));
	}

	int matches = 0;

	std::shared_ptr<leveldb::Iterator> it(_nick2jidDB->NewIterator(leveldb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next()) {

		std::smatch regexMatch;
		bool doesMatch = false;
		std::string nick = toLower(it->key().ToString());
		try {
			doesMatch = std::regex_search(nick, regexMatch, inputRegex);
		} catch (std::regex_error &e) {
			std::cout << "Regex exception thrown" << e.what() << std::endl;
		}

		if (doesMatch)
		{
			if (matches >= 10)
			{
				matchingRecords.append(" ... (too many matches)");
				return matchingRecords;
			}

			matches++;
			matchingRecords.append(" " + it->key().ToString() + " (" + it->value().ToString() + ")");
		}
	}

	if (!it->status().ok())
		SendMessage("Something bad happened during search");

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
