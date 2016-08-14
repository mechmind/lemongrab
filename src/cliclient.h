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
	void SetXMPPHandler(XMPPHandler *handler) override;
	bool Connect(const std::string &jid, const std::string &password) override;
	bool Disconnect() override;
	bool JoinRoom(const std::string &jid) override;
	void SendMessage(const std::string &message, const std::string &recipient) override;

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
