#include "leaguelookup.h"

#include <glog/logging.h>
#include <json/reader.h>
#include <json/value.h>
#include <cpr/cpr.h>
#include <cpr/util.h>

#include "util/stringops.h"
#include "util/thread_util.h"

#include <chrono>
#include <thread>

LeagueLookup::LeagueLookup(LemonBot *bot)
	: LemonHandler("leaugelookup", bot)
{
	if (bot)
	{
		_api._key = bot->GetRawConfigValue("LOL.ApiKey");
		_api._region = bot->GetRawConfigValue("LOL.Region");
	}

	if (_api._key.empty())
	{
		LOG(WARNING) << "Riot API Key is not set";
		return;
	}

	if (!InitializeChampions() || !InitializeSpells())
		LOG(ERROR) << "Failed to initialize static Riot API data";

	if (_api._region.empty())
	{
		LOG(WARNING) << "Region / platform are not set, defaulting to EUNE";
		_api._region = "eun1";
	}
}

LeagueLookup::~LeagueLookup()
{
	if (_lookupHelper && _lookupHelper->joinable())
		_lookupHelper->join();
}

LemonHandler::ProcessingResult LeagueLookup::HandleMessage(const ChatMessage &msg)
{
	if (_api._key.empty())
		return ProcessingResult::KeepGoing;

	std::string args;
	if (getCommandArguments(msg._body, "!ll", args))
	{
		if (!args.empty())
			SendMessage(lookupCurrentGame(args));
		else
		{
			// FIXME: use future/promise here to avoid locking on multiple calls
			if (_lookupHelper && _lookupHelper->joinable())
				_lookupHelper->join();

			_lookupHelper = std::make_shared<std::thread>([&]{ LookupAllSummoners(this, _api); });
			nameThread(*_lookupHelper.get(), "League Lookup worker");
		}
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!addsummoner", args))
	{
		SendMessage(AddSummoner(args));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!delsummoner", args))
	{
		DeleteSummoner(args);
		return ProcessingResult::StopProcessing;
	}

	if (msg._body == "!listsummoners")
	{
		SendMessage(ListSummoners());
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string LeagueLookup::GetHelp() const
{
	return "!ll %summonername% - check if summoner is currently in game\n"
			"!ll without arguments - look up every summoner on the watchlist. This may take a while\n"
			"!addsummoner %id% - add summoner to watchlist\n"
			"!delsummoner %id% - remove summoner from watchlist\n"
		   "!listsummoners - list watchlist content";
}

LeagueLookup::RiotAPIResponse LeagueLookup::RiotAPIRequest(const std::string &request, Json::Value &output)
{
	auto apiResponse = cpr::Get(request);

	switch (apiResponse.status_code)
	{
	case 403:
		return RiotAPIResponse::AccessDenied;
	case 404:
		return RiotAPIResponse::NotFound;
	case 429:
		LOG(ERROR) << "Rate limit reached!";
		return RiotAPIResponse::RateLimitReached;
	case 200:
	{
		const auto &strResponse = apiResponse.text;

		Json::Reader reader;
		if (!reader.parse(strResponse, output))
		{
			LOG(ERROR) << "Failed to parse JSON: " << strResponse;
			return RiotAPIResponse::InvalidJSON;
		}

		return RiotAPIResponse::OK;
	}
	default:
		LOG(ERROR) << "RiotAPI unexpected response: " << apiResponse.status_code;
		return RiotAPIResponse::UnexpectedResponseCode;
	}
}

std::string LeagueLookup::lookupCurrentGame(const std::string &name) const
{
	auto id = getSummonerIDFromName(name);
	if (id == -1)
		return "Summoner not found";

	if (id == -2)
		return "Something went horribly wrong";

	std::string apiRequest = "https://" + _api._region + ".api.riotgames.com/lol/spectator/v3/active-games/by-summoner/" + std::to_string(id) + "?api_key=" + _api._key;

	Json::Value response;

	switch (RiotAPIRequest(apiRequest, response))
	{
	case RiotAPIResponse::NotFound:
		return "Summoner is not currently in game";
	case RiotAPIResponse::RateLimitReached:
		return "Rate limit reached";
	case RiotAPIResponse::AccessDenied:
		return "Access denied";
	case RiotAPIResponse::UnexpectedResponseCode:
		return "RiotAPI returned unexpected return code";
	case RiotAPIResponse::InvalidJSON:
		return "RiotAPI returned invalid JSON";
	case RiotAPIResponse::OK:
	{
		auto players = getSummonerNamesFromJSON(response);

		std::string message = "Currently playing:\n";
		std::string teamA;
		std::string teamB;

		for (const auto &player : players)
		{
			auto &team = player._team ? teamA : teamB;
			team += player._name + " -> " + player._champion + " (" + player._summonerSpell1 + " & " + player._summonerSpell2 + ")\n";
		}

		message += "= Red Team  =\n" + teamA + "\n";
		message += "= Blue Team =\n" + teamB;

		return message;
	}
	}

	return "This should never happen";
}

int LeagueLookup::getSummonerIDFromName(const std::string &name) const
{
	std::string apiRequest = "https://" + _api._region + ".api.riotgames.com/lol/summoner/v3/summoners/by-name/" + cpr::util::urlEncode(name) + "?api_key=" + _api._key;

	Json::Value response;

	switch (RiotAPIRequest(apiRequest, response))
	{
	case RiotAPIResponse::NotFound:
		return -1;
	case RiotAPIResponse::AccessDenied:
	case RiotAPIResponse::RateLimitReached:
	case RiotAPIResponse::UnexpectedResponseCode:
	case RiotAPIResponse::InvalidJSON:
		return -2;
	case RiotAPIResponse::OK:
		return response["id"].asInt();
	}

	return -2;
}

std::list<Summoner> LeagueLookup::getSummonerNamesFromJSON(const Json::Value &root) const
{
	std::list<Summoner> result;

	const auto &participants = root["participants"];
	if (!participants.isArray())
		return result;

	for (Json::ArrayIndex index = 0; index < participants.size(); index++)
	{
		Summoner summoner;
		summoner._name = participants[index]["summonerName"].asString();
		try {
			summoner._champion = _champions.at(participants[index]["championId"].asInt());
			summoner._summonerSpell1 = _spells.at(participants[index]["spell1Id"].asInt());
			summoner._summonerSpell2 = _spells.at(participants[index]["spell2Id"].asInt());
		} catch (...) {
			LOG(WARNING) << "Unknown Champion ID or Spell ID";
		}

		summoner._team = participants[index]["teamId"].asInt() == 100;
		result.push_back(summoner);
	}

	return result;
}

bool LeagueLookup::InitializeChampions()
{
	std::string apiRequest = "https://ddragon.leagueoflegends.com/cdn/6.24.1/data/en_US/champion.json";

	Json::Value response;
	if (RiotAPIRequest(apiRequest, response) != RiotAPIResponse::OK)
		return false;

	const auto &data = response["data"];
	try {
		for(auto champion = data.begin() ; champion != data.end() ; champion++)
			_champions[(*champion)["id"].asInt()] = (*champion)["name"].asString() + ", " + (*champion)["title"].asString();
	} catch (Json::Exception &e) {
		LOG(ERROR) << "Error parsing Champion data: " << e.what();
	}

	return true;
}

bool LeagueLookup::InitializeSpells()
{
	std::string apiRequest = "https://ddragon.leagueoflegends.com/cdn/6.24.1/data/en_US/summoner.json";

	Json::Value response;
	if (RiotAPIRequest(apiRequest, response) != RiotAPIResponse::OK)
		return false;

	const auto &data = response["data"];
	try {
		for(auto spell = data.begin() ; spell != data.end() ; spell++)
			_spells[(*spell)["id"].asInt()] = (*spell)["name"].asString();
	} catch (Json::Exception &e) {
		LOG(ERROR) << "Error parsing Spell data: " << e.what();
	}

	return true;
}

std::string LeagueLookup::GetSummonerNameByID(const std::string &id) const
{
	std::string apiRequest = "https://" + _api._region + ".api.riotgames.com/lol/summoner/v3/summoners/" + id + "?api_key=" + _api._key;

	Json::Value response;
	if (RiotAPIRequest(apiRequest, response) != RiotAPIResponse::OK)
		return "";

	return response["name"].asString();
}

void LeagueLookup::LookupAllSummoners(LeagueLookup *_parent, ApiOptions &api)
{
	std::list<std::string> inGame;
	std::list<std::string> broken;

	using namespace sqlite_orm;
	for (auto &summoner : _parent->getStorage().get_all<DB::LLSummoner>(limit(maxSummoners)))
	{
		std::string apiRequest = "https://" + api._region + ".api.riotgames.com/lol/spectator/v3/active-games/by-summoner/"
				+ std::to_string(summoner.summonerID) + "?api_key=" + api._key;

		Json::Value response;
		switch (RiotAPIRequest(apiRequest, response))
		{
		case RiotAPIResponse::NotFound:
			break;
		case RiotAPIResponse::AccessDenied:
			_parent->SendMessage("Access denied");
			return;
		case RiotAPIResponse::RateLimitReached:
			_parent->SendMessage("Rate limit reached");
			return;
		case RiotAPIResponse::UnexpectedResponseCode:
		case RiotAPIResponse::InvalidJSON:
			broken.push_back(summoner.nickname);
			break;
		case RiotAPIResponse::OK:
			inGame.push_back(summoner.nickname);
			break;
		}

		// to avoid breaking dev api key rates
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::string output;
	if (inGame.empty())
		output = "No one is playing";
	else
	{
		output = "Currently in game:";
		for (const auto &summoner : inGame)
			output += " " + summoner;
	}

	if (!broken.empty())
	{
		output += " | Following lookups failed:";
		for (const auto &summoner : broken)
			output += " " + summoner;
	}

	_parent->SendMessage(output);
}

std::string LeagueLookup::AddSummoner(const std::string &id)
{
	auto summonerID = from_string<int>(id);
	if (!summonerID) {
		return "Invalid ID";
	}

	auto name = GetSummonerNameByID(id);
	if (name.empty()) {
		return "Summoner not found";
	}

	DB::LLSummoner newSummoner = { -1, *summonerID, name };

	if (getStorage().insert(newSummoner))
		return "Summoner with ID " + id + " added as " + name;
	else
		return "Failed to add summoner to database";
}

void LeagueLookup::DeleteSummoner(const std::string &id)
{
	using namespace sqlite_orm;
	try {
		getStorage().remove_all<DB::LLSummoner>(where(is_equal(&DB::LLSummoner::summonerID,
															   from_string<int>(id).value_or(0))));
		SendMessage("Summoner deleted");
	} catch (std::exception &e) {
		LOG(ERROR) << e.what();
	}
}

std::string LeagueLookup::ListSummoners()
{
	std::string output;

	for (auto &summoner : getStorage().get_all<DB::LLSummoner>())
		output.append(std::to_string(summoner.summonerID) + " : " + summoner.nickname + "\n");

	if (output.empty())
		return "Watchlist is empty";

	return output;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <iostream>

bool operator==(const std::string &lhs, const Summoner &rhs)
{
	return rhs._name == lhs;
}

class LeagueLookupBot : public LemonBot
{
public:
	LeagueLookupBot() : LemonBot(":memory:") {
		_storage.sync_schema();
	}

	void SendMessage(const std::string &text)
	{

	}
};

TEST(LeagueLookupTest, PlayerList)
{
	LeagueLookupBot b;
	LeagueLookup t(&b);

	std::ifstream apiresponse("test/getSpectatorGameInfo.json");
	std::vector<char> responseStr((std::istreambuf_iterator<char>(apiresponse)),
								  std::istreambuf_iterator<char>());

	Json::Value tmp;
	Json::Reader reader;

	std::list<std::string> expectedNames = {
		"Rito Xalean",
		"ephiphonee",
		"cilla16",
		"LaaaNce",
		"Rito Asepta",
		"TR10 EsOToxic ",
		"LYLsky",
		"ArtOfOneShoots",
		"PH SkidRow",
		"saulens437",
	};

	ASSERT_TRUE(reader.parse(responseStr.data(), responseStr.data() + responseStr.size(), tmp));

	auto res = t.getSummonerNamesFromJSON(tmp);
	ASSERT_EQ(expectedNames.size(), res.size());
	EXPECT_TRUE(std::equal(expectedNames.begin(), expectedNames.end(), res.begin()));
}

#endif // LCOV_EXCL_STOP
