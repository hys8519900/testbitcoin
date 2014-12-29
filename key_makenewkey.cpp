#include "key.h"
#include <iostream>
#include "base58.h"

using namespace std;

int main()
{
	CKey secret;
	secret.MakeNewKey(true);

	CPubKey pubkey = secret.GetPubKey();
	CKeyID pubkeyid = pubkey.GetID();
	
	CBitcoinAddress address(pubkeyid);
	cout << "address: " << address.ToString() << endl;	

	//CBitcoinAddress address2(secret.GetPubKey().GetID());
	//cout << address2.ToString() <<endl;
	
	CBitcoinSecret priv(secret);
	cout << "private key base58: " << priv.ToString() <<endl;
}
