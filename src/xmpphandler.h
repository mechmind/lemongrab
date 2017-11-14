#pragma once

#include <string>
#include <memory>

class ChatMessage
{
public:
	ChatMessage() = default;
	ChatMessage(const std::string &nick,
	            const std::string &jid,
	            const std::string &roomjid,
	            const std::string &body,
	            bool priv)
	    : _nick(nick)
	    , _jid(jid)
	    , _roomjid(roomjid)
	    , _body(body)
	    , _isPrivate(priv)
	{ }

	std::string _nick;
	std::string _jid;
	std::string _roomjid;
	std::string _body;
	bool _isPrivate = false;
	bool _isAdmin = false;

	std::string _module_name;
};

class XMPPHandler
{
public:
	virtual ~XMPPHandler() { }

	virtual void OnConnect() = 0;
	virtual void OnMessage(ChatMessage &msg) = 0;
	virtual void OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick) = 0;
};
