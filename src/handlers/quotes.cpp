#include "quotes.h"

#include <glog/logging.h>

#include <chrono>

#include "util/stringops.h"

Quotes::Quotes(LemonBot *bot)
	: LemonHandler("quotes", bot)
	, _generator(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{

}

LemonHandler::ProcessingResult Quotes::HandleMessage(const ChatMessage &msg)
{
	std::string arg;
	if (getCommandArguments(msg._body, "!gq", arg))
	{
		SendMessage(msg._nick + ": " + GetQuote(arg));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!aq", arg) && !arg.empty())
	{
		AddQuote(arg)
				? SendMessage(msg._nick + ": quote added")
				: SendMessage(msg._nick + ": can't add quote");
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!dq", arg) && !arg.empty())
	{
		if (!msg._isAdmin)
		{
			SendMessage(msg._nick + ": only admin can delete quotes");
			return ProcessingResult::StopProcessing;
		}

		if (auto id = from_string<int>(arg)) {
			DeleteQuote(*id)
					? SendMessage(msg._nick + ": quote deleted")
					: SendMessage(msg._nick + ": quote doesn't exist or access denied");
		} else {
			SendMessage("Invalid ID");
		}
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!fq", arg) && !arg.empty())
	{
		auto searchResults = FindQuote(arg);
		SendMessage(msg._nick + ": " + searchResults);
		return ProcessingResult::StopProcessing;
	}

	if (msg._body == "!regenquotes")
	{
		if (!msg._isAdmin)
		{
			SendMessage(msg._nick + ": only admin can regenerate index");
			return ProcessingResult::StopProcessing;
		}

		RegenerateIndex();
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string Quotes::GetHelp() const
{
	return "!aq %text% - add quote, !dq %id% - delete quote\n"
		   "!fq %regex% - find quote, !gq %id% - get quote, if id is empty then quote is random\n"
		   "!regenquotes - regenerate index";
}

std::string Quotes::GetQuote(const std::string &id)
{
	using namespace sqlite_orm;

	std::string LastID;
	if (auto maxID = getStorage().max(&DB::Quote::humanIndex)) {
		LastID = std::to_string(*maxID);
	} else {
		return "No quotes";
	}

	if (id.empty()) {
		auto quotes = getStorage().get_all<DB::Quote>(order_by(sqlite_orm::random()));

		return "(" + std::to_string(quotes.at(0).humanIndex) + "/" + LastID + ") " + quotes.at(0).quote;
	}

	auto quotes = getStorage().get_all<DB::Quote>(where(is_equal(&DB::Quote::humanIndex, from_string<int>(id).value_or(0))));
	if (quotes.size() == 1) {
		const auto &quote = *quotes.begin();
		return "(" + std::to_string(quote.humanIndex) + "/" + LastID + ") " + quote.quote;
	}

	return FindQuote(id);
}

bool Quotes::AddQuote(const std::string &text)
{
	int newID = 1;
	if (auto maxID = getStorage().max(&DB::Quote::humanIndex)) {
		newID = *maxID + 1;
	}

	DB::Quote newQuote = { -1, newID, text, "", "" };

	try {
		getStorage().insert(newQuote);
		return true;
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to add quote: " << std::string(e.what());
		return false;
	}
}

bool Quotes::DeleteQuote(int id)
{
	try {
		getStorage().remove<DB::Quote>(id);
		return true;
	} catch (std::exception &e) {
		LOG(ERROR) << "Failed to delete quote: " << std::string(e.what());
		return false;
	}
}

std::string Quotes::FindQuote(const std::string &request) // FIXME const
{
	using namespace sqlite_orm;
	std::string lastID;
	if (auto maxID = getStorage().max(&DB::Quote::humanIndex)) {
		lastID = std::to_string(*maxID);
	} else {
		return "No matches";
	}

	auto quotes = getStorage().get_all<DB::Quote>(
				where(like(&DB::Quote::quote, "%" + request + "%")));

	if (quotes.size() == 1)
		return "(" + std::to_string(quotes.front().humanIndex) + "/" + lastID + ") " + quotes.front().quote;

	if (quotes.size() == 0)
		return "No matches";

	if (quotes.size() > maxMatches)
		return "Too many matches (" + std::to_string(quotes.size()) + ")";

	std::string searchResults("Matching quote IDs:");
	for (const auto &match : quotes)
		searchResults.append(" " + std::to_string(match.humanIndex));
	return searchResults;
}

void Quotes::RegenerateIndex()
{
	int index = 0;
	for (auto &quote : getStorage().get_all<DB::Quote>())
	{
		quote.humanIndex = ++index;
		getStorage().update(quote);
	}

	SendMessage("Index regenerated. New count: " + std::to_string(index));
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

class QuoteTestBot : public LemonBot
{
public:
	QuoteTestBot() : LemonBot(":memory:") {
		_storage.sync_schema();
	}

	void SendMessage(const std::string &text);
	std::vector<std::string> _received;
};

void QuoteTestBot::SendMessage(const std::string &text)
{
	_received.push_back(text);
}

TEST(QuotesTest, General)
{
	QuoteTestBot tb;
	Quotes q(&tb);

	std::string testquote = "This is a test quote";
	ASSERT_TRUE(q.AddQuote(testquote));

	auto quote = q.GetQuote("1");
	auto pos = quote.find(testquote);
	EXPECT_NE(quote.npos, pos);
	EXPECT_TRUE(q.DeleteQuote(1));
}

TEST(QuotesTest, Search)
{
	QuoteTestBot tb;
	Quotes q(&tb);

	ASSERT_TRUE(q.AddQuote("testquote1"));

	auto quote = q.FindQuote("test");
	auto pos = quote.find("testquote1");
	EXPECT_NE(quote.npos, pos);

	ASSERT_TRUE(q.AddQuote("testquote2"));

	auto quoteids = q.FindQuote("test");
	auto pos1 = quoteids.find("1");
	auto pos2 = quoteids.find("2");
	EXPECT_NE(quoteids.npos, pos1);
	EXPECT_NE(quoteids.npos, pos2);

	EXPECT_TRUE(q.DeleteQuote(1));
	EXPECT_TRUE(q.DeleteQuote(2));
}

TEST(QuotesTest, RegenIndex)
{
	QuoteTestBot tb;
	Quotes q(&tb);

	ASSERT_TRUE(q.AddQuote("testquote1"));
	ASSERT_TRUE(q.AddQuote("testquote2"));
	ASSERT_TRUE(q.AddQuote("testquote3"));

	auto quote2 = q.GetQuote("2");
	EXPECT_EQ("(2/3) testquote2", quote2);

	EXPECT_TRUE(q.DeleteQuote(2));
	q.RegenerateIndex();

	auto quote3 = q.GetQuote("2");
	EXPECT_EQ("(2/2) testquote3", quote3);
}

#endif // LCOV_EXCL_STOP
