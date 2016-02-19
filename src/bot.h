#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <list>
#include <unordered_map>

#include "xmpphandler.h"
#include "settings.h"
#include "handlers/lemonhandler.h"

class GlooxClient;

class NewBot
		: public XMPPHandler
		, public LemonBot
{
public:
	NewBot(Settings &settings);

	void Run(); // Locks thread

	// XMPPHandler interface
	void OnConnect();
	void OnMessage(const std::string &nick, const std::string &text);
	void OnPresence(const std::string &nick, const std::string &jid, bool connected);

	// LemonBot interface
	void SendMessage(const std::string &text) const;
	const std::string GetRawConfigValue(const std::string &name) const;

private:
	template <class LemonHandler> void RegisterHandler()
	{
		_messageHandlers.push_back(std::make_shared<LemonHandler>(this));
		_handlersByName[_messageHandlers.back()->GetName()] = _messageHandlers.back();
	}

	const std::string GetVersion() const;
	const std::string GetHelp(const std::string &module) const;

private:
	std::shared_ptr<GlooxClient> _gloox;
	Settings &_settings;

	std::list<std::shared_ptr<LemonHandler>> _messageHandlers;
	std::unordered_map<std::string, std::shared_ptr<LemonHandler>> _handlersByName;
	std::chrono::system_clock::time_point _startTime;
};
