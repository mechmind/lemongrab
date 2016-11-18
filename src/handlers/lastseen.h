#pragma once

#include "lemonhandler.h"

#include "util/persistentmap.h"

#include <string>

class LastSeen : public LemonHandler
{
public:
	LastSeen(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;

	void HandlePresence(const std::string &from, const std::string &jid, bool connected) override;
	const std::string GetHelp() const override;

private:
	static constexpr int maxSearchResults = 20;

	std::string GetStats() const;
	std::string GetUserInfo(const std::string &wantedUser) const;

private:
	LevelDBPersistentMap _lastSeenDB;
	LevelDBPersistentMap _lastActiveDB;
	LevelDBPersistentMap _nick2jidDB;
};
