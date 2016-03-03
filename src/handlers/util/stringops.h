#pragma once

#include <string>
#include <vector>
#include <list>

void initLocale();

std::string toLower(const std::string &input);

std::vector<std::string> tokenize(const std::string &input, char separator);

class URL
{
public:
	URL(const std::string &url_, const std::string &hostname_)
		: url(url_)
		, hostname(hostname_)
	{ }
	std::string url;
	std::string hostname;
};

std::list<URL> findURLs(const std::string &input);
