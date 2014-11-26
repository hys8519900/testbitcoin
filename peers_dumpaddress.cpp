#include "net.h"
#include "addrman.h"
#include "netbase.h"
#include <boost/filesystem.hpp>

//change constructor
class CAddrDB_m : public CAddrDB
{
private:
	boost::filesystem::path pathAddr;
public:
	CAddrDB_m()
	{
		pathAddr = "peers.dat";
	}

	//override to use pathAddr
	bool Write(const CAddrMan& addr)
	{
    	// Generate random temporary filename
    	unsigned short randv = 0;
    	RAND_bytes((unsigned char *)&randv, sizeof(randv));
    	std::string tmpfn = strprintf("peers.dat.%04x", randv);

    	// serialize addresses, checksum data up to that point, then append csum
    	CDataStream ssPeers(SER_DISK, CLIENT_VERSION);
    	ssPeers << FLATDATA(Params().MessageStart());
    	ssPeers << addr;
    	uint256 hash = Hash(ssPeers.begin(), ssPeers.end());
    	ssPeers << hash;

    	// open temp output file, and associate with CAutoFile
    	boost::filesystem::path pathTmp = GetDataDir() / tmpfn;
    	FILE *file = fopen(pathTmp.string().c_str(), "wb");
    	CAutoFile fileout = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    	if (!fileout)
        	return error("%s : Failed to open file %s", __func__, pathTmp.string());

    	// Write and commit header, data
    	try {
        	fileout << ssPeers;
    	}
    	catch (std::exception &e) {
        	return error("%s : Serialize or I/O error - %s", __func__, e.what());
    	}
    	FileCommit(fileout);
    	fileout.fclose();

    	// replace existing peers.dat, if any, with new peers.dat.XXXX
    	if (!RenameOver(pathTmp, pathAddr))
        	return error("%s : Rename-into-place failed", __func__);

    	return true;
	}

	
	//override to use pathAddr
	bool Read(CAddrMan& addr)
	{
    	// open input file, and associate with CAutoFile
    	FILE *file = fopen(pathAddr.string().c_str(), "rb");
    	CAutoFile filein = CAutoFile(file, SER_DISK, CLIENT_VERSION);
    	if (!filein)
        	return error("%s : Failed to open file %s", __func__, pathAddr.string());

    	// use file size to size memory buffer
    	int fileSize = boost::filesystem::file_size(pathAddr);
    	int dataSize = fileSize - sizeof(uint256);
    	// Don't try to resize to a negative number if file is small
		if (dataSize < 0)
      	  dataSize = 0;
    	vector<unsigned char> vchData;
    	vchData.resize(dataSize);
    	uint256 hashIn;

    	// read data and checksum from file
    	try {
        	filein.read((char *)&vchData[0], dataSize);
        	filein >> hashIn;
    	}
    	catch (std::exception &e) {
        	return error("%s : Deserialize or I/O error - %s", __func__, e.what());
    	}
    	filein.fclose();

    	CDataStream ssPeers(vchData, SER_DISK, CLIENT_VERSION);

    	// verify stored checksum matches input data
    	uint256 hashTmp = Hash(ssPeers.begin(), ssPeers.end());
    	if (hashIn != hashTmp)
        	return error("%s : Checksum mismatch, data corrupted", __func__);

    	unsigned char pchMsgTmp[4];
    	try {
        	// de-serialize file header (network specific magic number) and ..
        	ssPeers >> FLATDATA(pchMsgTmp);

        	// ... verify the network matches ours
        	if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
            	return error("%s : Invalid network magic number", __func__);

        	// de-serialize address data into one CAddrMan object
        	ssPeers >> addr;
    	}
    	catch (std::exception &e) {
        	return error("%s : Deserialize or I/O error - %s", __func__, e.what());
    	}
	
    return true;
	}
	
};

unsigned int m_pnSeed[]
{
	0x7e6a692e, 0x7d04d1a2, 0x6c0c17d9, 0xdb330ab9, 0xc649c7c6, 0x7895484d, 0x047109b0, 0xb90ca5bc,
};

int main()
{
	CAddrDB_m adb;
	CAddrMan addrman;	
	//CAddress addr(CService("94.113.93.139"));
	//addrman.Add(addr, addr);
	//CAddress addr;
	//addr.SetIP(CNetAddr("34.34.34.34"));
	//addrman.Add(addr, addr);
	
	for(unsigned int i = 0; i < ARRAYLEN(m_pnSeed); i++)
	{
		struct in_addr ip;
		memcpy(&ip, &m_pnSeed[i], sizeof(ip));
		CAddress addr(CService(ip, 9333));
		addrman.Add(addr, addr);
	}

	adb.Write(addrman);
}
