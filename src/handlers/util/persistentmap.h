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
		ValuesAsRegex,
	};

	virtual ~PersistentMap();
	virtual bool init(const std::string &name) = 0;
	virtual bool isOK() const = 0;
	virtual bool Get(const std::string &key, std::string &value) const = 0;
	virtual bool Set(const std::string &key, const std::string &value) = 0;
	virtual bool Delete(const std::string &key) = 0;
	virtual void ForEach(std::function<bool (std::pair<std::string, std::string>)> call) const = 0;
	virtual std::pair<std::string, std::string> GetLastRecord() const = 0;
	virtual bool PopFront() = 0;
	virtual std::list<std::pair<std::string, std::string>> Find(const std::string &input,
																FindOptions options,
																bool caseSensitive = false) const = 0;
	virtual void GenerateNumericIndex() = 0;
	virtual void Clear() = 0;
	virtual int Size() const = 0;
	bool isEmpty() const;
	const std::string getName() const;

protected:
	std::string _name;
};

class LevelDBPersistentMap
		: public PersistentMap
{
public:
	bool init(const std::string &name);
	bool isOK() const;
	bool Get(const std::string &key, std::string &value) const;
	bool Set(const std::string &key, const std::string &value);
	bool Delete(const std::string &key);
	void ForEach(std::function<bool (std::pair<std::string, std::string>)> call) const;
	std::pair<std::string, std::string> GetLastRecord() const;
	bool PopFront();
	std::list<std::pair<std::string, std::string>> Find(const std::string &input,
														FindOptions options,
														bool caseSensitive = false) const;
	void GenerateNumericIndex();
	void Clear();
	int Size() const;
private:
	std::shared_ptr<leveldb::DB> _database;
	std::shared_ptr<leveldb::Comparator> _comparator;
};
