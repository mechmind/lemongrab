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

	leveldb::Status status = leveldb::DB::Open(options, "db/lastseen", &db);

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

	if (_currentConnections.find(input) != _currentConnections.end()
			&& _currentConnections.at(input)) // FIXME two searches but should be fast enough for chatroom
	{
		SendMessage(input + " is still here");
		return false;
	}

	std::string record = "0";
	db->Get(leveldb::ReadOptions(), input, &record);

	long lastSeenDiff = 0;
	long lastSeenTime = 0;
	try {
		time_t now;
		std::time(&now);
		lastSeenTime = std::stol(record);
		lastSeenDiff = now - lastSeenTime;
	} catch (std::exception e) {
		SendMessage("Something broke");
		return false;
	}

	if (lastSeenTime > 0)
		SendMessage(input + " last seen " + CustomTimeFormat(lastSeenDiff) + " ago");
	else
		SendMessage(input + "? Who's that?");

	return false;
}

bool LastSeen::HandlePresence(const std::string &from, bool connected)
{
	// Use chrono here?
	time_t now;
	std::time(&now);
	_currentConnections[from] = connected; // FIXME kinda leaky

	db->Put(leveldb::WriteOptions(), from, std::to_string(now));
	return false;
}

const std::string LastSeen::GetVersion() const
{
	return "LastSeen 0.1";
}
