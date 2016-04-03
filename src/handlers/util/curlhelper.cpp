#include "curlhelper.h"

#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}

std::pair<std::string, long> CurlRequest(std::string url)
{
	std::pair<std::string, long> response = {"", -1};
	CURL *curl = nullptr;
	curl = curl_easy_init();
	if (!curl)
		return {"curl Fail", -1};

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.first);
	if (curl_easy_perform(curl) == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.second);

	curl_easy_cleanup(curl);

	return response;
}
