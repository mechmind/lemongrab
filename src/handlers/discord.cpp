#include "discord.h"

#include <thread>

#include <cstdlib>
#include <iostream>
#include <hexicord/gateway_client.hpp>
#include <hexicord/rest_client.hpp>

#include <glog/logging.h>
#include "util/stringops.h"

#include <cpr/cpr.h>

#include <boost/algorithm/string/replace.hpp>

Discord::Discord(LemonBot *bot)
	: LemonHandler("discord", bot)
{

}

LemonHandler::ProcessingResult Discord::HandleMessage(const ChatMessage &msg)
{
	if (msg._module_name != GetName()) {
		if (!_webhookURL.empty()) {

			auto mappedId = GetRawConfigValue("discord.idmap", msg._jid);

			std::string avatar_url;
			if (!_users[mappedId]._avatar.empty()) {
				avatar_url = "https://cdn.discordapp.com/avatars/" + mappedId + "/" + _users[mappedId]._avatar + ".png";
			}
			cpr::Post(cpr::Url{_webhookURL},
					  cpr::Payload{
						  {"username", msg._nick},
						  {"content", sanitizeDiscord(msg._body)},
						  {"avatar_url", avatar_url}
					  });

		} else if (_channelID != 0) {
			rclientSafeSend(msg._body);
		}
	}

	if (msg._body == "!discord") {
		std::string result = "Discord users:";

		for (const auto &user : _users) {
			if (!user.second._status.empty()
					&& user.second._status != "offline") {
				result += "\n" + user.second._nick;
				if (user.second._status != "idle") {
					result += " (" + user.second._status + ")";
				}
			}
		}

		SendMessage(result);
		rclientSafeSend(result);
		return LemonHandler::ProcessingResult::StopProcessing;
	}

	if ((msg._body == "!jabber" || msg._body == "!xmpp")
			&& msg._module_name == "discord") {
		//SendMessage(_botPtr->GetOnlineUsers());
		rclientSafeSend("\n" + _botPtr->GetOnlineUsers()
						+ "\n`this message is invisible to xmpp users to avoid highlighting`");
		return ProcessingResult::StopProcessing;
	}

	return LemonHandler::ProcessingResult::KeepGoing;
}

Discord::~Discord()
{
	if (_isEnabled) {
		gclient->disconnect();
		ioService.stop();

		_clientThread.join();
	}
}

std::string Discord::sanitizeDiscord(const std::string &input)
{
	std::string output = input;
	boost::algorithm::replace_all(output, "\\", "\\\\");

	if (output.find('@') != output.npos) {
		for (auto user : _users) {
			if (!user.second._nick.empty()) {
				LOG(INFO) << "Replacing @" + user.second._nick + "with <@!" + user.first + ">";
				boost::algorithm::replace_all(output, "@" + user.second._nick, "<@!" + user.first + ">");
			}
		}
	}

	return output;
}

