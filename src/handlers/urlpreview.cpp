#include "urlpreview.h"

#include <map>
#include <regex>

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cpr/cpr.h>

#include "util/stringops.h"

std::string formatHTMLchars(std::string input);

UrlPreview::UrlPreview(LemonBot *bot)
	: LemonHandler("url", bot)
{
	if (_urlHistory.init("urlhistory"))
		_historyLength = _urlHistory.Size();

	_urlWhitelist.init("urlwhitelist");
	_urlBlacklist.init("urlblacklist");
}

LemonHandler::ProcessingResult UrlPreview::HandleMessage(const ChatMessage &msg)
{
	std::string args;
	auto &body = msg._body;
	if (getCommandArguments(body, "!url", args))
	{
		SendMessage(findUrlsInHistory(args));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!!!url", args))
	{
		SendMessage(findUrlsInHistory(args, true));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!wlisturl", args))
	{
		addRuleToRuleset(args, _urlWhitelist);
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!blisturl", args))
	{
		addRuleToRuleset(args, _urlBlacklist);
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!wdelisturl", args))
	{
		delRuleFromRuleset(args, _urlWhitelist);
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!bdelisturl", args))
	{
		delRuleFromRuleset(args, _urlBlacklist);
		return ProcessingResult::StopProcessing;
	}

	if (body == "!urlrules")
	{
		ShowURLRules();
		return ProcessingResult::StopProcessing;
	}

	auto sites = findURLs(body);
	if (sites.empty())
		return ProcessingResult::KeepGoing;

	for (auto &site : sites)
	{
		auto acceptLanguage = GetRawConfigValue("URL.AcceptLanguage");
		auto page = cpr::Get(cpr::Url{site._url},
							 cpr::Timeout{2000},
							 cpr::Header{{"Accept-Language",
										  acceptLanguage.empty() ? "ru,en" : acceptLanguage}});

		std::string title = "";
		if (page.status_code != 200)
		{
			LOG(INFO) << "URL: " << site._url << " | Status code: " << page.status_code
					  << " | Error: " << page.error.message;
		} else {
			const auto &siteContent = page.text;
			title = getTitle(siteContent);
		}

		auto id = easy_stoll(_urlHistory.GetLastRecord().first);
		_urlHistory.Set(std::to_string(++id), title + " " + site._url);

		if (id > regenIndicesAfter)
			_urlHistory.GenerateNumericIndex();

		if (_historyLength < maxLength)
			_historyLength++;
		else
			_urlHistory.PopFront();

		if (shouldPrintTitle(site._url))
			SendMessage(formatHTMLchars(title));
	}

	return ProcessingResult::KeepGoing;
}

const std::string UrlPreview::GetHelp() const
{
	return "!url %regex% - search in URL history by title or url\n"
		   "!wlisturl %regex% and !blisturl %regex% - enable/disable notifications for specific urls\n"
		   "!wdelisturl %id% and !bdelisturl %id% - delete existing rules. !urlrules - print existing rules and their ids";
}

std::string UrlPreview::getTitle(const std::string &content) const
{
	// FIXME: maybe should use actual HTML parser here?
	auto titleBegin = content.find("<title>");
	if (titleBegin == content.npos)
		return "";

	auto titleEnd = content.find("</title>", titleBegin);
	if (titleEnd == content.npos)
		return "";

	auto title = content.substr(titleBegin + 7, titleEnd - titleBegin - 7);
	boost::trim(title);

	try {
		return boost::locale::conv::utf_to_utf<char>(title.c_str(), boost::locale::conv::stop);
	} catch (boost::locale::conv::conversion_error &e) {
		return "{Non-unicode header: " + std::string(e.what()) + "}";
	}
}

std::string UrlPreview::findUrlsInHistory(const std::string &request, bool withIndices)
{
	long moreMatches = 0;
	std::string searchResults;
	auto urls = _urlHistory.Find(request, LevelDBPersistentMap::FindOptions::ValuesOnly);

	if (urls.size() > maxURLsInSearch)
	{
		moreMatches = urls.size() - maxURLsInSearch;
		urls.reverse();
		urls.resize(maxURLsInSearch);
	}

	if (urls.empty())
		searchResults = "No matches";
	else
	{
		searchResults = "Matching URLs: \n";
		for (const auto &url : urls)
			searchResults += withIndices ? (url.first + ") " + url.second + "\n") : (url.second + "\n");

		if (moreMatches > 0)
			searchResults += "\n...and " + std::to_string(moreMatches) + " more";
	}

	return searchResults;
}

bool UrlPreview::shouldPrintTitle(const std::string &url) const
{
	bool whitelisted = isFoundInRules(url, _urlWhitelist);
	bool blacklisted = isFoundInRules(url, _urlBlacklist);

	return !blacklisted || whitelisted;
}

bool UrlPreview::isFoundInRules(const std::string &url, const LevelDBPersistentMap &ruleset) const
{
	if (!ruleset.isOK() || ruleset.isEmpty())
		return false;

	auto ruleMatches = ruleset.Find(url, LevelDBPersistentMap::FindOptions::ValuesAsRegex);
	if (!ruleMatches.empty())
	{
		LOG(INFO) << "URL is found in ruleset " << ruleset.getName() << " in rule " << (*ruleMatches.begin()).second;
		return true;
	}

	return false;
}

bool UrlPreview::addRuleToRuleset(const std::string &rule, LevelDBPersistentMap &ruleset)
{
	if (!ruleset.isOK())
		return false;

	long long index = easy_stoll(ruleset.GetLastRecord().first);
	auto ok = ruleset.Set(std::to_string(++index), rule);

	ok ? SendMessage("Rule successfully added") : SendMessage("Failed to add rule");
	return ok;
}

bool UrlPreview::delRuleFromRuleset(const std::string &ruleID, LevelDBPersistentMap &ruleset)
{
	if (!ruleset.isOK())
		return false;

	auto ok = ruleset.Delete(ruleID);
	ok ? SendMessage("Rule successfully removed") : SendMessage("Failed to remove rule");

	return ok;
}

void UrlPreview::ShowURLRules()
{
	std::string whitelist;
	std::string blacklist;

	if (_urlWhitelist.isOK())
	{
		_urlWhitelist.ForEach([&](std::pair<std::string, std::string> record){
			whitelist += "\n" + record.first + ") " + record.second;
			return true;
		});
	}

	if (_urlBlacklist.isOK())
	{
		_urlBlacklist.ForEach([&](std::pair<std::string, std::string> record){
			blacklist += "\n" + record.first + ") " + record.second;
			return true;
		});
	}

	SendMessage("URL Whitelist: " + whitelist);
	SendMessage("URL Blacklist: " + blacklist);
}

std::string formatHTMLchars(std::string input)
{
	std::map<std::string, std::string> chars
			= {{"&quot;", "\""},
			   {"&amp;", "&"},
			   {"&lt;", "<"},
			   {"&gt;", ">"},
			   {"&apos;", "'"},
			   {"&#39;", "'"},
			   {"&bull;", "•"},
			   {"&mdash;", "—"},
			   {"&ndash;", "–"},
			  };

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

	std::string _lastMessage;
};

TEST(URLPreview, HTMLSpecialChars)
{
	std::string input = "&quot;&amp;&gt;&lt;";
	EXPECT_EQ("\"&><", formatHTMLchars(input));
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
		std::string title = testUnit.getTitle(content);
		EXPECT_EQ("This is a test title", formatHTMLchars(title));
	}
}
/*
TEST(URLPreview, History)
{
	UrlPreview t(nullptr);
	t._urlHistory.Clear();
	t.HandleMessage("Bob", "http://example.com/?test http://test.com/page#anchor");
	t.HandleMessage("Alice", "http://test.ru/");

	std::list<std::pair<std::string, std::string>> expectedHistory = {
		{"http://test.ru/", ""},
		{"http://test.com/page#anchor", ""},
		{"http://example.com/?test", ""},
	};
	ASSERT_EQ(expectedHistory.size(), t._urlHistory.Size());
	auto expectedRecord = expectedHistory.begin();
	t._urlHistory.ForEach([&](std::pair<std::string, std::string> record){
		EXPECT_EQ(*expectedRecord, record);
		return true;
	});
}*/

#endif // LCOV_EXCL_STOP
