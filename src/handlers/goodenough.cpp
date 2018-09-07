#include "goodenough.h"

#include "util/stringops.h"

#include <list>
#include <algorithm>

namespace {
	std::string response = "https://youtu.be/WgYhYw-lS_s";
}

LemonHandler::ProcessingResult GoodEnough::HandleMessage(const ChatMessage &msg)
{
	static std::list<std::string> magicPhrases = {
		"так сойдет",
		"так сойдёт",
		"пока так",
		"потом поправлю",
		"good enough"
	};

	auto lowercase = toLower(msg._body);

	for (auto &phrase : magicPhrases)
	{
		auto loc = lowercase.find(phrase);
		if (loc != lowercase.npos)
		{
            SendMessage(msg._nick + ": " + response, msg._discordChannel);
			return ProcessingResult::StopProcessing;
		}
	}

	return ProcessingResult::KeepGoing;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

class GoodEnoughBot : public LemonBot
{
public:
	GoodEnoughBot() : LemonBot(":memory:") { }

	void SendMessage(const std::string &text)
	{
		_success = (text == "TestUser: " + response);
	}

	bool _success = false;
};

TEST(GoodEnoughTest, SimpleTrigger)
{
	GoodEnoughBot testbot;
	GoodEnough test(&testbot);

	test.HandleMessage(ChatMessage("TestUser", "", "This test shoud be good enough", false));
	EXPECT_TRUE(testbot._success);
}

#endif // LCOV_EXCL_STOP

