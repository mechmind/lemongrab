#include "glooxclient.h"

#include "xmpphandler.h"

#include <gloox/presence.h>
#include <iostream>

void GlooxClient::SetXMPPHandler(XMPPHandler *handler)
{
	_handler = handler;
}

bool GlooxClient::Connect(const std::string &jid, const std::string &password)
{
	gloox::JID GlooxJid(jid);

	_client = std::make_shared<gloox::Client>(GlooxJid, password);
	_client->registerMessageHandler(this);
	_client->registerConnectionListener(this);

	if (!_client->connect())
	{
		std::cout << "Can't connect" << std::endl;
		return false;
	}

	return true;
}

bool GlooxClient::Disconnect()
{
	if (_room)
		_room->leave();

	_room.reset();

	if (_client)
	{
		_client->disconnect();
		return true;
	}

	return false;
}

bool GlooxClient::JoinRoom(const std::string &jid)
{
	if (_room)
		_room->leave();

	_room = std::make_shared<gloox::MUCRoom>(_client.get(), jid, this, nullptr);
	_room->join();

	return true;
}

void GlooxClient::SendMessage(const std::string &message)
{
	if (_room)
		_room->send(message);
}

void GlooxClient::onConnect()
{
	_handler->OnConnect();
}

void GlooxClient::onDisconnect(gloox::ConnectionError e)
{
	std::cout << "Disconnected, error code: " << e << std::endl;
}

bool GlooxClient::onTLSConnect(const gloox::CertInfo &info)
{
	return true;
}

void GlooxClient::handleMessage(const gloox::Message &stanza, gloox::MessageSession *session)
{

}

void GlooxClient::handleMUCParticipantPresence(gloox::MUCRoom *room, const gloox::MUCRoomParticipant participant, const gloox::Presence &presence)
{
	std::string jid = participant.jid ? participant.jid->bare() : "unknown@unknown";
	_handler->OnPresence(participant.nick->resource(), jid, presence.presence() < gloox::Presence::Unavailable, participant.newNick);
}

void GlooxClient::handleMUCMessage(gloox::MUCRoom *room, const gloox::Message &msg, bool priv)
{
	if (msg.from().resource() == _room->nick()
			|| msg.from().resource() == ""
			|| msg.when()
			|| priv)
		return;

	_handler->OnMessage(msg.from().resource(), msg.body());
}

void GlooxClient::handleMUCError(gloox::MUCRoom *room, gloox::StanzaError error)
{
	std::cout << "MUC Error: " << error << std::endl;
}

void GlooxClient::handleMUCInviteDecline(gloox::MUCRoom *room, const gloox::JID &invitee, const std::string &reason)
{

}

bool GlooxClient::handleMUCRoomCreation(gloox::MUCRoom *room)
{
	return true;
}

void GlooxClient::handleMUCSubject(gloox::MUCRoom *room, const std::string &nick, const std::string &subject)
{

}

void GlooxClient::handleMUCInfo(gloox::MUCRoom *room, int features, const std::string &name, const gloox::DataForm *infoForm)
{

}

void GlooxClient::handleMUCItems(gloox::MUCRoom *room, const gloox::Disco::ItemList &items)
{

}
