#include "quotes.h"

#include <glog/logging.h>

#include <chrono>

#include "util/stringops.h"

Quotes::Quotes(LemonBot *bot)
	: LemonHandler("quotes", bot)
	, _bot(bot)
	, _generator(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
	_quotesDB.init("quotes", bot->GetDBPathPrefix());
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
		auto id = AddQuote(arg);
		id.empty() ? SendMessage(msg._nick + ": can't add quote") : SendMessage(msg._nick + ": quote added with id " + id);
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(msg._body, "!dq", arg) && !arg.empty())
	{
		if (!msg._isAdmin)
		{
			SendMessage(msg._nick + ": only admin can delete quotes");
			return ProcessingResult::StopProcessing;
		}

		DeleteQuote(arg) ? SendMessage(msg._nick + ": quote deleted") : SendMessage(msg._nick + ": quote doesn't exist or access denied");
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
	if (!_quotesDB.isOK())
		return "database connection error";

	std::string quote;
	auto lastID = easy_stoll(_quotesDB.GetLastRecord().first);

	if (lastID == 0)
		return "Database is empty";

	if (id.empty())
	{
		int attempts = 1;
		const int maxAttempts = 100;
		bool quoteFound = false;
		std::string strId;
		while (attempts < maxAttempts && !quoteFound)
		{
			attempts++;
			std::uniform_int_distribution<int> dis(1, lastID);
			auto randomID = dis(_generator);
			strId = std::to_string(randomID);
			quoteFound = _quotesDB.Get(strId, quote);
		}

		return attempts < maxAttempts ? "(" + strId + "/" + std::to_string(lastID) + ") " + quote : "Too many deleted quotes or database is empty";
	}

	if (!_quotesDB.Get(id, quote))
		return FindQuote(id);

	return "(" + id + "/" + std::to_string(lastID) + ") " + quote;
}

std::string Quotes::AddQuote(const std::string &text)
{
	if (!_quotesDB.isOK())
		return "";

	auto lastID = easy_stoll(_quotesDB.GetLastRecord().first);
	std::string id(std::to_string(++lastID));
	if (!_quotesDB.Set(id, text))
		return "";

	return id;
}

bool Quotes::DeleteQuote(const std::string &id)
{
	if (!_quotesDB.isOK())
		return false;

	std::string tmp;
	if (!_quotesDB.Get(id, tmp))
		return false;

	return _quotesDB.Delete(id);
}

std::string Quotes::FindQuote(const std::string &request) const
{
	if (!_quotesDB.isOK())
		return "database connection error";

	auto lastID = _quotesDB.GetLastRecord().first;
	auto quotes = _quotesDB.Find(request, LevelDBPersistentMap::FindOptions::ValuesOnly);
	if (quotes.size() == 1)
		return "(" + quotes.front().first + "/" + lastID + ") " + quotes.front().second;

	if (quotes.size() == 0)
		return "No matches";

	if (quotes.size() > maxMatches)
		return "Too many matches (" + std::to_string(quotes.size()) + ")";

	std::string searchResults("Matching quote IDs:");
	for (const auto &match : quotes)
		searchResults += " " + match.first;
	return searchResults;
}

void Quotes::RegenerateIndex()
{
	auto oldCount = _quotesDB.GetLastRecord().first;
	_quotesDB.GenerateNumericIndex();
	auto newCount = _quotesDB.GetLastRecord().first;
	SendMessage("Index regenerated. Size: " + oldCount + " -> " + newCount);
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

class QuoteTestBot : public LemonBot
{
public:
	void SendMessage(const std::string &text)
	{
		_received.push_back(text);
	}

	std::string GetDBPathPrefix() const { return "testdb/"; }

	std::vector<std::string> _received;
};

TEST(QuotesTest, General)
{
	QuoteTestBot tb;
	Quotes q(&tb);

	std::string testquote = "This is a test quote";
	auto id = q.AddQuote(testquote);
	ASSERT_FALSE(id.empty());

	auto quote = q.GetQuote(id);
	auto pos = quote.find(testquote);
	EXPECT_NE(quote.npos, pos);
	EXPECT_TRUE(q.DeleteQuote(id));
	EXPECT_FALSE(q.DeleteQuote(id));
}

TEST(QuotesTest, Search)
{
	QuoteTestBot tb;
	Quotes q(&tb);

	auto id1 = q.AddQuote("testquote1");
	ASSERT_FALSE(id1.empty());

	auto quote = q.FindQuote("test");
	auto pos = quote.find("testquote1");
	EXPECT_NE(quote.npos, pos);

	auto id2 = q.AddQuote("testquote2");
	ASSERT_FALSE(id2.empty());

	auto quoteids = q.FindQuote("test");
	auto pos1 = quoteids.find(id1);
	auto pos2 = quoteids.find(id2);
	EXPECT_NE(quoteids.npos, pos1);
	EXPECT_NE(quoteids.npos, pos2);

	EXPECT_TRUE(q.DeleteQuote(id1));
	EXPECT_TRUE(q.DeleteQuote(id2));
}

#endif // LCOV_EXCL_STOP
