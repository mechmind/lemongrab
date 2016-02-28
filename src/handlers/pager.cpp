#include "pager.h"

#include <map>

bool Pager::HandleMessage(const std::string &from, const std::string &body)
{
	if (body.find("!pager_stats") == 0)
	{
		PrintPagerStats();
		return true;
	}

	// TODO: preserve messages between restarts via leveldb?
	if (body.length() < 7 || body.substr(0, 6) != "!pager")
		return false;

	auto input = body.substr(7);
	size_t space = input.find(' ');
	if (space == input.npos)
	{
		SendMessage(GetHelp());
		return true;
	}

	auto to = input.substr(0, space);
	auto text = input.substr(space + 1);
	StoreMessage(to, from, text);

	return true;
}

bool Pager::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	if (connected && !_messages.empty())
	{
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
	return false;
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
		_recieved = (text == "Alice! You have a message >> Bob: test");
	}

	bool _recieved = false;
};

TEST(PagerTest, MsgByNickCheckPresenseHandling)
{
	PagerTestBot testbot;
	Pager pager(&testbot);

	pager.StoreMessage("Alice", "Bob", "test");
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(true, testbot._recieved);
}

TEST(PagerTest, MsgByJidCheckPresenseHandling)
{
	PagerTestBot testbot;
	Pager pager(&testbot);

	pager.StoreMessage("alice@jabber.com", "Bob", "test");
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Alice", "alice@jabber.com", false);
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Bob", "bob@jabber.com", true);
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("alice@jabber.com", "john@jabber.com", true);
	EXPECT_EQ(false, testbot._recieved);

	pager.HandlePresence("Alice", "alice@jabber.com", true);
	EXPECT_EQ(true, testbot._recieved);
}

#endif // LCOV_EXCL_STOP
