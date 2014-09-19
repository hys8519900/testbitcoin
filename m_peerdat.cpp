#include "db.h"
#include "serialize.h"
#include "net.h"
#include "addrman.h"
#include <stdlib.h>
#include <iostream>

using namespace std;

// CAutoFile open a peer.dat file
// use >> get CAddrDB

int main()
{
	//FILE *f = fopen("peers.dat","r");
	//CAutoFile cf(f, SER_DISK, CLIENT_VERSION);
	CAddrDB db;
	CAddrMan addrman;
	bool r = db.Read(addrman);
	if(r)
	{
		cout << "read success" << endl;
	}
	//fclose(f);
}
