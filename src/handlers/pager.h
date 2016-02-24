#pragma once

#include "lemonhandler.h"

#include <list>
#include <chrono>

class Pager : public LemonHandler
{
public:
	Pager(LemonBot *bot) : LemonHandler("pager", bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	bool HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	void PrintPagerStats();

private:
	class Message
	{
	public:
		Message(std::string to, std::string text)
			: _recepient(to)
			, _text(text)
			, _expiration(std::chrono::system_clock::now() + std::chrono::hours(72))
		{ }

		std::string _recepient;
		std::string _senderJID;
		std::string _text;

		std::chrono::system_clock::time_point _expiration;
	};

	std::list<Message> _messages;
};
