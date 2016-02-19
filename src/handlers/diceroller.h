#pragma once

#include "lemonhandler.h"

class DiceRoller : public LemonHandler
{
public:
	DiceRoller(LemonBot *bot) : LemonHandler("dice", bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;
};
