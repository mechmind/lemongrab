#include "vote.h"

#include "util/stringops.h"

Voting::Voting(LemonBot *bot)
	: LemonHandler("voting", bot)
{

}

LemonHandler::ProcessingResult Voting::HandleMessage(const std::string &from, const std::string &body)
{
	std::string args;
	if (body == "!polls")
	{
		ListPolls();
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!addpoll", args)) {
		AddPoll(args, _botPtr->GetJidByNick(from));
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!pollinfo", args)) {
		PrintPoll(args);
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!vote", args)) {
		Vote(args, _botPtr->GetJidByNick(from));
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!unvote", args)) {
		Unvote(args, _botPtr->GetJidByNick(from));
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!closepoll", args)) {
		ClosePoll(args, _botPtr->GetJidByNick(from));
		return ProcessingResult::StopProcessing;
	} else if (getCommandArguments(body, "!invite", args)) {
		Invite(args, _botPtr->GetJidByNick(from));
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

void Voting::HandlePresence(const std::string &from, const std::string &jid, bool connected)
{
	if (!connected)
		return;

	for (auto &poll : _activePolls)
	{
		auto invite = poll.second.invitations.find(jid);

		if (invite == poll.second.invitations.end())
			invite = poll.second.invitations.find(from);

		if (invite == poll.second.invitations.end())
			continue;

		SendMessage(from + " you've been invited to vote on poll " + poll.first + "\n" + poll.second.Print(false));
		poll.second.invitations.erase(invite);
	}
}

const std::string Voting::GetHelp() const
{
	return "!polls - list polls\n"
		   "!addpoll [#]question|option1[|option2...] - new poll (start with # for multioption)\n"
		   "!pollinfo %pollid%\n"
		   "!vote %pollid% %optionid% - cast your vote\n"
		   "!unvote %pollid% - remove your vote(s)\n"
		   "!closepoll - end poll and show results\n"
		   "!invite %pollid%|%jid/nick%[|%jid/nick%...] - invite users to vote on poll";
}

void Voting::ListPolls()
{
	std::string result;
	for (const auto &poll : _activePolls)
		result += poll.first + " :: " + poll.second.theme;

	if (_activePolls.empty())
		result = "No active polls";

	SendMessage(result);
}

void Voting::PrintPoll(const std::string &id)
{
	auto poll = _activePolls.find(id);

	if (poll == _activePolls.end())
	{
		SendMessage("No such poll");
		return;
	}

	SendMessage(poll->second.Print(false));
}

void Voting::Vote(const std::string &args, const std::string &voter)
{
	auto tokens = tokenize(args, ' ');
	if (tokens.size() < 2)
	{
		SendMessage("Not enough paramaters");
		return;
	}

	auto poll = _activePolls.find(tokens.at(0));
	if (poll == _activePolls.end())
	{
		SendMessage("No such poll");
		return;
	}

	size_t vote = easy_stoll(tokens.at(1));
	if (vote >= poll->second.options.size() || vote < 0)
	{
		SendMessage("No such option");
		return;
	}

	if (!poll->second.multioption)
	{
		for (const auto &vote : poll->second.votes)
			if (vote.count(voter) > 0)
			{
				SendMessage("This poll does not allow multiple option selection. Use !unvote to remove your vote first");
				return;
			}
	}

	poll->second.votes.at(vote).insert(voter);
	SendMessage("Vote cast");
}

void Voting::Unvote(const std::string &args, const std::string &voter)
{
	auto poll = _activePolls.find(args);
	if (poll == _activePolls.end())
	{
		SendMessage("No such poll");
		return;
	}

	for (auto &vote : poll->second.votes)
		vote.erase(voter);
}

void Voting::AddPoll(const std::string &description, const std::string &owner)
{
	auto tokens = tokenize(description, '|');
	if (tokens.size() < 3)
	{
		SendMessage("Not enough tokens in poll description");
		return;
	}

	Poll newPoll;
	newPoll.theme = *tokens.begin();
	newPoll.owner = owner;

	if (newPoll.theme.at(0) == '#')
		newPoll.multioption = true;

	for (size_t index = 1; index < tokens.size(); index++)
		newPoll.options.push_back(tokens.at(index));

	newPoll.votes.resize(newPoll.options.size());

	std::hash<std::string>hashfn;
	auto hash = hashfn(newPoll.theme);
	auto id = std::to_string(hash);
	id.resize(4);
	_activePolls[id] = newPoll;

	SendMessage("Poll added with ID: " + id);
}

void Voting::ClosePoll(const std::string &args, const std::string &requester)
{
	auto poll = _activePolls.find(args);
	if (poll == _activePolls.end())
	{
		SendMessage("No such poll");
		return;
	}

	if (poll->second.owner != requester)
	{
		SendMessage("Only owner can close a poll");
		return;
	}

	SendMessage(poll->second.Print(true));
	_activePolls.erase(args);
}

void Voting::Invite(const std::string &args, const std::string &requester)
{
	auto tokens = tokenize(args, '|');
	if (tokens.size() < 2)
	{
		SendMessage("Not enough tokens");
		return;
	}

	auto poll = _activePolls.find(tokens.at(0));
	if (poll == _activePolls.end())
	{
		SendMessage("No such poll");
		return;
	}

	for (size_t index = 1; index < tokens.size(); index++)
		poll->second.invitations.insert(tokens.at(index));
}

std::string Poll::Print(bool votecount) const
{
	std::string result;
	result = theme;

	for (size_t index = 0; index < options.size(); index++)
	{
		result += "\n" + std::to_string(index) + ") " + options.at(index);
		if (votecount)
			result += " [" + std::to_string(votes.at(index).size()) + "]";
	}

	return result;
}
