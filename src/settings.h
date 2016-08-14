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

	std::string GetRawString(const std::string &name) const;
	std::list<std::string> GetStringList(const std::string &name) const;
	std::set<std::string> GetStringSet(const std::string &name) const;

private:
	std::shared_ptr<cpptoml::table> _config;

	std::string _JID;
	std::string _MUC;
	std::string _password;

	std::string _originalPath;
};
