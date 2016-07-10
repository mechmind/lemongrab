#pragma once

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

#include <memory>

#include "xmppclient.h"

class XMPPHandler;

class GlooxClient
		: public XMPPClient
		, public gloox::ConnectionListener
		, public gloox::MessageHandler
		, public gloox::MUCRoomHandler
{
public:
	// XMPPClient implementation
	void SetXMPPHandler(XMPPHandler *handler) override;

	bool Connect(const std::string &jid, const std::string &password) override;
	bool Disconnect() override;

	bool JoinRoom(const std::string &jid) override;
	void SendMessage(const std::string &message) override;

public:
	// Connection callbacks
	void onConnect() override;
	void onDisconnect(gloox::ConnectionError e) override;
	bool onTLSConnect(const gloox::CertInfo &info) override;

	// Message callbacks
	void handleMessage(const gloox::Message& stanza, gloox::MessageSession* session) override;

	// MUC callbacks
	void handleMUCParticipantPresence(gloox::MUCRoom *room, const gloox::MUCRoomParticipant participant, const gloox::Presence &presence) override;
	void handleMUCMessage(gloox::MUCRoom *room, const gloox::Message &msg, bool priv) override;
	void handleMUCError(gloox::MUCRoom *room, gloox::StanzaError error) override;
	void handleMUCInviteDecline(gloox::MUCRoom *room, const gloox::JID &invitee, const std::string &reason) override;
	bool handleMUCRoomCreation(gloox::MUCRoom *room) override;
	void handleMUCSubject(gloox::MUCRoom *room, const std::string &nick, const std::string &subject) override;
	void handleMUCInfo(gloox::MUCRoom *room, int features, const std::string &name, const gloox::DataForm *infoForm) override;
	void handleMUCItems(gloox::MUCRoom *room, const gloox::Disco::ItemList &items) override;

private:
	std::shared_ptr<gloox::MUCRoom> _room;
	std::shared_ptr<gloox::Client> _client;

	XMPPHandler *_handler = nullptr;
};


