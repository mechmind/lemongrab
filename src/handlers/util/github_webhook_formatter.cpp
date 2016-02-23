#include "github_webhook_formatter.h"

#include <jsoncpp/json/json.h>

std::string PrintIssue(const Json::Value &input)
{
	auto action = input["action"].asString();

	if (action == "labeled" || action == "unlabeled" || action == "unassigned")
		return "";

	auto sender = input["sender"]["login"].asString();
	auto repo   = input["repository"]["full_name"].asString();
	auto title  = input["issue"]["title"].asString();
	auto number = input["issue"]["number"].asString();

	return "Issue #" + number + " \"" + title + "\" " + action + " by " + sender + " in " + repo;
}

std::string PrintFork(const Json::Value &input)
{
	auto forkName = input["forkee"]["full_name"].asString();
	auto repoName = input["repository"]["full_name"].asString();
	return "Repository " + repoName + " forked as " + forkName;
}

std::string PrintStarred(const Json::Value &input)
{
	auto repo = input["repository"]["full_name"].asString();
	auto user = input["sender"]["login"].asString();
	return "Repository " + repo + " was starred by " + user;
}

std::string PrintPullrequest(const Json::Value &input)
{
	auto action = input["action"].asString();

	if (action == "labeled"
			|| action == "unlabeled"
			|| action == "unassigned"
			|| action == "synchronize")
		return "";

	auto sender = input["sender"]["login"].asString();
	auto repo   = input["repository"]["full_name"].asString();
	auto title  = input["pull_request"]["title"].asString();
	auto number = input["pull_request"]["number"].asString();

	return "Pull request #" + number + " \"" + title + "\" " + action + " by " + sender + " in " + repo;
}

GithubWebhookFormatter::FormatResult GithubWebhookFormatter::FormatWebhookMessage(const std::string &hookType, const std::vector<char> &input, std::string &output)
{
	Json::Value root;
	Json::Reader reader;

	bool success = reader.parse(input.data(), input.data() + input.size(), root);

	if (!success)
		return FormatResult::JSONParseError;

	if (hookType == "issues")
		output = PrintIssue(root);
	else if (hookType == "fork")
		output = PrintFork(root);
	else if (hookType == "watch")
		output = PrintStarred(root);
	else if (hookType == "pull_request")
		output = PrintPullrequest(root);

	return !output.empty() ? FormatResult::OK : FormatResult::IgnoredHook;
}
