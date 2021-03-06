#pragma once

#include <string>
#include <thread>
#include <future>

#include "lemonhandler.h"

class Warframe : public LemonHandler
{
public:
	explicit Warframe(LemonBot *bot);
	bool Init() final;
	~Warframe() final;
	ProcessingResult HandleMessage(const ChatMessage &msg) final;
	const std::string GetHelp() const override;

	std::thread _updateThread;
	bool _isRunning = false;
	int _updateSecondsMax = 0;
	friend void UpdateThread(Warframe *parent);
	bool isOfIntereest(const std::string &description);

	void Update();

private:
	std::set<std::string> _guids;

	std::promise<void> _stopThread;
};
