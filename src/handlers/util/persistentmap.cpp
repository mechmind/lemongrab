#include "persistentmap.h"

#include <leveldb/db.h>
#include <glog/logging.h>

bool PersistentMap::init(const std::string &name)
{
	if (name.empty())
	{
		LOG(ERROR) << "Database name is not specified";
		return false;
	}

	_name = name;
	leveldb::Options options;
	options.create_if_missing = true;

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
