#pragma once

#include "lemonhandler.h"

#include "util/persistentmap.h"

#include <string>
#include <chrono>

class LastSeen : public LemonHandler
{
public:
	LastSeen(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;

	void HandlePresence(const std::string &from, const std::string &jid, bool connected) override;
	const std::string GetHelp() const override;

private:
	static constexpr int maxSearchResults = 20;

	class LastStatus
	{
	public:
		std::chrono::system_clock::duration when;
		std::string jid;
		std::string _error;
	};

	class LastActivity
	{
	public:
		std::chrono::system_clock::duration when;
		std::string what;
	};

	std::string GetStats() const;
	std::string GetUserInfo(const std::string &wantedUser) const;

	LastStatus GetLastStatus(const std::string &name) const;
	LastActivity GetLastActive(const std::string &jid) const;

private:
	LevelDBPersistentMap _lastSeenDB;
	LevelDBPersistentMap _lastActiveDB;
	LevelDBPersistentMap _nick2jidDB;
};
