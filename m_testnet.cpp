#include "net.h"
#include <netdb.h>
#include <iostream>
#include "chainparamsbase.h"
#include "chainparams.h" 

using namespace std;

typedef long long int64;
typedef unsigned long long uint64;

#define BITCOIN_SEED_NONCE 0x0539a019ca550825

///////////////////////////////////////////////////////
//this is for net magic bff9feb4 port 9333           //
//usage: m_testnet 22.244.180.174                    //
///////////////////////////////////////////////////////

bool ProcessMessage(CNode* pnode, std::string strCommand, CDataStream& vRecv, int64_t nTimeReceived);

////////////////////////////////////////////////////////

void sendgetdata(CNode *pnode)
{
	vector<CInv> vGetData;
	uint256 hash("0000327c652b3730b38cf68b700b4bbfdd221f4f2cd9ba311a20e01c5950e372");
	vGetData.push_back(CInv(MSG_BLOCK, hash));
	pnode->PushMessage("getdata", vGetData);
	
}


void PushVersion(CNode *pnode, CAddress you)
{
	int64 nTime = time(NULL);
	uint64 nLocalNonce = BITCOIN_SEED_NONCE;
	int64 nLocalServices = 0;
	CAddress me(CService("0,0,0,0"));
	int nBestHeight = 2300000;
	std::string ver = "/bitcoin-seeder:0.01/";
	pnode->BeginMessage("version");	
	pnode->ssSend << 60000 << nLocalServices << nTime << you << me << nLocalNonce << ver << nBestHeight;
	pnode->EndMessage();
	//pnode->PushMessage("version", 600000, nLocalServices, nTime, you, me,
	//					nLocalNonce, ver);
}

bool Send(CNode *pnode)
{
	if(pnode->hSocket == INVALID_SOCKET) return false;
	CDataStream &vSend = pnode->ssSend;
	//if(vSend.vch.size() == 0) return false;
	//test vSend
	cout<< "size()" << vSend.size() << endl;	
	if(vSend.empty())
		cout<< "empty()" << endl;	

	int nBytes = send(pnode->hSocket, &vSend[0], vSend.size(), 0);
	if(nBytes > 0)
	{
	    vSend.erase(vSend.begin(), vSend.begin() + nBytes);
	}
	else
	{
		close(pnode->hSocket);
		pnode->hSocket = INVALID_SOCKET;
		return false;
	}	
	return true;
}

bool HandleSocket(CNode* pnode)
{
	//struct timeval timeout;
	//timeout.tv_sec = 0;
	//timeout.tv_usec = 50000;
	fd_set fdsetRecv;	
	FD_ZERO(&fdsetRecv);
	FD_SET(pnode->hSocket, &fdsetRecv);

	int ret = select(pnode->hSocket+1, &fdsetRecv, 0, &fdsetRecv, NULL);
	if(ret == SOCKET_ERROR)
	{
		cout<< "select error" <<endl;
		return false;
	}
	
	
		char pchBuf[0x10000];
		int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), 0);
		if(nBytes >0)
		{
			if(!pnode->ReceiveMsgBytes(pchBuf, nBytes))
				cout<< "ReceiveMsgBtyes false"<<endl;
			pnode->nLastRecv = GetTime();
			pnode->nRecvBytes += nBytes;
			pnode->RecordBytesRecv(nBytes);
			cout<<"Total recv: "<<pnode->GetTotalBytesRecv()<<endl;
		}
		else
		{
			cout<< "recv failed" <<endl;
			return false;
		}
		
		return true;
}

bool ProcessNodeMsg(CNode* pnode)
{
	//node message
	std::deque<CNetMessage>::iterator it = pnode->vRecvMsg.begin();
	while(it!= pnode->vRecvMsg.end())
	{
		CNetMessage &msg = *it;

		CMessageHeader& hdr = msg.hdr;
		if(!hdr.IsValid())
		{
			cout<<"message head error"<<endl;
			return false;
		}
		
		it++;		

		std::string strCommand = hdr.GetCommand();
		
		unsigned int nMessageSize = hdr.nMessageSize;

		cout<<"Message: "<< strCommand<< "            size:  " << nMessageSize<<endl;

		//Checksum
		CDataStream& vRecv = msg.vRecv;
		uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
		unsigned int nChecksum = 0;
		memcpy(&nChecksum, &hash, sizeof(nChecksum));
		if(nChecksum != hdr.nChecksum)
		{
			cout<<"checksum error"<<endl;
			return false;
		}
		
		ProcessMessage(pnode, strCommand, vRecv, msg.nTime);
	}
	
	//erase VRecvMsg
	pnode->vRecvMsg.erase(pnode->vRecvMsg.begin(), it);
}

