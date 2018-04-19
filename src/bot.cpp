#include "bot.h"

#include "handlers/diceroller.h"
#include "handlers/urlpreview.h"
#include "handlers/lastseen.h"
#include "handlers/pager.h"
#include "handlers/githubwebhooks.h"
#include "handlers/goodenough.h"
#include "handlers/quotes.h"
#include "handlers/leaguelookup.h"
#include "handlers/vote.h"
#include "handlers/rss.h"
#include "handlers/discord.h"
#include "handlers/warframe.h"

#include "handlers/util/stringops.h"

#include "glooxclient.h"

#include "signal.h"

#include <cpptoml.h>
#include <glog/logging.h>

#include <algorithm>

static Bot *signalHandlingInstance = nullptr;

void handleSigTerm(int) {
	if (signalHandlingInstance)
		signalHandlingInstance->OnSIGTERM();
}

static void RegisterSignalHandler(Bot *bot) {
	signalHandlingInstance = bot;

	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &handleSigTerm;
	sigaction(SIGTERM, &action, nullptr);
}

static void UnregisterSignalHandler() {
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	sigaction(SIGTERM, &action, nullptr);
}

Bot::Bot(XMPPClient *client, Settings &settings)
	: LemonBot(settings.GetDBPrefixPath() + "/local.db")
	, _xmpp(client)
	, _settings(settings)
{
	_storage.sync_schema(true);
	_xmpp->SetXMPPHandler(this);
	RegisterSignalHandler(this);
}

Bot::~Bot()
{
	UnregisterSignalHandler();
}

Bot::ExitCode Bot::Run()
{
	_startTime = std::chrono::system_clock::now();
	_lastMessage = std::chrono::system_clock::now();

	RegisterAllHandlers();

	LOG(INFO) << "Connecting to XMPP server";
	_xmpp->Connect(_settings.GetUserJID(), _settings.GetPassword());

	return _exitCode;
}

void Bot::RegisterAllHandlers()
{
	LOG(INFO) << "Registering handlers";
	RegisterHandler<Discord>();
	RegisterHandler<DiceRoller>();
	RegisterHandler<UrlPreview>();
	RegisterHandler<LastSeen>();
	RegisterHandler<Pager>();
	RegisterHandler<GithubWebhooks>();
	RegisterHandler<Quotes>();
	RegisterHandler<LeagueLookup>();
	RegisterHandler<Voting>();
	RegisterHandler<RSSWatcher>();
	RegisterHandler<Warframe>();

	for (const auto &handler : _handlersByName)
		LOG(INFO) << "Handler loaded: " << handler.first;

	EnableHandlers(_settings.GetStringSet("General.Modules"), _settings.GetStringSet("General.ModulesBlacklist"));
}

void Bot::UnregisterAllHandlers()
{
	_handlersByName.clear();
	_chatEventHandlers.clear();
}

void Bot::OnConnect()
{
	const auto &muc = _settings.GetMUC();
	LOG(INFO) << "Joining room";
	google::FlushLogFiles(google::INFO);
	_xmpp->JoinRoom(muc);
}

void Bot::OnMessage(ChatMessage &msg)
{
	if (msg._jid.empty())
		msg._jid = GetJidByNick(msg._nick);

	msg._isAdmin |= !msg._jid.empty()
			&& GetRawConfigValue("General.admin") == msg._jid;

	if (_settings.verboseLogging())
	{
		LOG(INFO) << ">>> Message: " << msg._jid << " as " << msg._nick << " > " << msg._body
				  << " [ Priv? " << msg._isPrivate << " Module? " << msg._module_name << " Discord embed? " << msg._hasDiscordEmbed << " ]";
	}

	auto &text = msg._body;

	if (text == "!uptime")
	{
		// FIXME: dirty hack
		if (msg._module_name != "discord")
			dynamic_cast<Discord*>(_handlersByName["discord"].get())->HandleMessage(msg);

		auto currentTime = std::chrono::system_clock::now();
		std::string uptime("Uptime: " + CustomTimeFormat(currentTime - _startTime));
		return SendMessage(uptime);
	}

	if (text == "!die" && msg._isAdmin)
	{
		// FIXME: dirty hack
		if (msg._module_name != "discord")
			dynamic_cast<Discord*>(_handlersByName["discord"].get())->HandleMessage(msg);

		LOG(WARNING) << "Termination requested (!die command received)";
		_exitCode = ExitCode::TerminationRequested;
		_xmpp->Disconnect();
		return;
	}

	if (text == "!restart" && msg._isAdmin)
	{
		// FIXME: dirty hack
		if (msg._module_name != "discord")
			dynamic_cast<Discord*>(_handlersByName["discord"].get())->HandleMessage(msg);

		LOG(WARNING) << "Restart requested";
		_exitCode = ExitCode::RestartRequested;
		// _chatEventHandlers.clear();
		_xmpp->Disconnect();
		return;
	}

	if (text == "!reload" && msg._isAdmin)
	{
		// FIXME: dirty hack
		if (msg._module_name != "discord")
			dynamic_cast<Discord*>(_handlersByName["discord"].get())->HandleMessage(msg);

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
		// FIXME: dirty hack
		if (msg._module_name != "discord")
			dynamic_cast<Discord*>(_handlersByName["discord"].get())->HandleMessage(msg);

		const auto &module = args;
		return SendMessage(GetHelp(module));
	}

	for (auto &handler : _chatEventHandlers) {
//		if (handler->GetName() == msg._module_name)
//			continue;

		if (handler->HandleMessage(msg) == LemonHandler::ProcessingResult::StopProcessing)
			break;
	}
}

