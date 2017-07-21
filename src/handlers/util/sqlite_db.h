#pragma once

#include <memory>

#include <sqlite_orm/sqlite_orm.h>

namespace DB // ?
{
	class RssFeed
	{
	public:
		int id = -1;
		std::string URL = "";
		std::string GUID = "";
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
									  )
						   );
}

typedef decltype(initStorage("")) Storage;
