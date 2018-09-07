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
			//FIXME: empty ID map crashes the bot
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
            rclientSafeSend(msg._body, "");
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
        rclientSafeSend(result, "");
		return LemonHandler::ProcessingResult::StopProcessing;
	}

	if ((msg._body == "!jabber" || msg._body == "!xmpp")
			&& msg._module_name == "discord") {
		//SendMessage(_botPtr->GetOnlineUsers());
        rclientSafeSend("\n" + _botPtr->GetOnlineUsers()
                        + "\n`this message is invisible to xmpp users to avoid highlighting`",
                        msg._discordChannel);
		return ProcessingResult::StopProcessing;
	}

	return LemonHandler::ProcessingResult::KeepGoing;
}

Discord::~Discord()
{
	if (_isEnabled) {
		gclient->disconnect();
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

void Discord::rclientSafeSend(const std::string &message, const std::string &channel)
{
	if (!_isEnabled)
		return;

	const auto text = sanitizeDiscord(message);

	try {
		if (text.size() < 200) {
            rclient->sendTextMessage(channel.empty() ? Hexicord::Snowflake(_channelID) : Hexicord::Snowflake(channel), text);
		} else {
			for (size_t i = 0; i <= text.length(); i+=200) {
				auto cutText = text.substr(i, 200);
                rclient->sendTextMessage(channel.empty() ? Hexicord::Snowflake(_channelID) : Hexicord::Snowflake(channel), cutText);
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	} catch (Hexicord::RESTError &e) {
		LOG(ERROR) << "Failed to deliver message \"" << message << "\" via REST, REST error: " << e.what();
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to deliver message \"" << message << "\" via REST: " << e.what();
	}
}

void Discord::createRole(Hexicord::Snowflake guildid, const std::string &rolename, const Hexicord::Snowflake &userid)
{
	if (rolename == "dwarf")
		return;

	try {
		std::string roleid;

		auto roles = rclient->getRoles(guildid);
		for (const auto &role : roles) {
			if (role["name"].get<std::string>() == rolename) {
				roleid = role["id"].get<std::string>();
				LOG(INFO) << "Role " << rolename << " found: " << roleid;
				break;
			}
		}

		if (roleid.empty()) {
			// create new role
			nlohmann::json newRole;
			newRole["name"] = rolename;
			newRole["mentionable"] = true;

			std::ostringstream o;
			o << guildid.value;

			auto url = "https://discordapp.com/api/guilds/" + o.str() + "/roles";
			cpr::Response response = cpr::Post(cpr::Url{url},
											   cpr::Header{{"Authorization"
														   , "Bot " + rclient->token}},
											   cpr::Body{newRole.dump()});

			LOG(WARNING) << response.status_code;
			LOG(WARNING) << response.text;

			nlohmann::json newroleresult = nlohmann::json::parse(response.text);

			//auto newroleresult = rclient->createRole(guildid, newRole);
			roleid = newroleresult["id"].get<std::string>();
		}

		if (roleid.empty()) {
			LOG(WARNING) << "Couldn't create another role!";
			return;
		};

		Hexicord::Snowflake roleidflake(roleid);
		auto member = rclient->getMember(guildid, userid);
		auto oldroles = member["roles"];

		oldroles.emplace_back(roleid);

		std::ostringstream o;
		o << guildid.value;

		std::ostringstream u;
		u << userid.value;

		auto url = "https://discordapp.com/api/guilds/" + o.str() + "/members/" + u.str();

		nlohmann::json newrolestuff;
		newrolestuff["roles"] = oldroles;

		cpr::Response response = cpr::Patch(cpr::Url{url},
										   cpr::Header{{"Authorization"
													   , "Bot " + rclient->token},
											{"Content-Type", "application/json"}},
										  cpr::Body{newrolestuff.dump()});

		LOG(INFO) << "Role added:" << response.status_code;

		//rclient->setMemberRoles(guildid, userid, newRoles);
	} catch (nlohmann::json::parse_error& e) {
		LOG(ERROR) << "Message: " << e.what()
				   << "Id: " << e.id
				   << "Byte: " << e.byte;
	} catch (std::exception &e) {
		LOG(ERROR) << e.what();
	}
}

bool Discord::Init()
{
	auto botToken = GetRawConfigValue("discord.token");
	_selfWebhook = GetRawConfigValue("discord.webhookid");
	auto ownerIdStr = GetRawConfigValue("discord.owner");
	Hexicord::Snowflake ownerId = ownerIdStr.empty() ? Hexicord::Snowflake() : Hexicord::Snowflake(ownerIdStr);

	auto guildIdStr = GetRawConfigValue("discord.guild");
	Hexicord::Snowflake guildId = guildIdStr.empty() ? Hexicord::Snowflake() : Hexicord::Snowflake(guildIdStr);

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

	rclient.reset(new Hexicord::RestClient(botToken));
	auto gatewayurl = rclient->getGatewayUrlBot();
	std::cout << "Gateway URL: " << gatewayurl.first << std::endl;


	gclient.reset(new Hexicord::GatewayClient(botToken));

	gclient->eventDispatcher.addHandler(Hexicord::Event::Ready, [&](const nlohmann::json& json) {
		_myID = json["user"]["id"].get<std::string>();
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

	gclient->eventDispatcher.addHandler(Hexicord::Event::GuildCreate, [&](const nlohmann::json& json) {
		try {
			auto channels = json["channels"];
			for (const auto &channel : channels) {
				const auto channelID = Hexicord::Snowflake(channel["id"].get<std::string>());
				const auto channelName = channel["name"].get<std::string>();
				_channels[channelID] = channelName;
				LOG(INFO) << "Channel " << channelID << " has name " << channelName;
			};
		} catch (...) {

		}
	});

	gclient->eventDispatcher.addHandler(Hexicord::Event::MessageCreate, [&](const nlohmann::json& json) {
		try
		{
			Hexicord::Snowflake senderId(json["author"].count("id") ? json["author"]["id"].get<std::string>()
				: json["author"]["webhook_id"].get<std::string>());

			auto id = json["author"]["id"].get<std::string>();

			// Avoid responing to messages of bot.
			if (senderId == Hexicord::Snowflake(_myID)
					|| senderId == Hexicord::Snowflake(_selfWebhook)) return;

			std::string text = json["content"];

			std::string args;
			if (getCommandArguments(text, "!sub", args)) {
				createRole(guildId, args, Hexicord::Snowflake(id));
			}

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
				text.append("\n" + embed.value<std::string>("title", "<no title>"));
			}

			const auto channel = _channels.find(Hexicord::Snowflake(json["channel_id"].get<std::string>()));
			std::string channelName;
			if (channel != _channels.end() && Hexicord::Snowflake(_channelID) != (*channel).first) {
				channelName = "[#" + (*channel).second + "] ";
			}


			ChatMessage jabberTunneledMessage;
			jabberTunneledMessage._body = channelName + "<" + _users[id]._nick + "> " + text;
			jabberTunneledMessage._origin = ChatMessage::Origin::Discord;
			this->SendMessage(jabberTunneledMessage);

			ChatMessage msg;
			msg._nick = _users[id]._nick;
			msg._body = text;
			msg._jid = _users[id]._username;
			msg._isAdmin = senderId == ownerId;
			msg._isPrivate = false;
			msg._hasDiscordEmbed = hasEmbeds;
			msg._origin = ChatMessage::Origin::Discord;
            msg._discordChannel = json["channel_id"].get<std::string>();
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

	gclient->init(rclient->getGatewayUrlBot().first,
				  Hexicord::GatewayClient::NoSharding, Hexicord::GatewayClient::NoSharding,
 {
					  { "since", nullptr   },
					  { "status", "online" },
					  { "game", {{ "name", "LEMONGRAB"}, { "type", 0 }}},
					  { "afk", false }
				  });
	gclient->connect();

    LOG(INFO) << "Connected to discord";

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

void Discord::SendToDiscord(const std::string &text, const std::string &discordChannel)
{
    rclientSafeSend(text, discordChannel);
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

