#include "pager.h"

bool Pager::HandleMessage(const std::string &from, const std::string &body)
{
	// TODO: preserve messages between restarts via leveldb
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
				SendMessage(from + ": " + message->_text);
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
	return GetName() + ": beta";
}

const std::string Pager::GetHelp() const
{
	return "Use !pager %jid% %message%";
}
