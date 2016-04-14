#include "lemonhandler.h"

LemonHandler::LemonHandler(const std::string &moduleName, LemonBot *bot)
	: _moduleName(moduleName)
	, _botPtr(bot)
{

}

LemonHandler::~LemonHandler()
{

}

const std::string LemonHandler::GetHelp() const
{
	return "This module has no commands";
}

const std::string &LemonHandler::GetName() const
{
	return _moduleName;
}

void LemonHandler::SendMessage(const std::string &text)
{
	if (_botPtr)
		_botPtr->SendMessage(text);
}

const std::string LemonHandler::GetRawConfigValue(const std::string &name) const
{
	return _botPtr ? _botPtr->GetRawConfigValue(name) : "";
}
