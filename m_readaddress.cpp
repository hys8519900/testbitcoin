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
	adb.Read(addrman);
	cout << "size: " << addrman.size() << endl;
}
