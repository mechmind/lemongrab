#include "leaguelookup.h"

#include <glog/logging.h>
#include <curl/curl.h>

#include "util/stringops.h"

// TODO: move to separate module code from here and urlpreview

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}

static std::pair<std::string, long> CurlRequest(std::string url)
{
	std::pair<std::string, long> response = {"", -1};
	CURL *curl = nullptr;
	curl = curl_easy_init();
	if (!curl)
		return {"curl Fail", -1};

	// TODO Handle return codes
	std::string readBuffer;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	if (curl_easy_perform(curl) == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.second);

	curl_easy_cleanup(curl);


	return response;
}


LeagueLookup::LeagueLookup(LemonBot *bot)
	: LemonHandler("leaugelookup", bot)
{
	if (bot)
	{
		_apiKey = bot->GetRawConfigValue("RiotApiKey");
		_region = bot->GetRawConfigValue("RiotRegion");
		_platformID = bot->GetRawConfigValue("RiotPlatform");
	}

	if (_apiKey.empty())
		LOG(WARNING) << "Riot API Key is not set";

	if (_region.empty() || _platformID.empty())
	{
		LOG(WARNING) << "Region / platform are not set, defaulting to EUNE";
		_region = "eune";
		_platformID = "EUN1";
	}
}

LemonHandler::ProcessingResult LeagueLookup::HandleMessage(const std::string &from, const std::string &body)
{
	if (_apiKey.empty())
		return ProcessingResult::KeepGoing;

	std::string args;
	if (getCommandArguments(body, "!findsummoner", args))
	{
		SendMessage(lookupCurrentGame(args));
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string LeagueLookup::GetVersion() const
{
	return "0.1";
}

std::string LeagueLookup::lookupCurrentGame(const std::string &name)
{
	auto id = getSummonerIDFromName(name);
	if (id == -1)
		return "Summoner not found";

	std::string apiRequest = "https://" + _region + ".api.pvp.net/observer-mode/rest/consumer/getSpectatorGameInfo/"
			+ _platformID + "/" + std::to_string(id) + "?api_key=" + _apiKey;
	auto apiResponse = CurlRequest(apiRequest);

	if (apiResponse.second == 404)
		return "Summoner is not currently in game";

	if (apiResponse.second != 200)
	{
		LOG(ERROR) << "RiotAPI unexpected response: " << apiResponse.second;
		return "Something went horribly wrong (response code " + std::to_string(apiResponse.second) + ")";
	}

	auto strResponse = apiResponse.first;

	Json::Reader reader;
	Json::Value parsedResponse;

	if (!reader.parse(strResponse.data(), strResponse.data() + strResponse.size(), parsedResponse))
		LOG(WARNING) << "Failed to parse JSON: " << strResponse;

	auto players = getSummonerNamesFromJSON(parsedResponse);

	std::string message = "Currently playing: { ";
	for (auto player : players)
		message += player + " ";
	message += "}";

	return message;
}

int LeagueLookup::getSummonerIDFromName(const std::string &name)
{
	std::string apiRequest = "https://" + _region + ".api.pvp.net/api/lol/" + _region + "/v1.4/summoner/by-name/" + name + "?api_key=" + _apiKey;
	auto apiResponse = CurlRequest(apiRequest);
	if (apiResponse.second == 404)
		return -1;

	if (apiResponse.second != 200)
	{
		LOG(ERROR) << "RiotAPI unexpected response: " << apiResponse.second;
		return -1;
	}

	auto strResponse = apiResponse.first;

	Json::Reader reader;
	Json::Value parsedResponse;

	if (!reader.parse(strResponse.data(), strResponse.data() + strResponse.size(), parsedResponse))
		LOG(WARNING) << "Failed to parse JSON: " << strResponse;

	return getSummonerIdFromJSON(name, parsedResponse);
}

int LeagueLookup::getSummonerIdFromJSON(const std::string &name, const Json::Value &root)
{
	return root[toLower(name)]["id"].asInt();
}

std::list<std::string> LeagueLookup::getSummonerNamesFromJSON(const Json::Value &root)
{
	std::list<std::string> result;

	auto participants = root["participants"];
	if (!participants.isArray())
		return result;

	auto partNum = participants.size();

	for (Json::ArrayIndex index = 0; index < partNum; index++)
	{
		auto summonerName = participants[index]["summonerName"].asString();
		result.push_back(summonerName);
	}

	return result;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

#include <fstream>
#include <vector>
#include <iostream>

TEST(LeagueLookupTest, JsonParser)
{
	LeagueLookup t(nullptr);

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

	auto got = res.begin();
	auto want = expectedNames.begin();

	while (got != res.end())
	{
		EXPECT_EQ(*want, *got);
		++got;
		++want;
	}
}

TEST(LeagueLookupTest, SummonerByNameJson)
{
	LeagueLookup t(nullptr);

	std::ifstream apiresponse("test/summonerByName.json");
	std::vector<char> responseStr((std::istreambuf_iterator<char>(apiresponse)),
								  std::istreambuf_iterator<char>());

	Json::Value tmp;
	Json::Reader reader;

	ASSERT_TRUE(reader.parse(responseStr.data(), responseStr.data() + responseStr.size(), tmp));

	EXPECT_EQ(20374680, t.getSummonerIdFromJSON("JazzUSCM", tmp));
}

#endif // LCOV_EXCL_STOP
