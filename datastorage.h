#pragma once

#include <string>

class Credentials
{
public:
	std::string m_Password;
	std::string m_JID;
};

class IDataStorage
{
public:
	virtual ~IDataStorage() { }

	virtual bool ReadFromFile(std::string path) = 0;

	virtual const Credentials &GetCredentials() const = 0;
	virtual const std::string &GetConference() const = 0;

	virtual bool IsAdmin(std::string JID) const = 0;
};

class DataStorage
		: public IDataStorage
{
public:
	DataStorage();

	bool ReadFromFile(std::string path);

	const Credentials &GetCredentials() const;
	const std::string &GetConference() const;

	bool IsAdmin(std::string JID) const
	{
		return true;
	}

private:
	bool m_Initialized = false;
	Credentials m_Credentials;
	std::string m_Conference;
};
