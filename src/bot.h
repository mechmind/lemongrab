#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include <memory>
#include <list>
#include <set>
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

	enum class ExitCode
	{
		Error,
		TerminationRequested,
		RestartRequested,
	};

	ExitCode Run(); // Locks thread

	void RegisterAllHandlers();
	void UnregisterAllHandlers();

	// XMPPHandler interface
	void OnConnect() override;
	void OnMessage(ChatMessage &msg) final;
	void OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick) final;

	// Nick/jid maps
	std::string GetNickByJid(const std::string &jid) const final;
	std::string GetJidByNick(const std::string &nick) const final;
	std::string GetDBPathPrefix() const final;

	// LemonBot interface
	void SendMessage(const std::string &text) final;
	std::string GetRawConfigValue(const std::string &name) const final;

private:
	template <class LemonHandler> void RegisterHandler()
	{
		auto handler = std::make_shared<LemonHandler>(this);
		_handlersByName[handler->GetName()] = handler;
	}

	void EnableHandlers(const std::list<std::string> &whitelist, const std::list<std::string> &blacklist);
	bool EnableHandler(const std::string &name);

	// Global commands
	const std::string GetHelp(const std::string &module) const;

private:
	std::shared_ptr<XMPPClient> _xmpp;
	Settings &_settings;
	std::unordered_map<std::string, std::string> _nick2jid;
	std::unordered_map<std::string, std::string> _jid2nick;

	std::list<std::shared_ptr<LemonHandler>> _chatEventHandlers;
	std::unordered_map<std::string, std::shared_ptr<LemonHandler>> _handlersByName;

	ExitCode _exitCode = ExitCode::Error;
	std::chrono::system_clock::time_point _startTime;
	std::chrono::system_clock::time_point _lastMessage;
	std::mutex _sendMessageMutex;
	int _sendMessageThrottle = 1;
};
