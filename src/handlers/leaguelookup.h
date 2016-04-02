#pragma once

#include <json/json.h>

#include <list>

#include "lemonhandler.h"

class LeagueLookup : public LemonHandler
{
public:
	LeagueLookup(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;

	// private:
	std::string lookupCurrentGame(const std::string &name);
	int getSummonerIDFromName(const std::string &name);
	int getSummonerIdFromJSON(const std::string &name, const Json::Value &root);
	std::list<std::string> getSummonerNamesFromJSON(const Json::Value &root);

private:
	std::string _region;
	std::string _platformID;
	std::string _apiKey;
};

