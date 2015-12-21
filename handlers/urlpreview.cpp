#include "urlpreview.h"

#include <iostream>
#include <curl/curl.h>

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

bool UrlPreview::HandleMessage(const std::string &from, const std::string &body)
{
	auto loc = body.find("http://");

	if (loc == body.npos)
		loc = body.find("https://");

	if (loc == body.npos)
		return false;

	auto url = body.substr(loc, body.find(" ", loc));
	std::string site;
	try {
		site = url.substr(url.find("//") + 2, url.find("/", 9) - 2 - url.find("//")); // FIXME magic number
	} catch (...) {
		return false;
	}

	// FIXME make a whitelist
	if (site != "www.youtube.com" && site != "youtube.com" && site != "youtu.be")
		return false;

	std::string siteContent = CurlRequest(url);
	auto titleStart = siteContent.find("<title>");
	if (titleStart == siteContent.npos)
		return false;

	titleStart += 7;

	auto titleEnd = siteContent.find("</title>");
	if (titleEnd == siteContent.npos)
		return false;

	titleEnd -= titleStart;
	std::string title;
	try {
		title = siteContent.substr(titleStart, titleEnd);
	} catch (...)
	{
		return false;
	}
	SendMessage(title);
	return true;
}

const std::string UrlPreview::GetVersion() const
{
	return "UrlPreview: 0.1";
}
