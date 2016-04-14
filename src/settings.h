#pragma once

#include <string>
#include <unordered_map>

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

private:
	std::string _originalPath;
	std::string _JID;
	std::string _MUC;
	std::string _password;

	std::unordered_map<std::string, std::string> _rawSettings;
};
