#pragma once

#include "lemonhandler.h"

#include <set>

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class UrlPreview : public LemonHandler
{
public:
	UrlPreview(LemonBot *bot);
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	void readConfig(LemonBot *bot);
	bool findURL(const std::string &input, std::string &site, std::string &url);
	bool getTitle(const std::string &content, std::string &title);

private:
	std::set<std::string> _URLwhitelist;

#ifdef _BUILD_TESTS
	FRIEND_TEST(URLPreview, GetTitle);
#endif
};
