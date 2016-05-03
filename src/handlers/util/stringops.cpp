#include "stringops.h"

#include <regex>
#include <boost/locale.hpp>

#include "glog/logging.h"

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

bool beginsWith(const std::string &input, const std::string &prefix)
{
	if (prefix.length() > input.length())
		return false;

	return input.compare(0, prefix.length(), prefix) == 0;
}

bool getCommandArguments(const std::string &input, const std::string &command, std::string &arguments)
{
	if (!beginsWith(input, command))
		return false;

	if (input.size() == command.size())
		return true;

	if (input.at(command.size()) != ' ')
		return false;

	arguments = input.substr(command.size() + 1);
	return true;
}

std::vector<std::string> tokenize(const std::string &input, char separator)
{
	std::vector<std::string> tokens;

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

std::list<URL> findURLs(const std::string &input)
{
	std::list<URL> output;

	static const std::regex trivial_url("(https?://(?:www.)?([[:alnum:].]+)/?[[:alnum:]\\-._~:/?#\\[\\]@!$&'()*+,;=%]*)");

	for (std::sregex_iterator i(input.begin(), input.end(), trivial_url);
		 i != std::sregex_iterator(); ++i)
		output.push_back(URL(i->str(1), i->str(2)));

	return output;
}

std::string CustomTimeFormat(std::chrono::system_clock::duration input)
{
	std::string output;
	output += std::to_string(std::chrono::duration_cast<std::chrono::hours>  (input).count() / 24) + "d ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::hours>  (input).count() % 24) + "h ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::minutes>(input).count() % 60) + "m ";
	output += std::to_string(std::chrono::duration_cast<std::chrono::seconds>(input).count() % 60) + "s";

	return output;
}

long long easy_stoll(const std::string &input)
{
	if (input.empty())
		return 0;

	try {
		return std::stoll(input);
	} catch (std::exception &e) {
		LOG(ERROR) << "stoll for " << input << " failed: " << e.what();
	}

	return 0;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

TEST(StringOps, toLower_ruRU)
{
	boost::locale::generator gen;
	std::locale local = gen.generate("ru_RU.utf8");
	std::locale::global(local);

	std::string input = u8"ТеСт TeSt";

	auto lc = toLower(input);
	EXPECT_EQ("тест test", lc);

	initLocale();
}

TEST(StringOps, findURLs)
{
	auto urls = findURLs("123 https://example.com/test/ 34 http://ya.ru/ 5 http://goo.gl/allo/this?is=a&test#thingy end https://www.youtube.com/watch?v=abcde test");

	std::list<URL> expectedURLs = {
		{"https://example.com/test/", "example.com"},
		{"http://ya.ru/", "ya.ru"},
		{"http://goo.gl/allo/this?is=a&test#thingy", "goo.gl"},
		{"https://www.youtube.com/watch?v=abcde", "youtube.com"},
	};
	ASSERT_EQ(expectedURLs.size(), urls.size());
	EXPECT_TRUE(std::equal(expectedURLs.begin(), expectedURLs.end(), urls.begin()));

	urls = findURLs("https://www.youtube.com/watch?v=lxQjwbUiM9w");
	ASSERT_EQ(1, urls.size());
	EXPECT_EQ("https://www.youtube.com/watch?v=lxQjwbUiM9w", urls.begin()->url);
	EXPECT_EQ("youtube.com", urls.begin()->hostname);
}

TEST(StringOps, beginsWith)
{
	EXPECT_TRUE(beginsWith("!test", "!test"));
	EXPECT_FALSE(beginsWith("!toost", "!test"));
	EXPECT_TRUE(beginsWith("!test", "!test"));
	EXPECT_TRUE(beginsWith("!test!", "!test"));
	EXPECT_FALSE(beginsWith("!tes", "!test"));
	EXPECT_FALSE(beginsWith("?test", "!test"));
}

TEST(StringOps, getArguments)
{
	std::string output;
	EXPECT_FALSE(getCommandArguments("!toost this", "!test", output));
	EXPECT_TRUE(output.empty());

	EXPECT_TRUE(getCommandArguments("!test", "!test", output));
	EXPECT_TRUE(output.empty());

	EXPECT_FALSE(getCommandArguments("!testfail", "!test", output));

	EXPECT_TRUE(getCommandArguments("!test ", "!test", output));
	EXPECT_TRUE(output.empty());

	EXPECT_TRUE(getCommandArguments("!test args", "!test", output));
	EXPECT_EQ("args", output);
}

#endif // LCOV_EXCL_STOP
