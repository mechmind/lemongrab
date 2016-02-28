#include "urlpreview.h"

#include <iostream>
#include <map>

#include <curl/curl.h>

#include "util/stringops.h"

std::string formatHTMLchars(std::string input);

// Curl support
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static std::string CurlRequest(std::string url)
{
	CURL* curl = NULL;
	curl = curl_easy_init();
	if (!curl)
		return "curl Fail";

	// TODO Handle return codes
	std::string readBuffer;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return readBuffer;
}

UrlPreview::UrlPreview(LemonBot *bot)
	: LemonHandler("url", bot)
{
	readConfig(bot);
}

bool UrlPreview::HandleMessage(const std::string &from, const std::string &body)
{
	auto sites = findURLs(body);
	if (sites.empty())
		return false;

	for (auto &site : sites)
	{
		if (_URLwhitelist.find(site.hostname) == _URLwhitelist.end())
			continue;

		std::string siteContent = CurlRequest(site.url);
		std::string title = "Can't get page title";
		getTitle(siteContent, title);

		SendMessage(formatHTMLchars(title));
	}

	return false;
}

const std::string UrlPreview::GetVersion() const
{
	return "0.2";
}

void UrlPreview::readConfig(LemonBot *bot)
{
	auto unparsedWhitelist = bot->GetRawConfigValue("URLwhitelist");

	auto urls = tokenize(unparsedWhitelist, ';');
	for (auto url : urls)
	{
		std::cout << "Whitelisted URL: " << url << std::endl;
		_URLwhitelist.insert(url);
	}
}

bool UrlPreview::getTitle(const std::string &content, std::string &title)
{
	auto titleStart = content.find("<title>");
	if (titleStart == content.npos)
		return false;

	titleStart += 7;

	auto titleEnd = content.find("</title>");
	if (titleEnd == content.npos)
		return false;

	titleEnd -= titleStart;

	try {
		title = content.substr(titleStart, titleEnd);
		return true;
	} catch (...) {
		return false;
	}
}

std::string formatHTMLchars(std::string input)
{
	std::map<std::string, std::string> chars
			= {{"&quot;", "\""},
			   {"&amp;", "&"},
			   {"&lt;", "<"},
			   {"&gt;", ">"}};

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
		std::ifstream youtube("test/youtube.html");
		std::string content((std::istreambuf_iterator<char>(youtube)), std::istreambuf_iterator<char>());
		youtube.close();
		std::string title;
		testUnit.getTitle(content, title);
		EXPECT_EQ("\"Beach Cats\" (Ep. 3 & 4) - Bee and PuppyCat (Cartoon Hangover) - YouTube", formatHTMLchars(title));
	}

	{
		std::ifstream steam("test/steam.html");
		std::string content((std::istreambuf_iterator<char>(steam)), std::istreambuf_iterator<char>());
		steam.close();
		std::string title;
		testUnit.getTitle(content, title);
		EXPECT_EQ("Undertale on Steam", formatHTMLchars(title));
	}
}

#endif // LCOV_EXCL_STOP
