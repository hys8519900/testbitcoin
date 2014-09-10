#include "core.h"
#include "random.h"
#include "util.h"
#include <stdio.h>

using namespace std;

int main()
{
	uint64_t max = 10;
	printf("%d\n",GetRandInt(10));	
	printf("%llu\n",GetRand(max));
	uint256 hash = GetRandHash();
	printf("%s\n", hash.ToString().c_str());
	printf("time: %llu\n", GetTime());
	CBlock block;
	printf("block: %s\n", block.GetHash().ToString().c_str());

	const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
	CMutableTransaction txNew;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
	txNew.vout[0].nValue = 50 * COIN;
	txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

	CBlock genesis;
	genesis.vtx.push_back(txNew);
	genesis.hashPrevBlock = 0;
	genesis.hashMerkleRoot = genesis.BuildMerkleTree();
	genesis.nVersion = 1;
	genesis.nTime = 1231006505;
	genesis.nBits = 0x1d00ffff;
	genesis.nNonce = 2083236893;

	printf("genesis hash: %s\n",genesis.GetHash().ToString().c_str());
}
