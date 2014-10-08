#include "bot.h"

#include <sstream>
#include <iostream>

void Bot::BotMUCHandler::handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence)
{

}

void Bot::BotMUCHandler::handleMUCMessage(MUCRoom *room, const Message &msg, bool priv)
{
	std::cout << msg.body() << std::endl;
	std::stringstream Command(msg.body());

	std::string word;
}

void Bot::BotMUCHandler::handleMUCError(MUCRoom *room, StanzaError error) {
	std::cout << "MUC Error " << error << std::endl;
}

void Bot::BotMUCHandler::handleMUCInviteDecline(MUCRoom *room, const JID &invitee, const std::string &reason)
{

}

bool Bot::BotMUCHandler::handleMUCRoomCreation(MUCRoom *room)
{
	return true;
}

void Bot::BotMUCHandler::handleMUCSubject(MUCRoom *room, const std::string &nick, const std::string &subject)
{

}

void Bot::BotMUCHandler::handleMUCInfo(MUCRoom *room, int features, const std::string &name, const DataForm *infoForm)
{

}

void Bot::BotMUCHandler::handleMUCItems(MUCRoom *room, const Disco::ItemList &items)
{

}

void Bot::Init()
{
	flag = true;
	JID jid(GetSettings().GetUserJID());
	j = new Client(jid, GetSettings().GetPassword());
	j->registerMessageHandler( this );
	j->registerConnectionListener( this );
	if (!j->connect())
		std::cout << "Can't connect";
}

void Bot::handleMessage( const Message& stanza,	MessageSession* session)
{
	std::cout << stanza.body() << std::endl;
}

void Bot::joinroom()
{
	BotMUCHandler* myHandler = new BotMUCHandler;
	JID roomJID( GetSettings().GetMUC() );
	m_room = new MUCRoom( j, roomJID, myHandler, 0 );
	m_room->join();
}

void Bot::onConnect()
{
	joinroom();
}

void Bot::onDisconnect(ConnectionError e)
{

}

bool Bot::onTLSConnect(const CertInfo &info)
{
	return true;
}
