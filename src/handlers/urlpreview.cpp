#include "urlpreview.h"

#include <iostream>
#include <map>

#include <curl/curl.h>

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
	: LemonHandler(bot)
{
	readConfig(bot);
}

bool UrlPreview::HandleMessage(const std::string &from, const std::string &body)
{
	std::string site, url;
	if (!findURL(body, site, url))
		return false;

	if (_URLwhitelist.find(site) == _URLwhitelist.end())
		return false;

	std::string siteContent = CurlRequest(url);
	std::string title = "Can't get page title";
	getTitle(siteContent, title);

	SendMessage(formatHTMLchars(title));
	return true;
}

const std::string UrlPreview::GetVersion() const
{
	return "UrlPreview: 0.2";
}

void UrlPreview::readConfig(LemonBot *bot)
{
	auto unparsedWhitelist = bot->GetRawConfigValue("URLwhitelist");
	size_t prevPos = 0;
	for (size_t pos = 0; pos < unparsedWhitelist.length(); pos++)
	{
		if (unparsedWhitelist.at(pos) == ';')
		{
			auto url = unparsedWhitelist.substr(prevPos, pos - prevPos);
			std::cout << "Whitelisted URL: " << url << std::endl;
			_URLwhitelist.insert(url);
			prevPos = pos + 1;
		}
	}
}

bool UrlPreview::findURL(const std::string &input, std::string &site, std::string &url)
{
	auto loc = input.find("http://");

	if (loc == input.npos)
		loc = input.find("https://");

	if (loc == input.npos)
		return false;

	url = input.substr(loc, input.find_first_of(" \n;)", loc) - loc);

	try {
		site = url.substr(url.find("//") + 2, url.find("/", 9) - 2 - url.find("//")); // FIXME magic number
	} catch (...) {
		return false;
	}

	if (site.length() > 3 && site.substr(0, 4) == "www.")
		site = site.substr(4);

	return true;
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

#ifdef _BUILD_TESTS

#include <gtest/gtest.h>
#include <fstream>

class TestBot : public LemonBot
{
public:
	void SendMessage(const std::string &text) const
	{
		_lastMessage = text;
	}

	const std::string GetRawConfigValue(const std::string &name) const
	{
		if (name == "URLwhitelist")
			return "youtube.com;youtu.be;store.steampowered.com;";
		else
			return "";
	}

	mutable std::string _lastMessage;
};

TEST(URLPreview, HTMLSpecialChars)
{
	std::string input = "&quot;&amp;&gt;&lt;";

	EXPECT_EQ("\"&><", formatHTMLchars(input));
}

TEST(URLPreview, GetTitle)
{
	TestBot testBot;
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

#endif
