#include "net.h"
#include <iostream>
#include "chainparamsbase.h"
#include "chainparams.h" 
#include "compat.h"

using std::cout;
using std::endl;

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
	}

}
