#include "net.h"
#include "addrman.h"
#include "netbase.h"

//change constructor
class CAddrDB_m : public CAddrDB
{
private:
	boost::filesystem::path pathAddr;
public:
	CAddrDB_m()
	{
		pathAddr = "./peers.dat";
	}
};

int main()
{
	CAddrDB_m adb;
	CAddrMan addrman;	
	CAddress addr(CService("94.113.93.139"));
	addrman.Add(addr, addr);
	addr.SetIP(CNetAddr("94.23.55.224"));
	addrman.Add(addr, addr);
	
	adb.Write(addrman);
}
