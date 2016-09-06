#pragma once

#include <thread>
#include <string>
#include <list>
#include <unordered_map>

#include "lemonhandler.h"

class event_base; // NOLINT
class event; // NOLINT
class evdns_base; // NOLINT
class bufferevent; // NOLINT

class TS3 : public LemonHandler
{
public:
	TS3(LemonBot *bot);
	bool Init() final;
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

private:
	void StartServerQueryClient();
	void TS3Connected(const std::string &clid, const std::string &nick);
	void TS3Disconnected(const std::string &clid);
	void TS3Message(const std::string &nick, const std::string &text);

	void SendTS3Message(const std::string &nick, const std::string &jid, const std::string &text);

private:
	enum class State
	{
		NotConnected,
		ServerQueryConnected,
		Authrozied,
		VirtualServerConnected,
		NickSet,
		Subscribed,
	};

	// FIXME: must be an atomic container instead!
	std::string _outgoingMessage;

	std::string _nickname = "Lemongrab";
	std::int64_t _channelID;
	std::thread _telnetClient;
	State _sqState = State::NotConnected;
	std::unordered_map<std::string, std::string> _clients;

	event_base *_base = nullptr;
	evdns_base *_dns_base = nullptr;
	event *_msg_event = nullptr;
	bufferevent *_bev = nullptr;

	// custom events
	static void terminateClient(int, short int, void *arg);
	static void sendTS3message(int, short int, void *parentPtr);

	static void telnetClientThread(TS3 * parent, std::string server);
	static void telnetEvent(bufferevent *bev, short events, void *parentPtr);
	static void telnetMessage(bufferevent *bev, void *parentPtr);
};

