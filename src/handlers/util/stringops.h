#pragma once

#include <string>
#include <vector>
#include <list>
#include <chrono>

void initLocale();

std::string toLower(const std::string &input);

bool beginsWith(const std::string &input, const std::string &prefix);

bool getCommandArguments(const std::string &input, const std::string &command, std::string &arguments);

std::vector<std::string> tokenize(const std::string &input, char separator);

class URL
{
public:
	URL(const std::string &url_, const std::string &hostname_)
		: _url(url_)
		, _hostname(hostname_)
	{ }
	std::string _url;
	std::string _hostname;

	bool operator==(const URL& rhs)
	{
		return _url == rhs._url
				&& _hostname == rhs._hostname;
	}
};

std::list<URL> findURLs(const std::string &input);

std::string CustomTimeFormat(std::chrono::system_clock::duration input);

long long easy_stoll(const std::string &input);
