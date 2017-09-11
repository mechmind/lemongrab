#pragma once

#include <set>
#include <string>
#include <mutex>
#include <thread>
#include <optional>

#include "lemonhandler.h"

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class RSSItem
{
public:
	std::string title;
	std::string pubDate;
	std::string link;
	std::string description;
	std::string guid;

	std::string Format() const;
};

class RSSWatcher : public LemonHandler
{
public:
	RSSWatcher(LemonBot *bot);
	~RSSWatcher();
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const final;

private:
	void RegisterFeed(const std::string &feed);
	void UnregisterFeed(int id);
	std::string ListRSSFeeds();

	void UpdateFeeds();

	std::optional<RSSItem> GetLatestItem(const std::string &feedURL) const;
	std::optional<std::string> fetchRawRSS(const std::string &feedURL) const;
	std::optional<RSSItem> parseRawRSS(const std::string &rawRSS) const;

	std::thread _updateThread;
	bool _isRunning = true;
	int _updateSecondsCurrent = 0;
	int _updateSecondsMax = 0;
	friend void UpdateThread(RSSWatcher *parent);

#ifdef _BUILD_TESTS
	FRIEND_TEST(RSSReader, Parse);
#endif
};
