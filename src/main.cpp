#include <iostream>

#include "bot.h"
#include "settings.h"

#include "datastorage.h"

#include "gtest/gtest.h"

int main(int argc, char **argv)
{
	// FIXME: add actual option, like --test
	if (argc > 1)
	{
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
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
