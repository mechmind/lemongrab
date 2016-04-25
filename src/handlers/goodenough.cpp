#include "goodenough.h"

#include "util/stringops.h"

#include <list>
#include <algorithm>

namespace {
	std::string response = "https://youtu.be/WgYhYw-lS_s";
}

LemonHandler::ProcessingResult GoodEnough::HandleMessage(const std::string &from, const std::string &body)
{
	static std::list<std::string> magicPhrases = {
		"так сойдет",
		"так сойдёт",
		"пока так",
		"потом поправлю",
		"good enough"
	};

	auto lowercase = toLower(body);

	for (auto &phrase : magicPhrases)
	{
		auto loc = lowercase.find(phrase);
		if (loc != lowercase.npos)
		{
			SendMessage(from + ": " + response);
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

	test.HandleMessage("TestUser", "This test shoud be good enough");
	EXPECT_TRUE(testbot._success);
}

#endif // LCOV_EXCL_STOP

