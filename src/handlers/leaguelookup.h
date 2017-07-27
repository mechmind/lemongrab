#pragma once

#include <list>
#include <unordered_map>
#include <thread>

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
	std::string _name;
	bool _team;
	std::string _champion;
	std::string _summonerSpell1;
	std::string _summonerSpell2;
};

class ApiOptions
{
public:
	std::string _region;
	std::string _key;
};

class LeagueLookup : public LemonHandler
{
public:
	LeagueLookup(LemonBot *bot);
	~LeagueLookup() override;
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

private:
	enum class RiotAPIResponse
	{
		OK,
		NotFound,
		AccessDenied,
		UnexpectedResponseCode,
		RateLimitReached,
		InvalidJSON,
	};

	static RiotAPIResponse RiotAPIRequest(const std::string &request, Json::Value &output);

	std::string lookupCurrentGame(const std::string &name) const;
	int getSummonerIDFromName(const std::string &name) const;
	std::list<Summoner> getSummonerNamesFromJSON(const Json::Value &root) const;

	bool InitializeChampions();
	bool InitializeSpells();
	std::string GetSummonerNameByID(const std::string &id) const;

	static void LookupAllSummoners(LeagueLookup *_parent, ApiOptions &api);
	std::string AddSummoner(const std::string &id);
	void DeleteSummoner(const std::string &id);
	std::string ListSummoners();
private:
	std::shared_ptr<std::thread> _lookupHelper;
	std::unordered_map<int, std::string> _champions;
	std::unordered_map<int, std::string> _spells;

	static constexpr int maxSummoners = 500;

	ApiOptions _api;

#ifdef _BUILD_TESTS
	FRIEND_TEST(LeagueLookupTest, PlayerList);
	FRIEND_TEST(LeagueLookupTest, SummonerByNameJson);
#endif
};

