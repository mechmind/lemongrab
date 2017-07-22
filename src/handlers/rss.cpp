#include "rss.h"

#include <chrono>

#include <cpr/cpr.h>
#include <pugixml.hpp>

#include <glog/logging.h>

#include "util/stringops.h"
#include "util/thread_util.h"

#include "util/persistentmap.h"

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
	auto updateRate = easy_stoll(GetRawConfigValue("RSS.UpdateSeconds"));

	if (updateRate <= 0)
	{
		LOG(WARNING) << "Invalid RSS update rate, defaulting to 1 hour";
		updateRate = 60 * 60;
	}

	Migrate();

	_updateSecondsMax = updateRate;

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
		UnregisterFeed(easy_stoll(args));
		return ProcessingResult::StopProcessing;
	} else if (msg._body == "!listrss") {
		SendMessage(ListRSSFeeds());
	} else if (msg._body == "!updaterss"
			   && msg._isAdmin) {
		UpdateFeeds();
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(msg._body, "!readrss", args)) {
		auto item = GetLatestItem(args);
		SendMessage(item.Format());
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
	DB::RssFeed newFeed { -1, feed };

	auto feeds = getStorage().get_all<DB::RssFeed>(where(is_equal(&DB::RssFeed::URL, feed)));
	if (feeds.size() > 0)
	{
		SendMessage("Feed already exists");
		return;
	}

	try {
		getStorage().insert(newFeed);
	} catch (std::exception &e) {
		SendMessage("Can't insert feed: " + std::string(e.what()));
		return;
	}

	SendMessage("Feed " + feed + " added");
}

void RSSWatcher::UnregisterFeed(const int id)
{
	try {
		getStorage().remove<DB::RssFeed>(id);
	} catch (std::exception &e) {
		SendMessage("Failed to remove feed: " + std::string(e.what()));
	}

	SendMessage("Feed removed");
}

std::string RSSWatcher::ListRSSFeeds()// FIXME const
{
	std::string result = "Registered feeds: ";
	for (auto &feed : getStorage().get_all<DB::RssFeed, std::list<DB::RssFeed>>())
	{
		result.append("\n" + getStorage().dump(feed));
	}

	return result;
}

void RSSWatcher::UpdateFeeds()
{
	try {
		for (auto &feed : getStorage().get_all<DB::RssFeed, std::list<DB::RssFeed>>())
		{
			auto item = GetLatestItem(feed.URL);

			if (!item._valid)
			{
				LOG(WARNING) << "Failed to update feed " << feed.URL
							 << " | Error: " << item._error;
				continue;
			}

			if (feed.GUID != item.guid)
			{
				feed.GUID = item.guid;
				getStorage().update(feed);
				SendMessage(item.Format());
			}
		}
	} catch (std::exception &e) {
		SendMessage("UpdateFeeds failed: " + std::string(e.what()));
	}
}

RSSItem RSSWatcher::GetLatestItem(const std::string &feedURL)
{
	RSSItem result;
	auto feedContent = cpr::Get(cpr::Url(feedURL), cpr::Timeout(2000));

	// Avoid INFO log clutter
	// LOG(INFO) << "Checking feed: " << feedURL << " | Result: " << feedContent.status_code;

	if (feedContent.status_code != 200)
	{
		result._error = "Status code is not 200 OK: " + std::to_string(feedContent.status_code) + " | " + feedContent.error.message;
		return result;
	}

	pugi::xml_document doc;
	auto parsingResult = doc.load_string(feedContent.text.c_str());

	if (parsingResult)
	{
		try {
			auto rss = doc.child("rss");
			auto channel = rss.child("channel");
			auto items = channel.children("item");

			result.title = items.begin()->child_value("title");
			result.link = items.begin()->child_value("link");
			result.pubDate = items.begin()->child_value("pubDate");
			result.description = items.begin()->child_value("description");
			result.guid = items.begin()->child_value("guid");
			result._valid = true;

			return result;
		} catch (...) {
			result._error = "XML parser exploded violently";
			return result;
		}
	}

	result._error = "Invalid XML in feed";
	return result;
}

void RSSWatcher::Migrate()
{
	LevelDBPersistentMap feeds;
	if (!feeds.init("rss", _botPtr->GetDBPathPrefix()))
		return;

	if (feeds.isEmpty())
		return;

	bool success = true;
	int recordCount = 0;
	feeds.ForEach([&](std::pair<std::string, std::string> record)->bool{
		try {
			DB::RssFeed feed = { -1, record.first, record.second };
			getStorage().insert(feed);
			recordCount++;
		} catch (std::exception &e) {
			SendMessage("Migration failed: " + std::string(e.what()));
			success = false;
		}
		return true;
	});

	if (success) {
		feeds.Clear();
		SendMessage("RSS Migration done: " + std::to_string(recordCount));
	}
}

std::string RSSItem::Format() const
{
	if (_valid)
		return title + " @ " + pubDate + \
				" ( " + link + " )" + \
				"\n\n" + description;
	else
		return _error;
}
