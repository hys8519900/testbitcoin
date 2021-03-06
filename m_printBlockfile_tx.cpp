#include "txdb.h"
#include "version.h"
#include <cassert>
#include <iostream>
#include "leveldb/db.h"
#include <unistd.h>
//for sleep
#include "main.h"

using namespace std;

map<uint256, CBlockIndex*> mapBlockIndex;


CBlockIndex * InsertBlockIndex(uint256 hash)
{
	if(hash == 0)
		return NULL;
	
	//Return existing
	map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
	if(mi != mapBlockIndex.end())
		return (*mi).second;

	//Create new
	CBlockIndex* pindexNew = new CBlockIndex();
	if(!pindexNew)
		throw runtime_error("LoadBlockIndex() : new CBlockINdex failed");
	mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
	pindexNew->phashBlock = &((*mi).first);

	return pindexNew;
}

bool ReadBlockFromDisk(CBlock& block, const CDiskBlockPos& pos)
{
	block.SetNull();

	FILE* file = fopen("/home/john/testbitcoin/blk00000.dat","rb+");
	if(!file)
	{
		cout<<"Unable to open file\n"<<endl;
		return false;
	}
	if(pos.nPos)
	{
		if(fseek(file, pos.nPos, SEEK_SET))
		{
			cout<<"Unable to seek to position "<<pos.nPos<<endl;
			fclose(file);
			return false;	
		}
	}
	
	CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
	if(!filein)
	{
		cout<<"unable to open CAutofile"<<endl;
		return false;
	}
	
	try{
		filein >> block;
	}
	catch(std::exception &e)
	{
		return error("%s : DESERIALIZE or I/O error -%s", __func__, e.what());
	}

	//skip check the header
	//CheckProofOfWork

	return true;
}

int main()
{
	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = false;
	leveldb::Status status = leveldb::DB::Open(options, "/home/john/testbitcoin/blockindex", &db);
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
			
			cout<<"height: "<<diskindex.nHeight<<endl;
			cout<<"block hash: "<<diskindex.GetBlockHash().ToString()<<endl;
			cout<<"nFile: "<<diskindex.nFile<<endl;
			cout<<"DataPos: "<<diskindex.nDataPos<<endl;
			cout<<"UndoPos: "<<diskindex.nUndoPos<<endl;
			cout<<"Version: "<<diskindex.nVersion<<endl;
			cout<<"hashMerkleRoot: "<<diskindex.hashMerkleRoot.ToString()<<endl;
			cout<<"Time: "<<diskindex.nTime<<endl;
			cout<<"Bits: "<<diskindex.nBits<<endl;
			cout<<"Nonce: "<<diskindex.nNonce<<endl;
			cout<<"Status: "<<diskindex.nStatus<<endl;
			cout<<"Tx: "<<diskindex.nTx<<endl;
			cout<<endl;

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

	//read blockindex to mapBlockIndex finsh
	//use mapBlockIndex read Block
	for(map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
	{
		CBlockIndex* pindex = (*mi).second;
		cout<<"file: "<<pindex->GetBlockPos().nFile<<" pos: "<<pindex->GetBlockPos().nPos<<endl;
		CBlock block;
		if(ReadBlockFromDisk(block, pindex->GetBlockPos()))
		{
			cout<<"block hash: "<<block.GetHash().ToString()<<endl;
			for(unsigned int i = 0; i < block.vtx.size(); i++)
			{
				cout<<" "<<block.vtx[i].ToString()<<" ";
			}
			sleep(1);
		}
	}
}
