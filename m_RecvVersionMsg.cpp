#include "net.h"
#include <netdb.h>
#include <iostream>
#include "chainparamsbase.h"
#include "chainparams.h" 
using std::cout;
using std::endl;

typedef long long int64;
typedef unsigned long long uint64;

#define BITCOIN_SEED_NONCE 0x0539a019ca550825

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


int main()
{
	SelectParams(CBaseChainParams::MAIN);
	fPrintToConsole = true;
	fDebug = true;

	CAddress addr;
	const char *pszDest = "192.168.1.105";
	CNode *pnode = ConnectNode(addr, pszDest);
	if(pnode){
		cout<< "ConnectNode() " <<endl;
	}
	else
	{
		cout<< "can't ConnectNode" <<endl;
		return 1;
	}
	cout << "Sent Bytes: "<<pnode->GetTotalBytesSent() <<endl;	
	PushVersion(pnode, addr);
	//pnode->PushMessage("ping");
	/*
	if(Send(pnode)){
		cout<< "Send success" << endl;
	}
	else
	{
		cout << "Send Failed" << endl;
	}
	*/
	
	cout << "Sent Bytes: "<<pnode->GetTotalBytesSent() <<endl;	

	//receive socket	

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000;
	fd_set fdsetRecv;	
	FD_ZERO(&fdsetRecv);
	FD_SET(pnode->hSocket, &fdsetRecv);

	int ret = select(pnode->hSocket+1, &fdsetRecv, 0, &fdsetRecv, NULL);
	if(ret == SOCKET_ERROR)
	{
		cout<< "select error" <<endl;
		return 1;
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
			cout<<"receive finish"<<endl;
			cout<<"Total recv: "<<pnode->GetTotalBytesRecv()<<endl;
		}
		else
		{
			cout<< "recv failed" <<endl;
		}


	//node message
	std::deque<CNetMessage>::iterator it = pnode->vRecvMsg.begin();
	while(it!= pnode->vRecvMsg.end())
	{
		CNetMessage &msg = *it;

		CMessageHeader& hdr = msg.hdr;
		if(!hdr.IsValid())
		{
			cout<<"message head error"<<endl;
			return 1;
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
			return 1;
		}
		
		//ProcessMessage
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
	}
}
