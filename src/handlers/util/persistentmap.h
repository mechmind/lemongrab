#pragma once

#include <memory>
#include <string>
#include <list>
#include <functional>

namespace leveldb
{
	class DB;
	class Comparator;
}

class PersistentMap
{
public:
	enum class FindOptions
	{
		KeysOnly,
		ValuesOnly,
		All,
	};

	bool init(const std::string &name);
	bool isOK() const;
	bool Get(const std::string &key, std::string &value);
	bool Set(const std::string &key, const std::string &value);
	bool Delete(const std::string &key);
	void ForEach(std::function<bool (std::pair<std::string, std::string>)> call);
	std::pair<std::string, std::string> GetLastRecord() const;
	bool PopFront();
	std::list<std::pair<std::string, std::string>> Find(const std::string &input,
														FindOptions options,
														bool caseSensitive = false);
	int Size();

private:
	std::string _name;
	std::shared_ptr<leveldb::DB> _database;
	std::shared_ptr<leveldb::Comparator> _comparator;
};
