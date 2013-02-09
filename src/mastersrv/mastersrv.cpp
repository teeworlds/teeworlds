/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/kernel.h>
#include <engine/storage.h>

#include <engine/shared/config.h>
#include <engine/shared/netban.h>
#include <engine/shared/network.h>

#include "mastersrv.h"

int main(int argc, const char **argv) // ignore_convention
{
	dbg_logger_stdout();
	net_init();

	IKernel *pKernel = IKernel::Create();
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	IConfig *pConfig = CreateConfig();
	IConsole *pConsole = CreateConsole(CFGFLAG_MASTER);
	IMastersrv *pMastersrv = CreateMastersrv();

	bool RegisterFail = !pKernel->RegisterInterface(pStorage);
	RegisterFail |= !pKernel->RegisterInterface(pConsole);
	RegisterFail |= !pKernel->RegisterInterface(pConfig);
	RegisterFail |= !pKernel->RegisterInterface(pMastersrv);

	if(RegisterFail)
		return -1;

	pConfig->Init();
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	int Result;

	if((Result = pMastersrv->Init()) != 0)
	{
		dbg_msg("mastersrv", "initialisation failed (%d)", Result);
		return Result;
	}

	Result = pMastersrv->Run();

	delete pMastersrv;
	delete pConsole;
	delete pConfig;
	delete pStorage;
	delete pKernel;

	return Result;
}

void IMastersrvSlave::NetaddrToMastersrv(CMastersrvAddr *pOut, const NETADDR *pIn)
{
	dbg_assert(pIn->type == NETTYPE_IPV6 || pIn->type == NETTYPE_IPV4, "nettype not supported");

	if(pIn->type == NETTYPE_IPV6)
	{
		mem_copy(pOut->m_aIp, pIn->ip, sizeof(pOut->m_aIp));
	}
	else if(pIn->type == NETTYPE_IPV4)
	{
		static const unsigned char aIPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };
		mem_copy(pOut->m_aIp, aIPV4Mapping, sizeof(aIPV4Mapping));
		mem_copy(pOut->m_aIp + 12, pIn->ip, 4);
	}

	pOut->m_aPort[0] = (pIn->port>>8)&0xff; // big endian
	pOut->m_aPort[1] = (pIn->port>>0)&0xff;
}

class CMastersrv : public IMastersrv
{
public:
	enum
	{
		MAX_SERVERS=1200,
		MAX_CHECKSERVERS=MAX_SERVERS,
		MAX_PACKETS=16,

		EXPIRE_TIME=90,
		BAN_REFRESH_TIME=300,
		PACKET_REFRESH_TIME=5,
	};
	CMastersrv();
	~CMastersrv();

	virtual int Init();
	virtual int Run();

	virtual void AddServer(const NETADDR *pAddr, void *pUserData, int Version);
	virtual void AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData, int Version);
	virtual void SendList(const NETADDR *pAddr, void *pUserData, int Version);
	virtual int GetCount() const;

	virtual int Send(int Socket, const CNetChunk *pPacket, TOKEN PacketToken);
	virtual int SendRaw(int Socket, const NETADDR *pAddr, const void *pData, int DataSize);

	struct CRecvRawUserData
	{
		CMastersrv *m_pSelf;
		int m_Socket;
	};
	static int RecvRaw(const NETADDR *pAddr, const void *pData, int DataSize, void *pUserData)
	{
		CRecvRawUserData *pInfo = (CRecvRawUserData *)pUserData;
		return pInfo->m_pSelf->RecvRawImpl(pInfo->m_Socket, pAddr, pData, DataSize);
	}
	int RecvRawImpl(int Socket, const NETADDR *pAddr, const void *pData, int DataSize);
	CRecvRawUserData m_aRecvRawUserData[NUM_SOCKETS];

	void PurgeServers();
	void UpdateServers();
	void BuildPackets();

	void ReloadBans();

	static const char *SlaveString(int Version);

