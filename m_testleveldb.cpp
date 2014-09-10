#include <cassert>
#include "leveldb/db.h"
#include <iostream>
#include "leveldb/options.h"
#include "leveldb/cache.h"

using namespace std;

int main()
{
	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = false;
	options.block_cache = leveldb::NewLRUCache(100 * 1048576);

	leveldb::Env *penv;

	leveldb::Status status = leveldb::DB::Open(options, "/home/john/testbitcoin/blockindex", &db);
	assert(status.ok());

	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	it->SeekToFirst();
	while(it->Valid())
	{
		cout << it->key().ToString() << ": " << it->value().ToString() <<endl;
		it->Next();
	}
	assert(it->status().ok());

	delete it;
	delete db;
	delete options.block_cache;
}
