#include "glooxclient.h"

#include "xmpphandler.h"

#include <gloox/presence.h>
#include <glog/logging.h>

void GlooxClient::SetXMPPHandler(XMPPHandler *handler)
{
	_handler = handler;
}

bool GlooxClient::Connect(const std::string &jid, const std::string &password)
{
	gloox::JID glooxJid(jid);

	_client = std::make_shared<gloox::Client>(glooxJid, password);
	_client->registerMessageHandler(this);
	_client->registerConnectionListener(this);

	if (!_client->connect())
	{
		LOG(ERROR) << "Can't connect to xmpp server";
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

void GlooxClient::SendMessage(const std::string &message, const std::string &recipient)
{
#ifdef EVENT_LOGGING
	LOG(INFO) << "SendMessage: " << message;
#endif

	if (_room && recipient.empty())
		_room->send(message);
}

void GlooxClient::onConnect()
{
	_handler->OnConnect();
}

void GlooxClient::onDisconnect(gloox::ConnectionError e)
{
	LOG(ERROR) << "Disconnected, error code: " << e;
}

bool GlooxClient::onTLSConnect(const gloox::CertInfo &info)
{
	return true;
}

// BUG: Groupchat messages that contain threads count as plain messages for some reason
void GlooxClient::handleMessage(const gloox::Message &msg, gloox::MessageSession *session)
{
#ifdef EVENT_LOGGING
	LOG(INFO) << "Message: " << msg.body();
#endif

	if (msg.when() != nullptr) // history
	{
#ifdef EVENT_LOGGING
	LOG(INFO) << "Message: " << msg.body() << " ignored because it's history";
#endif
		return;
	}

	ChatMessage message(msg.from().resource(), "", msg.body(), false);
	if (message._nick == _room->nick()
			|| message._nick.empty())
	{
#ifdef EVENT_LOGGING
	LOG(INFO) << "Message: " << msg.body() << " ignored because it's ours or has empty nick";
#endif
		return;
	}

	_handler->OnMessage(message);
}

void GlooxClient::handleMUCParticipantPresence(gloox::MUCRoom *room, const gloox::MUCRoomParticipant participant, const gloox::Presence &presence)
{
	std::string jid = participant.jid ? participant.jid->bare() : "unknown@unknown";
	_handler->OnPresence(participant.nick->resource(), jid, presence.presence() < gloox::Presence::Unavailable, participant.newNick);
}

void GlooxClient::handleMUCMessage(gloox::MUCRoom *room, const gloox::Message &msg, bool priv)
{
#ifdef EVENT_LOGGING
	LOG(INFO) << "MUCMessage: " << msg.body();
#endif

	if (msg.when() != nullptr) // history
	{
#ifdef EVENT_LOGGING
	LOG(INFO) << "MUCMessage: " << msg.body() << " ignored because it's history";
#endif
		return;
	}

	ChatMessage message(msg.from().resource(), "", msg.body(), priv);
	if (message._nick == _room->nick()
			|| message._nick.empty()
			|| message._isPrivate)
	{
#ifdef EVENT_LOGGING
	LOG(INFO) << "MUCMessage: " << msg.body() << " ignored because it's private, ours or has empty nick";
#endif
		return;
	}

	_handler->OnMessage(message);
}

void GlooxClient::handleMUCError(gloox::MUCRoom *room, gloox::StanzaError error)
{
	LOG(ERROR) << "MUC Error: " << error;
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