private:
	IConsole *m_pConsole;

	CNetBan m_NetBan;
	CNet m_aNets[NUM_SOCKETS];

	int64 m_BanRefreshTime;
	int64 m_PacketRefreshTime;

	struct CServerEntry
	{
		NETADDR m_Address;
		void *m_pSlaveUserData;

		int m_Version;
		int64 m_Expire;
	};

	struct CCheckServer
	{
		NETADDR m_Address;
		NETADDR m_AltAddress;
		void *m_pSlaveUserData;

		int m_TryCount;
		int64 m_TryTime;

		int m_Version;
	};

	CServerEntry m_aServers[MAX_SERVERS];
	int m_NumServers;
	CCheckServer m_aCheckServers[MAX_CHECKSERVERS];
	int m_NumCheckServers;

	struct CMastersrvSlave
	{
		struct CPacket
		{
			char m_aData[NET_MAX_PAYLOAD];
			int m_Size;
		} m_aPackets[MAX_PACKETS];
		int m_NumPackets;

		IMastersrvSlave *m_pSlave;
	};

	CMastersrvSlave m_aSlaves[NUM_MASTERSRV];
};

IMastersrv *CreateMastersrv()
{
	return new CMastersrv();
}

CMastersrv::CMastersrv()
{
	for(int s = 0; s < NUM_MASTERSRV; s++)
		m_aSlaves[s].m_pSlave = 0;
}

CMastersrv::~CMastersrv()
{
	for(int s = 0; s < NUM_MASTERSRV; s++)
		if(m_aSlaves[s].m_pSlave)
			delete m_aSlaves[s].m_pSlave;
}

int CMastersrv::Init()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	m_NetBan.Init(m_pConsole, Kernel()->RequestInterface<IStorage>());

	ReloadBans();
	m_BanRefreshTime = time_get() + time_freq() * BAN_REFRESH_TIME;

	int Services = 0;
	if(str_find(g_Config.m_MsServices, "5"))
		Services |= 1<<MASTERSRV_0_5;
	if(str_find(g_Config.m_MsServices, "6"))
		Services |= 1<<MASTERSRV_0_6;
	if(str_find(g_Config.m_MsServices, "7"))
		Services |= 1<<MASTERSRV_0_7;
	if(str_find(g_Config.m_MsServices, "v"))
		Services |= 1<<MASTERSRV_VER;
	if(str_find(g_Config.m_MsServices, "w"))
		Services |= 1<<MASTERSRV_VER_LEGACY;

	for(int i = 0; i < NUM_SOCKETS; i++)
	{
		m_aRecvRawUserData[i].m_pSelf = this;
		m_aRecvRawUserData[i].m_Socket = i;
	}

	NETADDR BindAddr;
	if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// got bindaddr
		BindAddr.type = NETTYPE_ALL;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
	}

	if(Services&((1<<MASTERSRV_0_5)|(1<<MASTERSRV_0_6)|(1<<MASTERSRV_0_7)))
	{
		BindAddr.port = MASTERSERVER_PORT;
		if(!m_aNets[SOCKET_OP].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (op)");
			return -1;
		}
		BindAddr.port = MASTERSERVER_CHECKER_PORT;
		if(!m_aNets[SOCKET_CHECKER].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (checker)");
			return -1;
		}

		m_aNets[SOCKET_OP].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_OP]);
		m_aNets[SOCKET_CHECKER].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_CHECKER]);
	}
	if(Services&((1<<MASTERSRV_VER)|(1<<MASTERSRV_VER_LEGACY)))
	{
		BindAddr.port = VERSIONSERVER_PORT;
		if(!m_aNets[SOCKET_VERSION].Open(&BindAddr, 0, 0, NETFLAG_ALLOWSTATELESS))
		{
			dbg_msg("mastersrv", "couldn't start network (version)");
			return -1;
		}

		m_aNets[SOCKET_VERSION].SetRecvRawCallback(RecvRaw, &m_aRecvRawUserData[SOCKET_VERSION]);
	}

	// process pending commands
	m_pConsole->StoreCommands(false);

	for(int s = 0; s < NUM_MASTERSRV; s++)
		m_aSlaves[s].m_NumPackets = 0;

	if(Services&(1<<MASTERSRV_0_5))
		m_aSlaves[MASTERSRV_0_5].m_pSlave = CreateSlave_0_5(this);
	if(Services&(1<<MASTERSRV_0_6))
		m_aSlaves[MASTERSRV_0_6].m_pSlave = CreateSlave_0_6(this);
	if(Services&(1<<MASTERSRV_0_7))
		m_aSlaves[MASTERSRV_0_7].m_pSlave = CreateSlave_0_7(this);
	if(Services&(1<<MASTERSRV_VER))
		m_aSlaves[MASTERSRV_VER].m_pSlave = CreateSlave_Ver(this);
	if(Services&(1<<MASTERSRV_VER_LEGACY))
		m_aSlaves[MASTERSRV_VER_LEGACY].m_pSlave = CreateSlave_VerLegacy(this);

	return 0;
}

