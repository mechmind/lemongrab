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

private:
	std::random_device m_rnd;
	std::mt19937_64 m_gen;
};

#endif // DICEROLLER_H