bool ProcessMessage(CNode* pnode, std::string strCommand, CDataStream& vRecv, int64_t nTimeReceived)
{
	if(strCommand == "version")
	{
		int64_t nTime;
		CAddress addrMe;
		CAddress addrFrom;
		uint64_t nNonce = 1;
		std::string version;	
		int nBestHeight;
		vRecv >> pnode->nVersion >> pnode->nServices >> nTime >> addrMe >> addrFrom >> nNonce >> version >> nBestHeight;
		cout<< "nVersion: "<< pnode->nVersion <<endl;
		cout<< "nServices: " << pnode->nServices <<endl;
		cout<< "nTime: " << nTime << endl;
		cout<< "addrMe: " << addrMe.ToString() << endl;			
		cout<< "addrFrom: " << addrFrom.ToString() << endl;
		cout<< "Nonce: " << nNonce << endl;
		cout<< "Version: " << version << endl;
		cout<< "BestHeight: " << nBestHeight << endl;
		
		pnode->PushMessage("getaddr");
				
	}
	else if(strCommand == "addr")
	{
		vector<CAddress> vAddr;
		vRecv >> vAddr;
		if(vAddr.size() > 1000)
			cout<< "addr size: "<< vAddr.size() << " is >1000"<<endl;
		BOOST_FOREACH(CAddress& addr, vAddr)
		{
			cout<< addr.ToString() <<endl;
		}
	}
	else if(strCommand == "inv")
	{
		vector<CInv> vInv;
		vRecv >> vInv;
		for(unsigned int nInv = 0; nInv < vInv.size() ; nInv++)
		{
			const CInv &inv = vInv[nInv];
			pnode->AddInventoryKnown(inv);

			cout<< "got inv: "<< inv.ToString()<< endl;
		}
	}
	else if(strCommand == "block")
	{
		CBlock block;
		vRecv >> block;
		cout<<"block hash: "<<block.GetHash().ToString()<<endl;
		for(unsigned int i = 0; i < block.vtx.size(); i++)
		{
			cout<<" "<<block.vtx[i].ToString()<<" ";
		}
		sleep(1);
	}
}

int main(int argc, char* argv[])
{
	SelectParams(CBaseChainParams::MAIN);
	fPrintToConsole = true;
	fDebug = true;

	CAddress addr;
	const char *pszDest = argv[1];
	CNode *pnode = ConnectNode(addr, pszDest);
	if(pnode){
		cout<< "ConnectNode() " <<endl;
	}
	else
	{
		cout<< "can't ConnectNode" <<endl;
		return 1;
	}
	//PushVersion(pnode, addr);
	/*
	if(Send(pnode)){
		cout<< "Send success" << endl;
	}
	else
	{
		cout << "Send Failed" << endl;
	}
	*/
	


	bool sendgetdataflag = true;
	//receive socket	
	while(true)
	{

	if(HandleSocket(pnode))
	{
		cout << "Sent Bytes: "<<pnode->GetTotalBytesSent() <<endl;	
		cout<<endl;
	}
	else
	{
		cout << "HandleSocket() failed" <<endl;
		return 1;
	}

	ProcessNodeMsg(pnode);

	if(sendgetdataflag)
	{
		sendgetdata(pnode);
		sendgetdataflag = false;

		//getblocks
		vector<uint256> vHave;
		vHave.push_back(uint256("06ae980427bfb38ba32035ee6d2e577375a39853acb9ca45d5e3142028f4d028"));
		CBlockLocator locator(vHave);
		pnode->PushMessage("getblocks", locator, uint256(0));
	}
	}
}
