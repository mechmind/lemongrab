#include "lastseen.h"

#include <leveldb/db.h>
#include <leveldb/options.h>

#include <algorithm>
#include <iostream>
#include <ctime>
#include <regex>
#include <chrono>

#include "util/stringops.h"

const std::string LastSeen::_command = "!seen";

std::string CustomTimeFormat(std::chrono::system_clock::duration input)
{
	std::string output;
	output += std::to_string(std::chrono::duration_cast<std::chrono::hours>  (input).count() / 24) + "d ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::hours>  (input).count() % 24) + "h ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::minutes>(input).count() % 60) + "m ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::seconds>(input).count() % 60) + "s";

	return output;
}

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler("seen", bot)
{
	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::Status status = leveldb::DB::Open(options, "db/lastseen", &_lastSeenDB);
	if (!status.ok())
		std::cerr << status.ToString() << std::endl;

	status = leveldb::DB::Open(options, "db/nick2jid", &_nick2jidDB);
	if (!status.ok())
		std::cerr << status.ToString() << std::endl;
}

bool LastSeen::HandleMessage(const std::string &from, const std::string &body)
{
	if (body.length() < _command.length() + 2 || body.substr(0, _command.length()) != _command)
		return false;

	if (!_nick2jidDB || !_lastSeenDB)
	{
		SendMessage("Database connection error");
		return false;
	}

	std::string input = body.substr(_command.length() + 1, body.npos);

	std::string lastSeenRecord = "0";
	std::string jidRecord = input;
	leveldb::Status getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), input, &lastSeenRecord);

	if (getLastSeenByJID.IsNotFound())
	{
		leveldb::Status getJIDByNick = _nick2jidDB->Get(leveldb::ReadOptions(), input, &jidRecord);

		if (getJIDByNick.IsNotFound())
		{
			auto similarUsers = FindSimilar(input);
			if (similarUsers.empty())
				SendMessage(input + "? Who's that?");
			else
				SendMessage("Users similar to " + input + ":" + similarUsers);

			return false;
		}

		getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), jidRecord, &lastSeenRecord);

		if (getLastSeenByJID.IsNotFound())
		{
			SendMessage("Well this is weird, " + input + " resolved to " + jidRecord + " but I have no record for this jid");
			return false;
		}
	}

	auto currentNick = _botPtr->GetNickByJid(jidRecord);
	if (!currentNick.empty())
	{
		if (input != currentNick)
			SendMessage(input + " (" + jidRecord + ") is still here as " + currentNick);
		else
			SendMessage(input + " is still here");
		return false;
	}

	std::chrono::system_clock::duration lastSeenDiff;
	try {
		auto now = std::chrono::system_clock::now();
		auto lastSeenTime = std::chrono::system_clock::from_time_t(std::stol(lastSeenRecord));
		lastSeenDiff = now - lastSeenTime;
	} catch (std::exception e) {
		SendMessage("Something broke: " + std::string(e.what()));
		return false;
	}

	SendMessage(input + " (" + jidRecord + ") last seen " + CustomTimeFormat(lastSeenDiff) + " ago");
	return false;
}

bool LastSeen::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	auto now = std::chrono::system_clock::now();

	if (_nick2jidDB)
		_nick2jidDB->Put(leveldb::WriteOptions(), from, jid);

	if (_lastSeenDB)
		_lastSeenDB->Put(leveldb::WriteOptions(), jid, std::to_string(std::chrono::system_clock::to_time_t(now)));

	return false;
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
	} catch (std::regex_error e) {
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
		} catch (std::regex_error e) {
			std::cout << "Regex exception thrown" << e.what() << std::endl;
		}

		if (doesMatch)
		{
			if (matches >= 10)
			{
				matchingRecords.append("... (too many matches)");
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
