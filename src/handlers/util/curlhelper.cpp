#include "curlhelper.h"

#include <curl/curl.h>
#include <glog/logging.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	reinterpret_cast<std::string*>(userp)->append(reinterpret_cast<char*>(contents), size * nmemb);
	return size * nmemb;
}

std::pair<std::string, long> CurlRequest(std::string url, bool dummy)
{
	std::pair<std::string, long> response = {"", -1};

	if (dummy)
	{
		response.second = 200;
		return response;
	}

	CURL *curl = nullptr;
	curl = curl_easy_init();
	if (!curl)
		return {"curl Fail", -1};

	curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "charsets: utf-8");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXFILESIZE, 10*1024*1024);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.first);

	auto result = curl_easy_perform(curl);

	if (result == CURLE_OK)
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.second);
	else
		LOG(ERROR) << "Curl failed with code " << result;

	curl_easy_cleanup(curl);

	return response;
}
