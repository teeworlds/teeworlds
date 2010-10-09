#ifndef ENGINE_SHARED_NETWORK_H
#define ENGINE_SHARED_NETWORK_H

#include "ringbuffer.h"
#include "huffman.h"

/*

CURRENT:
	packet header: 3 bytes
		unsigned char flags_ack; // 4bit flags, 4bit ack
		unsigned char ack; // 8 bit ack
		unsigned char num_chunks; // 8 bit chunks
		
		(unsigned char padding[3])	// 24 bit extra incase it's a connection less packet
									// this is to make sure that it's compatible with the
									// old protocol

	chunk header: 2-3 bytes
		unsigned char flags_size; // 2bit flags, 6 bit size
		unsigned char size_seq; // 4bit size, 4bit seq
		(unsigned char seq;) // 8bit seq, if vital flag is set
*/

enum
{
	NETFLAG_ALLOWSTATELESS=1,
	NETSENDFLAG_VITAL=1,
	NETSENDFLAG_CONNLESS=2,
	NETSENDFLAG_FLUSH=4,
	
	NETSTATE_OFFLINE=0,
	NETSTATE_CONNECTING,
	NETSTATE_ONLINE,
	
	NETBANTYPE_SOFT=1,
	NETBANTYPE_DROP=2
};


enum
{
	NET_VERSION = 2,

	NET_MAX_CHUNKSIZE = 1024,
	NET_MAX_PAYLOAD = NET_MAX_CHUNKSIZE+16,
	NET_MAX_PACKETSIZE = NET_MAX_PAYLOAD+16,
	NET_MAX_CHUNKHEADERSIZE = 5,
	NET_PACKETHEADERSIZE = 3,
	NET_MAX_CLIENTS = 16,
	NET_MAX_SEQUENCE = 1<<10,
	NET_SEQUENCE_MASK = NET_MAX_SEQUENCE-1,

	NET_CONNSTATE_OFFLINE=0,
	NET_CONNSTATE_CONNECT=1,
	NET_CONNSTATE_PENDING=2,
	NET_CONNSTATE_ONLINE=3,
	NET_CONNSTATE_ERROR=4,

	NET_PACKETFLAG_CONTROL=1,
	NET_PACKETFLAG_CONNLESS=2,
	NET_PACKETFLAG_RESEND=4,
	NET_PACKETFLAG_COMPRESSION=8,

	NET_CHUNKFLAG_VITAL=1,
	NET_CHUNKFLAG_RESEND=2,
	
	NET_CTRLMSG_KEEPALIVE=0,
	NET_CTRLMSG_CONNECT=1,
	NET_CTRLMSG_CONNECTACCEPT=2,
	NET_CTRLMSG_ACCEPT=3,
	NET_CTRLMSG_CLOSE=4,
	
	NET_SERVER_MAXBANS=1024,
	
	NET_CONN_BUFFERSIZE=1024*16,
	
	NET_ENUM_TERMINATOR
};


typedef int (*NETFUNC_DELCLIENT)(int ClientID, const char* pReason, void *pUser);
typedef int (*NETFUNC_NEWCLIENT)(int ClientID, void *pUser);

struct CNetChunk
{
	// -1 means that it's a stateless packet
	// 0 on the client means the server
	int m_ClientID;
	NETADDR m_Address; // only used when client_id == -1
	int m_Flags;
	int m_DataSize;
	const void *m_pData;
};

class CNetChunkHeader
{
public:
	int m_Flags;
	int m_Size;
	int m_Sequence;
	
	unsigned char *Pack(unsigned char *pData);
	unsigned char *Unpack(unsigned char *pData);
};

class CNetChunkResend
{
public:
	int m_Flags;
	int m_DataSize;
	unsigned char *m_pData;

	int m_Sequence;
	int64 m_LastSendTime;
	int64 m_FirstSendTime;
};

class CNetPacketConstruct
{
public:
	int m_Flags;
	int m_Ack;
	int m_NumChunks;
	int m_DataSize;
	unsigned char m_aChunkData[NET_MAX_PAYLOAD];
};


class CNetConnection
{
	// TODO: is this needed because this needs to be aware of
	// the ack sequencing number and is also responible for updating
	// that. this should be fixed.
	friend class CNetRecvUnpacker;
private:
	unsigned short m_Sequence;
	unsigned short m_Ack;
	unsigned m_State;
	
	int m_Token;
	int m_RemoteClosed;
	
	TStaticRingBuffer<CNetChunkResend, NET_CONN_BUFFERSIZE> m_Buffer;
	
	int64 m_LastUpdateTime;
	int64 m_LastRecvTime;
	int64 m_LastSendTime;
	
	char m_ErrorString[256];
	
	CNetPacketConstruct m_Construct;
	
	NETADDR m_PeerAddr;
	NETSOCKET m_Socket;
	NETSTATS m_Stats;
	
	//
	void Reset();
	void ResetStats();
	void SetError(const char *pString);
	void AckChunks(int Ack);
	
	int QueueChunkEx(int Flags, int DataSize, const void *pData, int Sequence);
	void SendControl(int ControlMsg, const void *pExtra, int ExtraSize);
	void ResendChunk(CNetChunkResend *pResend);
	void Resend();

public:
	void Init(NETSOCKET Socket);
	int Connect(NETADDR *pAddr);
	void Disconnect(const char *pReason);

