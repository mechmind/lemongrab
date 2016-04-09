#include "quotes.h"

#include <leveldb/db.h>

#include <glog/logging.h>

#include <regex>
#include <chrono>

#include "util/stringops.h"

Quotes::Quotes(LemonBot *bot)
	: LemonHandler("quotes", bot)
	, _bot(bot)
	, _generator(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
	_quotesDB.init("quotes");
}

LemonHandler::ProcessingResult Quotes::HandleMessage(const std::string &from, const std::string &body)
{
	std::string arg;
	if (getCommandArguments(body, "!gq", arg))
	{
		SendMessage(from + ": " + GetQuote(arg));
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!aq", arg) && !arg.empty())
	{
		auto id = AddQuote(arg);
		id.empty() ? SendMessage(from + ": can't add quote") : SendMessage(from + ": quote added with id " + id);
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!dq", arg) && !arg.empty())
	{
		if (_bot && _bot->GetJidByNick(from) != GetRawConfigValue("admin"))
		{
			SendMessage(from + ": only admin can delete quotes");
			return ProcessingResult::StopProcessing;
		}

		DeleteQuote(arg) ? SendMessage(from + ": quote deleted") : SendMessage(from + ": quote doesn't exist or access denied");
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!fq", arg) && !arg.empty())
	{
		auto searchResults = FindQuote(arg);
		SendMessage(from + ": " + searchResults);
		return ProcessingResult::StopProcessing;
	}

	if (body == "!regenquotes")
	{
		if (_bot && _bot->GetJidByNick(from) != GetRawConfigValue("admin"))
		{
			SendMessage(from + ": only admin can regenerate index");
			return ProcessingResult::StopProcessing;
		}

		RegenerateIndex();
		return ProcessingResult::StopProcessing;
	}

	return ProcessingResult::KeepGoing;
}

const std::string Quotes::GetVersion() const
{
	return "0.1";
}

const std::string Quotes::GetHelp() const
{
	return "!aq %text% - add quote, !dq %id% - delete quote, !fq %regex% - find quote, !gq %id% - get quote, if id is empty then quote is random, !regenquotes - regenerate index";
}

std::string Quotes::GetQuote(const std::string &id)
{
	if (!_quotesDB.isOK())
		return "database connection error";

	std::string quote;
	std::string lastid;
	if (!_quotesDB.Get("lastid", lastid))
		return "Database is empty";

	if (id.empty())
	{
		int nLastID = 1;
		try {
			nLastID = std::stoi(lastid);
		} catch (std::exception &e) {
			return "database is empty";
		}

		int attempts = 1;
		const int maxAttempts = 100;
		bool quoteFound = false;
		std::string strId;
		while (attempts < maxAttempts && !quoteFound)
		{
			attempts++;
			std::uniform_int_distribution<int> dis(1, nLastID);
			auto randomID = dis(_generator);
			strId = std::to_string(randomID);
			quoteFound = _quotesDB.Get(strId, quote);
		}

		return attempts < maxAttempts ? "(" + strId + "/" + lastid + ") " + quote : "Too many deleted quotes or database is empty";
	}

	if (!_quotesDB.Get(id, quote))
		return "Quote not found or something exploded";

	return "(" + id + "/" + lastid + ") " + quote;
}

std::string Quotes::AddQuote(const std::string &text)
{
	if (!_quotesDB.isOK())
		return "";

	std::string sLastID("0");
	int lastID = 0;

	_quotesDB.Get("lastid", sLastID);
	try {
		lastID = std::stoi(sLastID);
	} catch (std::exception &e) {
		LOG(ERROR) << "Can't convert " << sLastID << " to integer: " << e.what();
		return "";
	}

	lastID++;
	std::string id(std::to_string(lastID));

	if (!_quotesDB.Set(id, text))
		return "";

	if (!_quotesDB.Set("lastid", id))
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

std::string Quotes::FindQuote(const std::string &request)
{
	if (!_quotesDB.isOK())
	{
		return "database connection error";
	}

	// FIXME put similar code from lastseen and this into utils?
	std::string searchResults("Matching quote IDs:");
	std::regex inputRegex;
	std::smatch regexMatch;
	try {
		inputRegex = std::regex(toLower(request));
	} catch (std::regex_error& e) {
		LOG(WARNING) << "Input regex " << request << " is invalid: " << e.what();
		return "Something broke: " + std::string(e.what());
	}

	int matches = 0;

	std::string onlyQuote;
	std::string onlyQuoteID;
	std::string lastID;

	_quotesDB.ForEach([&](std::pair<std::string, std::string> record)->bool{
		if (record.first == "lastid")
		{
			lastID == record.second;
			return true;
		}

		bool doesMatch = false;
		try {
			auto quote = toLower(record.second);
			doesMatch = std::regex_search(quote, regexMatch, inputRegex);
		} catch (std::regex_error &e) {
			LOG(ERROR) << "Regex exception thrown: " << e.what();
		}

		if (doesMatch)
		{
			if (onlyQuote.empty())
			{
				onlyQuote = record.second;
				onlyQuoteID = record.first;
			}

			if (matches >= 10)
			{
				searchResults.append(" ... (too many matches)");
				return false;
			}

			matches++;
			searchResults.append(" " + record.first);
		}

		return true;
	});

	if (matches == 1)
		return "(" + onlyQuoteID + "/" + lastID + ") " + onlyQuote;

	return searchResults;
}

void Quotes::RegenerateIndex()
{
	std::list<std::string> quotes;
	std::string lastid;
	int id = 0;

	_quotesDB.ForEach([&](std::pair<std::string, std::string> record)->bool{
		if (record.first == "lastid")
		{
			lastid = record.second;
			return true;
		}

		quotes.push_back(record.second);
		_quotesDB.Delete(record.first);
		return true;
	});

	for (const auto &quote : quotes)
		_quotesDB.Set(std::to_string(++id), quote);

	_quotesDB.Set("lastid", std::to_string(id));
	SendMessage("Index regenerated. Size: " + lastid + " -> " + std::to_string(id));
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