int CMastersrv::Run()
{
	dbg_msg("mastersrv", "started");

	while(1)
	{
		for(int i = 0; i < NUM_SOCKETS; i++)
			m_aNets[i].Update();

		for(int i = 0; i < NUM_SOCKETS; i++)
		{
			// process packets
			CNetChunk Packet;
			TOKEN Token;
			while(m_aNets[i].Recv(&Packet, &Token))
			{
				// check if the server is banned
				if(m_NetBan.IsBanned(&Packet.m_Address, 0, 0))
					continue;

				for(int s = 0; s < NUM_MASTERSRV; s++)
					if(m_aSlaves[s].m_pSlave)
					{
						if(m_aSlaves[s].m_pSlave->ProcessMessage(i, &Packet, Token) != 0)
							break;
					}
			}
		}

		int64 Now = time_get();
		int64 Freq = time_freq();
		if(m_BanRefreshTime < Now)
		{
			m_BanRefreshTime = Now + Freq * BAN_REFRESH_TIME;
			ReloadBans();
		}

		if(m_PacketRefreshTime < Now)
		{
			m_PacketRefreshTime = Now + Freq * PACKET_REFRESH_TIME;

			PurgeServers();
			UpdateServers();
			BuildPackets();
		}

		// be nice to the CPU
		thread_sleep(1);
	}

	return 0;
}

void CMastersrv::UpdateServers()
{
	int64 Now = time_get();
	int64 Freq = time_freq();
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		CCheckServer *pCheck = &m_aCheckServers[i];

		if(pCheck->m_TryTime + Freq < Now)
		{
			IMastersrvSlave *pSlave = m_aSlaves[pCheck->m_Version].m_pSlave;
			dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

			if(pCheck->m_TryCount == 10)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&pCheck->m_Address, aAddrStr, sizeof(aAddrStr), true);
				char aAltAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(&pCheck->m_AltAddress, aAltAddrStr, sizeof(aAltAddrStr), true);
				dbg_msg("mastersrv", "check failed: %s (%s)", aAddrStr, aAltAddrStr);

				// FAIL!!
				pSlave->SendError(&pCheck->m_Address, pCheck->m_pSlaveUserData);
				*pCheck = m_aCheckServers[m_NumCheckServers-1];
				m_NumCheckServers--;
				i--;
			}
			else
			{
				pCheck->m_TryCount++;
				pCheck->m_TryTime = Now;

				if(pCheck->m_TryCount&1)
					pSlave->SendCheck(&pCheck->m_Address, pCheck->m_pSlaveUserData);
				else
					pSlave->SendCheck(&pCheck->m_AltAddress, pCheck->m_pSlaveUserData);
			}
		}
	}
}

void CMastersrv::BuildPackets()
{
	bool aPreparePacket[NUM_MASTERSRV];
	int BytesWritten;

	for(int s = 0; s < NUM_MASTERSRV; s++)
	{
		m_aSlaves[s].m_NumPackets = 0;
		aPreparePacket[s] = true;
		for(int i = 0; i < MAX_PACKETS; i++)
			m_aSlaves[s].m_aPackets[i].m_Size = 0;
	}

	for(int i = 0; i < m_NumServers; i++)
	{
		CServerEntry *pServer = &m_aServers[i];
		CMastersrvSlave *pSlaveData = &m_aSlaves[pServer->m_Version];
		IMastersrvSlave *pSlave = pSlaveData->m_pSlave;
		CMastersrvSlave::CPacket *pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets - 1];
		dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

		if(aPreparePacket[pServer->m_Version])
		{
			if(pSlaveData->m_NumPackets != 0)
			{
				BytesWritten = pSlave->BuildPacketFinalize(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size);
				dbg_assert(BytesWritten >= 0, "build packet finalisation failed");
				pPacket->m_Size += BytesWritten;
			}

			pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets];
			pSlaveData->m_NumPackets++;

			BytesWritten = pSlave->BuildPacketStart(&pPacket->m_aData[0], NET_MAX_PAYLOAD);

			dbg_assert(BytesWritten >= 0, "build packet initialisation failed");
			pPacket->m_Size += BytesWritten;

			aPreparePacket[pServer->m_Version] = false;
		}

		BytesWritten = pSlave->BuildPacketAdd(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size,
			&pServer->m_Address, pServer->m_pSlaveUserData);
		if(BytesWritten < 0)
		{
			aPreparePacket[pServer->m_Version] = true;
			i--;
			continue;
		}
		pPacket->m_Size += BytesWritten;
	}

	for(int s = 0; s < NUM_MASTERSRV; s++)
	{
		CMastersrvSlave *pSlaveData = &m_aSlaves[s];
		IMastersrvSlave *pSlave = pSlaveData->m_pSlave;

		if(m_aSlaves[s].m_NumPackets > 0)
		{
			dbg_assert(pSlave != 0, "attempting to finalise packet for non-existant slave");

			CMastersrvSlave::CPacket *pPacket = &pSlaveData->m_aPackets[pSlaveData->m_NumPackets - 1];
			BytesWritten = pSlave->BuildPacketFinalize(&pPacket->m_aData[pPacket->m_Size], NET_MAX_PAYLOAD - pPacket->m_Size);
			dbg_assert(BytesWritten >= 0, "final build packet finalisation failed");
			pPacket->m_Size += BytesWritten;
		}
	}
}

