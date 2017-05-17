#pragma once

#include <thread>
#include <string>
#include <cstdint>

#include "lemonhandler.h"

class event_base; // NOLINT
class evhttp_request; // NOLINT
class event; // NOLINT
class evhttp; // NOLINT

class GithubWebhooks : public LemonHandler
{
public:
	GithubWebhooks(LemonBot *bot);
	bool Init() final;
	~GithubWebhooks() override;
	ProcessingResult HandleMessage(const ChatMessage &msg) final;

private:
	bool InitLibeventServer();

	std::thread _httpServer;
	event_base *_eventBase = nullptr;
	event *_breakLoop = nullptr;
	evhttp *_evhttp = nullptr;

	friend void terminateServer(int, short int, void *arg);
	friend void httpServerThread(GithubWebhooks * parent, std::uint16_t port);
	friend void httpHandler(evhttp_request *request, void *arg);
};
