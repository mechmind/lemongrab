#pragma once

#include <string>
#include <memory>

class XMPPHandler
{
public:
	virtual ~XMPPHandler() { }

	virtual void OnConnect() = 0;

	virtual void OnMessage(const std::string &nick, const std::string &text) = 0;
	virtual void OnPresence(const std::string &nick, const std::string &jid, bool online, const std::string &newNick) = 0;
};
