#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include <memory>
#include <list>
#include <unordered_map>

#include "xmpphandler.h"
#include "settings.h"
#include "handlers/lemonhandler.h"

class XMPPClient;

class Bot
		: public XMPPHandler
		, public LemonBot
{
public:
	Bot(XMPPClient *client, Settings &settings);

	void Run(); // Locks thread

	void RegisterAllHandlers();
	void UnregisterAllHandlers();

	// XMPPHandler interface
	void OnConnect();
	void OnMessage(const std::string &nick, const std::string &text);
	void OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick);

	// Nick/jid maps
	std::string GetNickByJid(const std::string &jid) const;
	std::string GetJidByNick(const std::string &nick) const;

	// LemonBot interface
	void SendMessage(const std::string &text);
	std::string GetRawConfigValue(const std::string &name) const;

private:
	template <class LemonHandler> void RegisterHandler()
	{
		_messageHandlers.push_back(std::make_shared<LemonHandler>(this));
		_handlersByName[_messageHandlers.back()->GetName()] = _messageHandlers.back();
	}

	const std::string GetHelp(const std::string &module) const;

private:
	std::shared_ptr<XMPPClient> _xmpp;
	Settings &_settings;
	std::unordered_map<std::string, std::string> _nick2jid;
	std::unordered_map<std::string, std::string> _jid2nick;

	std::list<std::shared_ptr<LemonHandler>> _messageHandlers;
	std::unordered_map<std::string, std::shared_ptr<LemonHandler>> _handlersByName;

	std::chrono::system_clock::time_point _startTime;
	std::chrono::system_clock::time_point _lastMessage;
	std::mutex _sendMessageMutex;
	int _sendMessageThrottle = 1;
};
