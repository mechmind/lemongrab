#include "githubwebhooks.h"

#include <vector>
#include <exception>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <event2/http.h>
#include <event2/thread.h>

#include <glog/logging.h>

#include "util/github_webhook_formatter.h"
#include "util/stringops.h"

GithubWebhooks::GithubWebhooks(LemonBot *bot)
	: LemonHandler("github", bot)
{

}

bool GithubWebhooks::Init()
{
	InitLibeventServer();
	return true;
}

GithubWebhooks::~GithubWebhooks()
{
	if (_breakLoop)
		event_active(_breakLoop, EV_READ, 0);
	_httpServer.join();
	if (_evhttp)
		evhttp_free(_evhttp);
}

LemonHandler::ProcessingResult GithubWebhooks::HandleMessage(const ChatMessage &msg)
{
	return ProcessingResult::KeepGoing;
}

void httpHandler(evhttp_request *request, void *arg) {
	GithubWebhooks * parent = static_cast<GithubWebhooks*>(arg);
	std::string githubHeader;

	auto *inputHeader = evhttp_request_get_input_headers(request);
	for (auto header = inputHeader->tqh_first; header; header = header->next.tqe_next)
	{
		auto headerName = toLower(std::string(header->key));
		if (headerName == "x-github-event")
			githubHeader = std::string(header->value);
	}

	if (githubHeader.empty())
	{
		auto *output = evhttp_request_get_output_buffer(request);
		evhttp_send_reply(request, HTTP_BADREQUEST, "Not a github webhook", output);
		evbuffer_add_printf(output, "X-GitHub-Event is missing");
		return;
	}

	auto *input = evhttp_request_get_input_buffer(request);
	auto *output = evhttp_request_get_output_buffer(request);
	size_t length = evbuffer_get_length(input);

	std::vector<char> buffer;
	buffer.resize(length);
	evbuffer_remove(input, buffer.data(), length);

	std::string textResponse;
	auto formatStatus = GithubWebhookFormatter::FormatWebhookMessage(githubHeader, buffer, textResponse);

	switch (formatStatus)
	{
	case GithubWebhookFormatter::FormatResult::JSONParseError:
		LOG(ERROR) << "Can't parse json payload";
		evhttp_send_reply(request, HTTP_INTERNAL, "Can't parse json payload", output);
		evbuffer_add_printf(output, "Can't parse json");
		return;

	case GithubWebhookFormatter::FormatResult::OK:
		parent->SendMessage(textResponse);

	case GithubWebhookFormatter::FormatResult::IgnoredHook:
		evhttp_send_reply(request, HTTP_OK, "OK", output);
	}
}

void terminateServer(int, short int, void * arg)
{
	GithubWebhooks * parent = static_cast<GithubWebhooks*>(arg);
	LOG(INFO) << "Terminating github webhooks listener...";
	event_base_loopbreak(parent->_eventBase);
}

void httpServerThread(GithubWebhooks * parent, std::uint16_t port)
{
	parent->_eventBase = event_base_new();
	parent->_evhttp = evhttp_new(parent->_eventBase);
	parent->_breakLoop = event_new(parent->_eventBase, -1, EV_READ, terminateServer, parent);
	event_add(parent->_breakLoop, nullptr);
	if (evhttp_bind_socket(parent->_evhttp , "0.0.0.0", port) == -1)
	{
		LOG(ERROR) << "Can't bind socket on port " << port;
		return;
	}
	evhttp_set_gencb(parent->_evhttp , httpHandler, parent);

	evthread_use_pthreads();
	evthread_make_base_notifiable(parent->_eventBase);

	if (event_base_dispatch(parent->_eventBase) == -1)
		LOG(ERROR) << "Failed to start event loop";
}

bool GithubWebhooks::InitLibeventServer()
{
	auto portFromOptions = GetRawConfigValue("Github.Port");

	std::uint16_t port = 5555;
	if (!portFromOptions.empty())
	{
		try {
			port = std::stol(portFromOptions);
		} catch (std::exception &e) {
			LOG(WARNING) << "Invalid Github.Port in config.ini (" << e.what() << ")";
		}
	}

	_httpServer = std::thread (&httpServerThread, this, port);
	return true;
}
