#ifndef BOT_H
#define BOT_H

#include <string>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

#include "settings.h"

using namespace gloox;

class Bot : public MessageHandler, public ConnectionListener
{
private:
	class BotMUCHandler : public MUCRoomHandler
	{
	public:
		void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence);
		void handleMUCMessage(MUCRoom *room, const Message &msg, bool priv);
		void handleMUCError(MUCRoom *room, StanzaError error);
		void handleMUCInviteDecline(MUCRoom *room, const JID &invitee, const std::string &reason);
		bool handleMUCRoomCreation(MUCRoom *room);
		void handleMUCSubject(MUCRoom *room, const std::string &nick, const std::string &subject);
		void handleMUCInfo(MUCRoom *room, int features, const std::string &name, const DataForm *infoForm);
		void handleMUCItems(MUCRoom *room, const Disco::ItemList &items);
	};

public:

	Bot(const Settings &settings)
		: m_Settings(settings)
	{

	}

	void Init();
	virtual void handleMessage( const Message& stanza,
								MessageSession* session);
	void joinroom();

	void onConnect();
	void onDisconnect(ConnectionError e);
	bool onTLSConnect(const CertInfo &info);

private:

private:
	bool flag;
	MUCRoom* m_room;
	Client* j;

	const Settings &m_Settings;

	const Settings &GetSettings() const
	{
		return m_Settings;
	}
};
#endif // BOT_H
