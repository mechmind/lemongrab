#pragma once

#include "lemonhandler.h"
#include "util/persistentmap.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class UrlPreview
		: public LemonHandler
{
public:
	UrlPreview(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body) override;
	const std::string GetHelp() const override;

private:
	bool getTitle(const std::string &content, std::string &title);
	std::string findUrlsInHistory(const std::string &request, bool withIndices = false);
	void StoreRecord(const std::string &record);

	bool shouldPrintTitle(const std::string &url);
	bool isFoundInRules(const std::string &url, const LevelDBPersistentMap &ruleset);
	bool addRuleToRuleset(const std::string &rule, LevelDBPersistentMap &ruleset);
	bool delRuleFromRuleset(const std::string &ruleID, LevelDBPersistentMap &ruleset);
	void ShowURLRules();

private:
	LevelDBPersistentMap _urlHistory;
	LevelDBPersistentMap _urlWhitelist;
	LevelDBPersistentMap _urlBlacklist;

	int _historyLength = 0;
	static constexpr int maxLength = 500;
	static constexpr int regenIndicesAfter = 30000;
	static constexpr int maxURLsInOneMessage = 15;
	static constexpr int maxURLsInSearch = 10;

#ifdef _BUILD_TESTS
	FRIEND_TEST(URLPreview, History);
	FRIEND_TEST(URLPreview, GetTitle);
	FRIEND_TEST(URLPreview, ConfigReader);
#endif
};
