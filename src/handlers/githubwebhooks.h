#pragma once

#include <thread>
#include <string>
#include <cstdint>

#include "lemonhandler.h"

class event_base; // NOLINT
class evhttp_request; // NOLINT
class event; // NOLINT

class GithubWebhooks : public LemonHandler
{
public:
	GithubWebhooks(LemonBot *bot);
	~GithubWebhooks();
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);

private:
	bool InitLibeventServer();

	std::thread _httpServer;
	event_base *_eventBase = nullptr;
	event *_breakLoop = nullptr;

	friend void terminateServer(int, short int, void *arg);
	friend void httpServerThread(GithubWebhooks * parent, std::uint16_t port);
	friend void httpHandler(evhttp_request *request, void *arg);
};
