#include "statusreporter.h"

#include <sstream>

bool StatusReporter::HandleMessage(const std::string &from, const std::string &body)
{
	if (body == "getversion")
	{
		std::stringstream reply;
		reply << from << ": ";

		SendMessage(reply.str());
		return true;
	}
	return false;
}

const std::string StatusReporter::GetVersion() const
{
	return "Status: 0.1";
}
