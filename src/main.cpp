#include <iostream>

#ifdef _BUILD_TESTS
	#include <gtest/gtest.h>
#endif

#include <glog/logging.h>

#include "bot.h"
#include "glooxclient.h"
#include "settings.h"
#include "handlers/util/stringops.h"

void InitGLOG(char **argv)
{
	std::cout << "Initializing glog..." << std::endl;

	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::INFO, "logs/info_");
	google::SetLogDestination(google::WARNING, "logs/warning_");
	google::SetLogDestination(google::ERROR, "logs/error_");
	google::SetLogDestination(google::FATAL, "logs/fatal_");
	google::InstallFailureSignalHandler();
}

int main(int argc, char **argv)
{
	initLocale();

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

	InitGLOG(argv);

	Settings settings;
	if (!settings.Open("config.ini"))
		LOG(FATAL) << "Failed to read config file";

	Bot bot(new GlooxClient(), settings);
	bot.Run();

	LOG(INFO) << "Exiting...";
	return 0;
}
