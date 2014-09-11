#include "net.h"
#include <iostream>
#include "chainparamsbase.h"
#include "chainparams.h" 
#include "compat.h"

using std::cout;
using std::endl;
using std::string;

typedef long long int64; 
typedef unsigned long long uint64; 


int HandleSocket(CNode *pnode)
{
	//handel pnode have data to receive
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 50000;

	fd_set fdsetRecv;
	fd_set fdsetSend;
	FD_ZERO(&fdsetRecv);
	FD_ZERO(&fdsetSend);

	if(!pnode->vSendMsg.empty())
	{
		FD_SET(pnode->hSocket, &fdsetSend);
	}
	if(pnode->vRecvMsg.empty() || !pnode->vRecvMsg.front().complete())
	{
		FD_SET(pnode->hSocket, &fdsetRecv);
	}

	int nSelect = select(pnode->hSocket + 1, &fdsetRecv, &fdsetSend, 0, &timeout);
	if(nSelect == SOCKET_ERROR)
	{
		cout<< "select error" << endl;
		return 1;
	}
	
	
	if(FD_ISSET(pnode->hSocket, &fdsetRecv))
	{
		char pchBuf[0x10000];
		int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), 0);
		if(nBytes > 0)
		{
			pnode->ReceiveMsgBytes(pchBuf, nBytes);
			pnode->nLastRecv = GetTime();
			pnode->nRecvBytes += nBytes;
			pnode->RecordBytesRecv(nBytes);

			cout<< "receive data " <<endl;
		}
		else
		{
			cout<< "recv failed"<<endl;
			return 1;
		}
	}
	

	//
	// Send
	//
	if(pnode->hSocket == INVALID_SOCKET)
	{
		cout<< "socket INVAILD" << endl;
		return 1;
	}
	if(FD_ISSET(pnode->hSocket, &fdsetSend))
	{
		cout<< "send data" <<endl;
		SocketSendData(pnode);
	}	
	

	return 0;
}

bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
		
}

bool ProcessNodeMessages(CNode* pfrom)
{
	std::deque<CNetMessage>::iterator it = pfrom->vRecvMsg.begin();
	while(it != pfrom->vRecvMsg.end())
	{
		CNetMessage& msg = *it;
	
		if(!msg.complete())
			break;
		
		it++;
		
		//Scan for message start
		if(memcmp(msg.hdr.pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE) != 0)
		{
			cout<< "PROCESSMESSAGE: INVAILD MESSAGESTART"<< endl;
			break;
		}

		//Read header
		CMessageHeader& hdr = msg.hdr;
		if(!hdr.IsValid())
		{
			cout<< "PROCESSMESSAGE: ERRORS IN HEADER"<< endl;
			continue;	
		}
		string strCommand = hdr.GetCommand();

		//Message size
		unsigned int nMessageSize = hdr.nMessageSize;
	
		//Checksum
		CDataStream& vRecv = msg.vRecv;
		uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
		unsigned int nChecksum = 0;
		memcpy(&nChecksum, &hash, sizeof(nChecksum));
		if(nChecksum != hdr.nChecksum)
		{
			cout << "Checksum error" << endl;
			continue;
		}

		//Process message
		bool fRet = false;
		fRet = ProcessMessage(pfrom, strCommand, vRecv);
		if(!fRet)
			cout << "ProcessMessage() failed" << endl;

		
	}
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
		

	while(true)
	{
		HandleSocket(pnode);
		//add ProcessMessage to let vSendMsg & vRecvMsg empty 
		ProcessNodeMessages(pnode);
	}

}
