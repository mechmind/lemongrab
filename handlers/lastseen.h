#ifndef LASTSEEN_H
#define LASTSEEN_H

#include "lemonhandler.h"

class LastSeen : public LemonHandler
{
public:
	LastSeen(LemonBot *bot) : LemonHandler(bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};

#endif // LASTSEEN_H
