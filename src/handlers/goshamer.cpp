#include "goshamer.h"

#include "util/stringops.h"

#include <algorithm>
#include <regex>
#include <vector>

namespace {
	const std::string defaultShamedJid = "deirz";
	const std::vector<std::string> shamefulChunks = {"го"};
}

GoShamer::GoShamer(LemonBot *bot)
	: LemonHandler("goshamer", bot)
	, _shamedJid(defaultShamedJid)
{
}

LemonHandler::ProcessingResult GoShamer::HandleMessage(const ChatMessage &msg)
{
	if (toLower(msg._jid).find(_shamedJid) == std::string::npos)
	{
		return ProcessingResult::KeepGoing;
	}

	const auto words = tokenize(toLower(msg._body), ' ');

	const auto it = std::find_if(words.begin(), words.end(),
	[](const auto &word)
	{
		return std::any_of(shamefulChunks.begin(), shamefulChunks.end(),
		[&word](const auto &chunk)
		{
			return (word.size() != chunk.size()) && (word.find(chunk) != std::string::npos);
		});
	});

	if (it == words.end())
	{
		return ProcessingResult::KeepGoing;
	}

	auto processedWord = *it;
	for (const auto &chunk : shamefulChunks)
	{
		processedWord = std::regex_replace(processedWord, std::regex(chunk), toUpper(chunk));
	}

	SendMessage(msg._nick + ": " + processedWord);
	return ProcessingResult::StopProcessing;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

class GoShamerBot : public LemonBot
{
public:
	GoShamerBot() : LemonBot(":memory:") { }

	void SendMessage(const std::string &text);
	std::vector<std::string> _received;
};

void GoShamerBot::SendMessage(const std::string &text)
{
	_received.push_back(text);
}

TEST(GoShamerTest, JidFiltering)
{
	GoShamerBot testbot;
	GoShamer test(&testbot);

	test.HandleMessage(ChatMessage("", "TestUser", "", "погоня", false));
	test.HandleMessage(ChatMessage("", "", "", "погоня", false));
	EXPECT_EQ(testbot._received.size(), 0);
	test.HandleMessage(ChatMessage("", "deirz", "", "погоня", false));
	EXPECT_EQ(testbot._received.size(), 1);
	test.HandleMessage(ChatMessage("", "gGg__420_golord_dEiRz_420_gGg", "", "погоня", false));
	EXPECT_EQ(testbot._received.size(), 2);
}

TEST(GoShamerTest, Messages)
{
	GoShamerBot testbot;
	GoShamer test(&testbot);

	test.HandleMessage(ChatMessage("", "deirz", "", "погоня", false));
	ASSERT_EQ(testbot._received.size(), 1);
	EXPECT_EQ(testbot._received.back(), ": поГОня");

	test.HandleMessage(ChatMessage("", "deirz", "", "а вот и нету", false));
	EXPECT_EQ(testbot._received.size(), 1);

	test.HandleMessage(ChatMessage("", "deirz", "", "Очень много многоногих", false));
	EXPECT_EQ(testbot._received.size(), 2);
	EXPECT_EQ(testbot._received.back(), ": мноГО");

	test.HandleMessage(ChatMessage("", "deirz", "", "Слово оогоооогооо", false));
	EXPECT_EQ(testbot._received.size(), 3);
	EXPECT_EQ(testbot._received.back(), ": ооГОоооГОоо");
}

#endif // LCOV_EXCL_STOP

