#include "pager.h"

#include <map>

#include "util/stringops.h"

LemonHandler::ProcessingResult Pager::HandleMessage(const std::string &from, const std::string &body)
{
	// TODO: preserve messages between restarts via leveldb?
	if (body == "!pager_stats")
	{
		PrintPagerStats();
		return ProcessingResult::StopProcessing;
	}

	std::string args;
	if (!getCommandArguments(body, "!pager", args))
		return ProcessingResult::KeepGoing;

	size_t space = args.find(' ');
	if (space == args.npos)
	{
		SendMessage(GetHelp());
		return ProcessingResult::StopProcessing;
	}

	auto recepient = args.substr(0, space);
	auto pagerMessage = args.substr(space + 1);
	StoreMessage(recepient, from, pagerMessage);

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
			_messages.erase(message++);
		} else if(message->_expiration < std::chrono::system_clock::now()) {
			SendMessage("Message for " + message->_recepient + " (" + message->_text + ") has expired");
			_messages.erase(message++);
		} else {
			++message;
		}
	}
}

const std::string Pager::GetVersion() const
{
	return "0.1";
}

const std::string Pager::GetHelp() const
{
	return "Use !pager %jid% %message% or !pager %nick% %message%. Paged messages are lost on bot restart or after 72 hours. Use !pager_stats to get number of paged messages";
}

void Pager::StoreMessage(const std::string &to, const std::string &from, const std::string &text)
{
	_messages.emplace_back(to, from + ": " + text);
}

void Pager::PrintPagerStats()
{
	std::map<std::string, int> messageStats;
	for (auto &message : _messages)
		++messageStats[message._recepient];

	std::string messageStatsString;
	for (auto &recepient : messageStats)
		messageStatsString.append(" " + recepient.first + " (" + std::to_string(recepient.second) + ")");

	if (messageStatsString.empty())
		messageStatsString = " none";

	SendMessage("Paged messages:" + messageStatsString);
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

class PagerTestBot : public LemonBot
{
public:
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

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(1, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.at(0));

	pager.HandleMessage("Bob", "!pager Alice test");
	EXPECT_EQ(1, testbot._received.size());

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(1, testbot._received.size());

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(1, testbot._received.size());

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(2, testbot._received.size());
	EXPECT_EQ("Paged messages: Alice (1)", testbot._received.at(1));

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(3, testbot._received.size());
	EXPECT_EQ("Alice! You have a message >> Bob: test", testbot._received.at(2));

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(4, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.at(3));
}

TEST(PagerTest, MsgByJidCheckPresenseHandling)
{
	PagerTestBot testbot;
	Pager pager(&testbot);

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(1, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.at(0));

	pager.HandleMessage("Bob", "!pager alice@jabber.com test");
	EXPECT_EQ(1, testbot._received.size());

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(1, testbot._received.size());

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(1, testbot._received.size());

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(2, testbot._received.size());
	EXPECT_EQ("Paged messages: alice@jabber.com (1)", testbot._received.at(1));

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(3, testbot._received.size());
	EXPECT_EQ("Alice! You have a message >> Bob: test", testbot._received.at(2));

	pager.HandleMessage("Bob", "!pager_stats");
	EXPECT_EQ(4, testbot._received.size());
	EXPECT_EQ("Paged messages: none", testbot._received.at(3));
}

#endif // LCOV_EXCL_STOP
