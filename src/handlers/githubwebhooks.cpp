#include "githubwebhooks.h"

#include <memory>
#include <vector>
#include <algorithm>

#include <event.h>
#include <event2/http.h>
#include <event2/thread.h>

#include <glog/logging.h>

#include "util/github_webhook_formatter.h"
#include "util/stringops.h"

#include <jsoncpp/json/reader.h>

GithubWebhooks::GithubWebhooks(LemonBot *bot)
	: LemonHandler("github", bot)
{
	InitLibeventServer();
}

GithubWebhooks::~GithubWebhooks()
{
	event_active(_breakLoop, EV_READ, 0);
	_httpServer.join();
}

LemonHandler::ProcessingResult GithubWebhooks::HandleMessage(const std::string &from, const std::string &body)
{
	return ProcessingResult::KeepGoing;
}

const std::string GithubWebhooks::GetVersion() const
{
	return "0.2";
}

void httpHandler(evhttp_request *request, void *arg) {
	// TODO: look up valid responses
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
		evhttp_send_reply(request, HTTP_BADREQUEST, "Can't parse json payload", output);
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
	auto httpServer = evhttp_new(parent->_eventBase);
	parent->_breakLoop = event_new(parent->_eventBase, -1, EV_READ, terminateServer, parent);
	event_add(parent->_breakLoop, nullptr);
	evhttp_bind_socket(httpServer, "0.0.0.0", port);
	evhttp_set_gencb(httpServer, httpHandler, parent);

	evthread_use_pthreads();
	evthread_make_base_notifiable(parent->_eventBase);

	event_base_dispatch(parent->_eventBase);
}

bool GithubWebhooks::InitLibeventServer()
{
	auto portFromOptions = GetRawConfigValue("GithubWebhookPort");

	std::uint16_t port = 5555;
	if (!portFromOptions.empty())
	{
		try {
			port = std::stol(portFromOptions);
		} catch (std::exception &e) {
			LOG(WARNING) << "Invalid GithubWebhookPort in config.ini (" << e.what() << ")";
		}
	}

	_httpServer = std::thread (&httpServerThread, this, port);
	return true;
}

