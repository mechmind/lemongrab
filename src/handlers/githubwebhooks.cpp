#include "githubwebhooks.h"

#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>

#include <event.h>
#include <event2/http.h>

#include "util/github_webhook_formatter.h"
#include "util/stringops.h"

#include <jsoncpp/json/reader.h>

GithubWebhooks::GithubWebhooks(LemonBot *bot)
	: LemonHandler("github", bot)
{
	InitLibeventServer();
}

bool GithubWebhooks::HandleMessage(const std::string &from, const std::string &body)
{
	return false;
}

const std::string GithubWebhooks::GetVersion() const
{
	return GetName() + ": 0.1";
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
		std::cout << "Can't parse json payload" << std::endl;
		evhttp_send_reply(request, HTTP_BADREQUEST, "Can't parse json payload", output);
		evbuffer_add_printf(output, "Can't parse json");
		return;

	case GithubWebhookFormatter::FormatResult::OK:
		parent->SendMessage(textResponse);

	case GithubWebhookFormatter::FormatResult::IgnoredHook:
		evhttp_send_reply(request, HTTP_OK, "OK", output);
	}
}

void httpServerThread(GithubWebhooks * parent, std::uint16_t port)
{
	auto _eventBase = event_base_new();
	auto _httpServer = evhttp_new(_eventBase);
	evhttp_bind_socket(_httpServer, "0.0.0.0", port);
	evhttp_set_gencb(_httpServer, httpHandler, parent);
	event_base_dispatch(_eventBase);
}

bool GithubWebhooks::InitLibeventServer()
{
	auto portFromOptions = GetRawConfigValue("GithubWebhookPort");

	std::uint16_t port = 5555;
	if (!portFromOptions.empty())
	{
		try {
			port = std::stol(portFromOptions);
		} catch (...) {
			std::cout << "Invalid GithubWebhookPort in config.ini" << std::endl;
		}
	}

	_httpServer = std::thread (&httpServerThread, this, port);
	return true;
}

