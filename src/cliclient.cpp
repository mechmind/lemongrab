#include "cliclient.h"
#include "xmpphandler.h"

#include "handlers/util/stringops.h"

#include <iostream>

ConsoleClient::ConsoleClient(const std::string &fakeUserJid)
	: _jid(fakeUserJid)
{

}

void ConsoleClient::SetXMPPHandler(XMPPHandler *handler)
{
	_handler = handler;
}

bool ConsoleClient::Connect(const std::string &jid, const std::string &password)
{
	std::cout << "Connected as " << jid << " with password *****" << std::endl;
	_connected = true;
	_handler->OnConnect();

	while (_connected)
	{
		std::string input;
		std::cin >> input;

		std::string args;
		if (getCommandArguments(input, "!setjid", args))
		{
			SetID(_nick, args);
			continue;
		}

		if (getCommandArguments(input, "!setnick", args))
		{
			SetID(args, _jid);
			continue;
		}

		FakeMessage(input);
	}

	return true;
}

bool ConsoleClient::Disconnect()
{
	_connected = false;
	FakeLeave(_nick, _jid);
	std::cout << "Disconnected" << std::endl;
	return true;
}

bool ConsoleClient::JoinRoom(const std::string &jid)
{
	std::cout << "Joined room " << jid << std::endl;
	FakeJoin(_nick, _jid);
	return true;
}

void ConsoleClient::SendMessage(const std::string &message)
{
	std::cout << "Bot> " << message << std::endl;
}

void ConsoleClient::FakeMessage(const std::string &message)
{
	_handler->OnMessage(_nick, message);
}

void ConsoleClient::FakeJoin(const std::string &nick, const std::string &jid)
{
	_handler->OnPresence(nick, jid, true, nick);
}

void ConsoleClient::FakeLeave(const std::string &nick, const std::string &jid)
{
	_handler->OnPresence(nick, jid, false, nick);
}

void ConsoleClient::FakeRename(const std::string &nick, const std::string &jid, const std::string &newnick)
{
	_handler->OnPresence(nick, jid, true, newnick);
}

void ConsoleClient::SetID(const std::string &nick, const std::string &jid)
{
	if (_connected && jid == _jid)
		FakeRename(_nick, _jid, nick);
	else if (_connected)
	{
		FakeLeave(_nick, _jid);
		FakeJoin(nick, jid);
	}

	_nick = nick;
	_jid = jid;
}
