#include "goodenough.h"

#include "util/stringops.h"

#include <list>
#include <algorithm>

bool GoodEnough::HandleMessage(const std::string &from, const std::string &body)
{
	static std::list<std::string> magicPhrases = {
		"так сойдет",
		"так сойдёт",
		"пока так",
		"потом поправлю",
		"good enough"
	};

	static std::string response = "https://youtu.be/WgYhYw-lS_s";

	auto lowercase = toLower(body);

	for (auto &phrase : magicPhrases)
	{
		auto loc = lowercase.find(phrase);
		if (loc != lowercase.npos)
		{
			SendMessage(from + ": " + response);
			return false;
		}
	}

	return true;
}

const std::string GoodEnough::GetVersion() const
{
	return "good enough";
}
