#include <iostream>
#include <boost/filesystem.hpp>
#include "init.h"
#include "util.h"
#include "txdb.h"

using namespace std;
using namespace boost;

//CBlockTreeDB *pblocktree = NULL; (already define in main.cpp)

void printmapblockindex()
{
	map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin();
	while(mi != mapBlockIndex.end())
	{
		cout<< (mi->second)->ToString() << endl;
		mi++;
	}
}

std::string blockToString(CBlock b)
{
	std::stringstream s;
	s<< strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
		b.GetHash().ToString(),
		b.nVersion,
		b.hashPrevBlock.ToString(),
		b.hashMerkleRoot.ToString(),
		b.nTime, b.nBits, b.nNonce,
		b.vtx.size());
	for(unsigned int i = 0; i < b.vtx.size(); i++)
	{
		s << "  " << b.vtx[i].ToString() << "\n";
	}	
	s << " vMerkleTree: ";
	for(unsigned int i = 0; i < b.vMerkleTree.size() ; i++)
		s << " " << b.vMerkleTree[i].ToString();
	s << "\n";
	return s.str();
}

void printblock()
{
	for(map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
	{
		//uint256 hash = (*mi).first;
		CBlockIndex* pindex = (*mi).second;
		CBlock block;
		ReadBlockFromDisk(block, pindex);
		block.BuildMerkleTree();
		cout<< blockToString(block) << endl;
	}	
}

int main()
{
	SelectParams(CBaseChainParams::MAIN);

	filesystem::path blocksDir = GetDataDir(false) / "blocks";	

	if(!filesystem::exists(blocksDir))
	{
		cout<< "dir dont exist" << endl;	
	}

	int nBlockTreeDBCache = 2097152;
	pblocktree = new CBlockTreeDB(nBlockTreeDBCache, false, false);
	
	if(!pblocktree->LoadBlockIndexGuts())
		cout<< "failed LoadBlockIndexGuts()"<<endl;

	
	//printblock();
	//printmapblockindex();
	//cout<< "block size: " <<mapBlockIndex.size() <<endl;

	//is loadblock finish?	
}
