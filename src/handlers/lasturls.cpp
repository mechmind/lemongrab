#include "lasturls.h"

#include <event.h>
#include <event2/http.h>
#include <event2/thread.h>

#include <glog/logging.h>

#include "util/stringops.h"

LastURLs::LastURLs(LemonBot *bot)
	: LemonHandler("lasturls", bot)
{
	InitLibeventServer();
}

LastURLs::~LastURLs()
{
	if (_breakLoop)
		event_active(_breakLoop, EV_READ, 0);
	_httpServer.join();
}

LemonHandler::ProcessingResult LastURLs::HandleMessage(const std::string &from, const std::string &body)
{
	auto sites = findURLs(body);

	for (auto &site : sites)
	{
		_urlHistory.push_front(site.url);
		if (_urlHistory.size() > maxLength)
			_urlHistory.pop_back();
	}

	return ProcessingResult::KeepGoing;
}

const std::string LastURLs::GetVersion() const
{
	return "0.1";
}

const std::string LastURLs::GetHelp() const
{
	return "URLs history is at: " + GetRawConfigValue("historyurl");
}

void LastURLs::httpHandlerUrls(evhttp_request *request, void *arg) {
	auto parent = static_cast<LastURLs*>(arg);
	auto output = evhttp_request_get_output_buffer(request);

	evbuffer_add_printf(output, "<!DOCTYPE html><html><head><title>URL history</title></head><body>");
	for (auto &site : parent->_urlHistory)
		evbuffer_add_printf(output, "<p><a href=\"%s\">%s</a></p>", site.c_str(), site.c_str());

	evbuffer_add_printf(output, "</body></html>");

	evhttp_send_reply(request, HTTP_OK, "OK", output);
}

void LastURLs::terminateServerUrls(int, short int, void * parentPtr)
{
	auto parent = static_cast<LastURLs*>(parentPtr);
	LOG(INFO) << "Terminating url history listener...";
	event_base_loopbreak(parent->_eventBase);
}

void LastURLs::httpServerThreadUrls(LastURLs * parent, std::uint16_t port)
{
	parent->_eventBase = event_base_new();
	auto httpServer = evhttp_new(parent->_eventBase);
	parent->_breakLoop = event_new(parent->_eventBase, -1, EV_READ, terminateServerUrls, parent);
	event_add(parent->_breakLoop, nullptr);
	if (evhttp_bind_socket(httpServer, "0.0.0.0", port) == -1)
		LOG(ERROR) << "Can't bind socket on port " << port;
	evhttp_set_gencb(httpServer, httpHandlerUrls, parent);

	evthread_use_pthreads();
	evthread_make_base_notifiable(parent->_eventBase);

	if (event_base_dispatch(parent->_eventBase) == -1)
		LOG(ERROR) << "Failed to start event loop";
}

bool LastURLs::InitLibeventServer()
{
	auto portFromOptions = GetRawConfigValue("LastURLsPort");

	std::uint16_t port = 6666;
	if (!portFromOptions.empty())
	{
		try {
			port = std::stol(portFromOptions);
		} catch (std::exception &e) {
			LOG(WARNING) << "Invalid LastURLsPort in config.ini (" << e.what() << ")";
		}
	}

	_httpServer = std::thread (&httpServerThreadUrls, this, port);
	return true;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

TEST(LastURLs, GatherTest)
{
	LastURLs t(nullptr);
	t.HandleMessage("Bob", "http://example.com/?test http://test.com/page#anchor");
	t.HandleMessage("Alice", "http://test.ru/");

	std::list<std::string> expectedHistory = {
		"http://test.ru/",
		"http://test.com/page#anchor",
		"http://example.com/?test",
	};
	ASSERT_EQ(expectedHistory.size(), t._urlHistory.size());
	EXPECT_TRUE(std::equal(expectedHistory.begin(), expectedHistory.end(), t._urlHistory.begin()));
}

#endif // LCOV_EXCL_STOP
