#pragma once

/*
 * DiceRoller module
 *
 * Syntax:
 * .XdY
 *
 */

#include <random>

#include "lemonhandler.h"

class DiceRoller : public LemonHandler
{
public:
	DiceRoller(LemonBot *bot) : LemonHandler(bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};
