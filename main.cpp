#include <iostream>

#include "bot.h"
#include "settings.h"

#include "datastorage.h"

int main()
{
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