void Bot::OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick)
{
	bool isNewConnection = false;
	if (online)
	{
		isNewConnection = _nick2jid.insert(std::pair(nick, jid)).second;
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

	for (auto &handler : _chatEventHandlers)
		handler->HandlePresence(nick, jid, isNewConnection);
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

std::string Bot::GetDBPathPrefix() const
{
	return _settings.GetDBPrefixPath();
}

std::string Bot::GetOnlineUsers() const
{
	std::string result = "Jabber users:";

	for (auto pair : _jid2nick) {
		result += "\n" + pair.second + " (" + pair.first + ")";
	};

	return result;
}

void Bot::SendMessage(const std::string &message)
{
	// FIXME: get rid of this
	ChatMessage msg;
	msg._body = message;
	msg._origin = ChatMessage::Origin::Bot;

	SendMessage(msg);
}

void Bot::SendMessage(const ChatMessage &message)
{
	if (message._body.empty()) {
		return;
	}

	std::lock_guard<std::mutex> lock(_sendMessageMutex);

	// Throttle
	auto currentTime = std::chrono::system_clock::now();
	if (_lastMessage + std::chrono::seconds(_sendMessageThrottle) > currentTime)
	{
		_sendMessageThrottle++;
		std::this_thread::sleep_for(std::chrono::seconds(_sendMessageThrottle));
	} else {
		_sendMessageThrottle = 1;
	}

	// Send
	_xmpp->SendMessage(message._body, ""); // FIXME: unused arg

	if (message._origin != ChatMessage::Origin::Discord) {
		dynamic_cast<Discord*>(_handlersByName["discord"].get())->SendToDiscord(message._body);
	}

	_lastMessage = std::chrono::system_clock::now();
}

void Bot::TunnelMessage(const ChatMessage &msg, const std::string &module_name)
{
	auto newMsg = msg;
	newMsg._module_name = module_name;
	OnMessage(newMsg);
}

std::string Bot::GetRawConfigValue(const std::string &name) const
{
	// FIXME expose _settings instead?
	return _settings.GetRawString(name);
}

std::string Bot::GetRawConfigValue(const std::string &table, const std::string &name) const
{
	return _settings.GetTable(table)->get_as<std::string>(name).value_or("");
}

void Bot::OnSIGTERM()
{
	LOG(WARNING) << "Termination requested (SIGTERM caught)";
	_exitCode = ExitCode::TerminationRequested;
	_xmpp->Disconnect();
}

void Bot::EnableHandlers(const std::set<std::string> &whitelist, const std::set<std::string> &blacklist)
{
	if (whitelist.empty())
	{
		LOG(WARNING) << "No handlers are set, enabling them all";
		for (auto &handler : _allChatEventHandlers)
		{
			if (blacklist.find(handler->GetName()) == blacklist.end()) {
				EnableHandler(handler);
			}
		}

		return;
	}

	for (const auto &name : whitelist)
	{
		if (blacklist.find(name) == blacklist.end()) {
			EnableHandler(name);
		}
	}
}

bool Bot::EnableHandler(const std::string &name)
{
	auto handler = _handlersByName.find(name);
	if (handler == _handlersByName.end())
	{
		LOG(WARNING) << "Handler not found: " << name;
		return false;
	}

	return EnableHandler(handler->second);
}

bool Bot::EnableHandler(std::shared_ptr<LemonHandler> &handler)
{
	if (!handler->Init())
	{
		LOG(WARNING) << "Init for handler " << handler->GetName() << " failed";
		return false;
	}

	_chatEventHandlers.push_back(handler);
	LOG(INFO) << "Handler enabled: " << handler->GetName();
	return true;
}

const std::string Bot::GetHelp(const std::string &module) const
{
	auto handler = _handlersByName.find(module);

	if (handler == _handlersByName.end())
	{
		std::string help = "Use !help %module_name%, where module_name is one of:";
		for (auto handlerPtr : _chatEventHandlers)
		{
			help.append(" " + handlerPtr->GetName());
		}
		return help;
	} else {
		return handler->second->GetHelp();
	}
}
