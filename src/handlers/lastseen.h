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
	bool HandlePresence(const std::string &from, bool connected);
	const std::string GetVersion() const;

private:
	std::map<std::string, bool> _currentConnections;
	leveldb::DB *db;
};
