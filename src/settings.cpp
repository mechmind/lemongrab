#include "settings.h"

#include <glog/logging.h>
#include <cpptoml.h>

template std::list<std::string> Settings::GetArray(const std::string &name) const;

Settings::Settings()
{
}

bool Settings::Open(const std::string &path)
{
	_originalPath = path;

	try {
		_config = cpptoml::parse_file(path);
	} catch (const cpptoml::parse_exception &e) {
		LOG(ERROR) << "Failed to read config file (" << e.what() << ")";
		return false;
	}

	auto jid = _config->get_qualified_as<std::string>("General.JID");
	auto password = _config->get_qualified_as<std::string>("General.Password");
	auto muc = _config->get_qualified_as<std::string>("General.MUC");

	if (!jid || !password || !muc)
	{
		LOG(ERROR) << "General.JID, Password and MUC parameters are mandatory";
		return false;
	}

	_JID = *jid;
	_password = *password;
	_MUC = *muc;
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

template <template<class...> class Container, class Element>
Container<Element> Settings::GetArray(const std::string &name) const
{
	Container<Element> result;

	auto array = _config->get_array_qualified(name);
	if (!array)
		return result;

	auto values = array->array_of<Element>();

	for (const auto& value : values)
	{
		if (value)
			result.push_back(value->get());
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

	auto set = test.GetArray<std::vector, std::int64_t>("TestGroup.NumArray");
	EXPECT_EQ(3, set.size());
	EXPECT_EQ(4, set.at(0));
	EXPECT_EQ(5, set.at(1));
	EXPECT_EQ(6, set.at(2));
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

	auto isEmpty = test.GetArray<std::list, std::string>("NonExistent").empty();
	EXPECT_TRUE(isEmpty);
}

#endif // LCOV_EXCL_STOP
