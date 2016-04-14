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

		std::string title = "Can't get page title";
		if (page.second != 200)
		{
			LOG(INFO) << "URL: " << site.url << " | Server responded with unexpected code: " << page.second;
		} else {
			const auto &siteContent = page.first;
			getTitle(siteContent, title);
		}

		_urlHistory.push_front({site.url, title});
		if (_urlHistory.size() > maxLength)
			_urlHistory.pop_back();

		if (_URLwhitelist.empty()
				|| _URLwhitelist.find(site.hostname) != _URLwhitelist.end()
				|| sites.size() < maxURLsInOneMessage)
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
	std::string searchResults("Matching URLs: \n");
	std::regex inputRegex;
	std::smatch regexMatch;
	int matches = 0;
	try {
		inputRegex = std::regex(toLower(request));
	} catch (std::regex_error& e) {
		LOG(WARNING) << "Input regex " << request << " is invalid: " << e.what();
		return "Something broke: " + std::string(e.what());
	}

	for (const auto &urlPair : _urlHistory)
	{
		bool doesMatch = false;
		try {
			auto url = toLower(urlPair.first);
			auto title = toLower(urlPair.second);
			doesMatch = std::regex_search(url, regexMatch, inputRegex)
					|| std::regex_search(title, regexMatch, inputRegex);
		} catch (std::regex_error &e) {
			LOG(ERROR) << "Regex exception thrown: " << e.what();
		}

		if (doesMatch)
		{
			++matches;
			if (matches < maxURLsInSearch)
				searchResults += urlPair.first + " (" + urlPair.second + ")\n";
			else
			{
				searchResults += "... too many matches";
				return searchResults;
			}
		}
	}

	return searchResults;
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

	EXPECT_EQ(expectedWhitelist.size(), testUnit._URLwhitelist.size());

	auto result = testUnit._URLwhitelist.begin();
	auto expected = expectedWhitelist.begin();

	while (result != testUnit._URLwhitelist.end())
	{
		EXPECT_EQ(*expected, *result);
		++expected;
		++result;
	}
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

TEST(URLPreview, History)
{
	UrlPreview t(nullptr);
	t.HandleMessage("Bob", "http://example.com/?test http://test.com/page#anchor");
	t.HandleMessage("Alice", "http://test.ru/");

	std::list<std::pair<std::string, std::string>> expectedHistory = {
		{"http://test.ru/", "Can't get page title"},
		{"http://test.com/page#anchor", "Can't get page title"},
		{"http://example.com/?test", "Can't get page title"},
	};
	ASSERT_EQ(expectedHistory.size(), t._urlHistory.size());
	EXPECT_TRUE(std::equal(expectedHistory.begin(), expectedHistory.end(), t._urlHistory.begin()));
}

#endif // LCOV_EXCL_STOP
