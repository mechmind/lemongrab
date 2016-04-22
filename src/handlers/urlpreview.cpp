#include "urlpreview.h"

#include <map>
#include <regex>

#include <curl/curl.h>
#include <glog/logging.h>

#include "util/stringops.h"
#include "util/curlhelper.h"

std::string formatHTMLchars(std::string input);

UrlPreview::UrlPreview(LemonBot *bot)
	: LemonHandler("url", bot)
{
	if (_urlHistory.init("urlhistory"))
		_historyLength = _urlHistory.Size();

	readConfig(bot);
}

LemonHandler::ProcessingResult UrlPreview::HandleMessage(const std::string &from, const std::string &body)
{
	std::string args;
	if (getCommandArguments(body, "!url", args))
	{
		SendMessage(findUrlsInHistory(args));
		return ProcessingResult::StopProcessing;
	}

	auto sites = findURLs(body);
	if (sites.empty())
		return ProcessingResult::KeepGoing;

	for (auto &site : sites)
	{
		auto page = CurlRequest(site.url, _botPtr == nullptr);

		std::string title = "";
		if (page.second != 200)
		{
			LOG(INFO) << "URL: " << site.url << " | Server responded with unexpected code: " << page.second;
		} else {
			const auto &siteContent = page.first;
			getTitle(siteContent, title);
		}

		// FIXME: This is going to overflow and explode eventually
		auto id = easy_stoll(_urlHistory.GetLastRecord().first);
		_urlHistory.Set(std::to_string(++id), title + " " + site.url);
		if (_historyLength < maxLength)
			_historyLength++;
		else
			_urlHistory.PopFront();

		if ((_URLwhitelist.empty()
				|| _URLwhitelist.find(site.hostname) != _URLwhitelist.end())
				&& sites.size() < maxURLsInOneMessage
				&& !title.empty())
			SendMessage(formatHTMLchars(title));
	}

	return ProcessingResult::KeepGoing;
}

const std::string UrlPreview::GetVersion() const
{
	return "0.3";
}

const std::string UrlPreview::GetHelp() const
{
	return "!url %regex% - search in URL history by title or url";
}

void UrlPreview::readConfig(LemonBot *bot)
{
	auto unparsedWhitelist = bot ? bot->GetRawConfigValue("URLwhitelist") : "";

	auto urls = tokenize(unparsedWhitelist, ';');
	for (const auto &url : urls)
	{
		LOG(INFO) << "Whitelisted URL: " << url << std::endl;
		_URLwhitelist.insert(url);
	}
}

bool UrlPreview::getTitle(const std::string &content, std::string &title)
{
	// FIXME: maybe should use actual HTML parser here?
	auto titleBegin = content.find("<title>");
	if (titleBegin == content.npos)
		return false;

	auto titleEnd = content.find("</title>", titleBegin);
	if (titleEnd == content.npos)
		return false;

	title = content.substr(titleBegin + 7, titleEnd - titleBegin - 7);
	return true;
}

std::string UrlPreview::findUrlsInHistory(const std::string &request)
{
	std::string searchResults;
	auto urls = _urlHistory.Find(request, PersistentMap::FindOptions::ValuesOnly);

	if (urls.size() > maxURLsInSearch)
		searchResults = "Too many matches";
	else if (urls.empty())
		searchResults = "No matches";
	else
	{
		searchResults = "Matching URLs: \n";
		for (const auto &url : urls)
			searchResults += url.second + "\n";
	}

	return searchResults;
}

void UrlPreview::StoreRecord(const std::string &record)
{

}

std::string formatHTMLchars(std::string input)
{
	std::map<std::string, std::string> chars
			= {{"&quot;", "\""},
			   {"&amp;", "&"},
			   {"&lt;", "<"},
			   {"&gt;", ">"},
			   {"&apos;", "'"},
			   {"&#39;", "'"}};

	for (auto &specialCharPair : chars)
	{
		auto &coded = specialCharPair.first;
		auto &decoded = specialCharPair.second;
		size_t pos = input.find(coded);
		while (pos != input.npos)
		{
			input.replace(pos, coded.length(), decoded);
			pos = input.find(coded);
		}
	}

	return input;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>
#include <fstream>

class UrlPreviewTestBot : public LemonBot
{
public:
	void SendMessage(const std::string &text)
	{
		_lastMessage = text;
	}

	std::string GetRawConfigValue(const std::string &name) const
	{
		if (name == "URLwhitelist")
			return "youtube.com;youtu.be;store.steampowered.com";
		else
			return "";
	}

	std::string _lastMessage;
};

TEST(URLPreview, HTMLSpecialChars)
{
	std::string input = "&quot;&amp;&gt;&lt;";

	EXPECT_EQ("\"&><", formatHTMLchars(input));
}

TEST(URLPreview, ConfigReader)
{
	UrlPreviewTestBot testBot;
	UrlPreview testUnit(&testBot);

	std::set<std::string> expectedWhitelist = {"youtube.com", "youtu.be", "store.steampowered.com"};

	ASSERT_EQ(expectedWhitelist.size(), testUnit._URLwhitelist.size());
	EXPECT_TRUE(std::equal(expectedWhitelist.begin(), expectedWhitelist.end(), testUnit._URLwhitelist.begin()));
}

TEST(URLPreview, GetTitle)
{
	UrlPreviewTestBot testBot;
	UrlPreview testUnit(&testBot);

	{
		std::string content("<html>\n"
							"<head><title>This is a test title</title></head>\n"
							"<body><p>Test</p></body>"
							"</html>\n");
		std::string title;
		testUnit.getTitle(content, title);
		EXPECT_EQ("This is a test title", formatHTMLchars(title));
	}
}
// Need a persisten map implementation for tests
/*
TEST(URLPreview, History)
{
	UrlPreview t(nullptr);
	t.HandleMessage("Bob", "http://example.com/?test http://test.com/page#anchor");
	t.HandleMessage("Alice", "http://test.ru/");

	std::list<std::pair<std::string, std::string>> expectedHistory = {
		{"http://test.ru/", ""},
		{"http://test.com/page#anchor", ""},
		{"http://example.com/?test", ""},
	};
	ASSERT_EQ(expectedHistory.size(), t._urlHistory.size());
	EXPECT_TRUE(std::equal(expectedHistory.begin(), expectedHistory.end(), t._urlHistory.begin()));
}*/

#endif // LCOV_EXCL_STOP
