#pragma once

#include <list>
#include <unordered_map>
#include <thread>

#include "lemonhandler.h"
#include "util/persistentmap.h"

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

class apiOptions
{
public:
	std::string region;
	std::string platformID;
	std::string key;
};

class LeagueLookup : public LemonHandler
{
public:
	LeagueLookup(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetHelp() const;

private:
	enum class RiotAPIResponse
	{
		OK,
		NotFound,
		UnexpectedResponseCode,
		RateLimitReached,
		InvalidJSON,
	};

	static RiotAPIResponse RiotAPIRequest(const std::string &request, Json::Value &output);

	std::string lookupCurrentGame(const std::string &name);
	int getSummonerIDFromName(const std::string &name);
	int getSummonerIdFromJSON(const std::string &name, const Json::Value &root);
	std::list<Summoner> getSummonerNamesFromJSON(const Json::Value &root);

	bool InitializeChampions();
	bool InitializeSpells();
	std::string GetSummonerNameByID(const std::string &id);

	static void LookupAllSummoners(PersistentMap &starredSummoners, LeagueLookup *_parent, apiOptions &api);
	void AddSummoner(const std::string &id);
	void DeleteSummoner(const std::string &id);
	void ListSummoners();
private:
	std::shared_ptr<std::thread> _lookupHelper;
	std::unordered_map<int, std::string> _champions;
	std::unordered_map<int, std::string> _spells;

	static constexpr int maxSummoners = 500;

	apiOptions _api;
	PersistentMap _starredSummoners;

#ifdef _BUILD_TESTS
	FRIEND_TEST(LeagueLookupTest, PlayerList);
	FRIEND_TEST(LeagueLookupTest, SummonerByNameJson);
#endif
};

