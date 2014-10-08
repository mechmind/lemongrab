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
			m_RawSettings[name] = value;
		}
	}

	// Parsing config
	m_UserJID = m_RawSettings["JID"];
	if (m_UserJID.empty())
		return false;

	m_Password = m_RawSettings["Password"];
	if (m_Password.empty())
		return false;

	m_MUC = m_RawSettings["MUC"];
	if (m_MUC.empty())
		return false;

	return true;
}
