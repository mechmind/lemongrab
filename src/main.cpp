#include <iostream>

#include <gtest/gtest.h>

#include "bot.h"
#include "settings.h"

#include "datastorage.h"

int main(int argc, char **argv)
{
	if (argc > 1 && std::string(argv[1]) == "--test")
	{
#ifdef _BUILD_TESTS
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
#else
		std::cout << "Tests are not built" << std::endl;
		return 1;
#endif
	}

	DataStorage data;

	Settings settings;
	if (!settings.Open("config.ini"))
	{
		std::cout << "Failed to read config file" << std::endl;
		return 1;
	}

	Bot bot(settings);
	bot.Init();
	return 0;
}
