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
