#ifndef SETTINGS_H
#define SETTINGS_H

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
		return m_UserJID;
	}

	const std::string &GetMUC() const
	{
		return m_MUC;
	}

	const std::string &GetPassword() const
	{
		return m_Password;
	}

private:
	std::string m_UserJID;
	std::string m_MUC;
	std::string m_Password;

	std::unordered_map<std::string, std::string> m_RawSettings;
};

#endif // SETTINGS_H