int CMastersrv::Send(int Socket, const CNetChunk *pPacket, TOKEN PacketToken)
{
	dbg_assert(Socket >= 0 && Socket < NUM_SOCKETS, "attempting to send via non-existant socket");

	m_aNets[Socket].Send((CNetChunk *)pPacket, PacketToken);

	return 0;
}

int CMastersrv::SendRaw(int Socket, const NETADDR *pAddr, const void *pData, int DataSize)
{
	dbg_assert(Socket >= 0 && Socket < NUM_SOCKETS, "attempting to send via non-existant socket");

	m_aNets[Socket].SendRaw(pAddr, pData, DataSize);

	return 0;
}

int CMastersrv::RecvRawImpl(int Socket, const NETADDR *pAddr, const void *pData, int DataSize)
{
	// check if the server is banned
	if(m_NetBan.IsBanned(pAddr, 0, 0))
		return 0;

	for(int s = 0; s < NUM_MASTERSRV; s++)
		if(m_aSlaves[s].m_pSlave)
		{
			if(m_aSlaves[s].m_pSlave->ProcessMessageRaw(Socket, pAddr, pData, DataSize) != 0)
				return 1;
		}
	return 0;
}

void CMastersrv::AddServer(const NETADDR *pAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");

	bool Found = false;
	for(int i = 0; i < m_NumCheckServers && !Found; i++)
	{
		if((net_addr_comp(&m_aCheckServers[i].m_Address, pAddr) == 0
			|| net_addr_comp(&m_aCheckServers[i].m_AltAddress, pAddr) == 0)
			&& m_aCheckServers[i].m_Version == Version)

		{
			m_NumCheckServers--;
			m_aCheckServers[i] = m_aCheckServers[m_NumCheckServers];
			Found = true;
		}
	}

	if(!Found) // only allow this for servers which are actually being checked
		return;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pAddr, aAddrStr, sizeof(aAddrStr), true);

	// see if server already exists in list
	for(int i = 0; i < m_NumServers; i++)
	{
		if(net_addr_comp(&m_aServers[i].m_Address, pAddr) == 0 && m_aServers[i].m_Version == Version)
		{
			dbg_msg("mastersrv", "updated: %s", aAddrStr);
			m_aServers[i].m_pSlaveUserData = pUserData;
			m_aServers[i].m_Expire = time_get() + time_freq() * EXPIRE_TIME;
			IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
			dbg_assert(pSlave != 0, "attempting to access uninitalised slave");
			pSlave->SendOk(pAddr, pUserData);
			return;
		}
	}

	// add server
	if(m_NumServers == MAX_SERVERS)
	{
		dbg_msg("mastersrv", "error: mastersrv is full: %s", aAddrStr);
		return;
	}

	dbg_msg("mastersrv", "added: %s", aAddrStr);
	m_aServers[m_NumServers].m_Address = *pAddr;
	m_aServers[m_NumServers].m_pSlaveUserData = pUserData;
	m_aServers[m_NumServers].m_Expire = time_get() + time_freq() * EXPIRE_TIME;
	m_aServers[m_NumServers].m_Version = Version;
	m_NumServers++;

	IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
	dbg_assert(pSlave != 0, "attempting to access uninitalised slave");
	pSlave->SendOk(pAddr, pUserData);
}

