#include "github_webhook_formatter.h"

#include <json/json.h>

std::string PrintIssue(const Json::Value &input)
{
	auto action = input["action"].asString();

	if (action == "labeled" || action == "unlabeled" || action == "unassigned")
		return "";

	auto sender = input["sender"]["login"].asString();
	auto repo   = input["repository"]["full_name"].asString();
	auto title  = input["issue"]["title"].asString();
	auto number = input["issue"]["number"].asString();
	auto url    = input["issue"]["html_url"].asString();

	if (action == "assigned")
	{
		auto assignee = input["assignee"]["login"].asString();
		return "Issue #" + number + " \"" + title + "\" assigned for " + assignee + " by " + sender + " in " + repo + " | " + url;
	} else
		return "Issue #" + number + " \"" + title + "\" " + action + " by " + sender + " in " + repo + " | " + url;
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
	auto url    = input["pull_request"]["html_url"].asString();

	return "Pull request #" + number + " \"" + title + "\" " + action + " by " + sender + " in " + repo + " | " + url;
}

std::string PrintRelease(const Json::Value &input)
{
	auto repo = input["repository"]["full_name"].asString();
	auto tag  = input["release"]["tag_name"].asString();
	auto url  = input["release"]["html_url"].asString();
	auto sender = input["sender"]["login"].asString();

	return "New release " + tag + " published by " + sender + " in " + repo + " | " + url;
}

GithubWebhookFormatter::FormatResult GithubWebhookFormatter::FormatWebhookMessage(const std::string &hookType, const std::vector<char> &input, std::string &output)
{
	Json::Value root;
	Json::Reader reader;

	if (!reader.parse(input.data(), input.data() + input.size(), root))
		return FormatResult::JSONParseError;

	if (hookType == "issues")
		output = PrintIssue(root);
	else if (hookType == "fork")
		output = PrintFork(root);
	else if (hookType == "watch")
		output = PrintStarred(root);
	else if (hookType == "pull_request")
		output = PrintPullrequest(root);
	else if (hookType == "release")
		output = PrintRelease(root);

	return !output.empty() ? FormatResult::OK : FormatResult::IgnoredHook;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include "gtest/gtest.h"

#include <fstream>
#include <streambuf>
#include <iostream>

TEST(GithubFormatter, Print)
{
	{
		std::ifstream issueFile("test/issue.json");
		std::vector<char> issue((std::istreambuf_iterator<char>(issueFile)),
								std::istreambuf_iterator<char>());
		std::string issuePrinted;

		ASSERT_EQ(GithubWebhookFormatter::FormatResult::OK, GithubWebhookFormatter::FormatWebhookMessage("issues", issue, issuePrinted));
		std::cout << issuePrinted << std::endl;
	}

	{
		std::ifstream releaseFile("test/release.json");
		std::vector<char> release((std::istreambuf_iterator<char>(releaseFile)),
								  std::istreambuf_iterator<char>());
		std::string releasePrinted;

		ASSERT_EQ(GithubWebhookFormatter::FormatResult::OK, GithubWebhookFormatter::FormatWebhookMessage("release", release, releasePrinted));
		std::cout << releasePrinted << std::endl;
	}
}

#endif // LCOV_EXCL_STOP
