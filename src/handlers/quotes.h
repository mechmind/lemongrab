#pragma once

#include "lemonhandler.h"

#include <random>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class Quotes : public LemonHandler
{
public:
	Quotes(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

private:
	std::string GetQuote(const std::string &id);
	bool AddQuote(const std::string &text);
	bool DeleteQuote(int id);
	std::string FindQuote(const std::string &request);
	void RegenerateIndex();

private:
	std::mt19937_64 _generator;

	static constexpr int maxMatches = 10;
#ifdef _BUILD_TESTS
	FRIEND_TEST(QuotesTest, General);
	FRIEND_TEST(QuotesTest, Search);
	FRIEND_TEST(QuotesTest, RegenIndex);
#endif
};
