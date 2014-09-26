#include "chainparams.h"
#include "core.h"
#include <iostream>
#include "script.h"

using namespace std;

int main()
{
	SelectParams(CChainParams::MAIN);
	cout << Params().GenesisBlock().vtx[0].vin[0].scriptSig.ToString() << endl;
	vector<vector<unsigned char> > stack;
	//copy the genesis block
	CTransaction tx = Params().GenesisBlock().vtx[0];

	//change scriptSig
	const char* pszTimestamp = "test for sig";
	tx.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

	//test for eval
	if(!EvalScript(stack, tx.vin[0].scriptSig, tx, 0, false, 0))
		cout << "Eval Script" << endl;
}
