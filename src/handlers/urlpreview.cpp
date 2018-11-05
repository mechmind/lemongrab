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

std::string formatHTMLchars(std::string input);

UrlPreview::UrlPreview(LemonBot *bot)
	: LemonHandler("url", bot)
{

}

LemonHandler::ProcessingResult UrlPreview::HandleMessage(const ChatMessage &msg)
{
	std::string args;
	auto &body = msg._body;
	if (getCommandArguments(body, "!url", args))
	{
		SendMessage(concatenateURLs(findUrlsInHistory(args), false));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!!!url", args))
	{
		SendMessage(concatenateURLs(findUrlsInHistory(args), true));
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
		if (auto ruleID = from_string<int>(args)) {
			delRuleFromRuleset(*ruleID);
		};
		return ProcessingResult::StopProcessing;
	}

	if (body == "!urlrules")
	{
		SendMessage(ShowURLRules());
		return ProcessingResult::StopProcessing;
	}

	if (msg._hasDiscordEmbed) {
		return ProcessingResult::KeepGoing;
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

		if (shouldPrintTitle(site._url) && urlsFound < maxURLsInOneMessage) {
			auto formattedTitle = formatHTMLchars(title);
			if (!formattedTitle.empty()) {
				SendMessage(formattedTitle, msg._discordChannel);
			}
		}

		urlsFound++;
	}

	return ProcessingResult::KeepGoing;
}

const std::string UrlPreview::GetHelp() const
{
	return "!url %regex% - search in URL history by title or url\n"
		   "!wlisturl %regex% and !blisturl %regex% - enable/disable notifications for specific urls\n"
		   "!delisturl %id% - delete existing rules. !urlrules - print existing rules and their ids";
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

	if (title.length() > 360) {
		return "Title is too long";
	}

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

std::vector<DB::LoggedURL> UrlPreview::findUrlsInHistory(const std::string &request)
{
	using namespace sqlite_orm;
	return getStorage().get_all<DB::LoggedURL>(
				where(like(&DB::LoggedURL::fullText, "%" + request + "%")),
				order_by(&DB::LoggedURL::id).desc(),
				limit(maxURLsInSearch));
}

std::string UrlPreview::concatenateURLs(const std::vector<DB::LoggedURL> &urls, bool withIndices) const
{
	if (urls.empty())
		return "No matches";

	std::string searchResults;
	searchResults = std::to_string(maxURLsInSearch) + " recent matching URLs: \n";
	for (const auto &url : urls)
	{
		searchResults += withIndices
				? (std::to_string(url.id) + ") " + url.URL + " " + url.title + "\n")
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

bool UrlPreview::delRuleFromRuleset(int ruleID)
{
	try {
		getStorage().remove<DB::URLRule>(ruleID);
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
	UrlPreviewTestBot() : LemonBot(":memory:") {
		_storage.sync_schema(true);
	}

	void SendMessage(const std::string &text);
	std::string _lastMessage;
};

void UrlPreviewTestBot::SendMessage(const std::string &text)
{
	_lastMessage = text;
}

TEST(URLPreview, HTMLSpecialChars)
{
	std::string input = "&quot;&amp;&gt;&lt;";
	EXPECT_EQ("\"&><", formatHTMLchars(input));
}

TEST(URLPreview, GetTitle)
{
	UrlPreviewTestBot testBot;
	UrlPreview testUnit(&testBot);

	std::string content("<html>\n"
						"<head><title>This is a test title</title></head>\n"
						"<body><p>Test</p></body>"
						"</html>\n");
	std::string title = testUnit.getTitle(content);
	EXPECT_EQ("This is a test title", formatHTMLchars(title));
}

TEST(URLPreview, History)
{
	UrlPreviewTestBot testBot;
	UrlPreview t(&testBot);

	t.HandleMessage(ChatMessage("Bob", "", "http://example.com/?test http://test.com/page#anchor", false));
	t.HandleMessage(ChatMessage("Alice", "", "http://test.ru/", false));

	std::list<std::string> expectedHistory = {
		"http://test.ru/",
		"http://test.com/page", // FIXME: anchor lost
		"http://example.com/?test",
	};

	auto expectedRecord = expectedHistory.begin();
	auto history = t.findUrlsInHistory("");
	for (const auto &record : history) {
		EXPECT_EQ(record.URL, *expectedRecord++);
	}
}

#endif // LCOV_EXCL_STOP