void CMastersrv::AddCheckserver(const NETADDR *pAddr, const NETADDR *pAltAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");

	char aAddrStr[2 * NETADDR_MAXSTRSIZE + 3]; // 3 == sizeof(' ()')
	char aTmp1[NETADDR_MAXSTRSIZE], aTmp2[NETADDR_MAXSTRSIZE];

	net_addr_str(pAddr, aTmp1, sizeof(aTmp1), true);
	net_addr_str(pAltAddr, aTmp2, sizeof(aTmp2), true);

	str_format(aAddrStr, sizeof(aAddrStr), "%s (%s)", aTmp1, aTmp2);

	// see if server already exists in list
	for(int i = 0; i < m_NumCheckServers; i++)
	{
		if(net_addr_comp(&m_aCheckServers[i].m_Address, pAddr) == 0
			&& m_aCheckServers[i].m_Version == Version)
		{
			dbg_msg("mastersrv/check", "warning: updated: %s", aAddrStr);
			m_aCheckServers[i].m_AltAddress = *pAltAddr;
			m_aCheckServers[i].m_Version = Version;
			m_aCheckServers[i].m_pSlaveUserData = pUserData;
			m_aCheckServers[i].m_TryCount = 0;
			m_aCheckServers[i].m_TryTime = 0;
			return;
		}
	}

	// add server
	if(m_NumCheckServers == MAX_CHECKSERVERS)
	{
		dbg_msg("mastersrv/check", "error: mastersrv is full: %s", aAddrStr);
		return;
	}

	dbg_msg("mastersrv/check", "added: %s", aAddrStr);
	m_aCheckServers[m_NumCheckServers].m_Address = *pAddr;
	m_aCheckServers[m_NumCheckServers].m_AltAddress = *pAltAddr;
	m_aCheckServers[m_NumCheckServers].m_Version = Version;
	m_aCheckServers[m_NumCheckServers].m_pSlaveUserData = pUserData;
	m_aCheckServers[m_NumCheckServers].m_TryCount = 0;
	m_aCheckServers[m_NumCheckServers].m_TryTime = 0;
	m_NumCheckServers++;
}

void CMastersrv::SendList(const NETADDR *pAddr, void *pUserData, int Version)
{
	dbg_assert(Version >= 0 && Version < NUM_MASTERSRV, "version out of range");
	IMastersrvSlave *pSlave = m_aSlaves[Version].m_pSlave;
	dbg_assert(pSlave != 0, "attempting to access uninitalised slave");

	dbg_msg("mastersrv", "requested %s, responding with %d packets", SlaveString(Version),m_aSlaves[Version].m_NumPackets);

	for(int i = 0; i < m_aSlaves[Version].m_NumPackets; i++)
	{
		CMastersrvSlave::CPacket *pPacket = &m_aSlaves[Version].m_aPackets[i];
		pSlave->SendList(pAddr, pPacket->m_aData, pPacket->m_Size, pUserData);
	}
}

int CMastersrv::GetCount() const
{
	dbg_msg("mastersrv", "requesting count, responding with %d", m_NumServers);
	return m_NumServers;
}

void CMastersrv::PurgeServers()
{
	int64 Now = time_get();
	int i = 0;
	while(i < m_NumServers)
	{
		if(m_aServers[i].m_Expire > 0 && m_aServers[i].m_Expire < Now)
		{
			// remove server
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&m_aServers[i].m_Address, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("mastersrv", "expired: %s", aAddrStr);
			m_aServers[i] = m_aServers[m_NumServers-1];
			m_NumServers--;
		}
		else
			i++;
	}
}

void CMastersrv::ReloadBans()
{
	m_NetBan.UnbanAll();
	m_pConsole->ExecuteFile("master.cfg");
}

const char *CMastersrv::SlaveString(int Version)
{
	switch(Version)
	{
	case MASTERSRV_0_5: return "mastersrv 0.5";
	case MASTERSRV_0_6: return "mastersrv 0.6";
	case MASTERSRV_0_7: return "mastersrv 0.7";
	case MASTERSRV_VER: return "versionsrv";
	case MASTERSRV_VER_LEGACY: return "versionsrv_legacy";
	}
	return "(invalid)";
}

