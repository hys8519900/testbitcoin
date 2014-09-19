#include "net.h"
#include "addrman.h"

using namespace std;

int main()
{
	fPrintToConsole = true;

	// Add a address in addrman
	CAddrMan addrman;
	CAddress addr(CService("23.244.180.174",9333));
	//cout << "addr : " << addr.ToString()  << endl;
	//cout << (addr.IsRoutable() ? "isroutable" : "not routable") << endl;
	cout << "before add: " << addrman.size() << endl;
	if(!addrman.Add(addr, addr))
		cout<< "Add failed" << endl;	
	cout << "after add: " << addrman.size() << endl;
}
