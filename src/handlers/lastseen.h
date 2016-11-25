#pragma once

#include "lemonhandler.h"

#include "util/persistentmap.h"

#include <string>
#include <chrono>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

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
		LastStatus()
		{ }

		LastStatus(const std::string &JID)
			: jid(JID)
		{ }

		std::chrono::system_clock::duration when;
		std::string jid;
		std::string _error;
	};

	class LastActivity
	{
	public:
		std::chrono::system_clock::duration when;
		std::string what;
		bool _valid = false;
	};

	std::string GetStats() const;
	std::string GetUserInfo(const std::string &wantedUser) const;

	LastStatus GetLastStatus(const std::string &name) const;
	LastActivity GetLastActive(const std::string &jid) const;

private:
	LevelDBPersistentMap _lastSeenDB;
	LevelDBPersistentMap _lastActiveDB;
	LevelDBPersistentMap _nick2jidDB;

#ifdef _BUILD_TESTS
	FRIEND_TEST(LastSeen, GetLastStatus_OnlineOffline);
#endif
};
