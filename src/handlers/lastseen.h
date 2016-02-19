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
	bool HandleMessage(const std::string &from, const std::string &body);
	bool HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	static const std::string _command;

	std::map<std::string, std::string> _currentConnections;
	leveldb::DB *_lastSeenDB;
	leveldb::DB *_nick2jidDB;
};
