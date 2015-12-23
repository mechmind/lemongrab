#ifndef DICEROLLER_H
#define DICEROLLER_H

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

#endif // DICEROLLER_H
