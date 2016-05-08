#pragma once

#include "lemonhandler.h"

#include <random>

#include "util/persistentmap.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class Quotes : public LemonHandler
{
public:
	Quotes(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetHelp() const;

private:
	std::string GetQuote(const std::string &id);
	std::string AddQuote(const std::string &text);
	bool DeleteQuote(const std::string &id);
	std::string FindQuote(const std::string &request);
	void RegenerateIndex();

private:
	LevelDBPersistentMap _quotesDB;
	LemonBot *_bot = nullptr;

	std::mt19937_64 _generator;

	static constexpr int maxMatches = 10;
#ifdef _BUILD_TESTS
	FRIEND_TEST(QuotesTest, General);
	FRIEND_TEST(QuotesTest, Search);
#endif
};
