#pragma once

#include "lemonhandler.h"

#include "util/persistentmap.h"

#include <list>
#include <chrono>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class Pager : public LemonHandler
{
public:
	Pager(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	void HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetHelp() const;

private:
	void StoreMessage(const std::string &to, const std::string &from, const std::string &text);
	void PurgeMessageFromDB(long long id);
	void PrintPagerStats();

private:
	class Message
	{
	public:
		Message(const std::string &serializedInput, long long id);
		Message(std::string to, std::string text, long long id);

		bool isValid();
		inline bool operator==(const Message &rhs);

		long long _id = 0;
		std::string _recepient;
		std::string _text;

		std::chrono::system_clock::time_point _expiration;

		std::string Serialize();
	};

	long long _lastId = 0;
	std::list<Message> _messages;

	LevelDBPersistentMap _persistentMessages;

#ifdef _BUILD_TESTS
	FRIEND_TEST(PagerTest, MsgByNickCheckPresenseHandling);
	FRIEND_TEST(PagerTest, MsgByJidCheckPresenseHandling);
	FRIEND_TEST(PagerTest, MessageSerializer);
#endif
};
