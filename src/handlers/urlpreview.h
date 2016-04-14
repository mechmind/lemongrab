#pragma once

#include "lemonhandler.h"

#include <set>
#include <list>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class UrlPreview : public LemonHandler
{
public:
	UrlPreview(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	void readConfig(LemonBot *bot);
	bool getTitle(const std::string &content, std::string &title);
	std::string findUrlsInHistory(const std::string &request);

private:
	std::set<std::string> _URLwhitelist;
	std::list<std::pair<std::string, std::string>> _urlHistory;
	static constexpr int maxLength = 100;
	static constexpr int maxURLsInOneMessage = 15;
	static constexpr int maxURLsInSearch = 10;

#ifdef _BUILD_TESTS
	FRIEND_TEST(URLPreview, History);
	FRIEND_TEST(URLPreview, GetTitle);
	FRIEND_TEST(URLPreview, ConfigReader);
#endif
};
