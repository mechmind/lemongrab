#pragma once

#include "lemonhandler.h"

#include "util/persistentmap.h"

#include <map>
#include <string>
#include <memory>

class LastSeen : public LemonHandler
{
public:
	LastSeen(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	void HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	enum class FindBy {
		Jid,
		User,
	};

	std::string FindSimilar(std::string input, FindBy searchOptions);

private:
	PersistentMap _lastSeenDB;
	PersistentMap _nick2jidDB;
};
