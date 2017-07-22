#include "urlpreview.h"

#include <map>
#include <regex>
#include <chrono>

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <cpr/cpr.h>

#include "util/stringops.h"

#include "util/persistentmap.h"

std::string formatHTMLchars(std::string input);

UrlPreview::UrlPreview(LemonBot *bot)
	: LemonHandler("url", bot)
{
	MigrateHistory();
	MigrateRules();
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
		addRuleToRuleset(args, false)
			? SendMessage("Rule added")
			: SendMessage("Failed to add rule");
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!blisturl", args))
	{
		addRuleToRuleset(args, true)
			? SendMessage("Rule added")
			: SendMessage("Failed to add rule");
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!delisturl", args))
	{
		delRuleFromRuleset(args);
		return ProcessingResult::StopProcessing;
	}

	if (body == "!urlrules")
	{
		SendMessage(ShowURLRules());
		return ProcessingResult::StopProcessing;
	}

	auto sites = findURLs(body);
	if (sites.empty())
		return ProcessingResult::KeepGoing;

	int urlsFound = 0;
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

		// FIXME: should we ever delete urls now?
		auto now = std::chrono::system_clock::now();
		DB::LoggedURL record = { -1, site._url, title,
								 std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count(),
							   site._url + " " + title};

		getStorage().insert(record);

		if (shouldPrintTitle(site._url) && urlsFound < maxURLsInOneMessage)
			SendMessage(formatHTMLchars(title));

		urlsFound++;
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
	const std::string titleOp("<title>");
	const std::string titleClose("</title>");
	auto titleBegin = content.find(titleOp);
	if (titleBegin == content.npos)
		return "";

	auto titleEnd = content.find(titleClose, titleBegin);
	if (titleEnd == content.npos)
		return "";

	auto title = content.substr(titleBegin + titleOp.size(), titleEnd - titleBegin - titleOp.size());
	boost::trim(title);

	try {
		return boost::locale::conv::utf_to_utf<char>(title.c_str(), boost::locale::conv::stop);
	} catch (boost::locale::conv::conversion_error &e) {
		LOG(INFO) << "Non-unicode header: " + std::string(e.what()) + "}";
	}

	auto codepage = getMetaCodepage(content);

	if (!codepage.empty()) {
		try {
			return boost::locale::conv::to_utf<char>(title.c_str(), codepage);
		} catch (boost::locale::conv::conversion_error &e) {
			LOG(INFO) << "Failed to convert from " + codepage + ": " + std::string(e.what()) + "}";
		}
	}

	return "{ Unsupported code page in title }";
}

std::string UrlPreview::getMetaCodepage(const std::string &content) const
{
	// Hack to support some old Russian websites
	const std::string metaOp = "meta charset=\"";
	auto codepageBegin = content.find(metaOp);
	if (codepageBegin == content.npos)
		return "";

	auto codepageEnd = content.find("\"", codepageBegin + metaOp.size());
	if (codepageEnd == content.npos)
		return "";

	auto codepage = content.substr(codepageBegin + metaOp.size(), codepageEnd - codepageBegin - metaOp.size());
	boost::trim(codepage);

	return codepage;
}

std::string UrlPreview::findUrlsInHistory(const std::string &request, bool withIndices)
{
	using namespace sqlite_orm;
	auto urls = getStorage().get_all<DB::LoggedURL>(
				where(like(&DB::LoggedURL::fullText, "%" + request + "%")),
				order_by(&DB::LoggedURL::id).desc(),
				limit(maxURLsInSearch));

	if (urls.empty())
		return "No matches";

	std::string searchResults;

	searchResults = std::to_string(maxURLsInSearch) + " recent matching URLs: \n";
	for (const auto &url : urls)
	{
		searchResults += withIndices
				? (url.id + ") " + url.URL + " " + url.title + "\n")
				: (url.URL + " " + url.title + "\n");
	}

	return searchResults;
}

bool UrlPreview::shouldPrintTitle(const std::string &url) // FIXME const
{
	bool blacklisted = false;
	bool whitelisted = false;

	for (const auto &rule : getStorage().get_all<DB::URLRule>())
	{
		std::smatch regexMatch;
		auto searchRegex = std::regex(toLower(rule.rule));
		auto doesMatch = std::regex_search(url, regexMatch, searchRegex);

		if (doesMatch) {
			LOG(INFO) << "URL is found in rule: " << getStorage().dump(rule);
			rule.blacklist ? blacklisted = true : whitelisted = true;
		}
	}

	return !blacklisted || whitelisted;
}

bool UrlPreview::addRuleToRuleset(const std::string &rule, bool blacklist)
{
	DB::URLRule newRule = { -1, rule, blacklist };
	try {
		getStorage().insert(newRule);
		return true;
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to add rule: " << e.what();
		return false;
	}
}

bool UrlPreview::delRuleFromRuleset(const std::string &ruleID)
{
	try {
		getStorage().remove<DB::URLRule>(easy_stoll(ruleID));
		return true;
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to delete rule: " << e.what();
		return false;
	}
}

std::string UrlPreview::ShowURLRules()
{
	std::string result = "URL rules:";
	for (auto &rule : getStorage().get_all<DB::URLRule>())
		result.append("\n" + getStorage().dump(rule));

	return result;
}

void UrlPreview::MigrateHistory()
{
	LevelDBPersistentMap urlHistory;
	urlHistory.init("urlhistory", _botPtr->GetDBPathPrefix());

	if (!urlHistory.isOK() || urlHistory.isEmpty())
		return;

	int recordCount = 0;
	urlHistory.ForEach([&](std::pair<std::string, std::string> record)->bool{
		try {
			auto now = std::chrono::system_clock::now();
			auto splitter = record.second.find_first_of(' ');

			if (splitter == record.second.npos)
				throw(std::runtime_error("No splitter found"));

			auto url = record.second.substr(0, splitter);
			auto title = record.second.substr(splitter + 1);

			DB::LoggedURL newRecord = { -1, url, title,
								 std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count(),
									  url + " " + title};
			getStorage().insert(newRecord);
			LOG(INFO) << "Imported URL #" << newRecord.id;
			recordCount++;
		} catch (std::exception &e) {
			LOG(ERROR) << "Failed to import url record: " << e.what();
		}

		return true;
	});

	urlHistory.Clear();
	SendMessage("URLs from history imported: " + std::to_string(recordCount));
}

void UrlPreview::MigrateRules()
{
	LevelDBPersistentMap urlWhitelist;
	LevelDBPersistentMap urlBlacklist;
	urlWhitelist.init("urlwhitelist", _botPtr->GetDBPathPrefix());
	urlBlacklist.init("urlblacklist", _botPtr->GetDBPathPrefix());

	if (!urlWhitelist.isOK() || !urlBlacklist.isOK())
		return;

	if (urlWhitelist.isEmpty() && urlBlacklist.isEmpty())
		return;

	int recordCount = 0;
	urlWhitelist.ForEach([&](std::pair<std::string, std::string> record)->bool{
		addRuleToRuleset(record.second, false);
		recordCount++;
		return true;
	});

	urlBlacklist.ForEach([&](std::pair<std::string, std::string> record)->bool{
		addRuleToRuleset(record.second, true);
		recordCount++;
		return true;
	});

	urlWhitelist.Clear();
	urlBlacklist.Clear();
	SendMessage("URL Rules migrated: " + std::to_string(recordCount));
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
	UrlPreviewTestBot() : LemonBot(":memory:") { }

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
