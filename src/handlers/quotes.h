#pragma once

#include "lemonhandler.h"

#include <random>
#include <memory>

namespace leveldb
{
	class DB;
}

class Quotes : public LemonHandler
{
public:
	Quotes(LemonBot *bot);
	ProcessingResult HandleMessage(const std::string &from, const std::string &body);
	const std::string GetVersion() const;
	const std::string GetHelp() const;

private:
	std::string GetQuote(const std::string &id);
	std::string AddQuote(const std::string &text);
	bool DeleteQuote(const std::string &id);
	std::string FindQuote(const std::string &request);

private:
	std::shared_ptr<leveldb::DB> _quotesDB;

	std::mt19937_64 _generator;
};
