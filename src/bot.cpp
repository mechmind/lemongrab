#include "bot.h"

#include "handlers/diceroller.h"
#include "handlers/urlpreview.h"
#include "handlers/lastseen.h"
#include "handlers/pager.h"
#include "handlers/githubwebhooks.h"
#include "handlers/goodenough.h"
#include "handlers/quotes.h"
#include "handlers/ts3.h"
#include "handlers/leaguelookup.h"
#include "handlers/vote.h"

#include "handlers/util/stringops.h"

#include "glooxclient.h"

#include <glog/logging.h>

Bot::Bot(XMPPClient *client, Settings &settings)
	: _xmpp(client)
	, _settings(settings)
{
	_xmpp->SetXMPPHandler(this);
}

void Bot::Run()
{
	_startTime = std::chrono::system_clock::now();
	_lastMessage = std::chrono::system_clock::now();

	RegisterAllHandlers();

	LOG(INFO) << "Connecting to XMPP server";
	_xmpp->Connect(_settings.GetUserJID(), _settings.GetPassword());
}

void Bot::RegisterAllHandlers()
{
	LOG(INFO) << "Registering handlers";
	RegisterHandler<DiceRoller>();
	RegisterHandler<UrlPreview>();
	RegisterHandler<LastSeen>();
	RegisterHandler<Pager>();
	RegisterHandler<GithubWebhooks>();
	RegisterHandler<Quotes>();
	RegisterHandler<LeagueLookup>();
	RegisterHandler<TS3>();
	RegisterHandler<Voting>();

	for (const auto &handler : _handlersByName)
		LOG(INFO) << "Handler loaded: " << handler.first;
}

void Bot::UnregisterAllHandlers()
{
	_handlersByName.clear();
	_messageHandlers.clear();
}

void Bot::OnConnect()
{
	const auto &muc = _settings.GetMUC();
	LOG(INFO) << "Joining room";
	google::FlushLogFiles(google::INFO);
	_xmpp->JoinRoom(muc);
}

void Bot::OnMessage(const std::string &nick, const std::string &text)
{
	bool isAdmin = false;
	auto admin = GetRawConfigValue("admin");
	if (!admin.empty() && GetJidByNick(nick) == admin)
		isAdmin = true;

	if (text == "!uptime")
	{
		auto currentTime = std::chrono::system_clock::now();
		std::string uptime("Uptime: " + CustomTimeFormat(currentTime - _startTime) + " | Build date: " + std::string(__DATE__));
		return SendMessage(uptime);
	}

	if (text == "!die" && isAdmin)
	{
		LOG(INFO) << "Termination requested";
		_messageHandlers.clear();
		_xmpp->Disconnect();
		return;
	}

	if (text == "!reload" && isAdmin)
	{
		LOG(INFO) << "Config reload requested";
		if (_settings.Reload())
		{
			SendMessage("Settings successfully reloaded, re-registering handlers...");
			UnregisterAllHandlers();
			RegisterAllHandlers();
			SendMessage("Done");
		}
		else
			SendMessage("Failed to reload settings");
	}

	std::string args;
	if (getCommandArguments(text, "!help", args))
	{
		const auto &module = args;
		return SendMessage(GetHelp(module));
	}

	for (auto &handler : _messageHandlers)
		if (handler->HandleMessage(nick, text) == LemonHandler::ProcessingResult::StopProcessing)
			break;
}

void Bot::OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick)
{
	bool newConnection = false;
	if (online)
	{
		newConnection = _nick2jid.insert(std::pair<std::string, std::string>(nick, jid)).second;
		_jid2nick[jid] = nick;
	}
	else
	{
		_nick2jid.erase(nick);
		if (!newNick.empty())
		{
			_nick2jid[newNick] = jid;
			_jid2nick[jid] = newNick;
		} else {
			_jid2nick.erase(jid);
		}
	}

	for (auto &handler : _messageHandlers)
		handler->HandlePresence(nick, jid, newConnection);
}

std::string Bot::GetNickByJid(const std::string &jid) const
{
	auto nick = _jid2nick.find(jid);
	return nick != _jid2nick.end() ? nick->second : "";
}

std::string Bot::GetJidByNick(const std::string &nick) const
{
	auto jid = _nick2jid.find(nick);
	return jid != _nick2jid.end() ? jid->second : "";
}

void Bot::SendMessage(const std::string &text)
{
	std::lock_guard<std::mutex> lock(_sendMessageMutex);
	auto currentTime = std::chrono::system_clock::now();
	if (_lastMessage + std::chrono::seconds(_sendMessageThrottle) > currentTime)
	{
		_sendMessageThrottle++;
		std::this_thread::sleep_for(std::chrono::seconds(_sendMessageThrottle));
	} else {
		_sendMessageThrottle = 1;
	}

	_xmpp->SendMessage(text);
	_lastMessage = std::chrono::system_clock::now();
}

std::string Bot::GetRawConfigValue(const std::string &name) const
{
	return _settings.GetRawString(name);
}

const std::string Bot::GetHelp(const std::string &module) const
{
	auto handler = _handlersByName.find(module);

	if (handler == _handlersByName.end())
	{
		std::string help = "Use !help %module_name%, where module_name is one of:";
		for (auto handlerPtr : _messageHandlers)
		{
			help.append(" " + handlerPtr->GetName());
		}
		return help;
	} else {
		return handler->second->GetHelp();
	}
}
