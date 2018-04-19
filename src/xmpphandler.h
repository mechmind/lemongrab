#pragma once

#include <string>
#include <memory>

class ChatMessage
{
public:
	enum class Origin {
		Unknown,
		XMPP,
		Discord,
		Bot,
	};

	ChatMessage() = default;
	ChatMessage(const std::string &nick,
	            const std::string &jid,
	            const std::string &body,
	            bool priv)
	    : _nick(nick)
	    , _jid(jid)
	    , _body(body)
	    , _isPrivate(priv)
	{ }

	std::string _nick;
	std::string _jid;
	std::string _body;
	bool _isPrivate = false;
	bool _isAdmin = false;
	bool _hasDiscordEmbed = false;

	std::string _module_name;

	Origin _origin = Origin::Unknown;
};

class XMPPHandler
{
public:
	virtual ~XMPPHandler() { }

	virtual void OnConnect() = 0;
	virtual void OnMessage(ChatMessage &msg) = 0;
	virtual void OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick) = 0;
};
