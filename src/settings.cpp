#include "settings.h"

#include <fstream>
#include <iostream>

Settings::Settings()
{
}

bool Settings::Open(const std::string &path)
{
	// TODO: Better config reading
	std::ifstream ini(path, std::ifstream::in);

	std::string line;
	while (std::getline(ini, line))
	{
		unsigned int loc = line.find_first_of('=');
		if (loc != line.npos)
		{
			auto name = line.substr(0, loc);
			auto value = line.substr(loc + 1, line.npos);
			std::cout << "Reading config: " << name << " is set to " << value << std::endl;
			_rawSettings[name] = value;
		}
	}

	// Parsing config
	_JID = _rawSettings["JID"];
	if (_JID.empty())
		return false;

	_password = _rawSettings["Password"];
	if (_password.empty())
		return false;

	_MUC = _rawSettings["MUC"];
	if (_MUC.empty())
		return false;

	return true;
}
