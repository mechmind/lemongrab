#pragma once

#include <string>

class XMPPHandler;

class XMPPClient
{
public:
	virtual ~XMPPClient() {}

	virtual void SetXMPPHandler(XMPPHandler * handler) = 0;

	virtual bool Connect(const std::string &jid, const std::string &password) = 0;
	virtual bool Disconnect() = 0;
	virtual bool JoinRoom(const std::string &jid) = 0;
	virtual void SendMessage(const std::string &message, const std::string &recipient) = 0;
};
