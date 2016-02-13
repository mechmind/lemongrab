#pragma once

#include "lemonhandler.h"

class UrlPreview : public LemonHandler
{
public:
	UrlPreview(LemonBot *bot) : LemonHandler(bot) {}
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
};
