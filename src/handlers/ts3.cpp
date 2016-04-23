#include "ts3.h"

#include "util/stringops.h"

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>

#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include "util/stringops.h"

std::string FormatTS3Name(const std::string &raw);
std::string ReplaceTS3Spaces(const std::string &input, bool backwards = false);

TS3::TS3(LemonBot *bot)
	: LemonHandler("ts3", bot)
{
	_nickname = GetRawConfigValue("ts3name");
	if (_nickname.empty())
		_nickname = "Unseen\\svoice";

	_channelID = GetRawConfigValue("ts3channel");
	StartServerQueryClient();
}

LemonHandler::ProcessingResult TS3::HandleMessage(const std::string &from, const std::string &body)
{
	std::string message;
	if (getCommandArguments(body, "!tsay", message))
	{
		SendTS3Message(from, message);
		return ProcessingResult::StopProcessing;
	}

	if (body != "!ts")
		return ProcessingResult::KeepGoing;

	if (_clients.empty())
		SendMessage("TeamSpeak server is empty");
	else
	{
		std::string online;
		for (auto &user : _clients)
			online.append(" " + user.second);

		SendMessage("Current TeamSpeak users online:" + online);
	}
	return ProcessingResult::StopProcessing;
}

const std::string TS3::GetVersion() const
{
	return "0.2";
}

const std::string TS3::GetHelp() const
{
	return "!ts - get online teamspeak users, !tsay %message% - chat with teamspeak channel";
}

void TS3::telnetEvent(bufferevent *bev, short event, void *parentPtr)
{
	if (event & BEV_EVENT_CONNECTED) {
		LOG(INFO) << "Connected to TeamSpeak server";
	} else if (event & BEV_EVENT_TIMEOUT) {
		bufferevent_enable(bev, EV_READ|EV_WRITE);
		evbuffer_add_printf(bufferevent_get_output(bev), "whoami\n");
	} else if (event & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		auto parent = static_cast<TS3*>(parentPtr);
		if (event & BEV_EVENT_ERROR) {
			int err = bufferevent_socket_get_dns_error(bev);
			if (err)
				LOG(ERROR) << "Can't connect to TeamSpeak server, DNS error: " << evutil_gai_strerror(err);
		}
		LOG(ERROR) << "TeamSpeak connection closed";
		// FIXME needs a reliable restart mechanism, see thread
		parent->SendMessage("TS3 ServerQuery connection closed, fix me please");
		bufferevent_free(bev);
		event_base_loopexit(parent->base, nullptr);
	}
}

void TS3::telnetMessage(bufferevent *bev, void *parentPtr)
{
	auto parent = static_cast<TS3*>(parentPtr);
	std::string s;
	char buf[1024];
	int n;
	evbuffer *input = bufferevent_get_input(bev);
	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0)
		s.append(buf, n);

	// trim CR LF
	if (s.size() > 2)
		s.erase(s.size() - 2);
	else
		LOG(ERROR) << "Received telnet line that is too short!";

	switch (parent->_sqState)
	{
	case TS3::State::NotConnected:
		// FIXME: very poor criteria for connection
		static const std::string welcome = "Welcome to the TeamSpeak 3 ServerQuery interface";
		if (s.find(welcome) != s.npos)
		{
			LOG(INFO) << "Connected to ServerQuery interface";
			parent->_sqState = TS3::State::ServerQueryConnected;
			evbuffer_add_printf(bufferevent_get_output(bev), "login %s %s\n",
								parent->GetRawConfigValue("TS3QueryLogin").c_str(),
								parent->GetRawConfigValue("TS3QueryPassword").c_str());
		}
		break;

	case TS3::State::ServerQueryConnected:
		static const std::string okMsg = "error id=0 msg=ok";
		if (beginsWith(s, okMsg))
		{
			LOG(INFO) << "Authorized on TS3 server";
			parent->_sqState = TS3::State::Authrozied;
			evbuffer_add_printf(bufferevent_get_output(bev), "use port=9987\n");
		}
		break;

	case TS3::State::Authrozied:
		if (beginsWith(s, okMsg))
		{
			LOG(INFO) << "Connected to Virtual Server";
			parent->_sqState = TS3::State::VirtualServerConnected;
			evbuffer_add_printf(bufferevent_get_output(bev), "clientupdate client_nickname=%s\n", parent->_nickname.c_str());
		}
		break;

	case TS3::State::VirtualServerConnected:
		if (beginsWith(s, okMsg))
		{
			LOG(INFO) << "Nickname is set";
			parent->_sqState = TS3::State::NickSet;
			evbuffer_add_printf(bufferevent_get_output(bev), "servernotifyregister event=server\n");
		}

		break;

	case TS3::State::NickSet:
		if (beginsWith(s, okMsg))
		{
			LOG(INFO) << "Subscribed to connects/disconnects";
			parent->_sqState = TS3::State::Subscribed;
			if (!parent->_channelID.empty())
				evbuffer_add_printf(bufferevent_get_output(bev), "servernotifyregister event=textchannel id=%s\n", parent->_channelID.c_str());
		}
		break;

	case TS3::State::Subscribed:
		if (beginsWith(s, "notifycliententerview"))
		{
			// FIXME we need only 1-2 tokens, no need to tokenize whole thing?
			auto tokens = tokenize(s, ' ');
			try {
				parent->TS3Connected(tokens.at(4).substr(5), tokens.at(6).substr(16));
			} catch (std::exception &e) {
				LOG(ERROR) << "Can't parse message: \"" << s << "\" | Exception: " << e.what();
			}
			break;
		}

		if (beginsWith(s, "notifyclientleftview"))
		{
			auto tokens = tokenize(s, ' ');
			try {
				parent->TS3Disconnected(tokens.at(5).substr(5, tokens.at(5).size() - 5));
			} catch (std::exception &e) {
				LOG(ERROR) << "Can't parse message: \"" << s << "\" | Exception: " << e.what();
			}
			break;
		}

		if (beginsWith(s, "notifytextmessage"))
		{
			auto tokens = tokenize(s, ' ');
			try {
				auto nick = ReplaceTS3Spaces(tokens.at(4).substr(12));
				auto message = ReplaceTS3Spaces(tokens.at(2).substr(4));
				parent->TS3Message(nick, message);
			} catch (std::exception &e) {
				LOG(ERROR) << "Can't parse message: \"" << s << "\" | Exceptions: " << e.what();
			}
			break;
		}

		break;
	}

}

