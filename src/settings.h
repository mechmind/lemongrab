#pragma once

#include <string>
#include <list>
#include <memory>
#include <set>

namespace cpptoml {
	class table;
}

class Settings
{
public:
	Settings();

	bool Open(const std::string &path);
	bool Reload();

public:
	const std::string &GetUserJID() const;
	const std::string &GetMUC() const;
	const std::string &GetPassword() const;

	const std::string &GetLogPrefixPath() const;
	const std::string &GetDBPrefixPath() const;

	std::string GetRawString(const std::string &name) const;
	std::set<std::string> GetStringSet(const std::string &name) const;

private:
	std::shared_ptr<cpptoml::table> _config;

	std::string _JID;
	std::string _MUC;
	std::string _password;

	std::string _originalPath;

	std::string _logPrefixPath = "/var/logs/lemongrab";
	std::string _dbPrefixPath = "/var/lib/lemongrab";
};
