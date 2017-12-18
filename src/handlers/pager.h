#pragma once

#include "lemonhandler.h"

#include <list>
#include <chrono>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class Pager : public LemonHandler
{
public:
	explicit Pager(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	void HandlePresence(const std::string &from, const std::string &jid, bool connected) override;
	const std::string GetHelp() const override;

private:
	void RestoreMessages();
	void StoreMessage(const std::string &to, const std::string &from, const std::string &text);
	void PurgeMessageFromDB(long long id);
	std::string GetPagerStats() const;

private:
	class Message
	{
	public:
		explicit Message(const DB::PagerMsg &dbmsg);
		Message(const std::string &to, const std::string &text);

		inline bool operator==(const Message &rhs);

		long long _id = 0;
		std::string _recepient;
		std::string _text;

		std::chrono::system_clock::time_point _expiration;
	};

	std::list<Message> _messages;

#ifdef _BUILD_TESTS
	FRIEND_TEST(PagerTest, MsgByNickCheckPresenseHandling);
	FRIEND_TEST(PagerTest, MsgByJidCheckPresenseHandling);
	FRIEND_TEST(PagerTest, MessageSerializer);
#endif
};
