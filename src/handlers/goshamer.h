#pragma once

#include "lemonhandler.h"

class GoShamer : public LemonHandler
{
public:
	GoShamer(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
private:
	std::string _shamedJid;
};
