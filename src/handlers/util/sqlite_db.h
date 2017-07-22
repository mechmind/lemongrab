#pragma once

#include <sqlite_orm/sqlite_orm.h>

namespace DB
{
	class RssFeed
	{
	public:
		int id = -1;
		std::string URL = "";
		std::string GUID = "";
	};

	class URLRule
	{
	public:
		int id = -1;
		std::string rule = "";
		bool blacklist = true;
	};

	class LoggedURL
	{
	public:
		int id = -1;
		std::string URL = "";
		std::string title = "";
		long timestamp = 0;
		std::string fullText = "";
	};

	class Quote
	{
	public:
		int id = -1;
		int humanIndex = 0;
		std::string quote = "";
		std::string author = "";
		std::string author_id = "";
	};

	class PagerMsg
	{
	public:
		int id = -1;
		std::string recepient = "";
		std::string message = "";
		int timepoint = 0;
	};
}

inline auto initStorage(const std::string &path)
{
	using namespace sqlite_orm;
	return make_storage(path,
						make_table("rss",
								   make_column("id",
											   &DB::RssFeed::id,
											   autoincrement(),
											   primary_key()),
								   make_column("URL",
											   &DB::RssFeed::URL),
								   make_column("GUID",
											   &DB::RssFeed::GUID)
								   ),
						make_table("url_rules",
								   make_column("id",
											   &DB::URLRule::id,
											   autoincrement(),
											   primary_key()),
								   make_column("rule", &DB::URLRule::rule),
								   make_column("blacklist", &DB::URLRule::blacklist)
								   ),
						make_table("url_log",
								   make_column("id",
											   &DB::LoggedURL::id,
											   autoincrement(),
											   primary_key()),
								   make_column("URL", &DB::LoggedURL::URL),
								   make_column("title", &DB::LoggedURL::title),
								   make_column("time", &DB::LoggedURL::timestamp),
								   make_column("fulltext", &DB::LoggedURL::fullText)
								   ),
						make_table("quotes",
								   make_column("id", &DB::Quote::id, autoincrement(), primary_key()),
								   make_column("index", &DB::Quote::humanIndex),
								   make_column("quote", &DB::Quote::quote),
								   make_column("author", &DB::Quote::author),
								   make_column("author_id", &DB::Quote::author_id)
								   ),
						make_table("pager",
								   make_column("id",
											   &DB::PagerMsg::id,
											   autoincrement(),
											   primary_key()),
								   make_column("recepient", &DB::PagerMsg::recepient),
								   make_column("message", &DB::PagerMsg::message),
								   make_column("timepoint", &DB::PagerMsg::timepoint)
								   )
						);
}

typedef decltype(initStorage("")) Storage;
