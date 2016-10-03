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
	virtual bool Exists(const std::string &key) const = 0;
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
	bool init(const std::string &name) override;
	bool isOK() const override;
	bool Get(const std::string &key, std::string &value) const override;
	bool Set(const std::string &key, const std::string &value) override;
	bool Exists(const std::string &key) const override;
	bool Delete(const std::string &key) override;
	void ForEach(std::function<bool (std::pair<std::string, std::string>)> call) const override;
	std::pair<std::string, std::string> GetLastRecord() const override;
	bool PopFront() override;
	std::list<std::pair<std::string, std::string>> Find(const std::string &input,
														FindOptions options,
														bool caseSensitive = false) const override;
	void GenerateNumericIndex() override;
	void Clear() override;
	int Size() const override;
private:
	std::shared_ptr<leveldb::DB> _database;
	std::shared_ptr<leveldb::Comparator> _comparator;
};
