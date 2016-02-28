#include "stringops.h"

#include <boost/locale.hpp>

void initLocale()
{
	boost::locale::generator gen;
	std::locale local = gen.generate("");
	std::locale::global(local);
}

std::string toLower(const std::string &input)
{
	return boost::locale::to_lower(input);
}

std::list<std::string> tokenize(const std::string &input, char separator)
{
	std::list<std::string> tokens;

	size_t prevPos = 0;
	for (size_t pos = 0; pos < input.length(); pos++)
	{
		if (input.at(pos) == separator)
		{
			auto token = input.substr(prevPos, pos - prevPos);
			tokens.push_back(token);
			prevPos = pos + 1;
		}
	}
	if (prevPos != input.length())
		tokens.push_back(input.substr(prevPos));

	return tokens;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

TEST(toLower, ruRU)
{
	boost::locale::generator gen;
	std::locale local = gen.generate("ru_RU.utf8");
	std::locale::global(local);

	std::string input = u8"ТеСт TeSt";

	auto lc = toLower(input);
	EXPECT_EQ("тест test", lc);

	initLocale();
}

#endif // LCOV_EXCL_STOP
