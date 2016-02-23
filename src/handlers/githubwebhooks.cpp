#include "githubwebhooks.h"

#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>

#include <event.h>
#include <event2/http.h>

#include <jsoncpp/json/reader.h>

GithubWebhooks::GithubWebhooks(LemonBot *bot)
	: LemonHandler("github", bot)
{
	InitLibeventServer();
}

bool GithubWebhooks::HandleMessage(const std::string &from, const std::string &body)
{
	return true;
}

const std::string GithubWebhooks::GetVersion() const
{
	return GetName() + ": 0.1";
}

void httpHandler(evhttp_request *request, void *arg) {
	// FIXME: look up valid responses
	// FIXME: clean this mess up
	GithubWebhooks * parent = static_cast<GithubWebhooks*>(arg);
	std::string githubHeader;

	// anno domini 2016, let's enjoy bare linked lists in a C library
	auto *inputHeader = evhttp_request_get_input_headers(request);
	for (auto header = inputHeader->tqh_first; header; header = header->next.tqe_next)
	{
		auto headerName = std::string(header->key);
		std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
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
	size_t length = evbuffer_get_length(input);

	std::vector<char> buffer;
	buffer.resize(length);
	evbuffer_remove(input, buffer.data(), length);

	Json::Value root;
	Json::Reader reader;

	bool success = reader.parse(buffer.data(), buffer.data() + length, root);

	if (!success)
	{
		std::cout << "Can't parse json payload" << std::endl;
		auto *output = evhttp_request_get_output_buffer(request);
		evhttp_send_reply(request, HTTP_BADREQUEST, "Can't parse json payload", output);
		evbuffer_add_printf(output, "Can't parse json");
		return;
	}

	if (githubHeader == "issues")
	{
		auto action = root["action"].asString();
		auto sender = root["sender"]["login"].asString();
		auto repository = root["repository"]["full_name"].asString();
		auto title =  root["issue"]["title"].asString();

		parent->SendMessage("Issue " + action + " \"" + title + "\" by " + sender + " in " + repository);
	}

	auto *output = evhttp_request_get_output_buffer(request);
	evhttp_send_reply(request, HTTP_OK, "OK", output);
}

void httpServerThread(GithubWebhooks * parent)
{
	auto _eventBase = event_base_new();
	auto _httpServer = evhttp_new(_eventBase);
	evhttp_bind_socket(_httpServer, "0.0.0.0", 5555);
	evhttp_set_gencb(_httpServer, httpHandler, parent);
	event_base_dispatch(_eventBase);
}

bool GithubWebhooks::InitLibeventServer()
{
	_httpServer = std::thread (&httpServerThread, this);
	return true;
}

