#include "warframe.h"

#include <string>
#include <cpr/cpr.h>
#include <glog/logging.h>
#include <pugixml.hpp>
#include "util/stringops.h"
#include "util/thread_util.h"

static const std::string rssUrl = "http://content.warframe.com/dynamic/rss.php";

void UpdateThread(std::future<void> &&stop, Warframe *parent, int updateSeconds)
{
	bool running = true;
	do {
		auto result = stop.wait_for(std::chrono::seconds(updateSeconds));
		if (result == std::future_status::timeout) {
			parent->Update();
		} else {
			running = false;
		}
	} while (running);
}

Warframe::Warframe(LemonBot *bot)
	: LemonHandler("warframe", bot)
{
	_updateSecondsMax = from_string<int>(GetRawConfigValue("Warframe.UpdateSeconds")).value_or(300);
}

bool Warframe::Init()
{
	_stopThread = std::promise<void>();
	_isRunning = true;
	_updateThread = std::thread([&](){
		UpdateThread(std::move(_stopThread.get_future()), this, _updateSecondsMax);
	});
	nameThread(_updateThread, "Warframe updater");

	Update();
	return true;
}

Warframe::~Warframe()
{
	if (_isRunning) {
		_stopThread.set_value();
		_updateThread.join();
	}
}

LemonHandler::ProcessingResult Warframe::HandleMessage(const ChatMessage &msg)
{
	if (msg._body == "!wf") {
		Update();
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string Warframe::GetHelp() const
{
	return "This module has no commands";
}

bool Warframe::isOfIntereest(const std::string &description)
{
	if (description.find("Orokin Reactor") != description.npos
			|| description.find("Orokin Catalyst") != description.npos
			|| description.find("Nitain Extract") != description.npos)
//			|| description.find("Corrosive Projection") != description.npos)
		return true;

	return false;
}

void Warframe::Update()
{
	auto feedContent = cpr::Get(cpr::Url(rssUrl), cpr::Timeout(2000));

	if (feedContent.status_code != 200)
	{
		LOG(WARNING) << "Status code is not 200 OK: " + std::to_string(feedContent.status_code) + " | " + feedContent.error.message;
		return;
	}

	pugi::xml_document doc;
	auto parsingResult = doc.load_string(feedContent.text.c_str());
	if (!parsingResult) {
		LOG(WARNING) << "Invalid XML: " << std::endl << feedContent.text;
		return;
	}

	try {
		auto items = doc.child("rss").child("channel").children("item");

		std::set<std::string> newGuids;

		for (auto item : items) {
			auto guid = item.child_value("guid");
			newGuids.insert(guid);

			std::string author = item.child_value("author");

			if (_guids.count(guid) > 0) continue; // known guid

			auto title = item.child_value("title");
			LOG(INFO) << "New Warframe alert: " << guid << " " << title;

			if (isOfIntereest(title) || author.find("Tactical") != author.npos)
				SendMessage(std::string("New warframe alert: ") + title);
		}

		_guids = newGuids;
	} catch (std::exception &e) {
		LOG(WARNING) << "XML parser exploded violently: " << e.what();
		return;
	}
}
