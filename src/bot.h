#ifndef BOT_H
#define BOT_H

#include <string>
#include <memory>
#include <list>
#include <chrono>

#include <gloox/client.h>
#include <gloox/message.h>
#include <gloox/messagehandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/mucroom.h>
#include <gloox/mucroomhandler.h>

#include "handlers/lemonhandler.h"
#include "settings.h"

using namespace gloox;

class Bot : public LemonBot, public MessageHandler, public ConnectionListener
{
private:
	class BotMUCHandler : public MUCRoomHandler
	{
	public:
		BotMUCHandler(Bot *parent)
			: m_Parent(parent)
		{

		}

		void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant, const Presence &presence);
		void handleMUCMessage(MUCRoom *room, const Message &msg, bool priv);
		void handleMUCError(MUCRoom *room, StanzaError error);
		void handleMUCInviteDecline(MUCRoom *room, const JID &invitee, const std::string &reason);
		bool handleMUCRoomCreation(MUCRoom *room);
		void handleMUCSubject(MUCRoom *room, const std::string &nick, const std::string &subject);
		void handleMUCInfo(MUCRoom *room, int features, const std::string &name, const DataForm *infoForm);
		void handleMUCItems(MUCRoom *room, const Disco::ItemList &items);

	private:
		Bot *m_Parent;
	};

public:

	Bot(const Settings &settings)
		: m_Settings(settings)
	{

	}

	void SendMessage(const std::string &text) const;

	void Init();
	virtual void handleMessage( const Message& stanza,
								MessageSession* session);
	void joinroom();

	void MUCMessage(const std::string &from, const std::string &body) const;
	void MUCPresence(const std::string &from, bool connected) const;

	void onConnect();
	void onDisconnect(ConnectionError e);
	bool onTLSConnect(const CertInfo &info);

	const std::string GetVersion() const;
	const std::string GetRawConfigValue(const std::string &name) const;
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

	std::list<std::shared_ptr<LemonHandler>> m_MessageHandlers;
	std::chrono::system_clock::time_point m_Start;
};
#endif // BOT_H
