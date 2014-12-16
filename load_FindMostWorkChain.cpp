#include "txdb.h"
#include "version.h"
#include <cassert>
#include <iostream>
#include "leveldb/db.h"
#include <unistd.h>
#include <boost/foreach.hpp>
//for sleep
#include "main.h"

using namespace std;

void FindMostWorkChain()
{
	CBlockIndex *pindexNew = NULL;

	while(chainMostWork.Tip() && (chainMostWork.Tip()->nStatus & BLOCK_FAILED_MASK))
	{
		//setBlockIndexVaild.erase(chainMostWork.Tip());
		chainMostWork.SetTip(chainMostWork.Tip()->pprev);
	}
}

int main()
{
	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = false;
	leveldb::Status status = leveldb::DB::Open(options, "/home/john/.bitcoin/blocks/index", &db);
	assert(status.ok());

	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());

	CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
	ssKeySet << make_pair('b', uint256(0));	
	it->Seek(ssKeySet.str());

	while(it->Valid())
	{
	 try{
		leveldb::Slice slKey = it->key();
		CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
		char chType;
		ssKey >> chType;
		if(chType == 'b')
		{
			leveldb::Slice slValue = it->value();
			CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
			CDiskBlockIndex diskindex;
			ssValue >> diskindex;
			
			CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
			pindexNew->pprev         = InsertBlockIndex(diskindex.hashPrev);
			pindexNew->nHeight       = diskindex.nHeight;
			pindexNew->nFile         = diskindex.nFile;
			pindexNew->nDataPos      = diskindex.nDataPos;
			pindexNew->nUndoPos      = diskindex.nUndoPos;
			pindexNew->nVersion      = diskindex.nVersion;
			pindexNew->hashMerkleRoot= diskindex.hashMerkleRoot;
			pindexNew->nTime         = diskindex.nTime;
			pindexNew->nBits         = diskindex.nBits;
			pindexNew->nNonce        = diskindex.nNonce;
			pindexNew->nStatus       = diskindex.nStatus;
			pindexNew->nTx           = diskindex.nTx;

			it->Next();

		}
		else
		{
			break;
		}
	  } catch(std::exception &e) {
			return error("%s : Deserialize or I/O error - %s", __func__, e.what());
	  }
	}	
	

	delete it;
	delete db;


#ifdef COMM
	BOOST_FOREACH(const PAIRTYPE(uint256, CBlockIndex*)& item, mapBlockIndex)
	{
		CBlockIndex* pindex = item.second;
		if(item.first == *(pindex->phashBlock))
		{
			cout << item.first.ToString() << " height: " << pindex->nHeight<< endl;
		}
		else
		{
			cout << "error" << endl;
			return 1;
		}

		CBlock block;
		if(ReadBlockFromDisk(block, pindex->GetBlockPos()))
		{
			cout << block.GetBlockHeader().GetHash().ToString() << endl;
		}
	}
#endif
	
	// Calulate nChainWork
	vector<pair<int, CBlockIndex*> > vSortedByHeight;
	vSortedByHeight.reserve(mapBlockIndex.size());
	BOOST_FOREACH(const PAIRTYPE(uint256, CBlockIndex*)& item, mapBlockIndex)
	{
		CBlockIndex* pindex = item.second;
		vSortedByHeight.push_back(make_pair(pindex->nHeight, pindex));
	}
	

	FindMostWorkChain();
}
	
