#pragma once

#include "lemonhandler.h"

#include <list>
#include <thread>

class event_base;
class evhttp_request;
class event;

#ifdef _BUILD_TESTS
#include <gtest/gtest_prod.h>
#endif

class LastURLs : public LemonHandler
{
public:
	LastURLs(LemonBot *bot);
	~LastURLs();
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	std::list<std::string> _urlHistory;
	static constexpr int maxLength = 100;

	// FIXME put common code in util somewhere
	bool InitLibeventServer();

	std::thread _httpServer;
	event_base *_eventBase = nullptr;
	event *_breakLoop = nullptr;

	static void terminateServerUrls(int, short int, void *parentPtr);
	static void httpServerThreadUrls(LastURLs * parent, std::uint16_t port);
	static void httpHandlerUrls(evhttp_request *request, void *arg);

#ifdef _BUILD_TESTS
	FRIEND_TEST(LastURLs, GatherTest);
#endif
};
