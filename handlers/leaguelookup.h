#ifndef LEAGUELOOKUP_H
#define LEAGUELOOKUP_H

#include "lemonhandler.h"

class LeagueLookup : public LemonHandler
{
public:
	LeagueLookup(LemonBot *bot) : LemonHandler(bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};

#endif // LEAGUELOOKUP_H
