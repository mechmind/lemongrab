#ifndef STATUSREPORTER_H
#define STATUSREPORTER_H

#include "lemonhandler.h"

class StatusReporter : public LemonHandler
{
public:
	StatusReporter(LemonBot *bot) : LemonHandler(bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};

#endif // STATUSREPORTER_H
