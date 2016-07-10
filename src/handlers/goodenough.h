#pragma once

#include "lemonhandler.h"

class GoodEnough : public LemonHandler
{
public:
	GoodEnough(LemonBot *bot) : LemonHandler("goodenough", bot) {}
	ProcessingResult HandleMessage(const std::string &from, const std::string &body) override;
};
