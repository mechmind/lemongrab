#pragma once

#include <memory>
#include <thread>

#include "lemonhandler.h"

class evhttp_request;

class GithubWebhooks : public LemonHandler
{
public:
	GithubWebhooks(LemonBot *bot);
	bool HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;

private:
	bool InitLibeventServer();

	std::thread _httpServer;
	friend void httpHandler(evhttp_request *request, void *arg);
};
