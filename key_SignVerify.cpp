#include "key.h"
#include "base58.h"
#include "uint256.h"
#include <vector>
#include <iostream>

using namespace std;

int main()
{
	CKey key;
	key.MakeNewKey(true);
	
	uint256 hash;
	cout << "hash: " << hash.ToString() << endl;

	vector<unsigned char> vSig;
	if(!key.Sign(hash, vSig))
		cout << "sign failed" <<endl;

	if(key.GetPubKey().Verify(hash, vSig))
		cout << "vSig verifyed" << endl;
	else
		cout << "vSig verify failed" << endl;

	cout << "after changed vSig" << endl;
	if(vSig[0] != 'u')
		vSig[0] = 'u';
	else
		vSig[0] = 'e';

	if(key.GetPubKey().Verify(hash, vSig))
		cout << "vSig verifyed" << endl;
	else
		cout << "vSig verify failed" << endl;


}
