#pragma once

#include "lemonhandler.h"

#include <list>

class Pager : public LemonHandler
{
public:
	Pager(LemonBot *bot) : LemonHandler("pager", bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	bool HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	class Message
	{
	public:
		Message(std::string to, std::string text)
			: _recepient(to)
			, _text(text)
		{ }

		std::string _recepient;
		std::string _text;
	};

	std::list<Message> _messages;
};
