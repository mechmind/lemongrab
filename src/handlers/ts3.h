#pragma once

#include <thread>
#include <string>
#include <unordered_map>

#include "lemonhandler.h"

class event_base;
class evdns_base;
class bufferevent;

class TS3 : public LemonHandler
{
public:
	TS3(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	void StartServerQueryClient();
	void Connected(const std::string &clid, const std::string &nick);
	void Disconnected(const std::string &clid);

private:
	enum class State : int
	{
		NotConnected = 0,
		ServerQueryConnected = 1,
		Authrozied = 2,
		VirtualServerConnected = 3,
		Subscribed = 4,
	};

	std::thread _telnetClient;
	State _sqState = State::NotConnected;
	std::unordered_map<std::string, std::string> _clients;

	event_base *base = nullptr;
	evdns_base *dns_base = nullptr;
	bufferevent *bev = nullptr;

	friend void terminateClient(int, short int, void *arg);
	friend void telnetClientThread(TS3 * parent, std::string server);
	friend void eventcb(bufferevent *bev, short events, void *arg);
	friend void readcb(bufferevent *bev, void *arg);
};

