#include "xmpphandler.h"

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

class GlooxClient
		: public gloox::ConnectionListener
		, public gloox::MessageHandler
		, public gloox::MUCRoomHandler
{
public:
	std::shared_ptr<XMPPHandler> m_Handler;

	GlooxClient(std::shared_ptr<XMPPHandler> handler)
	{
		m_Handler = handler;
	}

	// Connection callbacks
	void onConnect()
	{

	}

	void onDisconnect(gloox::ConnectionError e)
	{

	}

	bool onTLSConnect(const gloox::CertInfo &info)
	{
		return true;
	}

	// Message callbacks
	void handleMessage(const gloox::Message& stanza, gloox::MessageSession* session)
	{

	}

	// MUC callbacks
	void handleMUCParticipantPresence(gloox::MUCRoom *room, const gloox::MUCRoomParticipant participant, const gloox::Presence &presence)
	{

	}

	void handleMUCMessage(gloox::MUCRoom *room, const gloox::Message &msg, bool priv)
	{
		m_Handler->GetMessageHandler().OnMessage("test", msg.from().resource(), msg.body());
	}

	void handleMUCError(gloox::MUCRoom *room, gloox::StanzaError error)
	{

	}

	void handleMUCInviteDecline(gloox::MUCRoom *room, const gloox::JID &invitee, const std::string &reason)
	{

	}

	bool handleMUCRoomCreation(gloox::MUCRoom *room)
	{
		return true;
	}

	void handleMUCSubject(gloox::MUCRoom *room, const std::string &nick, const std::string &subject)
	{

	}

	void handleMUCInfo(gloox::MUCRoom *room, int features, const std::string &name, const gloox::DataForm *infoForm)
	{

	}

	void handleMUCItems(gloox::MUCRoom *room, const gloox::Disco::ItemList &items)
	{

	}


private:
	gloox::MUCRoom * m_MUCRoom;
	gloox::Client * m_Client;

	std::shared_ptr<XMPPHandler> m_XMPPHandler;
};

XMPPHandler::XMPPHandler()
{

}

bool XMPPHandler::Connect(std::string JID, std::string password, std::string MUC)
{
	return true;
}

bool XMPPHandler::Disconnect()
{
	return true;
}

bool XMPPHandler::IsConnected()
{
	return m_IsConnected;
}

