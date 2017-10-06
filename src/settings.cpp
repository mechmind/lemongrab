#include "settings.h"

#include <iostream>
#include <cpptoml.h>

Settings::Settings()
{
}

bool Settings::Open(const std::string &path)
{
	_originalPath = path;

	try {
		_config = cpptoml::parse_file(path);
	} catch (const cpptoml::parse_exception &e) {
		std::cerr << "Failed to read config file (" << e.what() << ")" << std::endl;
		return false;
	}

	auto jid = _config->get_qualified_as<std::string>("General.JID");
	auto password = _config->get_qualified_as<std::string>("General.Password");
	auto muc = _config->get_qualified_as<std::string>("General.MUC");

	if (!jid || !password || !muc)
	{
		std::cerr << "General.JID, Password and MUC parameters are mandatory";
		return false;
	}

	_JID = *jid;
	_password = *password;
	_MUC = *muc;

	auto dbPath = _config->get_qualified_as<std::string>("General.DBPathPrefix");
	if (dbPath)
		_dbPrefixPath = *dbPath;

	auto logPath = _config->get_qualified_as<std::string>("General.LogPathPrefix");
	if (logPath)
		_logPrefixPath = *logPath;

	return true;
}

bool Settings::Reload()
{
	if (_originalPath.empty())
		return false;

	return Open(_originalPath);
}

const std::string &Settings::GetUserJID() const
{
	return _JID;
}

const std::string &Settings::GetMUC() const
{
	return _MUC;
}

const std::string &Settings::GetPassword() const
{
	return _password;
}

const std::string &Settings::GetLogPrefixPath() const
{
	return _logPrefixPath;
}

const std::string &Settings::GetDBPrefixPath() const
{
	return _dbPrefixPath;
}

std::string Settings::GetRawString(const std::string &name) const
{
	return _config->get_qualified_as<std::string>(name).value_or("");
}

std::set<std::string> Settings::GetStringSet(const std::string &name) const
{
	std::set<std::string> result;

	auto array = _config->get_array_qualified(name);
	if (!array)
		return result;

	auto values = array->array_of<std::string>();

	for (const auto& value : values)
	{
		if (value)
			result.insert(value->get());
	}

	return result;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

TEST(Settings, Read)
{
	Settings test;
	ASSERT_TRUE(test.Open("test/config.toml.test"));

	EXPECT_EQ("jid_value", test.GetUserJID());
	EXPECT_EQ("password_value", test.GetPassword());
	EXPECT_EQ("muc_value", test.GetMUC());

	EXPECT_EQ("StringValue", test.GetRawString("TestGroup.StringName"));
	auto stringSet = test.GetStringSet("TestGroup.StringSetName");

	EXPECT_EQ(2, stringSet.size());
	EXPECT_EQ("StringValue1", *stringSet.begin());
	EXPECT_EQ("StringValue2", *stringSet.rbegin());
}

TEST(Settings, Reload)
{
	Settings test;
	ASSERT_TRUE(test.Open("test/config.toml.test"));
	ASSERT_TRUE(test.Reload());
}

TEST(Settings, Errors)
{
	Settings test;

	EXPECT_FALSE(test.Open("non-existent"));
	ASSERT_TRUE(test.Open("test/config.toml.test"));

	EXPECT_TRUE(test.GetRawString("NonExistent").empty());
	EXPECT_TRUE(test.GetStringSet("NonExistent").empty());
}

#endif // LCOV_EXCL_STOP
