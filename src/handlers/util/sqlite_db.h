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
								   ));
}

typedef decltype(initStorage("")) Storage;
