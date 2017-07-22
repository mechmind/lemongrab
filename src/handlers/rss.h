#pragma once

#include <set>
#include <string>
#include <mutex>
#include <thread>

#include "lemonhandler.h"

class RSSItem
{
public:
	bool _valid = false;
	std::string _error = "No error specified";

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
	void UnregisterFeed(const int id);
	std::string ListRSSFeeds();

	void UpdateFeeds();

	RSSItem GetLatestItem(const std::string &feedURL);

	std::thread _updateThread;
	bool _isRunning = true;
	std::uint64_t _updateSecondsCurrent = 0;
	std::uint64_t _updateSecondsMax = 0;
	friend void UpdateThread(RSSWatcher *parent);

	void Migrate();
};
