#pragma once

#include <string>
#include <unordered_map>

class Settings
{
public:
	Settings();
	bool Open(const std::string &path);

public:
	const std::string &GetUserJID() const
	{
		return _JID;
	}

	const std::string &GetMUC() const
	{
		return _MUC;
	}

	const std::string &GetPassword() const
	{
		return _password;
	}

	std::string GetRawString(const std::string &name) const
	{
		auto it = _rawSettings.find(name);
		if (it == _rawSettings.end())
			return "";

		return it->second;
	}

private:
	std::string _JID;
	std::string _MUC;
	std::string _password;

	std::unordered_map<std::string, std::string> _rawSettings;
};
