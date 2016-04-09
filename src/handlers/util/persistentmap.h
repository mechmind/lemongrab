#pragma once

#include <memory>
#include <string>
#include <functional>

namespace leveldb
{
	class DB;
}

class PersistentMap
{
public:
	bool init(const std::string &name);
	bool isOK() const;
	bool Get(const std::string &key, std::string &value);
	bool Set(const std::string &key, const std::string &value);
	bool Delete(const std::string &key);
	void ForEach(std::function<bool (std::pair<std::string, std::string>)> call);

private:
	std::string _name;
	std::shared_ptr<leveldb::DB> _database;
};
