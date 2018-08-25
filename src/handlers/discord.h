#pragma once

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

#include "lemonhandler.h"

#include <thread>

namespace Hexicord {
	class GatewayClient;
	class RestClient;
	class Snowflake;
}

class DiscordUser
{
public:
	std::string _snowflake;
	std::string _username;
	std::string _nick;
	std::string _status;
	std::string _avatar;
};

class Discord : public LemonHandler
{
public:
	explicit Discord(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	bool Init() final;
	void HandlePresence(const std::string &from, const std::string &jid, bool connected) final;
	const std::string GetHelp() const final;

	void SendToDiscord(const std::string &text);

	virtual ~Discord() final;

private:
	std::string sanitizeDiscord(const std::string &input);
	void rclientSafeSend(const std::string &message);
	void createRole(Hexicord::Snowflake guildid, const std::string &rolename, const Hexicord::Snowflake &userid);

	bool _isEnabled = false;

	std::shared_ptr<Hexicord::GatewayClient> gclient;
	std::shared_ptr<Hexicord::RestClient> rclient;

	std::map<std::string, DiscordUser> _users;
	std::string _myID;

	std::uint64_t _channelID = 0;
	std::uint64_t _ownerID = 0;
	std::string _webhookURL;
	std::string _selfWebhook;

#ifdef _BUILD_TESTS
	FRIEND_TEST(DiscordTest, XMPP2DiscordNickTest);
	FRIEND_TEST(DiscordTest, DiscordSanitizeTest);
#endif
};
