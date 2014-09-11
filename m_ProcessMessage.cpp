#include "net.h"
#include <iostream>
#include "chainparamsbase.h"
#include "chainparams.h" 
#include "compat.h"
#include "main.h"

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
	
	if(FD_ISSET(pnode->hSocket, &fdsetRecv)) {
		char pchBuf[0x10000];
		int nBytes = recv(pnode->hSocket, pchBuf, sizeof(pchBuf), 0);
		if(nBytes > 0)
		{
			pnode->ReceiveMsgBytes(pchBuf, nBytes);
			pnode->nLastRecv = GetTime();
			pnode->nRecvBytes += nBytes;
			pnode->RecordBytesRecv(nBytes);

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
	LogPrintf("received: %s (%u bytes)\n", strCommand, vRecv.size());			

	if(strCommand == "version")
	{
		// Each connection can only send one version message
		if(pfrom->nVersion != 0)
		{
			pfrom->PushMessage("reject", strCommand, REJECT_DUPLICATE, string("Duplicate version message"));
			//delete Misbehaving
			return false;
		}

		int64_t nTime;
		CAddress addrMe;
		CAddress addrFrom;
		uint64_t nNonce = 1;
		vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;
		if(pfrom->nVersion < MIN_PEER_PROTO_VERSION)
		{
			//disconnect from peers older than this proto version
			LogPrintf("partner %s using obsolete version %i; disconnecting\n", pfrom->addr.ToString(), pfrom->nVersion);
			pfrom->PushMessage("reject", strCommand, REJECT_OBSOLETE, strprintf("Version must be %d or greater", MIN_PEER_PROTO_VERSION));
			pfrom->fDisconnect = true;
			return false;
		}

		if(pfrom->nVersion == 10300)
			pfrom->nVersion = 300;
		if(!vRecv.empty())
			vRecv >> addrFrom >> nNonce;
		if(!vRecv.empty()) {
			vRecv >> pfrom->strSubVer;
			pfrom->cleanSubVer = SanitizeString(pfrom->strSubVer);
		}
		if(!vRecv.empty())
			vRecv >> pfrom->nStartingHeight;
		if(!vRecv.empty())
			vRecv >> pfrom->fRelayTxes; // set to true after we get the first filter* message
		else
			pfrom->fRelayTxes = true;

		if(pfrom->fInbound && addrMe.IsRoutable())
		{
			pfrom->addrLocal = addrMe;
			SeenLocal(addrMe);
		}
			
		//Disconnect if we connected to ourself
		if(nNonce == nLocalHostNonce && nNonce > 1)
		{
			LogPrintf("connected to self at %s, disconnecting\n", pfrom->addr.ToString());
			pfrom->fDisconnect = true;
			return true;
		}		

		if(pfrom->fInbound)
			pfrom->PushVersion();

		pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);

		//Change version
		pfrom->PushMessage("verack");
		pfrom->ssSend.SetVersion(std::min(pfrom->nVersion, PROTOCOL_VERSION));

		if(!pfrom->fInbound)
		{
			// Advertise out address	
			// ...
		}

		// Relay alerts
		// ...

		pfrom->fSuccessfullyConnected = true;

		LogPrintf("receive version message: %s: version %d, blocks=%d, from=%s, peer=%s\n", pfrom->cleanSubVer, pfrom->nVersion, pfrom->nStartingHeight, addrFrom.ToString(), pfrom->addr.ToString());

		//AddTimeData(pfrom->addr, nTime);
		//cPeerBlockCounts.input(pfrom->nStartingHeight);
	}

	return true;
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

		break;
	}

	if(!pfrom->fDisconnect)
		pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

	return true;
}

bool SendNodeMessages(CNode* pnode)
{
	
}

void MessageHandler(CNode* pnode)
{
	ProcessNodeMessages(pnode);
	SendNodeMessages(pnode);	
}

int main()
{
	SelectParams(CBaseChainParams::MAIN);
	fPrintToConsole = true;

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
		MessageHandler(pnode);	
	}

}
