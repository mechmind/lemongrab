#include "rss.h"

#include <chrono>

#include <cpr/cpr.h>
#include <pugixml.hpp>

#include <glog/logging.h>

#include "util/stringops.h"
#include "util/thread_util.h"

void UpdateThread(RSSWatcher *parent)
{
	while (parent->_isRunning) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if (++parent->_updateSecondsCurrent >= parent->_updateSecondsMax)
		{
			parent->_updateSecondsCurrent = 0;
			parent->UpdateFeeds();
		}
	}
}

RSSWatcher::RSSWatcher(LemonBot *bot)
	: LemonHandler("rss", bot)
{
	_updateSecondsMax = from_string<int>(GetRawConfigValue("RSS.UpdateSeconds")).value_or(60*60);

	UpdateFeeds();

	_updateThread = std::thread(&UpdateThread, this);
	nameThread(_updateThread, "RSS updater");
}

RSSWatcher::~RSSWatcher()
{
	_isRunning = false;
	_updateThread.join();
}

LemonHandler::ProcessingResult RSSWatcher::HandleMessage(const ChatMessage &msg)
{
	std::string args;
	if (getCommandArguments(msg._body, "!addrss", args)
			&& msg._isAdmin)
	{
		RegisterFeed(args);
		UpdateFeeds();
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(msg._body, "!delrss", args)
			   && msg._isAdmin) {
		auto feedID = from_string<int>(args);
		if (feedID)
			UnregisterFeed(*feedID);
		return ProcessingResult::StopProcessing;
	} else if (msg._body == "!listrss") {
		SendMessage(ListRSSFeeds());
	} else if (msg._body == "!updaterss"
			   && msg._isAdmin) {
		UpdateFeeds();
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(msg._body, "!readrss", args)) {
		if (auto item = GetLatestItem(args)) {
			SendMessage(item->Format());
		} else {
			SendMessage("Failed to fetch or parse feed");
		}
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string RSSWatcher::GetHelp() const
{
	return "!addrss %url% - add feed to RSS watchlist (admin only)\n"
		   "!delrss %url% - remove feed from RSS watchlist (admin only)\n"
		   "!listrss - list registered feeds\n"
		   "!updaterss - force RSS feed update (admin only)\n"
		   "!readrss %url% - get latest news from a specific feed";
}

void RSSWatcher::RegisterFeed(const std::string &feed)
{
	using namespace sqlite_orm;
	auto feeds = getStorage().get_all<DB::RssFeed>(where(is_equal(&DB::RssFeed::URL, feed)));
	if (feeds.size() > 0)
	{
		SendMessage("Feed already exists");
		return;
	}

	try {
		DB::RssFeed newFeed { -1, feed };
		getStorage().insert(newFeed);
	} catch (std::exception &e) {
		SendMessage("Can't insert feed: " + std::string(e.what()));
		return;
	}

	SendMessage("Feed " + feed + " added");
}

void RSSWatcher::UnregisterFeed(int id)
{
	try {
		getStorage().remove<DB::RssFeed>(id);
	} catch (std::exception &e) {
		SendMessage("Failed to remove feed: " + std::string(e.what()));
	}

	SendMessage("Feed removed");
}

std::string RSSWatcher::ListRSSFeeds()
{
	std::string result = "Registered feeds: ";
	for (const auto &feed : getStorage().get_all<DB::RssFeed, std::list<DB::RssFeed>>())
	{
		result.append("\n" + getStorage().dump(feed));
	}

	return result;
}

void RSSWatcher::UpdateFeeds()
{
	for (auto &feed : getStorage().get_all<DB::RssFeed, std::list<DB::RssFeed>>())
	{
		const auto item = GetLatestItem(feed.URL);
		if (item && item->guid != feed.GUID)
		{
			feed.GUID = item->guid;
			getStorage().update(feed);
			SendMessage(item->Format());
		}
	}
}

std::optional<RSSItem> RSSWatcher::GetLatestItem(const std::string &feedURL) const
{
	auto feedContent = fetchRawRSS(feedURL);

	if (!feedContent)
		return {};

	return parseRawRSS(*feedContent);
}

std::optional<std::string> RSSWatcher::fetchRawRSS(const std::string &feedURL) const
{
	auto feedContent = cpr::Get(cpr::Url(feedURL), cpr::Timeout(2000));
	if (feedContent.status_code != 200)
	{
		LOG(WARNING) << "Status code is not 200 OK: " + std::to_string(feedContent.status_code) + " | " + feedContent.error.message;
		return {};
	}

	return feedContent.text;
}

std::optional<RSSItem> RSSWatcher::parseRawRSS(const std::string &rawRSS) const
{
	pugi::xml_document doc;
	auto parsingResult = doc.load_string(rawRSS.c_str());
	if (!parsingResult) {
		LOG(WARNING) << "Invalid XML: " << std::endl << rawRSS;
		return {};
	}

	try {
		auto items = doc.child("rss").child("channel").children("item");
		return RSSItem{
			items.begin()->child_value("title"),
			items.begin()->child_value("pubDate"),
			items.begin()->child_value("link"),
			items.begin()->child_value("description"),
			items.begin()->child_value("guid")
		};
	} catch (std::exception &e) {
		LOG(WARNING) << "XML parser exploded violently: " << e.what();
		return {};
	}
}

std::string RSSItem::Format() const
{
	return title + " @ " + pubDate + \
			" ( " + link + " )" + \
			"\n\n" + description;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

#include <fstream>
#include <streambuf>
#include <iostream>

class RssTestBot : public LemonBot
{
public:
	RssTestBot() : LemonBot(":memory:") {
		_storage.sync_schema();
	}

	void SendMessage(const std::string &text, const std::string &channel);
	std::vector<std::string> _received;
};

void RssTestBot::SendMessage(const std::string &text, const std::string &channel)
{
	_received.push_back(text);
}


TEST(RSSReader, Parse)
{
	{
		RssTestBot tb;
		RSSWatcher rss(&tb);
		std::ifstream rssFile("test/dev_now.rss");
		std::string contents{std::istreambuf_iterator<char>{rssFile}, {}};
		ASSERT_FALSE(contents.empty());

		auto rssitem = rss.parseRawRSS(contents);
		ASSERT_TRUE(rssitem.has_value());

		EXPECT_EQ("09/07/2017", rssitem->title);
		EXPECT_EQ("Thu, 07 Sep 2017 21:29:00 PDT", rssitem->pubDate);
		EXPECT_EQ("http://www.bay12games.com/dwarves/index.html#2017-09-07", rssitem->link);
		EXPECT_EQ("testitem", rssitem->description);
		EXPECT_EQ("testguid", rssitem->guid);
	}
}

#endif // LCOV_EXCL_STOP

