#pragma once

#include "lemonhandler.h"

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

	std::string GetStats();
	std::string GetUserInfo(const std::string &wantedUser);

	LastStatus GetLastStatus(const std::string &name);
	std::optional<LastActivity> GetLastActive(const std::string &jid);

private:
	void Migrate();

#ifdef _BUILD_TESTS
	FRIEND_TEST(LastSeen, GetLastStatus_OnlineOffline);
#endif
};
