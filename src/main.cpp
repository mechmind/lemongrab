#include <iostream>
#include <string>
#include <memory>

#ifdef _BUILD_TESTS
	#include <gtest/gtest.h>
#endif

#include <glog/logging.h>

#include "bot.h"
#include "glooxclient.h"
#include "cliclient.h"
#include "settings.h"
#include "handlers/util/stringops.h"
#include "handlers/util/thread_util.h"

void InitGLOG(char **argv, const std::string &prefix)
{
	std::cout << "Initializing glog..." << std::endl;

	google::InitGoogleLogging(argv[0]);
	google::SetLogDestination(google::INFO, (prefix + "/info_").c_str());
	google::SetLogDestination(google::WARNING, (prefix + "/warning_").c_str());
	google::SetLogDestination(google::ERROR, (prefix + "/error_").c_str());
	google::SetLogDestination(google::FATAL, (prefix + "/fatal_").c_str());
	google::InstallFailureSignalHandler();
}

int main(int argc, char **argv)
{
	initLocale();

	std::string configPath = "/etc/lemongrab/config.toml";
	bool cliTestMode = false;

	if (argc > 1 && std::string(argv[1]) == "--test")
	{
#ifdef _BUILD_TESTS
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
#else
		std::cout << "Tests are not built" << std::endl;
		return 1;
#endif
	} else if (argc > 1 && std::string(argv[1]) == "--cli") {
		configPath = "config.toml";
		cliTestMode = true;
	} else if (argc > 1 && std::string(argv[1]) == "--local") {
		configPath = "config.toml";
	}

	Settings settings;

	if (!settings.Open(configPath))
	{
		LOG(ERROR) << "Failed to read config file";
		return -1;
	}

	InitGLOG(argv, settings.GetLogPrefixPath());
	// nameThisThread("XMPP Client");

	Bot::ExitCode exitCode = Bot::ExitCode::Error;
	do {
		auto botPtr = std::make_shared<Bot>(cliTestMode ?
												static_cast<XMPPClient*>(new ConsoleClient())
											  : static_cast<XMPPClient*>(new GlooxClient()),
											settings);

		exitCode = botPtr->Run();

		if (exitCode == Bot::ExitCode::RestartRequested
				&& !settings.Open(configPath))
		{
			LOG(ERROR) << "Failed to reload config file";
			return -1;
		}
	} while (exitCode != Bot::ExitCode::TerminationRequested);

	LOG(INFO) << "Exiting...";
	return 0;
}
