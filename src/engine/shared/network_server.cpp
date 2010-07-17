// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <math.h>
#include <stdlib.h>

#include <base/system.h>
#include "network.h"

int CNetServer::s_BanList_entries = 0;
unsigned int CNetServer::s_aBanList_expires[NET_SERVER_MAXBANS];
unsigned char CNetServer::s_aBanList_addr[NET_SERVER_MAXBANS][4];

int CNetServer::char_comp(const char *char_1, const char *char_2)
{
	unsigned char i = 0, length;
	
	length = str_length(char_1);
	while(i < length)
	{
		if(char_1[i] != char_2[i])
			return 0;
		i++;
	}
	if(char_1[i] || char_2[i])
		return 0;
	
	return 1;
}

int CNetServer::CharhexToInt(char *hex_string)
{
	int hex = 0, i = 0, number;
	while(hex_string[i])
	{
		number = pow(16.0, (double)(str_length(hex_string)-i))/15-pow(16.0, (double)(str_length(hex_string)-i-1))/15;
		switch(hex_string[i])
		{
			case '0': break;
			case '1': hex += number; break;
			case '2': hex += 2 * number; break;
			case '3': hex += 3 * number; break;
			case '4': hex += 4 * number; break;
			case '5': hex += 5 * number; break;
			case '6': hex += 6 * number; break;
			case '7': hex += 7 * number; break;
			case '8': hex += 8 * number; break;
			case '9': hex += 9 * number; break;
			case 'a': hex += 10 * number; break;
			case 'b': hex += 11 * number; break;
			case 'c': hex += 12 * number; break;
			case 'd': hex += 13 * number; break;
			case 'e': hex += 14 * number; break;
			case 'f': hex += 15 * number; break;
		}
		i++;
	}
	return hex;
}
void CNetServer::IntToBinint(unsigned char *destination, unsigned int number)
{
	char add_zero, a_Hex[4], a_Hex_string[9], i, j, pointerHex;
	unsigned char a_Buffer[4];
	sprintf(a_Hex, "%x", number);
	sprintf(a_Hex_string, "%s", a_Hex);
	add_zero = 0;
	i = 8-str_length(a_Hex_string);
	j = 0;
	if(i)
	{
		char modulo;
		
		if((modulo = i%2))
		{
			i--;
			add_zero = 1;
		}
		while(i > 0)
		{
			a_Buffer[j] = 0;
			i -= 2;
			j++;
		}
	}
	pointerHex = 0;
	if(add_zero)
	{
		sprintf(a_Hex, "%c", a_Hex_string[pointerHex++]);
		a_Buffer[j] = CharhexToInt(a_Hex);
		j++;
	}
	while(j < sizeof(a_Buffer))
	{
		sprintf(a_Hex, "%c%c", a_Hex_string[pointerHex], a_Hex_string[pointerHex+1]);
		a_Buffer[j] = CharhexToInt(a_Hex);
		j++;
		pointerHex+=2;
	}
	i=0;
	while(i < sizeof(a_Buffer))
	{
		destination[i] = a_Buffer[i];
		i++;
	}
}
unsigned int CNetServer::BinintToInt(unsigned char *charbin)
{
	char i;
	unsigned int hex = 0, number;
	i = 0;
	while(i < sizeof(charbin))
	{
		number = pow(2.0, (double)((sizeof(charbin)-i)*8))/255-pow(2.0, (double)((sizeof(charbin)-i-1)*8))/255;
		hex += charbin[i] * number;
		i++;
	}
	return hex;
}

bool CNetServer::Open(NETADDR BindAddr, int MaxClients, int MaxClientsPerIP, int Flags)
{
	// zero out the whole structure
	mem_zero(this, sizeof(*this));
	
	// open socket
	m_Socket = net_udp_create(BindAddr);
	if(m_Socket == NETSOCKET_INVALID)
		return false;
	
	// clamp clients
	m_MaxClients = MaxClients;
	if(m_MaxClients > NET_MAX_CLIENTS)
		m_MaxClients = NET_MAX_CLIENTS;
	if(m_MaxClients < 1)
		m_MaxClients = 1;

	m_MaxClientsPerIP = MaxClientsPerIP;
	
	for(int i = 0; i < NET_MAX_CLIENTS; i++)
		m_aSlots[i].m_Connection.Init(m_Socket);

	return true;
}

