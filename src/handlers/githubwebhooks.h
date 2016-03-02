#pragma once

#include <memory>
#include <thread>

#include "lemonhandler.h"

class event_base;
class evhttp_request;
class event;

class GithubWebhooks : public LemonHandler
{
public:
	GithubWebhooks(LemonBot *bot);
	~GithubWebhooks();
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;

private:
	bool InitLibeventServer();

	std::thread _httpServer;
	event_base *_eventBase = nullptr;
	event *_breakLoop = nullptr;

	friend void terminateServer(int, short int, void *arg);
	friend void httpServerThread(GithubWebhooks * parent, std::uint16_t port);
	friend void httpHandler(evhttp_request *request, void *arg);
};
