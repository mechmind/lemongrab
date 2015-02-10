#include "lastseen.h"

bool LastSeen::HandleMessage(const std::string &from, const std::string &body)
{
	if (body.substr(0, 5) == "!seen")
	{
		SendMessage("YUP");
		return true;
	}
	return false;
}

const std::string LastSeen::GetVersion() const
{
	return "LastSeen: 0.1";
}
