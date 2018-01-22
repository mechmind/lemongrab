#include "cliclient.h"
#include "xmpphandler.h"

#include "handlers/util/stringops.h"

#include <iostream>
#include <chrono>
#include <thread>

ConsoleClient::ConsoleClient()
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
		std::getline(std::cin, input);

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

		if (input == "!fakerc") {
			std::cout << "Faking reconnect" << std::endl;
			FakeLeave(_nick, _jid);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			FakeJoin(_nick, _jid);
			std::cout << "Done" << std::endl;
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

void ConsoleClient::SendMessage(const std::string &message, const std::string &recipient)
{
	std::cout << "Bot> " << message << std::endl;
}

void ConsoleClient::FakeMessage(const std::string &message)
{
	ChatMessage msg;
	msg._body = message;
	msg._nick = _nick;
	msg._isAdmin = true;
	_handler->OnMessage(msg);
}

void ConsoleClient::FakeJoin(const std::string &nick, const std::string &jid)
{
	_handler->OnPresence(nick, jid, true, nick);
}

void ConsoleClient::FakeLeave(const std::string &nick, const std::string &jid)
{
	_handler->OnPresence(nick, jid, false, "");
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
