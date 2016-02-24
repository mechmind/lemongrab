#include "pager.h"

#include <map>

bool Pager::HandleMessage(const std::string &from, const std::string &body)
{
	if (body.find("!pager_stats") == 0)
	{
		PrintPagerStats();
		return true;
	}

	// TODO: preserve messages between restarts via leveldb?
	if (body.length() < 7 || body.substr(0, 6) != "!pager")
		return false;

	auto input = body.substr(7);
	size_t space = input.find(' ');
	if (space == input.npos)
	{
		SendMessage(GetHelp());
		return true;
	}

	_messages.emplace_back(input.substr(0, space), from + ": " + input.substr(space + 1));

	return true;
}

bool Pager::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	if (connected && !_messages.empty())
	{
		auto message = _messages.begin();
		while (message != _messages.end())
		{
			if (message->_recepient == from || message->_recepient == jid)
			{
				SendMessage(from + "! You have a message >> " + message->_text);
				_messages.erase(message++);
			} else if(message->_expiration < std::chrono::system_clock::now()) {
				SendMessage("Message for " + message->_recepient + " (" + message->_text + ") has expired");
				_messages.erase(message++);
			} else {
				++message;
			}
		}
	}
	return false;
}

const std::string Pager::GetVersion() const
{
	return GetName() + ": 0.1";
}

const std::string Pager::GetHelp() const
{
	return "Use !pager %jid% %message% or !pager %nick% %message%. Paged messages are lost on bot restart or after 72 hours. Use !pager_stats to get number of paged messages";
}

void Pager::PrintPagerStats()
{
	std::map<std::string, int> messageStats;
	for (auto &message : _messages)
		++messageStats[message._recepient];

	std::string messageStatsString;
	for (auto &recepient : messageStats)
		messageStatsString.append(" " + recepient.first + " (" + std::to_string(recepient.second) + ")");

	if (messageStatsString.empty())
		messageStatsString = " none";

	SendMessage("Paged messages:" + messageStatsString);
}
