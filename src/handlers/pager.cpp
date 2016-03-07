#include "pager.h"

#include <map>
#include <iostream>
#include <algorithm>
#include <memory>

#include "util/stringops.h"

#include <leveldb/db.h>

Pager::Message::Message(const std::string &serializedInput, long long id)
{
	auto tokens = tokenize(serializedInput, '\n');
	if (tokens.size() != 3)
		return;

	_id = id;
	_recepient = tokens.at(0);
	_text = tokens.at(1);
	std::string expirationStr = tokens.at(2);
	try {
		time_t expirationTime = std::stoull(expirationStr);
		_expiration = std::chrono::system_clock::from_time_t(expirationTime);
	} catch (std::exception &e) {
		std::cout << "Something broke: " << e.what() << std::endl;
		_recepient.clear();
		return;
	}
}

std::string Pager::Message::Serialize()
{
	time_t expiration = std::chrono::system_clock::to_time_t(_expiration);
	return _recepient + "\n"
			+ _text + "\n"
			+ std::to_string(expiration);
}


Pager::Message::Message(std::string to, std::string text, long long id)
	: _id(id)
	, _recepient(to)
	, _text(text)
	, _expiration(std::chrono::system_clock::now() + std::chrono::hours(72))
{
	std::replace(_text.begin(), _text.end(), '\n', ' ');
}

bool Pager::Message::isValid()
{
	return !_recepient.empty();
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
	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::DB *persistentMessages = nullptr;
	leveldb::Status status = leveldb::DB::Open(options, "db/pager", &persistentMessages);
	if (!status.ok())
		std::cerr << status.ToString() << std::endl;
	_persistentMessages.reset(persistentMessages);

	if (!_persistentMessages)
	{
		std::cout << "Database connection error: pager" << std::endl;
		return;
	}

	std::shared_ptr<leveldb::Iterator> it(_persistentMessages->NewIterator(leveldb::ReadOptions()));
	int loadedMessages = 0;
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		try {
			_lastId = std::stoll(it->key().ToString());
		} catch (std::exception &e) {
			std::cout << "Invalid pager id: " << it->key().ToString() << std::endl;
			_persistentMessages->Delete(leveldb::WriteOptions(), it->key().ToString());
			continue;
		}

		Message m(it->value().ToString(), _lastId);
		if (!m.isValid())
		{
			std::cout << "Invalid message: " << it->value().ToString() << std::endl;
			_persistentMessages->Delete(leveldb::WriteOptions(), it->key().ToString());
		}
		else
		{
			_messages.push_back(m);
			loadedMessages++;
		}
	}
	std::cout << "Loaded " << std::to_string(loadedMessages) << " message(s) for pager" << std::endl;
}

LemonHandler::ProcessingResult Pager::HandleMessage(const std::string &from, const std::string &body)
{
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

const std::string Pager::GetVersion() const
{
	return "0.1";
}

const std::string Pager::GetHelp() const
{
	return "Use !pager %jid% %message% or !pager %nick% %message%. Paged messages are lost after 72 hours. Use !pager_stats to get number of paged messages";
}

void Pager::StoreMessage(const std::string &to, const std::string &from, const std::string &text)
{
	_messages.emplace_back(to, from + ": " + text, ++_lastId);
	if (_persistentMessages)
		_persistentMessages->Put(leveldb::WriteOptions(), std::to_string(_lastId), _messages.back().Serialize());
	else
		std::cout << "Database connection error: pager" << std::endl;
}

void Pager::PurgeMessageFromDB(long long id)
{
	if (_persistentMessages)
		_persistentMessages->Delete(leveldb::WriteOptions(), std::to_string(id));
	else
		std::cout << "Database connection error: pager" << std::endl;
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

TEST(PagerTest, MessageSerializer)
{
	Pager::Message m("Bob", "Hello!", 0);
	m._expiration = std::chrono::system_clock::from_time_t(1234567);
	auto m_s = m.Serialize();
	Pager::Message m2(m_s, 0);
	EXPECT_TRUE(m2.isValid());
	EXPECT_TRUE(m2 == m);

	Pager::Message m3("What\nis\nthis\nthing?", 1);
	EXPECT_TRUE(!m3.isValid());
}

#endif // LCOV_EXCL_STOP