int CNetServer::SetCallbacks(NETFUNC_NEWCLIENT pfnNewClient, NETFUNC_DELCLIENT pfnDelClient, void *pUser)
{
	m_pfnNewClient = pfnNewClient;
	m_pfnDelClient = pfnDelClient;
	m_UserPtr = pUser;
	return 0;
}

int CNetServer::Close()
{
	// TODO: implement me
	return 0;
}

int CNetServer::Drop(int ClientID, const char *pReason)
{
	// TODO: insert lots of checks here
	NETADDR Addr = ClientAddr(ClientID);

	dbg_msg("net_server", "client dropped. cid=%d ip=%d.%d.%d.%d reason=\"%s\"",
		ClientID,
		Addr.ip[0], Addr.ip[1], Addr.ip[2], Addr.ip[3],
		pReason
		);
		
	m_aSlots[ClientID].m_Connection.Disconnect(pReason);

	if(m_pfnDelClient)
		m_pfnDelClient(ClientID, m_UserPtr);
		
	return 0;
}

void CNetServer::lock(char *file, char *type)
{
	// TODO: Locking the bans file to avoid problems with more than 1 server
}

int CNetServer::mutex(const char *type, const char *id)
{
	static char entries = 0, mutex_string[255][255];
	unsigned char i = 0;
	
	while(i < entries)
	{
		if(char_comp(id, mutex_string[i]))
			if(type == "create")
			{
				thread_sleep(33);
				mutex(type, id);
				return 0;
			}
			else if(type == "release")
			{
				unsigned char j = 0;
				
				while(j < entries-1)
				{
					unsigned char k = 0, length;
					
					length = str_length(mutex_string[j+1]);
					while(k < length)
					{
						mutex_string[j][k] = mutex_string[j+1][k];
						k++;
					}
					mutex_string[j][k] = 0;
					j++;
				}
				entries--;
				return 1;
			}
		i++;
	}
	
	if(type == "create")
	{
		unsigned char j = 0, length;
		
		length = str_length(type);
		while(j < length)
		{
			mutex_string[i][j] = type[j];
			j++;
		}
		mutex_string[i][j] = 0;
		entries++;
		return 1;
	}
	
	return -1;
}

int CNetServer::BanAdd(NETADDR Addr, int Seconds)
{
	char banFound;
	int i;
	unsigned int Stamp = 0;
	
	// remove the port
	Addr.port = 0;
	
	if(Seconds > 0)
		Stamp = time_timestamp() + Seconds;
	
	lock("bans.bin", "LOCK_WRITE");
	mutex("create", "CNetServer::s_BanList_entries");
	readBanFile(1);
	
	// search to see if it already exists
	banFound = BanSearch(Addr);
	
	if(banFound >= 0)
		CNetServer::s_aBanList_expires[banFound] = Stamp;
	else
	{
		i = banFound*-1-1;
		CNetServer::s_aBanList_expires[i] = Stamp;
		CNetServer::s_aBanList_addr[i][0] = Addr.ip[0];
		CNetServer::s_aBanList_addr[i][1] = Addr.ip[1];
		CNetServer::s_aBanList_addr[i][2] = Addr.ip[2];
		CNetServer::s_aBanList_addr[i][3] = Addr.ip[3];
		CNetServer::s_BanList_entries++;
	}
	
	// Store the information in a file
	writeBanFile();
	mutex("release", "CNetServer::s_BanList_entries");
	lock("bans.bin", "UNLOCK");

	// drop banned clients
	{
		char Buf[128];
		NETADDR BanAddr;
		
		int Mins = Seconds/60;
		if(Mins == 1)
			str_format(Buf, sizeof(Buf), "you have been banned for %d minute", Mins);
		else if(Mins > 1)
			str_format(Buf, sizeof(Buf), "you have been banned for %d minutes", Mins);
		else
			str_format(Buf, sizeof(Buf), "you have been banned for life");
		
		for(int i = 0; i < MaxClients(); i++)
		{
			BanAddr = m_aSlots[i].m_Connection.PeerAddress();
			BanAddr.port = 0;
			
			if(net_addr_comp(&Addr, &BanAddr) == 0)
				Drop(i, Buf);
		}
	}
	return 0;
}

