#pragma once

#include <list>

#include "lemonhandler.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

namespace Json
{
	class Value;
}

class LeagueLookup : public LemonHandler
{
public:
	LeagueLookup(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	enum class RiotAPIResponse
	{
		OK,
		NotFound,
		UnexpectedResponseCode,
		InvalidJSON,
	};

	RiotAPIResponse RiotAPIRequest(const std::string &request, Json::Value &output);

	std::string lookupCurrentGame(const std::string &name);
	int getSummonerIDFromName(const std::string &name);
	int getSummonerIdFromJSON(const std::string &name, const Json::Value &root);
	std::list<std::string> getSummonerNamesFromJSON(const Json::Value &root);

private:
	std::string _region;
	std::string _platformID;
	std::string _apiKey;

#ifdef _BUILD_TESTS
	FRIEND_TEST(LeagueLookupTest, PlayerList);
	FRIEND_TEST(LeagueLookupTest, SummonerByNameJson);
#endif
};

