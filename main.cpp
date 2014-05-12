#include <iostream>

#include <unistd.h>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

using namespace gloox;

class Bot : public MessageHandler
{
private:
	class BotMUCHandler : public MUCRoomHandler
	{
	public:
		void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence)
		{
			if (participant.nick->resource().find("Viol") != std::string::npos)
			{
				sleep(5);
				room->ban(participant.nick->resource(), "UNACCEPTABLE");
				sleep(5);
				room->setAffiliation(participant.nick->full(), AffiliationNone, "");
			}
		}

		void handleMUCMessage(MUCRoom *room, const Message &msg, bool priv)
		{
			std::cout << msg.body() << std::endl;
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



	Bot(std::string password)
	{
		flag = true;
		JID jid( "ash@gnoll.ru/Ian Holm" );
		j = new Client( jid, password );
		j->registerMessageHandler( this );
		j->connect();
	}

	virtual void handleMessage( const Message& stanza,
								MessageSession* session)
	{
		if (flag)
		{
			flag = false;
/*			Message msg (Message::Chat, stanza.from(), "hello world" );
			j->send( msg );*/

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
		JID roomJID( "dfwk@conference.jabber.ru/Ash-04" );
		m_room = new MUCRoom( j, roomJID, myHandler, 0 );
		m_room->join();
	}

private:
	bool flag;
	MUCRoom* m_room;
	Client* j;
};

int main( int argc, char* argv[] )
{
	std::string passwd(argv[1]);
	Bot b(passwd);
}
