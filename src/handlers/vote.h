#pragma once

#include "lemonhandler.h"

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>

class Poll
{
public:
	std::string theme;
	std::vector<std::unordered_set<std::string>> votes;
	std::vector<std::string> options;
	std::unordered_set<std::string> invitations;
	std::string owner;

	bool multioption = false;

	std::string Print(bool votecount) const;
};

class Voting : public LemonHandler
{
public:
	Voting(LemonBot * bot);

	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	void HandlePresence(const std::string &from, const std::string &jid, bool connected);
	const std::string GetHelp() const;

private:
	void ListPolls();
	void PrintPoll(const std::string &id);
	void Vote(const std::string &args, const std::string &voter);
	void Unvote(const std::string &args, const std::string &voter);
	void AddPoll(const std::string &description, const std::string &owner);
	void ClosePoll(const std::string &args, const std::string &requester);
	void Invite(const std::string &args, const std::string &requester);

private:
	std::unordered_map<std::string, Poll> _activePolls;
};
