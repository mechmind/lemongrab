#include "rss.h"

#include <chrono>

#include <cpr/cpr.h>
#include <pugixml.hpp>

#include <glog/logging.h>

#include "util/stringops.h"

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
	_feeds.init("rss", bot->GetDBPathPrefix());

	auto updateRate = easy_stoll(GetRawConfigValue("RSS.UpdateSeconds"));

	if (updateRate <= 0)
	{
		LOG(WARNING) << "Invalid RSS update rate, defaulting to 1 hour";
		updateRate = 60 * 60;
	}

	_updateSecondsMax = updateRate;

	UpdateFeeds();

	_updateThread = std::thread(&UpdateThread, this);
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
		UnregisterFeed(args);
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
	if (_feeds.Exists(feed))
	{
		SendMessage("Feed already exists");
		return;
	}

	_feeds.Set(feed, "");
	SendMessage("Feed " + feed + " added");
}

void RSSWatcher::UnregisterFeed(const std::string &feed)
{
	if (!_feeds.Delete(feed))
		SendMessage("No such feed");
	else
		SendMessage("Feed " + feed + " removed");
}

std::string RSSWatcher::ListRSSFeeds() const
{
	std::string result = "Registered feeds: ";
	_feeds.ForEach([&](std::pair<std::string, std::string> record)->bool{
		result.append("\n" + record.first + " | GUID: " + record.second);
		return true;
	});
	return result;
}

void RSSWatcher::UpdateFeeds()
{
	if (!_feeds.isOK())
		return;

	_feeds.ForEach([&](std::pair<std::string, std::string> record)->bool{
		auto item = GetLatestItem(record.first);

		if (!item._valid)
		{
			LOG(WARNING) << "Failed to update feed " << record.first << " : " << record.second
						 << " | Error: " << item._error;
			return true;
		}

		std::string oldGuid;
		_feeds.Get(record.first, oldGuid);

		if (oldGuid != item.guid)
		{
			_feeds.Set(record.first, item.guid);
			SendMessage(item.Format());
		}

		return true;
	});
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

std::string RSSItem::Format() const
{
	if (_valid)
		return title + " @ " + pubDate + \
				" ( " + link + " )" + \
				"\n\n" + description;
	else
		return _error;
}
