#include "lastseen.h"

#include <leveldb/db.h>
#include <leveldb/options.h>

#include <iostream>
#include <ctime>

std::string CustomTimeFormat(time_t input)
{
	std::string output;
	int seconds = input % 60;
	output = std::to_string(seconds) + "s";

	input/=60;
	if (input == 0) return output;
	int minutes = input % 60;
	output = std::to_string(minutes) + "m " + output;

	input/=60;
	if (input == 0) return output;
	int hours = input % 24;
	output = std::to_string(hours) + "h " + output;

	input/=24;
	if (input == 0) return output;
	output = std::to_string(input) + "d " + output;

	return output;
}

LastSeen::LastSeen(LemonBot *bot)
	: LemonHandler(bot)
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
	if (body.substr(0, 5) != "!seen")
		return true;

	std::string input;
	try
	{
		input = body.substr(body.find(' ', 0) + 1, body.npos);
	} catch (std::exception e) {
		SendMessage("Usage: !seen <nick> or !seen <jid>");
		return false;
	}

	std::string lastSeenRecord = "0";
	std::string jidRecord = input;
	leveldb::Status getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), input, &lastSeenRecord);

	if (getLastSeenByJID.IsNotFound())
	{
		leveldb::Status getJIDByNick = _nick2jidDB->Get(leveldb::ReadOptions(), input, &jidRecord);

		if (getJIDByNick.IsNotFound())
		{
			SendMessage(input + "? Who's that?");
			return false;
		}

		getLastSeenByJID = _lastSeenDB->Get(leveldb::ReadOptions(), jidRecord, &lastSeenRecord);

		if (getLastSeenByJID.IsNotFound())
		{
			SendMessage("Well this is weird, " + input + " resolved to " + jidRecord + " but I have no record for this jid");
			return false;
		}
	}

	auto currentNick = _currentConnections.find(jidRecord);
	if (_currentConnections.find(jidRecord) != _currentConnections.end())
	{
		if (input != currentNick->second)
			SendMessage(input + " (" + jidRecord + ") is still here as " + currentNick->second);
		else
			SendMessage(input + " is still here");
		return false;
	}

	long lastSeenDiff = 0;
	long lastSeenTime = 0;
	try {
		time_t now;
		std::time(&now);
		lastSeenTime = std::stol(lastSeenRecord);
		lastSeenDiff = now - lastSeenTime;
	} catch (std::exception e) {
		SendMessage("Something broke");
		return false;
	}

	SendMessage(input + " (" + jidRecord + ") last seen " + CustomTimeFormat(lastSeenDiff) + " ago");
	return false;
}

bool LastSeen::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	// Use chrono here?
	time_t now;
	std::time(&now);

	if (connected)
		_currentConnections[jid] = from;
	else
		_currentConnections.erase(jid);

	_nick2jidDB->Put(leveldb::WriteOptions(), from, jid);
	_lastSeenDB->Put(leveldb::WriteOptions(), jid, std::to_string(now));
	return false;
}

const std::string LastSeen::GetVersion() const
{
	return "LastSeen 0.1";
}
