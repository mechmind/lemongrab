#pragma once

#include "lemonhandler.h"

class GoodEnough : public LemonHandler
{
public:
	GoodEnough(LemonBot *bot) : LemonHandler("goodenough", bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};
