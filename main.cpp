#include <iostream>
#include <string>
#include <sstream>

#include <unistd.h>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

#include "settings.h"

using namespace gloox;

class Bot : public MessageHandler
{
private:
	class BotMUCHandler : public MUCRoomHandler
	{
	public:
		void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence)
		{

		}

		void handleMUCMessage(MUCRoom *room, const Message &msg, bool priv)
		{
			std::cout << msg.body() << std::endl;
			std::stringstream Command(msg.body());

			std::string word;
		}

		void handleMUCError(MUCRoom *room, StanzaError error) {
			std::cout << "MUC Error " << error << std::endl;
		}

		void handleMUCInviteDecline(MUCRoom *room, const JID &invitee, const std::string &reason) {}

		bool handleMUCRoomCreation(MUCRoom *room) { return true; }

		void handleMUCSubject(MUCRoom *room, const std::string &nick, const std::string &subject) {}

		void handleMUCInfo(MUCRoom *room, int features, const std::string &name, const DataForm *infoForm) {}

		void handleMUCItems(MUCRoom *room, const Disco::ItemList &items) {}

	};

public:

	Bot(const Settings &settings)
		: m_Settings(settings)
	{

	}

	void Init()
	{
		flag = true;
		JID jid(GetSettings().GetUserJID());
		j = new Client(jid, GetSettings().GetPassword());
		j->registerMessageHandler( this );
		if (!j->connect())
			std::cout << "Can't connect";
	}

	virtual void handleMessage( const Message& stanza,
								MessageSession* session)
	{
		if (flag)
		{
			flag = false;
						Message msg (Message::Chat, stanza.from(), "hello world" );
			j->send( msg );

			joinroom();
		}
		else {
			j->disconnect();
		}

		std::cout << stanza.body() << std::endl;
	}


	void joinroom()
	{
		BotMUCHandler* myHandler = new BotMUCHandler;
		JID roomJID( GetSettings().GetMUC() );
		m_room = new MUCRoom( j, roomJID, myHandler, 0 );
		m_room->join();
	}

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

int main()
{
	Settings settings;
	if (!settings.Open("config.ini"))
	{
		std::cout << "Failed to read config file" << std::endl;
		return 1;
	}

	Bot bot(settings);
	bot.Init();
	return 0;
}
