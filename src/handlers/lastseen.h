#pragma once

#include "lemonhandler.h"

#include <map>
#include <string>

namespace leveldb
{
	class DB;
}

class LastSeen : public LemonHandler
{
public:
	LastSeen(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	void HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	std::string FindSimilar(std::string input);

private:
	static const std::string _command;

	leveldb::DB *_lastSeenDB = nullptr;
	leveldb::DB *_nick2jidDB = nullptr;
};
