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
	void SetXMPPHandler(XMPPHandler *handler);

	bool Connect(const std::string &jid, const std::string &password);
	bool Disconnect();

	bool JoinRoom(const std::string &jid);
	void SendMessage(const std::string &message);

public:
	// Connection callbacks
	void onConnect();
	void onDisconnect(gloox::ConnectionError e);
	bool onTLSConnect(const gloox::CertInfo &info);

	// Message callbacks
	void handleMessage(const gloox::Message& stanza, gloox::MessageSession* session);

	// MUC callbacks
	void handleMUCParticipantPresence(gloox::MUCRoom *room, const gloox::MUCRoomParticipant participant, const gloox::Presence &presence);
	void handleMUCMessage(gloox::MUCRoom *room, const gloox::Message &msg, bool priv);
	void handleMUCError(gloox::MUCRoom *room, gloox::StanzaError error);
	void handleMUCInviteDecline(gloox::MUCRoom *room, const gloox::JID &invitee, const std::string &reason);
	bool handleMUCRoomCreation(gloox::MUCRoom *room);
	void handleMUCSubject(gloox::MUCRoom *room, const std::string &nick, const std::string &subject);
	void handleMUCInfo(gloox::MUCRoom *room, int features, const std::string &name, const gloox::DataForm *infoForm);
	void handleMUCItems(gloox::MUCRoom *room, const gloox::Disco::ItemList &items);

private:
	std::shared_ptr<gloox::MUCRoom> _room;
	std::shared_ptr<gloox::Client> _client;

	XMPPHandler *_handler = nullptr;
};


