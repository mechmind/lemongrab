#pragma once

#include "lemonhandler.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class UrlPreview
		: public LemonHandler
{
public:
	UrlPreview(LemonBot *bot);
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

private:
	std::string getTitle(const std::string &content) const;
	std::string getMetaCodepage(const std::string &content) const;

	std::string findUrlsInHistory(const std::string &request, bool withIndices = false);
	void StoreRecord(const std::string &record);

	bool shouldPrintTitle(const std::string &url);
	bool addRuleToRuleset(const std::string &rule, bool blacklist);
	bool delRuleFromRuleset(const std::string &ruleID);
	std::string ShowURLRules();

private:
	void MigrateHistory();
	void MigrateRules();

	static constexpr int maxLength = 500;
	static constexpr int maxURLsInOneMessage = 5;
	static constexpr int maxURLsInSearch = 15;

#ifdef _BUILD_TESTS
	FRIEND_TEST(URLPreview, History);
	FRIEND_TEST(URLPreview, GetTitle);
	FRIEND_TEST(URLPreview, ConfigReader);
#endif
};
