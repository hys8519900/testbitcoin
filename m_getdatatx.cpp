#include "net.h"
#include <iostream>
#include "chainparams.h" 
#include "compat.h"
#include "main.h"
#include "addrman.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

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
			// Get recent addresses
			if(pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || addrman.size() < 1000)
			{
				pfrom->PushMessage("getaddr");
				pfrom->fGetAddr = true;
			}
			addrman.Good(pfrom->addr);
		}
		else
		{
			//...
		}

		// Relay alerts
		// ...

		pfrom->fSuccessfullyConnected = true;

		LogPrintf("receive version message: %s: version %d, blocks=%d, from=%s, peer=%s\n", pfrom->cleanSubVer, pfrom->nVersion, pfrom->nStartingHeight, addrFrom.ToString(), pfrom->addr.ToString());

		//AddTimeData(pfrom->addr, nTime);
		//cPeerBlockCounts.input(pfrom->nStartingHeight);
	}

	else if(pfrom->nVersion == 0)
	{
		// Must have a version message before anything else
		//Misbehaving(pfrom->GetId(), 1);
		return false;
	}	

	else if(strCommand == "verack")
	{
		pfrom->SetRecvVersion(std::min(pfrom->nVersion, PROTOCOL_VERSION));	
	}

	else if(strCommand == "addr")
	{
		vector<CAddress> vAddr;
		vRecv >> vAddr;
		
		//Don't want addr from older versions unless seeding
		if(pfrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
			return true;
		if(vAddr.size() > 1000)
		{
			//Misbehaving(pfrom->GetId(), 20);
			return error("message addr size() = %u", vAddr.size());
		}

		// Store the new addresses
		vector<CAddress> vAddrOk;
		int64_t nNow = GetAdjustedTime();
		int64_t nSince = nNow - 10 * 60;
		BOOST_FOREACH(CAddress& addr, vAddr)
		{
			if(addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
				addr.nTime = nNow - 5 * 24 * 60 * 60;
			pfrom->AddAddressKnown(addr);
			bool fReachable = IsReachable(addr);
			if(addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
			{
				//...	
			}
			//...
			//Do not store address outside our network
			if(fReachable)
			vAddrOk.push_back(addr);

			//print each addr
			//cout << "addr: " << addr.ToString() << endl;
		}
		addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
		if(vAddr.size() < 1000)
			pfrom->fGetAddr = false;
		if(pfrom->fOneShot)
			pfrom->fDisconnect = true;
	}	
	
	else if(strCommand == "inv")
	{
		vector<CInv> vInv;
		vRecv >> vInv;	
		if(vInv.size() > MAX_INV_SZ)
		{
			//Misbehaving
		}
		
		for(unsigned int nInv = 0; nInv < vInv.size(); nInv++)
		{
			const CInv &inv = vInv[nInv];
			
			pfrom->AddInventoryKnown(inv);
			
			if(inv.type != MSG_BLOCK)
				pfrom->AskFor(inv); //this will insert inv into mapAskFor
		}
	}
		
	else if(strCommand == "tx")
	{
		cout << "recive tx message" << endl;		
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

bool SendNodeMessages(CNode* pto)
{
	//
	// Message: getdata (non-blocks)
	//

	int64_t nNow = GetTimeMicros();	
	vector<CInv> vGetData;

	while(!pto->fDisconnect && !pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
	{
		const CInv& inv = (*pto->mapAskFor.begin()).second;
		LogPrintf("sending getdata: %s\n", inv.ToString());
		vGetData.push_back(inv);
		if(vGetData.size() >= 3)
		{
			pto->PushMessage("getdata", vGetData);
			vGetData.clear();
		}
		pto->mapAskFor.erase(pto->mapAskFor.begin());
	}	
}

void MessageHandler(CNode* pnode)
{
	ProcessNodeMessages(pnode);
	SendNodeMessages(pnode);	
}

int main()
{
	SelectParams(CChainParams::MAIN);
	fPrintToConsole = false;
	LogPrintf("ssss\n");
	cout<< "aaa" << endl;
	return 0;

	CAddress addr;
	const char *pszDest = "151.225.32.8";
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
