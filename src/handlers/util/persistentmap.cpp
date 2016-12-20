#include "persistentmap.h"

#include "stringops.h"

#include <leveldb/db.h>
#include <leveldb/comparator.h>
#include <glog/logging.h>

#include <regex>

class PseudoNaturalSorting
		: public leveldb::Comparator {
public:
	int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override {
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

	// FIXME: actually illegal but is here because LevelDB has no mechanism for changing comparator for existing DB
	const char* Name() const override { return "leveldb.BytewiseComparator"; }
	void FindShortestSeparator(std::string*, const leveldb::Slice&) const override { }
	void FindShortSuccessor(std::string*) const override { }
};

bool LevelDBPersistentMap::init(const std::string &name)
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

	leveldb::DB *db = nullptr;
	auto status = leveldb::DB::Open(options, "db/" + _name, &db);
	if (!status.ok())
	{
		LOG(ERROR) << "Failed to open database \"" << _name << "\": " << status.ToString();
		return false;
	}

	_database.reset(db);
	return true;
}

bool LevelDBPersistentMap::isOK() const
{
	return static_cast<bool>(_database);
}

bool LevelDBPersistentMap::Get(const std::string &key, std::string &value) const
{
	auto status = _database->Get(leveldb::ReadOptions(), key, &value);

	if (!status.ok())
	{
		// Don't error on "NotFound"?
		// LOG(ERROR) << "Get for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

bool LevelDBPersistentMap::Set(const std::string &key, const std::string &value)
{
	auto status = _database->Put(leveldb::WriteOptions(), key, value);

	if (!status.ok())
	{
		LOG(ERROR) << "Put for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

bool LevelDBPersistentMap::Exists(const std::string &key) const
{
	std::string value;
	auto status = _database->Get(leveldb::ReadOptions(), key, &value);

	if (!status.ok())
	{
		// Don't error on NotFound?
		// LOG(ERROR) << "Get for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

bool LevelDBPersistentMap::Delete(const std::string &key)
{
	auto status = _database->Delete(leveldb::WriteOptions(), key);

	if (!status.ok())
	{
		LOG(ERROR) << "Delete for database \"" << _name << "\" failed: " << status.ToString();
		return false;
	}

	return true;
}

void LevelDBPersistentMap::ForEach(std::function<bool (std::pair<std::string, std::string>)> call) const
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		if (!call(std::pair<std::string, std::string>(it->key().ToString(), it->value().ToString())))
			break;
	}
}

std::pair<std::string, std::string> LevelDBPersistentMap::GetLastRecord() const
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	it->SeekToLast();
	if (it->Valid())
		return std::pair<std::string, std::string>(it->key().ToString(), it->value().ToString());
	else
		return std::pair<std::string, std::string>();
}

bool LevelDBPersistentMap::PopFront()
{
	std::shared_ptr<leveldb::Iterator> it(_database->NewIterator(leveldb::ReadOptions()));
	it->SeekToFirst();
	if (!it->Valid())
		return false;

	return Delete(it->key().ToString());
}

std::list<std::pair<std::string, std::string>> LevelDBPersistentMap::Find(const std::string &input,
																		  FindOptions options,
																		  bool caseSensitive) const
{
	std::list<std::pair<std::string, std::string>> result;

	std::regex searchRegex;

	if (options != FindOptions::ValuesAsRegex)
	{
		try {
			searchRegex = std::regex(caseSensitive ? input : toLower(input));
		} catch (std::regex_error& e) {
			LOG(WARNING) << "Input regex " << input << " is invalid: " << e.what();
			return result;
		}
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
				doesMatch = std::regex_search(key, regexMatch, searchRegex);
				break;
			case FindOptions::ValuesOnly:
				doesMatch = std::regex_search(value, regexMatch, searchRegex);
				break;
			case FindOptions::All:
				doesMatch = std::regex_search(key, regexMatch, searchRegex)
						|| std::regex_search(value, regexMatch, searchRegex);
				break;
			case FindOptions::ValuesAsRegex:
				searchRegex = std::regex(caseSensitive ? value : toLower(value));
				doesMatch = std::regex_search(input, regexMatch, searchRegex);
				break;
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

void LevelDBPersistentMap::GenerateNumericIndex()
{
	std::list<std::string> records;
	ForEach([&](std::pair<std::string, std::string> record){
		records.push_back(record.second);
		Delete(record.first);
		return true;
	});

	long long index = 1;
	for (const auto &record : records)
		Set(std::to_string(index++), record);
}

void LevelDBPersistentMap::Clear()
{
	ForEach([&](std::pair<std::string, std::string> record){
		Delete(record.first);
		return true;
	});
}

int LevelDBPersistentMap::Size() const
{
	int size = 0;
	ForEach([&size](std::pair<std::string, std::string> record)->bool{
		++size;
		return true;
	});
	return size;
}

PersistentMap::~PersistentMap()
{

}

bool PersistentMap::isEmpty() const
{
	return Size() == 0;
}

const std::string PersistentMap::getName() const
{
	return _name;
}

#ifdef _BUILD_TESTS // LCOV_EXCL_START

#include <gtest/gtest.h>

TEST(LevelDB, SaveLoad)
{
	{
		LevelDBPersistentMap testdb;
		EXPECT_TRUE(testdb.init("testdb"));
		ASSERT_TRUE(testdb.isOK());
		testdb.Clear();

		EXPECT_TRUE(testdb.Set("key1", "value1"));
	}

	{
		LevelDBPersistentMap testdb;
		EXPECT_TRUE(testdb.init("testdb"));
		ASSERT_TRUE(testdb.isOK());
		std::string value;
		EXPECT_TRUE(testdb.Get("key1", value));
		EXPECT_EQ("value1", value);
		testdb.Clear();
	}
}

TEST(LevelDB, SetGetDelete)
{
	LevelDBPersistentMap testdb;
	ASSERT_TRUE(testdb.init("testdb"));
	testdb.Clear();

	std::string output;
	EXPECT_FALSE(testdb.Get("key1", output));

	testdb.Set("key1", "value1");
	testdb.Set("key2", "value2");
	EXPECT_TRUE(testdb.Get("key1", output));
	EXPECT_EQ("value1", output);

	EXPECT_TRUE(testdb.Delete("key1"));
	EXPECT_FALSE(testdb.Get("key1", output));
}

TEST(LevelDB, Size)
{
	LevelDBPersistentMap testdb;
	ASSERT_TRUE(testdb.init("testdb"));
	testdb.Clear();
	EXPECT_TRUE(testdb.isEmpty());
	EXPECT_EQ(0, testdb.Size());

	ASSERT_TRUE(testdb.Set("key1", "value1"));
	EXPECT_FALSE(testdb.isEmpty());
	EXPECT_EQ(1, testdb.Size());

	ASSERT_TRUE(testdb.Set("key2", "value2"));
	EXPECT_FALSE(testdb.isEmpty());
	EXPECT_EQ(2, testdb.Size());

	ASSERT_TRUE(testdb.Set("key1", "value3"));
	EXPECT_FALSE(testdb.isEmpty());
	EXPECT_EQ(2, testdb.Size());

	testdb.Clear();
}

TEST(LevelDB, Search)
{
	LevelDBPersistentMap testdb;
	ASSERT_TRUE(testdb.init("testdb"));
	testdb.Clear();

	ASSERT_TRUE(testdb.Set("key1", "value1"));
	ASSERT_TRUE(testdb.Set("key2", "value2"));
	ASSERT_TRUE(testdb.Set("key3", "value3"));

	{
		auto result = testdb.Find("key", PersistentMap::FindOptions::KeysOnly);
		std::list<std::pair<std::string,std::string>> expectedResult = {
			{"key1", "value1"},
			{"key2", "value2"},
			{"key3", "value3"},
		};
		EXPECT_TRUE(std::equal(expectedResult.begin(), expectedResult.end(), result.begin()));
	}

	{
		auto result = testdb.Find("key[12]", PersistentMap::FindOptions::KeysOnly);
		std::list<std::pair<std::string,std::string>> expectedResult = {
			{"key1", "value1"},
			{"key2", "value2"},
		};
		EXPECT_TRUE(std::equal(expectedResult.begin(), expectedResult.end(), result.begin()));
	}

	{
		auto result = testdb.Find("VALUE2", PersistentMap::FindOptions::ValuesOnly, false);
		std::list<std::pair<std::string,std::string>> expectedResult = {
			{"key2", "value2"},
		};
		EXPECT_TRUE(std::equal(expectedResult.begin(), expectedResult.end(), result.begin()));
	}

	{
		auto result = testdb.Find("VALUE2", PersistentMap::FindOptions::ValuesOnly, true);
		EXPECT_TRUE(result.empty());
	}

	testdb.Clear();
}

TEST(LevelDB, IndexGenerator)
{
	LevelDBPersistentMap testdb;
	ASSERT_TRUE(testdb.init("testdb"));
	testdb.Clear();

	testdb.Set("2", "Test2");
	testdb.Set("3", "Test3");
	testdb.Set("5", "Test5");
	testdb.Set("4", "Test4");

	testdb.GenerateNumericIndex();
	EXPECT_EQ(4, testdb.Size());

	std::string value;
	ASSERT_TRUE(testdb.Get("1", value));
	EXPECT_EQ("Test2", value);
	ASSERT_TRUE(testdb.Get("2", value));
	EXPECT_EQ("Test3", value);
	ASSERT_TRUE(testdb.Get("3", value));
	EXPECT_EQ("Test4", value);
	ASSERT_TRUE(testdb.Get("4", value));
	EXPECT_EQ("Test5", value);}

#endif // LCOV_EXCL_STOP
