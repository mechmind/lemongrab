#include "leaguelookup.h"

#include <list>
#include <sstream>

#include <curl/curl.h>
#include <jsoncpp/json/json.h>

// Curl support
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::string CurlRequest(std::string url)
{
	CURL* curl = NULL;
	curl = curl_easy_init();
	if (!curl)
		return "curl Fail";

	std::string readBuffer;
	int res;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return readBuffer;
}
//

static std::map<std::string, int> Tiers = {{"CHALLENGER", 1},
										   {"MASTER", 2},
										   {"DIAMOND", 3},
										   {"PLATINUM", 4},
										   {"GOLD", 5},
										   {"SILVER", 6},
										   {"BRONZE", 7},
										   {"UNRANKED", 8}};

static std::map<std::string, int> Divisions = {{"I", 1},
											   {"II", 2},
											   {"III", 3},
											   {"IV", 4},
											   {"V", 5},
											   {"-", 6}};

class League
{
public:
	League()
	// FIXME
	{
		Tier = "UNRANKED";
		Division = "-";
		Points = 0;
	}

	std::string Tier;
	std::string Division;
	int Points;

public:
	std::string Print()
	{
		std::string response;
		response = Tier;
		response.append(" ");
		response.append(Division);
		response.append(" (");
		response.append(std::to_string(Points));
		response.append(" points)");
		return response;
	}

	bool operator> (League &l2)
	{
		int tier1 = Tiers[Tier];
		int tier2 = Tiers[l2.Tier];

		int div1 = Divisions[Division];
		int div2 = Divisions[l2.Division];

		if (tier1 < tier2)
			return true;

		if (tier1 > tier2)
			return false;

		if (div1 < div2)
			return true;

		if (div1 > div2)
			return false;

		return Points > l2.Points;
	}
};

class Player
{
public:
	std::string name;
	long id;
};

class JsonHelper
{
public:
	long GetGameID(const std::string &request)
	{
		Json::Value root;
		bool parsedSuccess = reader.parse(request, root, false);

		if(!parsedSuccess)
			return -1;

		Json::Value lastGameID = root["TEAM-88a9c224-9aae-4caf-973f-7631f7b2c5c1"]["matchHistory"][0]["gameId"];
		return lastGameID.asInt64();
	}

	std::list<Player> GetPlayers(const std::string &request)
	{
		std::list<Player> players;
		Json::Value root;
		bool parsedSuccess = reader.parse(request, root, false);

		if(!parsedSuccess)
		{
			return players;
		}

		Json::Value participants = root["participantIdentities"];

		for (int i = 0; i<participants.size(); ++i)
		{
			Json::Value player = participants[i];
			Player playerOBJ;
			playerOBJ.name = player["player"]["summonerName"].asString();
			playerOBJ.id = player["player"]["summonerId"].asInt64();

			if (playerOBJ.id != 27965258 && playerOBJ.id != 22174585 && playerOBJ.id != 20374680)
			{
				players.push_back(playerOBJ);
			}
		}

		return players;
	}

	std::string GetPlayerInfo(const std::string &request, long PlayerID)
	{
		Json::Value root;
		bool parsedSuccess = reader.parse(request, root, false);

		if(!parsedSuccess)
			return "fail";

		Json::Value player = root[std::to_string(PlayerID)];

		std::map<std::string, League> leagues;

		for (int i = 0; i < player.size(); ++i)
		{
			League league;
			league.Tier = player[i]["tier"].asString();
			league.Division = player[i]["entries"][0]["division"].asString();
			league.Points = player[i]["entries"][0]["leaguePoints"].asInt();
			League &old_league = leagues[player[i]["queue"].asString()];
			if (!(old_league > league))
				old_league = league;
		}
		std::string response;
		response.append("{ 3v3: ");
		response.append(leagues["RANKED_TEAM_3x3"].Print());
		response.append(" | SOLO: ");
		response.append(leagues["RANKED_SOLO_5x5"].Print());
		response.append(" | 5v5: ");
		response.append(leagues["RANKED_TEAM_5x5"].Print());
		response.append(" }");
		return response;
	}

private:
	Json::Reader reader;
};

bool LeagueLookup::HandleMessage(const std::string &from, const std::string &body)
{
	if (body != "!lolec")
		return false;

	JsonHelper json;

	const std::string teamID = "TEAM-88a9c224-9aae-4caf-973f-7631f7b2c5c1";

	std::string teamurl="https://eune.api.pvp.net/api/lol/eune/v2.4/team/";
	teamurl.append(teamID);
	teamurl.append("?api_key=");
	teamurl.append(GetRawConfigValue("RiotApiKey"));
	std::string teamRequest = CurlRequest(teamurl);

	std::string GameID = std::to_string(json.GetGameID(teamRequest));

	std::string matchurl="https://eune.api.pvp.net/api/lol/eune/v2.2/match/";
	matchurl.append(GameID);
	matchurl.append("?api_key=");
	matchurl.append(GetRawConfigValue("RiotApiKey"));

	std::string matchRequest = CurlRequest(matchurl);

	auto players = json.GetPlayers(matchRequest);

	std::stringstream response;
	response << "Last opponents: " << std::endl;
	for (auto it : players)
	{
		std::string playerurl="https://eune.api.pvp.net/api/lol/eune/v2.5/league/by-summoner/";
		playerurl.append(std::to_string(it.id));
		playerurl.append("/entry");
		playerurl.append("?api_key=");
		playerurl.append(GetRawConfigValue("RiotApiKey"));

		std::string playerRequest = CurlRequest(playerurl);
		response << it.name << " :: " << json.GetPlayerInfo(playerRequest, it.id) << std::endl;
	}

	SendMessage(response.str());

	return true;
}

const std::string LeagueLookup::GetVersion() const
{
	return "LeagueLookup: 0.1";
}

