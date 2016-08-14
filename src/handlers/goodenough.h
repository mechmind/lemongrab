#pragma once

#include "lemonhandler.h"

class GoodEnough : public LemonHandler
{
public:
	GoodEnough(LemonBot *bot) : LemonHandler("goodenough", bot) {}
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
};
