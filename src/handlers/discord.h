#pragma once

#include "lemonhandler.h"

#include <boost/asio/io_service.hpp>

#include <thread>

namespace Hexicord {
	class GatewayClient;
	class RestClient;
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
	Discord(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	bool Init() final;
	void HandlePresence(const std::string &from, const std::string &jid, bool connected) final;
	const std::string GetHelp() const final;

	void SendToDiscord(std::string text);

	virtual ~Discord() final;

private:
	std::string sanitizeDiscord(const std::string &input);

	bool _isEnabled = false;
	boost::asio::io_service ioService;

	std::shared_ptr<Hexicord::GatewayClient> gclient;
	std::shared_ptr<Hexicord::RestClient> rclient;

	std::map<std::string, DiscordUser> _users;

	std::uint64_t _channelID = 0;
	std::uint64_t _ownerID = 0;
	std::string _webhookURL;
	std::string _selfWebhook;
	std::thread _clientThread;
};