void Discord::rclientSafeSend(const std::string &message)
{
	if (!_isEnabled)
		return;

	const auto text = sanitizeDiscord(message);

	if (text.size() < 200) {
		rclient->sendTextMessage(_channelID, text);
	} else {
		for (size_t i = 0; i <= text.length(); i+=200) {
			rclient->sendTextMessage(_channelID, text.substr(i, 200));
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

bool Discord::Init()
{
	auto botToken = GetRawConfigValue("discord.token");
	_selfWebhook = GetRawConfigValue("discord.webhookid");
	auto ownerIdStr = GetRawConfigValue("discord.owner");
	Hexicord::Snowflake ownerId = ownerIdStr.empty() ? Hexicord::Snowflake() : Hexicord::Snowflake(ownerIdStr);

	//auto guildIdStr = GetRawConfigValue("discord.guild");
	//Hexicord::Snowflake guildId = guildIdStr.empty() ? Hexicord::Snowflake() : Hexicord::Snowflake(guildIdStr);

	auto channelIdStr = GetRawConfigValue("discord.channel");
	_webhookURL = GetRawConfigValue("discord.webhook");

	if (botToken.empty()) {
		LOG(WARNING) << "Bot token is empty, discord module disabled";
		return false;
	}

	if (auto channelID = from_string<std::uint64_t>(channelIdStr)) {
		if (*channelID == 0) {
			LOG(WARNING) << "Invalid channel ID, discord disabled";
			return false;
		}

		_channelID = *channelID;
	} else {
		LOG(WARNING) << "ChannelID is not set, discord disabled";
		return false;
	}

	gclient.reset(new Hexicord::GatewayClient(ioService, botToken));
	rclient.reset(new Hexicord::RestClient(ioService, botToken));

	_clientThread = std::thread([=]() noexcept
	{
		nlohmann::json me;
		gclient->eventDispatcher.addHandler(Hexicord::Event::Ready, [&me](const nlohmann::json& json) {
			me = json["user"];
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::GuildMemberUpdate, [&](const nlohmann::json& json) {
			try {
				auto id = json["user"]["id"].get<std::string>();
				auto nickname = json["user"]["username"].get<std::string>();
				if (json["nick"].is_string()) {
					nickname = json["nick"].get<std::string>();
				}

				_users[id]._nick = nickname;
				LOG(INFO) << "User " << id << " nick is set to " << nickname;
				if (json["user"]["avatar"].is_string()) {
					_users[id]._avatar = json["user"]["avatar"].get<std::string>();
				}
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::MessageCreate, [&](const nlohmann::json& json) {
			try {
				Hexicord::Snowflake senderId(json["author"].count("id") ? json["author"]["id"].get<std::string>()
					: json["author"]["webhook_id"].get<std::string>());

				auto id = json["author"]["id"].get<std::string>();

				// Avoid responing to messages of bot.
				if (senderId == Hexicord::Snowflake(me["id"].get<std::string>())
						|| senderId == Hexicord::Snowflake(_selfWebhook)) return;

				std::string text = json["content"];

				auto mentions =  json["mentions"];
				for (const auto &mention : mentions) {
					// FIXME: apparently mentions can have multiple formats? Can't find info on it anywhere
					boost::algorithm::replace_all(text, "<@!" + mention["id"].get<std::string>() + ">", "@" + _users[mention["id"].get<std::string>()]._nick);
					boost::algorithm::replace_all(text, "<@" + mention["id"].get<std::string>() + ">", "@" + _users[mention["id"].get<std::string>()]._nick);
				}

				auto attachements = json["attachments"];
				for (const auto &attachment : attachements) {
					text.append("\n[ " + attachment["url"].get<std::string>() + " ]");
				}

				bool hasEmbeds = false;
				auto embeds = json["embeds"];
				for (const auto &embed : embeds) {
					hasEmbeds = true;
					text.append("\n" + embed["title"].get<std::string>());
				}

				this->SendMessage("<" + _users[id]._nick + "> " + text);

				ChatMessage msg;
				msg._nick = _users[id]._nick;
				msg._body = text;
				msg._jid = _users[id]._username;
				msg._isAdmin = senderId == ownerId;
				msg._isPrivate = false;
				msg._hasDiscordEmbed = hasEmbeds;
				this->TunnelMessage(msg);
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::GuildMemberAdd, [&](const nlohmann::json& json) {
			try {
				auto id = json["user"]["id"].get<std::string>();
				auto nickname = json["user"]["username"].get<std::string>();
				if (json["nick"].is_string()) {
					nickname = json["nick"].get<std::string>();
				}

				_users[id]._nick = nickname;
				LOG(INFO) << "User " << id << " nick is set to " << nickname;

				if (json["user"]["avatar"].is_string()) {
					_users[id]._avatar = json["user"]["avatar"].get<std::string>();
				}
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::GuildMemberRemove, [&](const nlohmann::json& json) {
			try {
				auto id = json["user"]["id"].get<std::string>();
				_users.erase(id);
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::PresenceUpdate, [&](const nlohmann::json& json) {
			try {
				auto id = json["user"]["id"].get<std::string>();

				if (json["status"].is_string()) {
					_users[id]._status = json["status"].get<std::string>();
				}
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		gclient->eventDispatcher.addHandler(Hexicord::Event::GuildCreate, [&](const nlohmann::json& json) {
			try {
				auto members = json["members"];

				for (auto member : members) {
					auto id = member["user"]["id"].get<std::string>();

					if (member["nick"].is_string()) {
						_users[id]._nick = member["nick"].get<std::string>();
						LOG(INFO) << "User " << id << " nick is set to " << _users[id]._nick;
					} else {
						_users[id]._nick = member["user"]["username"].get<std::string>();
						LOG(INFO) << "User " << id << " nick is set to " << _users[id]._nick;
					};

					_users[id]._username = member["user"]["username"].get<std::string>() + "#" + member["user"]["discriminator"].get<std::string>();
					if (member["user"]["avatar"].is_string()) {
						_users[id]._avatar = member["user"]["avatar"].get<std::string>();
					}
				}
			} catch (std::exception &e) {
				LOG(ERROR) << e.what();
			}
		});

		bool noErrors = true;
		do {
			try {
				noErrors = true;
				gclient->connect(rclient->getGatewayUrlBot().first,
								 Hexicord::GatewayClient::NoSharding, Hexicord::GatewayClient::NoSharding,
				{
									 { "since", nullptr   },
									 { "status", "online" },
									 { "game", {{ "name", "LEMONGRAB"}, { "type", 0 }}},
									 { "afk", false }
								 });
				ioService.run();
			} catch (Hexicord::GatewayError &e) {
				LOG(ERROR) << "Gateway error: " << e.what();
			} catch (std::exception &e) {
				noErrors = false;
				LOG(ERROR) << "ioService error: " << e.what();
				gclient->disconnect();
				LOG(ERROR) << "Reconnecting in 10 seconds";
				std::this_thread::sleep_for(std::chrono::seconds(10));
			};
		} while (!noErrors);

		LOG(INFO) << "Discord thread terminated";
	});

	_isEnabled = true;
	return true;
}

void Discord::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{

}

const std::string Discord::GetHelp() const
{
	return "!discord - list current discord users online\n"
		   "!jabber - list current jabber users online (works only in discord)";
}

void Discord::SendToDiscord(std::string text)
{
	rclientSafeSend(text);
}


#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

#include <set>

class DiscordTestBot : public LemonBot
{
public:
	DiscordTestBot() : LemonBot(":memory:") { }

	void SendMessage(const std::string &text)
	{
		lastMSG = text;
	}

	int _lastResult = 0;

	std::string lastMSG;
};

TEST(DiscordTest, XMPP2DiscordNickTest)
{
	DiscordTestBot bot;
	Discord d(&bot);

	d._users["112233"]._nick = "You";

	EXPECT_EQ("<@!112233>: hello", d.sanitizeDiscord("@You: hello"));
	EXPECT_EQ("Hello, <@!112233>", d.sanitizeDiscord("Hello, @You"));
}

TEST(DiscordTest, DiscordSanitizeTest)
{
	DiscordTestBot bot;
	Discord d(&bot);
	EXPECT_EQ("Double\\\\slash", d.sanitizeDiscord("Double\\slash"));
}

#endif // LCOV_EXCL_STOP