void CNetServer::BanRemoveByAddr(NETADDR Addr)
{
	BanRemoveById(BanSearch(Addr));
}

void CNetServer::BanRemoveById(int BanIndex)
{
	lock("bans.bin", "LOCK_WRITE");
	mutex("create", "CNetServer::s_BanList_entries");
	readBanFile(1);
	while(BanIndex < CNetServer::s_BanList_entries-1)
	{
		CNetServer::s_aBanList_expires[BanIndex] = CNetServer::s_aBanList_expires[BanIndex + 1];
		CNetServer::s_aBanList_addr[BanIndex][0] = CNetServer::s_aBanList_addr[BanIndex + 1][0];
		CNetServer::s_aBanList_addr[BanIndex][1] = CNetServer::s_aBanList_addr[BanIndex + 1][1];
		CNetServer::s_aBanList_addr[BanIndex][2] = CNetServer::s_aBanList_addr[BanIndex + 1][2];
		CNetServer::s_aBanList_addr[BanIndex][3] = CNetServer::s_aBanList_addr[BanIndex + 1][3];
		BanIndex++;
	}
	
	CNetServer::s_BanList_entries--;
	writeBanFile();
	mutex("release", "CNetServer::s_BanList_entries");
	lock("bans.bin", "UNLOCK");
}

int CNetServer::BanSearch(NETADDR searchAddr)
{
	NETADDR net_addr;
	int i = 0;
	
	while(i < CNetServer::s_BanList_entries)
	{
		net_addr.type = NETTYPE_IPV4;
		net_addr.ip[0] = CNetServer::s_aBanList_addr[i][0];
		net_addr.ip[1] = CNetServer::s_aBanList_addr[i][1];
		net_addr.ip[2] = CNetServer::s_aBanList_addr[i][2];
		net_addr.ip[3] = CNetServer::s_aBanList_addr[i][3];
		net_addr.port = 0;
		if((CNetServer::s_aBanList_expires[i] == 0 || CNetServer::s_aBanList_expires[i] > time_timestamp()) && real_net_addr_comp(&searchAddr, &net_addr) == 1)
			return i;
		i++;
	}
	
	return (i+1)*-1;
}

int CNetServer::Update()
{
	int Now = time_timestamp();
	for(int i = 0; i < MaxClients(); i++)
	{
		m_aSlots[i].m_Connection.Update();
		if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_ERROR)
			Drop(i, m_aSlots[i].m_Connection.ErrorString());
	}
	
	mutex("create", "CNetServer::s_BanList_entries");
	readBanFile();
	mutex("release", "CNetServer::s_BanList_entries");
	
	return 0;
}

void CNetServer::readBanFile(char forceRead)
{
	static unsigned int s_last_update = time_timestamp();
	
	if(time_timestamp() > s_last_update || forceRead)
	{
		FILE *pFile;
		
		pFile = fopen("bans.bin", "rb");
		if(pFile != NULL)
		{
			char *pBuffer;
			unsigned char aCharbin[4];
			int i = 0, j = 0, size;
			
			// load the file
			fseek(pFile, 0, SEEK_END);
			size=ftell(pFile);
			fseek(pFile, 0, SEEK_SET);
			pBuffer=(char *)malloc(size);
			fread(pBuffer, 1, size, pFile);
			fclose(pFile);
			
			// search a ban
			while(i < size)
			{
				unsigned int expires;
				
				aCharbin[0] = pBuffer[i];
				aCharbin[1] = pBuffer[i+1];
				aCharbin[2] = pBuffer[i+2];
				aCharbin[3] = pBuffer[i+3];
				expires = CNetServer::BinintToInt(aCharbin);
				if(expires == 0 || expires > time_timestamp())
				{
					CNetServer::s_aBanList_expires[j] = expires;
					CNetServer::s_aBanList_addr[j][0] = pBuffer[i+4];
					CNetServer::s_aBanList_addr[j][1] = pBuffer[i+5];
					CNetServer::s_aBanList_addr[j][2] = pBuffer[i+6];
					CNetServer::s_aBanList_addr[j][3] = pBuffer[i+7];
					j++;
				}
				i += 8;
			}
			free(pBuffer);
			CNetServer::s_BanList_entries = j;
			if(j > 0)
			{
				lock("bans.bin", "LOCK_WRITE");
				writeBanFile();
				lock("bans.bin", "UNLOCK");
			}
		}
		s_last_update = time_timestamp();
	}
};

