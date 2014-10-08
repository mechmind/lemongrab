#include "lemonhandler.h"

LemonHandler::LemonHandler(LemonBot *bot)
	: m_Bot(bot)
{

}

LemonHandler::~LemonHandler()
{

}

void LemonHandler::SendMessage(const std::string &text)
{
	m_Bot->SendMessage(text);
}
