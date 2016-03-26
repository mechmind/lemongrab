#include "quotes.h"

#include <leveldb/db.h>

#include <glog/logging.h>

#include <regex>
#include <chrono>

#include "util/stringops.h"

Quotes::Quotes(LemonBot *bot)
	: LemonHandler("quotes", bot)
	, _generator(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
	leveldb::Options options;
	options.create_if_missing = true;

	leveldb::DB *quotesDB = nullptr;
	leveldb::Status status = leveldb::DB::Open(options, "db/quotes", &quotesDB);
	if (!status.ok())
		LOG(ERROR) << "Failed to open database: " << status.ToString();

	_quotesDB.reset(quotesDB);
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
		DeleteQuote(arg) ? SendMessage(from + ": quote deleted") : SendMessage(from + ": quote doesn't exist or access denied");
		return ProcessingResult::StopProcessing;
	}

	if (getCommandArguments(body, "!fq", arg) && !arg.empty())
	{
		auto searchResults = FindQuote(arg);
		SendMessage(from + ": " + searchResults);
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
	return "!aq %text% - add quote, !dq %id% - delete quote, !fq %regex% - find quote, !gq %id% - get quote, if id is empty then quote is random";
}

std::string Quotes::GetQuote(const std::string &id)
{
	if (!_quotesDB)
		return "database connection error";

	std::string lastid;
	std::string selectedId;
	_quotesDB->Get(leveldb::ReadOptions(), "lastid", &lastid);

	if (id.empty())
	{
		int nLastID = 1;
		try {
			nLastID = std::stoi(lastid);
		} catch (std::exception &e) {
			return "database is empty";
		}
		std::uniform_int_distribution<int> dis(1, nLastID);
		auto randomID = dis(_generator);
		selectedId = std::to_string(randomID);
	} else {
		selectedId = id;
	}

	std::string quote;
	auto getResult = _quotesDB->Get(leveldb::ReadOptions(), selectedId, &quote);
	if (!getResult.ok())
		return "Quote not found or something exploded";

	return "(" + selectedId + "/" + lastid + ") \"" + quote + "\"";
}

std::string Quotes::AddQuote(const std::string &text)
{
	if (!_quotesDB)
	{
		LOG(ERROR) << "Database pointer is null";
		return "";
	}

	std::string sLastID("0");
	int lastID = 0;

	_quotesDB->Get(leveldb::ReadOptions(), "lastid", &sLastID);

	try {
		lastID = std::stoi(sLastID);
	} catch (std::exception &e) {
		LOG(ERROR) << "Can't convert " << sLastID << " to integer: " << e.what();
		return "";
	}

	lastID++;
	std::string id(std::to_string(lastID));
	auto addRecordStatus = _quotesDB->Put(leveldb::WriteOptions(), id, text);
	if (!addRecordStatus.ok())
	{
		LOG(ERROR) << "leveldb::Put for quote failed: " << addRecordStatus.ToString();
		return "";
	}

	addRecordStatus = _quotesDB->Put(leveldb::WriteOptions(), "lastid", id);
	if (!addRecordStatus.ok())
	{
		LOG(ERROR) << "leveldb::Put for lastid failed" << addRecordStatus.ToString();
		return "";
	}

	return id;
}

bool Quotes::DeleteQuote(const std::string &id)
{
	if (!_quotesDB)
	{
		LOG(ERROR) << "Database pointer is null";
		return false;
	}

	std::string tmp;
	auto checkQuote = _quotesDB->Get(leveldb::ReadOptions(), id, &tmp);
	if (!checkQuote.ok())
		return false;

	auto removeStatus = _quotesDB->Delete(leveldb::WriteOptions(), id);
	return removeStatus.ok();
}

std::string Quotes::FindQuote(const std::string &request)
{
	if (!_quotesDB)
	{
		LOG(ERROR) << "Database pointer is null";
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

	std::shared_ptr<leveldb::Iterator> it(_quotesDB->NewIterator(leveldb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		if (it->key().ToString() == "lastid")
		{
			lastID = it->value().ToString();
			continue;
		}

		bool doesMatch = false;
		try {
			auto quote = toLower(it->value().ToString());
			doesMatch = std::regex_search(quote, regexMatch, inputRegex);
		} catch (std::regex_error &e) {
			LOG(ERROR) << "Regex exception thrown" << e.what();
		}

		if (doesMatch)
		{
			if (onlyQuote.empty())
			{
				onlyQuote = it->value().ToString();
				onlyQuoteID = it->key().ToString();
			}

			if (matches >= 10)
			{
				searchResults.append(" ... (too many matches)");
				return searchResults;
			}

			matches++;
			searchResults.append(" " + it->key().ToString());
		}
	}

	if (matches == 1)
	{
		return "(" + onlyQuoteID + "/" + lastID + ") \"" + onlyQuote + "\"";
	}

	return searchResults;
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
