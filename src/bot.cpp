#include "bot.h"

#include <sstream>
#include <iostream>

#include "handlers/diceroller.h"
#include "handlers/urlpreview.h"
#include "handlers/lastseen.h"

void Bot::BotMUCHandler::handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence)
{
	// FIXME Handle this better
	m_Parent->MUCPresence(participant.nick->resource(), participant.jid->bare(), presence.presence() < Presence::Unavailable);
	std::cout << "!!!" << participant.nick->resource() << " | " << participant.jid->bare() << " -> " << presence.presence() << std::endl;
}

void Bot::BotMUCHandler::handleMUCMessage(MUCRoom *room, const Message &msg, bool priv)
{
	std::cout << "###" << msg.body() << std::endl;
	if (msg.when()) // Probably part of room history
		return;
	m_Parent->MUCMessage(msg.from().resource(), msg.body());
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

	m_MessageHandlers.push_back(std::make_shared<DiceRoller>((LemonBot*)this));
	m_MessageHandlers.push_back(std::make_shared<UrlPreview>((LemonBot*)this));
	m_MessageHandlers.push_back(std::make_shared<LastSeen>((LemonBot*)this));
//	m_MessageHandlers.push_back(std::make_shared<StatusReporter>((LemonBot*)this));

	if (!j->connect())
		std::cout << "Can't connect";
}

void Bot::handleMessage( const Message& stanza,	MessageSession* session)
{
	std::cout << stanza.body() << std::endl;
}

void Bot::joinroom()
{
	m_Start = std::chrono::system_clock::now();
	BotMUCHandler* myHandler = new BotMUCHandler(this);
	JID roomJID( GetSettings().GetMUC() );
	m_room = new MUCRoom( j, roomJID, myHandler, 0 );
	m_room->join();
}

void Bot::SendMessage(const std::string &text) const
{
	m_room->send(text);
}

void Bot::MUCMessage(const std::string &from, const std::string &body) const
{
	if (body == "!getversion")
	{
		SendMessage(GetVersion());
		return;
	}
	else if (body == "!uptime")
	{
		auto CurrentTime = std::chrono::system_clock::now();
		std::string uptime("Uptime: ");
		// FIXME There must be a way to make this line shorter
		uptime.append(std::to_string(std::chrono::duration_cast<std::chrono::duration<int, std::ratio<3600*24>>>(CurrentTime - m_Start).count()));
		SendMessage(uptime);
		return;
	}

	for (auto handler : m_MessageHandlers)
		if (handler->HandleMessage(from, body))
			break;
}

void Bot::MUCPresence(const std::string &from, const std::string &jid, bool connected) const
{
	for (auto handler : m_MessageHandlers)
	{
		if (handler->HandlePresence(from, jid, connected))
			break;
	}
}

void Bot::onConnect()
{
	joinroom();
}

void Bot::onDisconnect(ConnectionError e)
{
	std::cout << "ConnectionError: " << e << std::endl;
}

bool Bot::onTLSConnect(const CertInfo &info)
{
	return true;
}

const std::string Bot::GetVersion() const
{
	std::string version = "Core: 0.1 (" + std::string(__DATE__) + ")";
	for (auto handler : m_MessageHandlers)
	{
		version.append(" ");
		version.append(handler->GetVersion());
	}
	return version;
}

const std::string Bot::GetRawConfigValue(const std::string &name) const
{
	return m_Settings.GetRawString(name);
}