void CNetServer::writeBanFile()
{
	FILE *pFile;
	char a_Buffer[8*NET_SERVER_MAXBANS];
	unsigned char a_Expires[4];
	int i = 0;
	
	while(i < CNetServer::s_BanList_entries)
	{
		CNetServer::IntToBinint(a_Expires, CNetServer::s_aBanList_expires[i]);
		a_Buffer[i*8] = a_Expires[0];
		a_Buffer[i*8+1] = a_Expires[1];
		a_Buffer[i*8+2] = a_Expires[2];
		a_Buffer[i*8+3] = a_Expires[3];
		a_Buffer[i*8+4] = CNetServer::s_aBanList_addr[i][0];
		a_Buffer[i*8+5] = CNetServer::s_aBanList_addr[i][1];
		a_Buffer[i*8+6] = CNetServer::s_aBanList_addr[i][2];
		a_Buffer[i*8+7] = CNetServer::s_aBanList_addr[i][3];
		i++;
	}
	pFile = fopen("bans.bin", "wb");
	if(CNetServer::s_BanList_entries)
		fwrite(a_Buffer, 1, CNetServer::s_BanList_entries*8, pFile);
	else
		fwrite(0, 0, 0, pFile);
	fclose(pFile);
}

/*
	TODO: chopp up this function into smaller working parts
*/
int CNetServer::Recv(CNetChunk *pChunk)
{
	unsigned Now = time_timestamp();
	
	for(;;)
	{
		NETADDR Addr;
			
		// check for a chunk
		if(m_RecvUnpacker.FetchChunk(pChunk))
			return 1;
		
		// TODO: empty the recvinfo
		int Bytes = net_udp_recv(m_Socket, &Addr, m_RecvUnpacker.m_aBuffer, NET_MAX_PACKETSIZE);

		// no more packets for now
		if(Bytes <= 0)
			break;
		
		if(CNetBase::UnpackPacket(m_RecvUnpacker.m_aBuffer, Bytes, &m_RecvUnpacker.m_Data) == 0)
		{
			int banFound, Found = 0;
			
			// search for a ban
			banFound = BanSearch(Addr);
			
			// check if we just should drop the packet
			if(banFound >= 0)
			{
				// banned, reply with a message
				char BanStr[128];
				if(CNetServer::s_aBanList_expires[banFound] > 0)
				{
					if(CNetServer::s_aBanList_expires[banFound] > Now)
					{
						int Mins = ((CNetServer::s_aBanList_expires[banFound] - Now)+59)/60;
						if(Mins == 1)
							str_format(BanStr, sizeof(BanStr), "banned for %d minute", Mins);
						else
							str_format(BanStr, sizeof(BanStr), "banned for %d minutes", Mins);
					}
				}
				else
					str_format(BanStr, sizeof(BanStr), "banned for life");
				CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, BanStr, str_length(BanStr)+1);
				continue;
			}
			
			if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONNLESS)
			{
				pChunk->m_Flags = NETSENDFLAG_CONNLESS;
				pChunk->m_ClientID = -1;
				pChunk->m_Address = Addr;
				pChunk->m_DataSize = m_RecvUnpacker.m_Data.m_DataSize;
				pChunk->m_pData = m_RecvUnpacker.m_Data.m_aChunkData;
				return 1;
			}
			else
			{			
				// TODO: check size here
				if(m_RecvUnpacker.m_Data.m_Flags&NET_PACKETFLAG_CONTROL && m_RecvUnpacker.m_Data.m_aChunkData[0] == NET_CTRLMSG_CONNECT)
				{
					Found = 0;
				
					// check if we already got this client
					for(int i = 0; i < MaxClients(); i++)
					{
						NETADDR PeerAddr = m_aSlots[i].m_Connection.PeerAddress();
						if(m_aSlots[i].m_Connection.State() != NET_CONNSTATE_OFFLINE &&
							net_addr_comp(&PeerAddr, &Addr) == 0)
						{
							Found = 1; // silent ignore.. we got this client already
							break;
						}
					}
					
					// client that wants to connect
					if(!Found)
					{
						// only allow a specific number of players with the same ip
						NETADDR ThisAddr = Addr, OtherAddr;
						int FoundAddr = 1;
						ThisAddr.port = 0;
						for(int i = 0; i < MaxClients(); ++i)
						{
							if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
								continue;

							OtherAddr = m_aSlots[i].m_Connection.PeerAddress();
							OtherAddr.port = 0;
							if(!net_addr_comp(&ThisAddr, &OtherAddr))
							{
								if(FoundAddr++ >= m_MaxClientsPerIP)
								{
									char aBuf[128];
									str_format(aBuf, sizeof(aBuf), "only %i players with same ip allowed", m_MaxClientsPerIP);
									CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, aBuf, sizeof(aBuf));
									return 0;
								}
							}
						}

						for(int i = 0; i < MaxClients(); i++)
						{
							if(m_aSlots[i].m_Connection.State() == NET_CONNSTATE_OFFLINE)
							{
								Found = 1;
								m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr);
								if(m_pfnNewClient)
									m_pfnNewClient(i, m_UserPtr);
								break;
							}
						}
						
						if(!Found)
						{
							const char FullMsg[] = "server is full";
							CNetBase::SendControlMsg(m_Socket, &Addr, 0, NET_CTRLMSG_CLOSE, FullMsg, sizeof(FullMsg));
						}
					}
				}
				else
				{
					// normal packet, find matching slot
					for(int i = 0; i < MaxClients(); i++)
					{
						NETADDR PeerAddr = m_aSlots[i].m_Connection.PeerAddress();
						if(net_addr_comp(&PeerAddr, &Addr) == 0)
						{
							if(m_aSlots[i].m_Connection.Feed(&m_RecvUnpacker.m_Data, &Addr))
							{
								if(m_RecvUnpacker.m_Data.m_DataSize)
									m_RecvUnpacker.Start(&Addr, &m_aSlots[i].m_Connection, i);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

int CNetServer::Send(CNetChunk *pChunk)
{
	if(pChunk->m_DataSize >= NET_MAX_PAYLOAD)
	{
		dbg_msg("netserver", "packet payload too big. %d. dropping packet", pChunk->m_DataSize);
		return -1;
	}
	
	if(pChunk->m_Flags&NETSENDFLAG_CONNLESS)
	{
		// send connectionless packet
		CNetBase::SendPacketConnless(m_Socket, &pChunk->m_Address, pChunk->m_pData, pChunk->m_DataSize);
	}
	else
	{
		int Flags = 0;
		dbg_assert(pChunk->m_ClientID >= 0, "errornous client id");
		dbg_assert(pChunk->m_ClientID < MaxClients(), "errornous client id");
		
		if(pChunk->m_Flags&NETSENDFLAG_VITAL)
			Flags = NET_CHUNKFLAG_VITAL;
		
		if(m_aSlots[pChunk->m_ClientID].m_Connection.QueueChunk(Flags, pChunk->m_DataSize, pChunk->m_pData) == 0)
		{
			if(pChunk->m_Flags&NETSENDFLAG_FLUSH)
				m_aSlots[pChunk->m_ClientID].m_Connection.Flush();
		}
		else
		{
			Drop(pChunk->m_ClientID, "error sending data");
		}
	}
	return 0;
}

void CNetServer::SetMaxClientsPerIP(int Max)
{
	// clamp
	if(Max < 1)
		Max = 1;
	else if(Max > NET_MAX_CLIENTS)
		Max = NET_MAX_CLIENTS;

	m_MaxClientsPerIP = Max;
}
