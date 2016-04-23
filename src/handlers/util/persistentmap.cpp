#include "persistentmap.h"

#include "stringops.h"

#include <leveldb/db.h>
#include <leveldb/comparator.h>
#include <glog/logging.h>

#include <regex>

class PseudoNaturalSorting
		: public leveldb::Comparator {
public:
	int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
		long long ai = 0;
		long long bi = 0;
		try {
			ai = std::stoll(a.ToString());
			bi = std::stoll(b.ToString());
			if (ai != 0 && bi != 0)
			{
				if (ai < bi)
					return -1;
				else if (ai > bi)
					return +1;
				else
					return 0;
			}
		} catch (std::exception &e) {
			// check if it's stoll that failed
		}
		return a.ToString().compare(b.ToString());
	}

	const char* Name() const { return "leveldb.BytewiseComparator"; }
	void FindShortestSeparator(std::string*, const leveldb::Slice&) const { }
	void FindShortSuccessor(std::string*) const { }
};

bool PersistentMap::init(const std::string &name)
{
	if (name.empty())
	{
		LOG(ERROR) << "Database name is not specified";
		return false;
	}

	_name = name;
	leveldb::Options options;
	_comparator = std::make_shared<PseudoNaturalSorting>();
	options.create_if_missing = true;
	options.comparator = _comparator.get();

	leveldb::DB *DB = nullptr;
	auto status = leveldb::DB::Open(options, "db/" + _name, &DB);
	if (!status.ok())
	{
		LOG(ERROR) << "Failed to open database \"" << _name << "\": " << status.ToString();
		return false;
	}

	_database.reset(DB);
	return true;
}

bool PersistentMap::isOK() const
{
	return static_cast<bool>(_database);
}

bool PersistentMap::Get(const std::string &key, std::string &value)
{
	auto status = _database->Get(leveldb::ReadOptions(), key, &value);

	if (!status.ok())
	{
		LOG(ERROR) << "Get for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

bool PersistentMap::Set(const std::string &key, const std::string &value)
{
	auto status = _database->Put(leveldb::WriteOptions(), key, value);

	if (!status.ok())
	{
		LOG(ERROR) << "Put for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

bool PersistentMap::Delete(const std::string &key)
{
	auto status = _database->Delete(leveldb::WriteOptions(), key);

	if (!status.ok())
	{
		LOG(ERROR) << "Delete for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

void PersistentMap::ForEach(std::function<bool (std::pair<std::string, std::string>)> call)
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		if (!call(std::pair<std::string, std::string>(it->key().ToString(), it->value().ToString())))
			break;
	}
}

std::pair<std::string, std::string> PersistentMap::GetLastRecord() const
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	it->SeekToLast();
	if (it->Valid())
		return std::pair<std::string, std::string>(it->key().ToString(), it->value().ToString());
	else
		return std::pair<std::string, std::string>();
}

bool PersistentMap::PopFront()
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	it->SeekToFirst();
	if (!it->Valid())
		return false;

	return Delete(it->key().ToString());
}

std::list<std::pair<std::string, std::string>> PersistentMap::Find(const std::string &input,
																   FindOptions options,
																   bool caseSensitive)
{
	std::list<std::pair<std::string, std::string>> result;

	std::regex inputRegex;
	try {
		inputRegex = std::regex(caseSensitive ? input : toLower(input));
	} catch (std::regex_error& e) {
		LOG(WARNING) << "Input regex " << input << " is invalid: " << e.what();
		return result;
	}

	std::smatch regexMatch;
	ForEach([&](std::pair<std::string, std::string> record)->bool{
		bool doesMatch = false;
		try {
			const auto &key   = caseSensitive ? record.first  : toLower(record.first);
			const auto &value = caseSensitive ? record.second : toLower(record.second);

			switch (options)
			{
			case FindOptions::KeysOnly:
				doesMatch = std::regex_search(key, regexMatch, inputRegex);
				break;
			case FindOptions::ValuesOnly:
				doesMatch = std::regex_search(value, regexMatch, inputRegex);
				break;
			case FindOptions::All:
				doesMatch = std::regex_search(key, regexMatch, inputRegex)
						|| std::regex_search(value, regexMatch, inputRegex);
			}
		} catch (std::regex_error &e) {
			LOG(ERROR) << "Regex exception thrown: " << e.what();
		}

		if (doesMatch)
			result.push_back(record);

		return true;
	});

	return result;
}

int PersistentMap::Size()
{
	int size = 0;
	ForEach([&size](std::pair<std::string, std::string> record)->bool{
		++size;
		return true;
	});
	return size;
}