	int Update();
	int Flush();	

	int Feed(CNetPacketConstruct *pPacket, NETADDR *pAddr);
	int QueueChunk(int Flags, int DataSize, const void *pData);

	const char *ErrorString();
	void SignalResend();
	int State() const { return m_State; }
	NETADDR PeerAddress() const { return m_PeerAddr; }
	
	void ResetErrorString() { m_ErrorString[0] = 0; }
	const char *ErrorString() const { return m_ErrorString; }
	
	// Needed for GotProblems in NetClient
	int64 LastRecvTime() const { return m_LastRecvTime; }
	
	int AckSequence() const { return m_Ack; }
};

struct CNetRecvUnpacker
{
public:
	bool m_Valid;
	
	NETADDR m_Addr;
	CNetConnection *m_pConnection;
	int m_CurrentChunk;
	int m_ClientID;
	CNetPacketConstruct m_Data;
	unsigned char m_aBuffer[NET_MAX_PACKETSIZE];

	CNetRecvUnpacker() { Clear(); }
	void Clear();
	void Start(const NETADDR *pAddr, CNetConnection *pConnection, int ClientID);
	int FetchChunk(CNetChunk *pChunk);	
};

// server side
class CNetServer
{
public:
	struct CBanInfo
	{
		NETADDR m_Addr;
		int m_Expires;
	};
	
private:
	class CSlot
	{
	public:
		CNetConnection m_Connection;
	};
	
	class CBan
	{
	public:
		CBanInfo m_Info;
		
		// hash list
		CBan *m_pHashNext;
		CBan *m_pHashPrev;
		
		// used or free list
		CBan *m_pNext;
		CBan *m_pPrev;
	};
	
	
	NETSOCKET m_Socket;
	CSlot m_aSlots[NET_MAX_CLIENTS];
	int m_MaxClients;
	int m_MaxClientsPerIP;

	CBan *m_aBans[256];
	CBan m_BanPool[NET_SERVER_MAXBANS];
	CBan *m_BanPool_FirstFree;
	CBan *m_BanPool_FirstUsed;

	NETFUNC_NEWCLIENT m_pfnNewClient;
	NETFUNC_DELCLIENT m_pfnDelClient;
	void *m_UserPtr;
	
	CNetRecvUnpacker m_RecvUnpacker;
	
	void BanRemoveByObject(CBan *pBan);
	
public:
	int SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser);

	//
	bool Open(NETADDR BindAddr, int MaxClients, int MaxClientsPerIP, int Flags);
	int Close();
	
	//
	int Recv(CNetChunk *pChunk);
	int Send(CNetChunk *pChunk);
	int Update();
	
	//
	int Drop(int ClientID, const char *pReason);

	// banning
	int BanAdd(NETADDR Addr, int Seconds, const char *pReason);
	int BanRemove(NETADDR Addr);
	int BanNum(); // caution, slow
	int BanGet(int Index, CBanInfo *pInfo); // caution, slow

	// status requests
	NETADDR ClientAddr(int ClientID) const { return m_aSlots[ClientID].m_Connection.PeerAddress(); }
	NETSOCKET Socket() const { return m_Socket; }
	int MaxClients() const { return m_MaxClients; }

	//
	void SetMaxClientsPerIP(int Max);
};



// client side
class CNetClient
{
	NETADDR m_ServerAddr;
	CNetConnection m_Connection;
	CNetRecvUnpacker m_RecvUnpacker;
	NETSOCKET m_Socket;
public:
	// openness
	bool Open(NETADDR BindAddr, int Flags);
	int Close();
	
	// connection state
	int Disconnect(const char *Reason);
	int Connect(NETADDR *Addr);
	
	// communication
	int Recv(CNetChunk *Chunk);
	int Send(CNetChunk *Chunk);
	
	// pumping
	int Update();
	int Flush();

	int ResetErrorString();
	
	// error and state
	int State();
	int GotProblems();
	const char *ErrorString();
};



// TODO: both, fix these. This feels like a junk class for stuff that doesn't fit anywere
class CNetBase
{
	static IOHANDLE ms_DataLogSent;
	static IOHANDLE ms_DataLogRecv;
	static CHuffman ms_Huffman;
public:
	static void OpenLog(const char *pSentlog, const char *pRecvlog);
	static void Init();
	static int Compress(const void *pData, int DataSize, void *pOutput, int OutputSize);
	static int Decompress(const void *pData, int DataSize, void *pOutput, int OutputSize);
	
	static void SendControlMsg(NETSOCKET Socket, NETADDR *pAddr, int Ack, int ControlMsg, const void *pExtra, int ExtraSize);
	static void SendPacketConnless(NETSOCKET Socket, NETADDR *pAddr, const void *pData, int DataSize);
	static void SendPacket(NETSOCKET Socket, NETADDR *pAddr, CNetPacketConstruct *pPacket);
	static int UnpackPacket(unsigned char *pBuffer, int Size, CNetPacketConstruct *pPacket);

	// The backroom is ack-NET_MAX_SEQUENCE/2. Used for knowing if we acked a packet or not
	static int IsSeqInBackroom(int Seq, int Ack);	
};


#endif
