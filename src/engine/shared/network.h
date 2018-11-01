/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_NETWORK_H
#define ENGINE_SHARED_NETWORK_H

#include "ringbuffer.h"
#include "huffman.h"

/*

CURRENT:
	packet header: 5 bytes (6 bytes for connless)
		unsigned char token[2]; // 16bit token
		unsigned char token_flags; // 4bit token, 4bit flags
		unsigned char ack; // 8bit ack
		unsigned char ack_numchunks; // 2bit ack, 6bit chunks
		// TTTTTTTT
		// TTTTTTTT
		// TTTTffff
		// aaaaaaaa
		// aaNNNNNN

	packet header (CONNLESS):
		unsigned char token[2]; // 16bit token
		unsigned char token_flag; // 4bit token, 4bit flags
		unsigned char version_responsetoken; // 4bit version, 4bit response token
		unsigned char responsetoken[2]; // 16bit response token

		// TTTTTTTT
		// TTTTTTTT
		// TTTTffff
		// vvvvRRRR
		// RRRRRRRR
		// RRRRRRRR

	if the token isn't explicitely set by any means, it must be set to
	0xfffff

	chunk header: 2-3 bytes
		unsigned char flags_size; // 2bit flags, 6 bit size
		unsigned char size_seq; // 6bit size, 2bit seq
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
	NETBANTYPE_DROP=2,

	NETCREATE_FLAG_RANDOMPORT=1,
};


enum
{
	NET_VERSION = 2,

	NET_MAX_CHUNKHEADERSIZE = 3,
	
	// packets
	NET_PACKETHEADERSIZE = 5,
	NET_PACKETHEADERSIZE_CONNLESS = NET_PACKETHEADERSIZE + 1,
	NET_MAX_PACKETHEADERSIZE = NET_PACKETHEADERSIZE_CONNLESS,

	NET_MAX_PACKETSIZE = 1400,
	NET_MAX_PAYLOAD = NET_MAX_PACKETSIZE-NET_MAX_PACKETHEADERSIZE,

	NET_PACKETVERSION=1,

	NET_PACKETFLAG_CONTROL=1,
	NET_PACKETFLAG_RESEND=2,
	NET_PACKETFLAG_COMPRESSION=4,
	NET_PACKETFLAG_CONNLESS=8,

	// token
	NET_SEEDTIME = 10,

	NET_TOKENCACHE_SIZE = 16,
	NET_TOKENCACHE_ADDRESSEXPIRY = NET_SEEDTIME/2,
	NET_TOKENCACHE_PACKETEXPIRY = NET_TOKENCACHE_ADDRESSEXPIRY,

	NET_TOKEN_MAX = 0xfffff,
	NET_TOKEN_NONE = NET_TOKEN_MAX,
	NET_TOKEN_MASK = NET_TOKEN_MAX,

	//
	NET_MAX_CLIENTS = 16,
	NET_MAX_CONSOLE_CLIENTS = 4,
	
	NET_MAX_SEQUENCE = 1<<10,
	NET_SEQUENCE_MASK = NET_MAX_SEQUENCE-1,

	NET_CONNSTATE_OFFLINE=0,
	NET_CONNSTATE_TOKEN=1,
	NET_CONNSTATE_CONNECT=2,
	NET_CONNSTATE_PENDING=3,
	NET_CONNSTATE_ONLINE=4,
	NET_CONNSTATE_ERROR=5,

	NET_CHUNKFLAG_VITAL=1,
	NET_CHUNKFLAG_RESEND=2,

	NET_CTRLMSG_KEEPALIVE=0,
	NET_CTRLMSG_CONNECT=1,
	NET_CTRLMSG_CONNECTACCEPT=2,
	NET_CTRLMSG_ACCEPT=3,
	NET_CTRLMSG_CLOSE=4,
	NET_CTRLMSG_TOKEN=5,

	NET_CONN_BUFFERSIZE=1024*32,

	NET_ENUM_TERMINATOR
};


typedef int (*NETFUNC_DELCLIENT)(int ClientID, const char* pReason, void *pUser);
typedef int (*NETFUNC_NEWCLIENT)(int ClientID, void *pUser);

typedef unsigned int TOKEN;

struct CNetChunk
{
	// -1 means that it's a connless packet
	// 0 on the client means the server
	int m_ClientID;
	NETADDR m_Address; // only used when cid == -1
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
	TOKEN m_Token;
	TOKEN m_ResponseToken; // only used in connless packets
	int m_Flags;
	int m_Ack;
	int m_NumChunks;
	int m_DataSize;
	unsigned char m_aChunkData[NET_MAX_PAYLOAD];
};


class CNetTokenManager
{
public:
	void Init(NETSOCKET Socket, int SeedTime = NET_SEEDTIME);
	void Update();

	void GenerateSeed();

	int ProcessMessage(const NETADDR *pAddr, const CNetPacketConstruct *pPacket, bool Notify);

	bool CheckToken(const NETADDR *pAddr, TOKEN Token, TOKEN ResponseToken, bool Notify);
	TOKEN GenerateToken(const NETADDR *pAddr) const;
	static TOKEN GenerateToken(const NETADDR *pAddr, int64 Seed);

private:
	NETSOCKET m_Socket;

	int64 m_Seed;
	int64 m_PrevSeed;

	TOKEN m_GlobalToken;
	TOKEN m_PrevGlobalToken;

	int m_SeedTime;
	int64 m_NextSeedTime;
};

typedef void(*FSendCallback)(int TrackID, void *pUser);
struct CSendCBData
{
	FSendCallback m_pfnCallback;
	void *m_pCallbackUser;
	int m_TrackID;
};

class CNetTokenCache
{
public:
	CNetTokenCache();
	~CNetTokenCache();
	void Init(NETSOCKET Socket, const CNetTokenManager *pTokenManager);
	void SendPacketConnless(const NETADDR *pAddr, const void *pData, int DataSize, CSendCBData *pCallbackData = 0);
	void PurgeStoredPacket(int TrackID);
	void FetchToken(const NETADDR *pAddr);
	void AddToken(const NETADDR *pAddr, TOKEN PeerToken);
	TOKEN GetToken(const NETADDR *pAddr);
	void Update();

private:
	class CConnlessPacketInfo
	{
	private:
		static int m_UniqueID;

	public:
		CConnlessPacketInfo() : m_TrackID(CConnlessPacketInfo::m_UniqueID++) {}

		NETADDR m_Addr;
		int m_DataSize;
		char m_aData[NET_MAX_PAYLOAD];
		int64 m_Expiry;
		const int m_TrackID;
		FSendCallback m_pfnCallback;
		void *m_pCallbackUser;
		CConnlessPacketInfo *m_pNext;
	};

	struct CAddressInfo
	{
		NETADDR m_Addr;
		TOKEN m_Token;
		int64 m_Expiry;
	};

	TStaticRingBuffer<CAddressInfo,
	                  NET_TOKENCACHE_SIZE * sizeof(CAddressInfo),
	                  CRingBufferBase::FLAG_RECYCLE> m_TokenCache;

	CConnlessPacketInfo *m_pConnlessPacketList; // TODO: enhance this, dynamic linked lists
	                                            // are bad for performance
	NETSOCKET m_Socket;
	const CNetTokenManager *m_pTokenManager;
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
	unsigned short m_PeerAck;
	unsigned m_State;

	int m_RemoteClosed;
	bool m_BlockCloseMsg;

	TStaticRingBuffer<CNetChunkResend, NET_CONN_BUFFERSIZE> m_Buffer;

	int64 m_LastUpdateTime;
	int64 m_LastRecvTime;
	int64 m_LastSendTime;

	char m_ErrorString[256];

	CNetPacketConstruct m_Construct;

	TOKEN m_Token;
	TOKEN m_PeerToken;
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
	void SendControlWithToken(int ControlMsg);
	void ResendChunk(CNetChunkResend *pResend);
	void Resend();

	static TOKEN GenerateToken(const NETADDR *pPeerAddr);

public:
	void Init(NETSOCKET Socket, bool BlockCloseMsg);
	int Connect(NETADDR *pAddr);
	void Disconnect(const char *pReason);

	void SetToken(TOKEN Token);

	TOKEN Token() const { return m_Token; }
	TOKEN PeerToken() const { return m_PeerToken; }
	NETSOCKET Socket() const { return m_Socket; }

	int Update();
	int Flush();

	int Feed(CNetPacketConstruct *pPacket, NETADDR *pAddr);
	int QueueChunk(int Flags, int DataSize, const void *pData);
	void SendPacketConnless(const char *pData, int DataSize);

	const char *ErrorString();
	void SignalResend();
	int State() const { return m_State; }
	const NETADDR *PeerAddress() const { return &m_PeerAddr; }

	void ResetErrorString() { m_ErrorString[0] = 0; }
	const char *ErrorString() const { return m_ErrorString; }

	// Needed for GotProblems in NetClient
	int64 LastRecvTime() const { return m_LastRecvTime; }
	int64 ConnectTime() const { return m_LastUpdateTime; }

	int AckSequence() const { return m_Ack; }
};

class CConsoleNetConnection
{
private:
	int m_State;

	NETADDR m_PeerAddr;
	NETSOCKET m_Socket;

	char m_aBuffer[NET_MAX_PACKETSIZE];
	int m_BufferOffset;

	char m_aErrorString[256];

	bool m_LineEndingDetected;
	char m_aLineEnding[3];

public:
	void Init(NETSOCKET Socket, const NETADDR *pAddr);
	void Disconnect(const char *pReason);

	int State() const { return m_State; }
	const NETADDR *PeerAddress() const { return &m_PeerAddr; }
	const char *ErrorString() const { return m_aErrorString; }

	void Reset();
	int Update();
	int Send(const char *pLine);
	int Recv(char *pLine, int MaxLength);
};

class CNetRecvUnpacker
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
	struct CSlot
	{
	public:
		CNetConnection m_Connection;
	};

	NETSOCKET m_Socket;
	class CNetBan *m_pNetBan;
	CSlot m_aSlots[NET_MAX_CLIENTS];
	int m_MaxClients;
	int m_MaxClientsPerIP;

	NETFUNC_NEWCLIENT m_pfnNewClient;
	NETFUNC_DELCLIENT m_pfnDelClient;
	void *m_UserPtr;

	CNetRecvUnpacker m_RecvUnpacker;

	CNetTokenManager m_TokenManager;
	CNetTokenCache m_TokenCache;

	int m_Flags;
public:
	int SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser);

	//
	bool Open(NETADDR BindAddr, class CNetBan *pNetBan, int MaxClients, int MaxClientsPerIP, int Flags);
	int Close();

	// the token parameter is only used for connless packets
	int Recv(CNetChunk *pChunk, TOKEN *pResponseToken = 0);
	int Send(CNetChunk *pChunk, TOKEN Token = NET_TOKEN_NONE);
	int Update();

	//
	int Drop(int ClientID, const char *pReason);

	// status requests
	const NETADDR *ClientAddr(int ClientID) const { return m_aSlots[ClientID].m_Connection.PeerAddress(); }
	NETSOCKET Socket() const { return m_Socket; }
	class CNetBan *NetBan() const { return m_pNetBan; }
	int NetType() const { return m_Socket.type; }
	int MaxClients() const { return m_MaxClients; }

	//
	void SetMaxClientsPerIP(int Max);
};

class CNetConsole
{
	struct CSlot
	{
		CConsoleNetConnection m_Connection;
	};

	NETSOCKET m_Socket;
	class CNetBan *m_pNetBan;
	CSlot m_aSlots[NET_MAX_CONSOLE_CLIENTS];

	NETFUNC_NEWCLIENT m_pfnNewClient;
	NETFUNC_DELCLIENT m_pfnDelClient;
	void *m_UserPtr;

	CNetRecvUnpacker m_RecvUnpacker;

public:
	void SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser);

	//
	bool Open(NETADDR BindAddr, class CNetBan *pNetBan, int Flags);
	int Close();

	//
	int Recv(char *pLine, int MaxLength, int *pClientID = 0);
	int Send(int ClientID, const char *pLine);
	int Update();

	//
	int AcceptClient(NETSOCKET Socket, const NETADDR *pAddr);
	int Drop(int ClientID, const char *pReason);

	// status requests
	const NETADDR *ClientAddr(int ClientID) const { return m_aSlots[ClientID].m_Connection.PeerAddress(); }
	class CNetBan *NetBan() const { return m_pNetBan; }
};



// client side
class CNetClient
{
	CNetConnection m_Connection;
	CNetRecvUnpacker m_RecvUnpacker;

	CNetTokenCache m_TokenCache;
	CNetTokenManager m_TokenManager;

	NETSOCKET m_Socket;
	int m_Flags;
public:
	// openness
	bool Open(NETADDR BindAddr, int Flags);
	int Close();

	// connection state
	int Disconnect(const char *Reason);
	int Connect(NETADDR *Addr);

	// communication
	int Recv(CNetChunk *pChunk, TOKEN *pResponseToken = 0);
	int Send(CNetChunk *pChunk, TOKEN Token = NET_TOKEN_NONE, CSendCBData *pCallbackData = 0);
	void PurgeStoredPacket(int TrackID);

	// pumping
	int Update();
	int Flush();

	int ResetErrorString();

	// error and state
	int NetType() const { return m_Socket.type; }
	int State() const;
	bool GotProblems() const;
	const char *ErrorString() const;
};



// TODO: both, fix these. This feels like a junk class for stuff that doesn't fit anywere
class CNetBase
{
	static IOHANDLE ms_DataLogSent;
	static IOHANDLE ms_DataLogRecv;
	static CHuffman ms_Huffman;
public:
	static void OpenLog(IOHANDLE DataLogSent, IOHANDLE DataLogRecv);
	static void CloseLog();
	static void Init();
	static int Compress(const void *pData, int DataSize, void *pOutput, int OutputSize);
	static int Decompress(const void *pData, int DataSize, void *pOutput, int OutputSize);

	static void SendControlMsg(NETSOCKET Socket, const NETADDR *pAddr, TOKEN Token, int Ack, int ControlMsg, const void *pExtra, int ExtraSize);
	static void SendControlMsgWithToken(NETSOCKET Socket, const NETADDR *pAddr, TOKEN Token, int Ack, int ControlMsg, TOKEN MyToken);
	static void SendPacketConnless(NETSOCKET Socket, const NETADDR *pAddr, TOKEN Token, TOKEN ResponseToken, const void *pData, int DataSize);
	static void SendPacket(NETSOCKET Socket, const NETADDR *pAddr, CNetPacketConstruct *pPacket);
	static int UnpackPacket(unsigned char *pBuffer, int Size, CNetPacketConstruct *pPacket);

	// The backroom is ack-NET_MAX_SEQUENCE/2. Used for knowing if we acked a packet or not
	static int IsSeqInBackroom(int Seq, int Ack);
};


#endif
