#include "lemonhandler.h"

LemonHandler::LemonHandler(const std::string &moduleName, LemonBot *bot)
	: _moduleName(moduleName)
	, _botPtr(bot)
{

}

LemonHandler::~LemonHandler()
{

}

const std::string &LemonHandler::GetName() const
{
	return _moduleName;
}

void LemonHandler::SendMessage(const std::string &text)
{
	_botPtr->SendMessage(text);
}

const std::string LemonHandler::GetRawConfigValue(const std::string &name) const
{
	return _botPtr->GetRawConfigValue(name);
}
