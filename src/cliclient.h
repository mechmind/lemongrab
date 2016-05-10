#pragma once

#include "xmppclient.h"

#include <memory>

class XMPPHandler;

class ConsoleClient
		: public XMPPClient
{
public:
	ConsoleClient(const std::string &fakeUserJid);

	// XMPPClient interface
	void SetXMPPHandler(XMPPHandler *handler);
	bool Connect(const std::string &jid, const std::string &password);
	bool Disconnect();
	bool JoinRoom(const std::string &jid);
	void SendMessage(const std::string &message);

	void FakeMessage(const std::string &message);
	void FakeJoin(const std::string &nick, const std::string &jid);
	void FakeLeave(const std::string &nick, const std::string &jid);
	void FakeRename(const std::string &nick, const std::string &jid, const std::string &newnick);

	void SetID(const std::string &nick, const std::string &jid);
private:
	XMPPHandler *_handler = nullptr;

	bool _connected = false;
	std::string _nick = "You";
	std::string _jid  = "test@test.net";
};