void TS3::telnetClientThread(TS3 * parent, std::string server)
{
	int port = 10011;
	parent->base = event_base_new();
	parent->dns_base = evdns_base_new(parent->base, 1);

	parent->msg_event = event_new(parent->base, -1, EV_READ, sendTS3message, parent);
	event_add(parent->msg_event, nullptr);

	parent->bev = bufferevent_socket_new(parent->base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(parent->bev, telnetMessage, nullptr, telnetEvent, parent);
	bufferevent_enable(parent->bev, EV_READ|EV_WRITE);
	timeval t;
	t.tv_sec = 60;
	t.tv_usec = 0;
	bufferevent_set_timeouts(parent->bev, &t, nullptr);
	bufferevent_socket_connect_hostname(
		parent->bev, parent->dns_base, AF_UNSPEC, server.c_str(), port);

	// FIXME needs a reliable restart mechanism
	auto result = event_base_dispatch(parent->base);

	if (result == -1)
		LOG(ERROR) << "Failed to start event loop";
}

void TS3::StartServerQueryClient()
{
	auto serverAddress = GetRawConfigValue("ts3server");
	if (!serverAddress.empty())
		_telnetClient = std::thread (&telnetClientThread, this, serverAddress);
	else
		LOG(INFO) << "No ts3server option in config, TS3 module disabled";
}

void TS3::TS3Connected(const std::string &clid, const std::string &nick)
{
	std::string formattedNick = FormatTS3Name(nick);
	_clients[clid] = formattedNick;
	SendMessage("Teamspeak user connected: " + formattedNick);
}

void TS3::TS3Disconnected(const std::string &clid)
{
	auto nick = _clients[clid];
	SendMessage("Teamspeak user disconnected: " + nick);
	_clients.erase(clid);
}

void TS3::TS3Message(const std::string &nick, const std::string &text)
{
	if (nick != _nickname)
		SendMessage("TS <" + nick + "> " + text);
}

void TS3::SendTS3Message(const std::string &nick, const std::string &text)
{
	std::string jid = "null";
	if (_botPtr)
		jid = _botPtr->GetJidByNick(nick);

	_outgoingMessage = "<" + nick + " [" + jid + "] > " + text;
	if (msg_event)
		event_active(msg_event, EV_READ, 0);
}

void TS3::sendTS3message(int, short, void *parentPtr)
{
	auto parent = static_cast<TS3*>(parentPtr);
	std::string message = "sendtextmessage targetmode=2 target=" + parent->_channelID + " msg=" + ReplaceTS3Spaces(parent->_outgoingMessage, true) + "\n";
	bufferevent_write(parent->bev, message.c_str(), message.size());
}

std::string FormatTS3Name(const std::string &raw)
{
	std::string result;
	auto fromIPpos = raw.find("\\sfrom\\s");
	if (fromIPpos != raw.npos) // ServerQuery user
		result = "ServerQuery<" + raw.substr(0, fromIPpos) + ">";
	else
		result = raw;

	return ReplaceTS3Spaces(result);
}

std::string ReplaceTS3Spaces(const std::string &input, bool backwards)
{
	static const std::list<std::pair<std::string, std::string>> ts3escape =
	{
		{" " , "\\s"},
		{"\n", "\\n"},
		{"/" , "\\/"},
	};

	std::string result = input;
	for (const auto &replacer : ts3escape)
	{
		if (backwards)
			boost::replace_all(result, replacer.first, replacer.second);
		else
			boost::replace_all(result, replacer.second, replacer.first);
	}

	if (!backwards)
	{
		boost::erase_all(result, "[URL]");
		boost::erase_all(result, "[/URL]");
	}

	return result;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

TEST(TS3, NameFormatter)
{
	EXPECT_EQ("Bob", FormatTS3Name("Bob"));
	EXPECT_EQ("Alice Bob", FormatTS3Name("Alice\\sBob"));
	EXPECT_EQ("ServerQuery<Alice Bob>", FormatTS3Name("Alice\\sBob\\sfrom\\s127.0.0.1"));
}

TEST(TS3, UrlFormatter)
{
	EXPECT_EQ("http://example.org/", ReplaceTS3Spaces("[URL]http:\\/\\/example.org\\/[/URL]"));
}

#endif // LCOV_EXCL_STOP
