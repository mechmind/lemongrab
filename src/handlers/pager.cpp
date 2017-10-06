#include "pager.h"

#include <map>
#include <algorithm>
#include <memory>

#include "util/stringops.h"

#include <glog/logging.h>

Pager::Message::Message(const DB::PagerMsg &dbmsg)
	: _id(dbmsg.id)
	, _recepient(dbmsg.recepient)
	, _text(dbmsg.message)
{
	try {
		_expiration = std::chrono::system_clock::from_time_t(dbmsg.timepoint);
	} catch (std::exception &e) {
		LOG(ERROR) << "Exception: " << e.what();
		_recepient.clear();
	}
}

Pager::Message::Message(const std::string &to, const std::string &text)
	: _recepient(to)
	, _text(text)
	, _expiration(std::chrono::system_clock::now() + std::chrono::hours(72))
{
	std::replace(_text.begin(), _text.end(), '\n', ' ');
}

bool Pager::Message::operator==(const Pager::Message &rhs)
{
	return _id == rhs._id
			&& _text == rhs._text
			&& _recepient == rhs._recepient
			&& _expiration == rhs._expiration;
}

Pager::Pager(LemonBot *bot)
	: LemonHandler("pager", bot)
{
	RestoreMessages();
}

LemonHandler::ProcessingResult Pager::HandleMessage(const ChatMessage &msg)
{
	if (msg._body == "!pager_stats")
	{
		SendMessage(GetPagerStats());
		return ProcessingResult::StopProcessing;
	}

	std::string args;
	if (!getCommandArguments(msg._body, "!pager", args))
		return ProcessingResult::KeepGoing;

	size_t space = args.find(' ');
	if (space == args.npos)
	{
		SendMessage(GetHelp());
		return ProcessingResult::StopProcessing;
	}

	auto recepient = args.substr(0, space);
	auto pagerMessage = args.substr(space + 1);
	StoreMessage(recepient, msg._nick, pagerMessage);
	SendMessage(msg._nick + ": message stored");

	return ProcessingResult::StopProcessing;
}

void Pager::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	if (_messages.empty())
		return;

	if (!connected)
		return;

	auto message = _messages.begin();
	while (message != _messages.end())
	{
		if (message->_recepient == jid
				|| (from.find('@') == from.npos && message->_recepient == from))
		{
			SendMessage(from + "! You have a message >> " + message->_text);
			PurgeMessageFromDB(message->_id);
			_messages.erase(message++);
		} else if(message->_expiration < std::chrono::system_clock::now()) {
			SendMessage("Message for " + message->_recepient + " (" + message->_text + ") has expired");
			PurgeMessageFromDB(message->_id);
			_messages.erase(message++);
		} else {
			++message;
		}
	}
}

const std::string Pager::GetHelp() const
{
	return "Use !pager %jid% %message% or !pager %nick% %message%. Paged messages are lost after 72 hours. Use !pager_stats to get number of paged messages";
}

void Pager::RestoreMessages()
{
	for (auto &msg : getStorage().get_all<DB::PagerMsg>())
	{
		_messages.emplace_back(msg);
	}
}

void Pager::StoreMessage(const std::string &to, const std::string &from, const std::string &text)
{
	auto msgtext = from + ": " + text;
	_messages.emplace_back(to, msgtext);

	DB::PagerMsg newMsg = { -1, to, msgtext, static_cast<int>(std::chrono::system_clock::to_time_t(_messages.back()._expiration)) };
	try {
		getStorage().insert(newMsg);
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to save message: " << e.what();
	}
}

void Pager::PurgeMessageFromDB(long long id)
{
	try {
		getStorage().remove<DB::PagerMsg>(id);
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to purge pager message with id: " << e.what() << id;
	}
}

std::string Pager::GetPagerStats() const
{
	std::map<std::string, int> messageStats;
	for (auto &message : _messages)
		++messageStats[message._recepient];

	std::string messageStatsString;
	for (auto &recepient : messageStats)
		messageStatsString.append(" " + recepient.first + " (" + std::to_string(recepient.second) + ")");

	if (messageStatsString.empty())
		messageStatsString = " none";

	return "Paged messages:" + messageStatsString;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

class PagerTestBot : public LemonBot
{
public:
	PagerTestBot() : LemonBot(":memory:") {
		_storage.sync_schema();
	}

	void SendMessage(const std::string &text)
	{
		_received.push_back(text);
	}

	std::vector<std::string> _received;
};

TEST(PagerTest, MsgByNickCheckPresenseHandling)
{
	PagerTestBot testbot;
	Pager pager(&testbot);

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(1, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.back());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager Alice test", false));
	EXPECT_EQ(2, testbot._received.size());

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(2, testbot._received.size());

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(2, testbot._received.size());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(3, testbot._received.size());
	EXPECT_EQ("Paged messages: Alice (1)", testbot._received.back());

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(4, testbot._received.size());
	EXPECT_EQ("Alice! You have a message >> Bob: test", testbot._received.back());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(5, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.back());
}

TEST(PagerTest, MsgByJidCheckPresenseHandling)
{
	PagerTestBot testbot;
	Pager pager(&testbot);

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(1, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.back());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager alice@jabber.com test", false));
	EXPECT_EQ(2, testbot._received.size());

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(2, testbot._received.size());

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(2, testbot._received.size());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(3, testbot._received.size());
	EXPECT_EQ("Paged messages: alice@jabber.com (1)", testbot._received.back());

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(4, testbot._received.size());
	EXPECT_EQ("Alice! You have a message >> Bob: test", testbot._received.back());

	pager.HandleMessage(ChatMessage("Bob", "", "", "!pager_stats", false));
	EXPECT_EQ(5, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.back());
}

#endif // LCOV_EXCL_STOP
