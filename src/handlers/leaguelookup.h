#pragma once

#include <list>
#include <unordered_map>

#include "lemonhandler.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

namespace Json
{
	class Value;
}

class Summoner
{
public:
	std::string name;
	bool team;
	std::string champion;
	std::string summonerSpell1;
	std::string summonerSpell2;
};

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
	std::list<Summoner> getSummonerNamesFromJSON(const Json::Value &root);

	bool InitializeChampions();
	bool InitializeSpells();

private:
	std::unordered_map<int, std::string> _champions;
	std::unordered_map<int, std::string> _spells;

	std::string _region;
	std::string _platformID;
	std::string _apiKey;

#ifdef _BUILD_TESTS
	FRIEND_TEST(LeagueLookupTest, PlayerList);
	FRIEND_TEST(LeagueLookupTest, SummonerByNameJson);
#endif
};

