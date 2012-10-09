/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm> // sort  TODO: remove this

#include <base/math.h>
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/memheap.h>
#include <engine/shared/network.h>
#include <engine/shared/protocol.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/masterserver.h>

#include <mastersrv/mastersrv.h>

#include "serverbrowser.h"

class SortWrap
{
	typedef bool (CServerBrowser::CServerFilter::*SortFunc)(int, int) const;
	SortFunc m_pfnSort;
	CServerBrowser::CServerFilter *m_pThis;
public:
	SortWrap(CServerBrowser::CServerFilter *t, SortFunc f) : m_pfnSort(f), m_pThis(t) {}
	bool operator()(int a, int b) { return (g_Config.m_BrSortOrder ? (m_pThis->*m_pfnSort)(b, a) : (m_pThis->*m_pfnSort)(a, b)); }
};

CServerBrowser::CServerBrowser()
{
	m_pMasterServer = 0;

	// favorites
	m_NumFavoriteServers = 0;
	m_FavLookup.m_LookupCount = 0;
	m_FavLookup.m_Active = false;

	//
	mem_zero(m_aServerlistIp, sizeof(m_aServerlistIp));

	m_pFirstReqServer = 0; // request list
	m_pLastReqServer = 0;
	m_NumRequests = 0;

	m_NeedRefresh = 0;

	m_NumServers = 0;
	m_NumServerCapacity = 0;

	m_NumPlayers = 0;

	// the token is to keep server refresh separated from each other
	m_CurrentToken = 1;

	m_ServerlistType = 0;
	m_BroadcastTime = 0;
}

int CServerBrowser::AddFilter(int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress)
{
	CServerFilter Filter;
	Filter.m_SortHash = SortHash;
	Filter.m_Ping = Ping;
	Filter.m_Country = Country;
	str_copy(Filter.m_aGametype, pGametype, sizeof(Filter.m_aGametype));
	str_copy(Filter.m_aServerAddress, pServerAddress, sizeof(Filter.m_aServerAddress));
	Filter.m_pSortedServerlist = 0;
	Filter.m_NumSortedServers = 0;
	Filter.m_NumSortedServersCapacity = 0;
	Filter.m_NumPlayers = 0;
	Filter.m_pServerBrowser = this;
	m_lFilters.add(Filter);

	return m_lFilters.size()-1;
}

void CServerBrowser::GetFilter(int Index, int *pSortHash, int *pPing, int *pCountry, char* pGametype, char* pServerAddress)
{
	CServerFilter *pFilter = &m_lFilters[Index];
	*pSortHash = pFilter->m_SortHash;
	*pPing = pFilter->m_Ping;
	*pCountry = pFilter->m_Country;
	str_copy(pGametype, pFilter->m_aGametype, sizeof(pFilter->m_aGametype));
	str_copy(pServerAddress, pFilter->m_aServerAddress, sizeof(pFilter->m_aServerAddress));
}

void CServerBrowser::SetFilter(int Index, int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress)
{
	CServerFilter *pFilter = &m_lFilters[Index];
	pFilter->m_SortHash = SortHash;
	pFilter->m_Ping = Ping;
	pFilter->m_Country = Country;
	str_copy(pFilter->m_aGametype, pGametype, sizeof(pFilter->m_aGametype));
	str_copy(pFilter->m_aServerAddress, pServerAddress, sizeof(pFilter->m_aServerAddress));

	pFilter->m_pServerBrowser->Update(true);
}

void CServerBrowser::RemoveFilter(int Index)
{
	m_lFilters.remove_index(Index);
}

void CServerBrowser::SetBaseInfo(class CNetClient *pClient, const char *pNetVersion)
{
	m_pNetClient = pClient;
	str_copy(m_aNetVersion, pNetVersion, sizeof(m_aNetVersion));
	m_pMasterServer = Kernel()->RequestInterface<IMasterServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pEngine = Kernel()->RequestInterface<IEngine>();
	m_pFriends = Kernel()->RequestInterface<IFriends>();
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	m_pConsole->Register("add_favorite", "s", CFGFLAG_CLIENT, ConAddFavorite, this, "Add a server as a favorite");
	m_pConsole->Register("remove_favorite", "s", CFGFLAG_CLIENT, ConRemoveFavorite, this, "Remove a server from favorites");
}

const CServerInfo *CServerBrowser::SortedGet(int FilterIndex, int Index) const
{
	if(Index < 0 || Index >= m_lFilters[FilterIndex].m_NumSortedServers)
		return 0;
	return &m_ppServerlist[m_lFilters[FilterIndex].m_pSortedServerlist[Index]]->m_Info;
}

const void *CServerBrowser::GetID(int FilterIndex, int Index) const
{
	return &m_lFilters[FilterIndex].m_pSortedServerlist[Index];
}

CServerBrowser::CServerFilter::~CServerFilter()
{
	mem_free(m_pSortedServerlist);
}

bool CServerBrowser::CServerFilter::SortCompareName(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	//	make sure empty entries are listed last
	return (a->m_GotInfo && b->m_GotInfo) || (!a->m_GotInfo && !b->m_GotInfo) ? str_comp_nocase(a->m_Info.m_aName, b->m_Info.m_aName) < 0 :
			a->m_GotInfo ? true : false;
}

bool CServerBrowser::CServerFilter::SortCompareMap(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	int Result = str_comp_nocase(a->m_Info.m_aMap, b->m_Info.m_aMap);
	return Result < 0 || (Result == 0 && (a->m_Info.m_Flags&FLAG_PURE) && !(b->m_Info.m_Flags&FLAG_PURE));
}

bool CServerBrowser::CServerFilter::SortComparePing(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	return a->m_Info.m_Latency < b->m_Info.m_Latency ||
		(a->m_Info.m_Latency == b->m_Info.m_Latency && (a->m_Info.m_Flags&FLAG_PURE) && !(b->m_Info.m_Flags&FLAG_PURE));
}

bool CServerBrowser::CServerFilter::SortCompareGametype(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	return str_comp_nocase(a->m_Info.m_aGameType, b->m_Info.m_aGameType) < 0;
}

bool CServerBrowser::CServerFilter::SortCompareNumPlayers(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	return a->m_Info.m_NumPlayers < b->m_Info.m_NumPlayers ||
		(a->m_Info.m_NumPlayers == b->m_Info.m_NumPlayers && !(a->m_Info.m_Flags&FLAG_PURE) && (b->m_Info.m_Flags&FLAG_PURE));
}

bool CServerBrowser::CServerFilter::SortCompareNumClients(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowser->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowser->m_ppServerlist[Index2];
	return a->m_Info.m_NumClients < b->m_Info.m_NumClients ||
		(a->m_Info.m_NumClients == b->m_Info.m_NumClients && !(a->m_Info.m_Flags&FLAG_PURE) && (b->m_Info.m_Flags&FLAG_PURE));
}

void CServerBrowser::CServerFilter::Filter()
{
	int NumServers = m_pServerBrowser->m_NumServers;
	m_NumSortedServers = 0;
	m_NumPlayers = 0;

	// allocate the sorted list
	if(m_NumSortedServersCapacity < NumServers)
	{
		if(m_pSortedServerlist)
			mem_free(m_pSortedServerlist);
		m_NumSortedServersCapacity = max(1000, NumServers+NumServers/2);
		m_pSortedServerlist = (int *)mem_alloc(m_NumSortedServersCapacity*sizeof(int), 1);
	}

	// filter the servers
	for(int i = 0; i < NumServers; i++)
	{
		int Filtered = 0;

		if(m_SortHash&FILTER_EMPTY && ((m_SortHash&FILTER_SPECTATORS && m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumPlayers == 0) || m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumClients == 0))
			Filtered = 1;
		else if(m_SortHash&FILTER_FULL && ((m_SortHash&FILTER_SPECTATORS && m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumPlayers == m_pServerBrowser->m_ppServerlist[i]->m_Info.m_MaxPlayers) ||
				m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumClients == m_pServerBrowser->m_ppServerlist[i]->m_Info.m_MaxClients))
			Filtered = 1;
		else if(m_SortHash&FILTER_PW && m_pServerBrowser->m_ppServerlist[i]->m_Info.m_Flags&FLAG_PASSWORD)
			Filtered = 1;
		else if(m_SortHash&FILTER_FAVORITE && !m_pServerBrowser->m_ppServerlist[i]->m_Info.m_Favorite)
			Filtered = 1;
		else if(m_SortHash&FILTER_PURE && !(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_Flags&FLAG_PURE))
			Filtered = 1;
		else if(m_SortHash&FILTER_PURE_MAP &&  !(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_Flags&FLAG_PUREMAP))
			Filtered = 1;
		else if(m_SortHash&FILTER_PING && m_Ping < m_pServerBrowser->m_ppServerlist[i]->m_Info.m_Latency)
			Filtered = 1;
		else if(m_SortHash&FILTER_COMPAT_VERSION && str_comp_num(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aVersion, m_pServerBrowser->m_aNetVersion, 3) != 0)
			Filtered = 1;
		else if(m_aServerAddress[0] && !str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aAddress, m_aServerAddress))
			Filtered = 1;
		else if(m_SortHash&FILTER_GAMETYPE_STRICT && m_aGametype[0] && str_comp_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aGameType, m_aGametype))
			Filtered = 1;
		else if(!(m_SortHash&FILTER_GAMETYPE_STRICT) && m_aGametype[0] && !str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aGameType, m_aGametype))
			Filtered = 1;
		else
		{
			if(m_SortHash&FILTER_COUNTRY)
			{
				Filtered = 1;
				// match against player country
				for(int p = 0; p < m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_Country == m_Country)
					{
						Filtered = 0;
						break;
					}
				}
			}

			if(!Filtered && g_Config.m_BrFilterString[0] != 0)
			{
				int MatchFound = 0;

				m_pServerBrowser->m_ppServerlist[i]->m_Info.m_QuickSearchHit = 0;

				// match against server name
				if(str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aName, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_pServerBrowser->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_SERVERNAME;
				}

				// match against players
				for(int p = 0; p < m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_aName, g_Config.m_BrFilterString) ||
						str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan, g_Config.m_BrFilterString))
					{
						MatchFound = 1;
						m_pServerBrowser->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_PLAYER;
						break;
					}
				}

				// match against map
				if(str_find_nocase(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aMap, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_pServerBrowser->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_MAPNAME;
				}

				if(!MatchFound)
					Filtered = 1;
			}
		}

		if(Filtered == 0)
		{
			// check for friend
			m_pServerBrowser->m_ppServerlist[i]->m_Info.m_FriendState = IFriends::FRIEND_NO;
			for(int p = 0; p < m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumClients; p++)
			{
				m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState = m_pServerBrowser->m_pFriends->GetFriendState(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_aName,
					m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan);
				m_pServerBrowser->m_ppServerlist[i]->m_Info.m_FriendState = max(m_pServerBrowser->m_ppServerlist[i]->m_Info.m_FriendState, m_pServerBrowser->m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState);
			}

			if(!(m_SortHash&FILTER_FRIENDS) || m_pServerBrowser->m_ppServerlist[i]->m_Info.m_FriendState != IFriends::FRIEND_NO)
			{
				m_pSortedServerlist[m_NumSortedServers++] = i;
				m_NumPlayers += m_pServerBrowser->m_ppServerlist[i]->m_Info.m_NumPlayers;
			}
		}
	}
}

int CServerBrowser::CServerFilter::SortHash() const
{
	int i = g_Config.m_BrSort&0xf;
	i |= g_Config.m_BrSortOrder<<4;
	if(m_SortHash&FILTER_EMPTY) i |= 1<<5;
	if(m_SortHash&FILTER_FULL) i |= 1<<6;
	if(m_SortHash&FILTER_SPECTATORS) i |= 1<<7;
	if(m_SortHash&FILTER_FRIENDS) i |= 1<<8;
	if(m_SortHash&FILTER_PW) i |= 1<<9;
	if(m_SortHash&FILTER_FAVORITE) i |= 1<<10;
	if(m_SortHash&FILTER_COMPAT_VERSION) i |= 1<<11;
	if(m_SortHash&FILTER_PURE) i |= 1<<12;
	if(m_SortHash&FILTER_PURE_MAP) i |= 1<<13;
	if(m_SortHash&FILTER_GAMETYPE_STRICT) i |= 1<<14;
	if(m_SortHash&FILTER_COUNTRY) i |= 1<<15;
	if(m_SortHash&FILTER_PING) i |= 1<<16;
	return i;
}

void CServerBrowser::CServerFilter::Sort()
{
	// create filtered list
	Filter();

	// sort
	switch(g_Config.m_BrSort)
	{
	case IServerBrowser::SORT_NAME:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::CServerFilter::SortCompareName));
		break;
	case IServerBrowser::SORT_PING:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::CServerFilter::SortComparePing));
		break;
	case IServerBrowser::SORT_MAP:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::CServerFilter::SortCompareMap));
		break;
	case IServerBrowser::SORT_NUMPLAYERS:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this,
					g_Config.m_BrFilterSpectators ? &CServerBrowser::CServerFilter::SortCompareNumPlayers : &CServerBrowser::CServerFilter::SortCompareNumClients));
		break;
	case IServerBrowser::SORT_GAMETYPE:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::CServerFilter::SortCompareGametype));
	}

	// set indexes
	/*for(i = 0; i < m_NumSortedServers; i++)
		m_ppServerlist[m_pSortedServerlist[i]]->m_Info.m_SortedIndex = i;*/

	m_SortHash = SortHash();
}

void CServerBrowser::RemoveRequest(CServerEntry *pEntry)
{
	if(pEntry->m_pPrevReq || pEntry->m_pNextReq || m_pFirstReqServer == pEntry)
	{
		if(pEntry->m_pPrevReq)
			pEntry->m_pPrevReq->m_pNextReq = pEntry->m_pNextReq;
		else
			m_pFirstReqServer = pEntry->m_pNextReq;

		if(pEntry->m_pNextReq)
			pEntry->m_pNextReq->m_pPrevReq = pEntry->m_pPrevReq;
		else
			m_pLastReqServer = pEntry->m_pPrevReq;

		pEntry->m_pPrevReq = 0;
		pEntry->m_pNextReq = 0;
		m_NumRequests--;
	}
}

inline int AddrHash(const NETADDR *pAddr)
{
	if(pAddr->type==NETTYPE_IPV4)
		return (pAddr->ip[0]+pAddr->ip[1]+pAddr->ip[2]+pAddr->ip[3])&0xFF;
	else
		return (pAddr->ip[0]+pAddr->ip[1]+pAddr->ip[2]+pAddr->ip[3]+pAddr->ip[4]+pAddr->ip[5]+pAddr->ip[6]+pAddr->ip[7]+
			pAddr->ip[8]+pAddr->ip[9]+pAddr->ip[10]+pAddr->ip[11]+pAddr->ip[12]+pAddr->ip[13]+pAddr->ip[14]+pAddr->ip[15])&0xFF;
}

CServerBrowser::CServerEntry *CServerBrowser::Find(const NETADDR &Addr)
{
	CServerEntry *pEntry = m_aServerlistIp[AddrHash(&Addr)];

	for(; pEntry; pEntry = pEntry->m_pNextIp)
	{
		if(net_addr_comp(&pEntry->m_Addr, &Addr) == 0)
			return pEntry;
	}
	return (CServerEntry*)0;
}

void CServerBrowser::QueueRequest(CServerEntry *pEntry)
{
	// add it to the list of servers that we should request info from
	pEntry->m_pPrevReq = m_pLastReqServer;
	if(m_pLastReqServer)
		m_pLastReqServer->m_pNextReq = pEntry;
	else
		m_pFirstReqServer = pEntry;
	m_pLastReqServer = pEntry;

	m_NumRequests++;
}

void CServerBrowser::SetInfo(CServerEntry *pEntry, const CServerInfo &Info)
{
	int Fav = pEntry->m_Info.m_Favorite;
	pEntry->m_Info = Info;
	pEntry->m_Info.m_Flags &= FLAG_PASSWORD;
	if(str_comp(pEntry->m_Info.m_aGameType, "DM") == 0 || str_comp(pEntry->m_Info.m_aGameType, "TDM") == 0 || str_comp(pEntry->m_Info.m_aGameType, "CTF") == 0 ||
		str_comp(pEntry->m_Info.m_aGameType, "SUR") == 0 ||	str_comp(pEntry->m_Info.m_aGameType, "LMS") == 0)
		pEntry->m_Info.m_Flags |= FLAG_PURE;
	if(str_comp(pEntry->m_Info.m_aMap, "dm1") == 0 || str_comp(pEntry->m_Info.m_aMap, "dm2") == 0 || str_comp(pEntry->m_Info.m_aMap, "dm6") == 0 ||
		str_comp(pEntry->m_Info.m_aMap, "dm7") == 0 || str_comp(pEntry->m_Info.m_aMap, "dm8") == 0 || str_comp(pEntry->m_Info.m_aMap, "dm9") == 0 ||
		str_comp(pEntry->m_Info.m_aMap, "ctf1") == 0 || str_comp(pEntry->m_Info.m_aMap, "ctf2") == 0 || str_comp(pEntry->m_Info.m_aMap, "ctf3") == 0 ||
		str_comp(pEntry->m_Info.m_aMap, "ctf4") == 0 || str_comp(pEntry->m_Info.m_aMap, "ctf5") == 0 || str_comp(pEntry->m_Info.m_aMap, "ctf6") == 0 ||
		str_comp(pEntry->m_Info.m_aMap, "ctf7") == 0)
		pEntry->m_Info.m_Flags |= FLAG_PUREMAP;
	pEntry->m_Info.m_Favorite = Fav;
	pEntry->m_Info.m_NetAddr = pEntry->m_Addr;
 
	m_NumPlayers += pEntry->m_Info.m_NumPlayers;

	pEntry->m_GotInfo = 1;
}

CServerBrowser::CServerEntry *CServerBrowser::Add(const NETADDR &Addr)
{
	// create new pEntry
	CServerEntry *pEntry = (CServerEntry *)m_ServerlistHeap.Allocate(sizeof(CServerEntry));
	mem_zero(pEntry, sizeof(CServerEntry));

	// set the info
	pEntry->m_Addr = Addr;
	pEntry->m_Info.m_NetAddr = Addr;

	pEntry->m_Info.m_Latency = 999;
	net_addr_str(&Addr, pEntry->m_Info.m_aAddress, sizeof(pEntry->m_Info.m_aAddress), true);
	str_copy(pEntry->m_Info.m_aName, pEntry->m_Info.m_aAddress, sizeof(pEntry->m_Info.m_aName));
	str_copy(pEntry->m_Info.m_aHostname, pEntry->m_Info.m_aAddress, sizeof(pEntry->m_Info.m_aHostname));

	// check if it's a favorite
	for(int i = 0; i < m_NumFavoriteServers; i++)
	{
		if(m_aFavoriteServers[i].m_State >= FAVSTATE_ADDR && net_addr_comp(&Addr, &m_aFavoriteServers[i].m_Addr) == 0)
			pEntry->m_Info.m_Favorite = 1;
	}

	// add to the hash list
	int Hash = AddrHash(&Addr);
	pEntry->m_pNextIp = m_aServerlistIp[Hash];
	m_aServerlistIp[Hash] = pEntry;

	if(m_NumServers == m_NumServerCapacity)
	{
		if(m_NumServerCapacity == 0)
		{
			// alloc start size
			m_NumServerCapacity = 1000;
			m_ppServerlist = (CServerEntry **)mem_alloc(m_NumServerCapacity*sizeof(CServerEntry*), 1);
		}
		else
		{
			// increase size
			m_NumServerCapacity += 100;
			CServerEntry **ppNewlist = (CServerEntry **)mem_alloc(m_NumServerCapacity*sizeof(CServerEntry*), 1);
			mem_copy(ppNewlist, m_ppServerlist, m_NumServers*sizeof(CServerEntry*));
			mem_free(m_ppServerlist);
			m_ppServerlist = ppNewlist;
		}
	}

	// add to list
	m_ppServerlist[m_NumServers] = pEntry;
	pEntry->m_Info.m_ServerIndex = m_NumServers;
	m_NumServers++;

	return pEntry;
}

void CServerBrowser::Set(const NETADDR &Addr, int Type, int Token, const CServerInfo *pInfo)
{
	CServerEntry *pEntry = 0;
	if(Type == SET_MASTER_ADD)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_INTERNET)
			return;

		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}
	/*else if(Type == SET_FAV_ADD)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_FAVORITES)
			return;

		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}*/
	else if(Type == SET_TOKEN)
	{
		if(Token != m_CurrentToken)
			return;

		pEntry = Find(Addr);
		if(!pEntry)
			pEntry = Add(Addr);
		if(pEntry)
		{
			SetInfo(pEntry, *pInfo);
			if(m_ServerlistType == IServerBrowser::TYPE_LAN)
				pEntry->m_Info.m_Latency = min(static_cast<int>((time_get()-m_BroadcastTime)*1000/time_freq()), 999);
			else
				pEntry->m_Info.m_Latency = min(static_cast<int>((time_get()-pEntry->m_RequestTime)*1000/time_freq()), 999);
			RemoveRequest(pEntry);
		}
	}

	for(int i = 0; i < m_lFilters.size(); i++)
		m_lFilters[i].Sort();
}

void CServerBrowser::Refresh(int Type)
{
	// clear out everything
	m_ServerlistHeap.Reset();
	m_NumServers = 0;
	m_NumPlayers = 0;
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		m_lFilters[i].m_NumSortedServers = 0;
		m_lFilters[i].m_NumPlayers = 0;
	}
	mem_zero(m_aServerlistIp, sizeof(m_aServerlistIp));
	m_pFirstReqServer = 0;
	m_pLastReqServer = 0;
	m_NumRequests = 0;

	// next token
	m_CurrentToken = (m_CurrentToken+1)&0xff;

	//
	m_ServerlistType = Type;

	if(Type == IServerBrowser::TYPE_LAN)
	{
		unsigned char Buffer[sizeof(SERVERBROWSE_GETINFO)+1];
		CNetChunk Packet;
		int i;

		mem_copy(Buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
		Buffer[sizeof(SERVERBROWSE_GETINFO)] = m_CurrentToken;

		/* do the broadcast version */
		Packet.m_ClientID = -1;
		mem_zero(&Packet, sizeof(Packet));
		Packet.m_Address.type = m_pNetClient->NetType()|NETTYPE_LINK_BROADCAST;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_DataSize = sizeof(Buffer);
		Packet.m_pData = Buffer;
		m_BroadcastTime = time_get();

		for(i = 8303; i <= 8310; i++)
		{
			Packet.m_Address.port = i;
			m_pNetClient->Send(&Packet);
		}

		if(g_Config.m_Debug)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", "broadcasting for servers");
	}
	else if(Type == IServerBrowser::TYPE_INTERNET)
		m_NeedRefresh = 1;
	/*else if(Type == IServerBrowser::TYPE_FAVORITES)
	{
		for(int i = 0; i < m_NumFavoriteServers; i++)
			if(m_aFavoriteServers[i].m_State >= FAVSTATE_ADDR)
				Set(m_aFavoriteServers[i].m_Addr, SET_FAV_ADD, -1, 0);
	}*/
}

void CServerBrowser::RequestImpl(const NETADDR &Addr, CServerEntry *pEntry) const
{
	unsigned char Buffer[sizeof(SERVERBROWSE_GETINFO)+1];
	CNetChunk Packet;

	if(g_Config.m_Debug)
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf),"requesting server info from %s", aAddrStr);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", aBuf);
	}

	mem_copy(Buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
	Buffer[sizeof(SERVERBROWSE_GETINFO)] = m_CurrentToken;

	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(Buffer);
	Packet.m_pData = Buffer;

	m_pNetClient->Send(&Packet);

	if(pEntry)
		pEntry->m_RequestTime = time_get();
}

void CServerBrowser::Request(const NETADDR &Addr) const
{
	RequestImpl(Addr, 0);
}


void CServerBrowser::Update(bool ForceResort)
{
	int64 Timeout = time_freq();
	int64 Now = time_get();
	int Count;
	CServerEntry *pEntry, *pNext;

	// do server list requests
	if(m_NeedRefresh && !m_pMasterServer->IsRefreshing())
	{
		NETADDR Addr;
		CNetChunk Packet;
		int i;

		m_NeedRefresh = 0;

		mem_zero(&Packet, sizeof(Packet));
		Packet.m_ClientID = -1;
		Packet.m_Flags = NETSENDFLAG_CONNLESS;
		Packet.m_DataSize = sizeof(SERVERBROWSE_GETLIST);
		Packet.m_pData = SERVERBROWSE_GETLIST;

		for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
		{
			if(!m_pMasterServer->IsValid(i))
				continue;

			Addr = m_pMasterServer->GetAddr(i);
			Packet.m_Address = Addr;
			m_pNetClient->Send(&Packet);
		}

		if(g_Config.m_Debug)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", "requesting server list");
	}

	// do timeouts
	pEntry = m_pFirstReqServer;
	while(1)
	{
		if(!pEntry) // no more entries
			break;

		pNext = pEntry->m_pNextReq;

		if(pEntry->m_RequestTime && pEntry->m_RequestTime+Timeout < Now)
		{
			// timeout
			RemoveRequest(pEntry);
		}

		pEntry = pNext;
	}

	// do timeouts
	pEntry = m_pFirstReqServer;
	Count = 0;
	while(1)
	{
		if(!pEntry) // no more entries
			break;

		// no more then 10 concurrent requests
		if(Count == g_Config.m_BrMaxRequests)
			break;

		if(pEntry->m_RequestTime == 0)
			RequestImpl(pEntry->m_Addr, pEntry);

		Count++;
		pEntry = pEntry->m_pNextReq;
	}

	// update favorites
	UpdateFavorites();

	// check if we need to resort
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		CServerFilter *pFilter = &m_lFilters[i];
		if(pFilter->m_SortHash != pFilter->SortHash() || ForceResort)
			pFilter->Sort();
	}
}


void CServerBrowser::UpdateFavorites()
{
	// check if hostname lookup for favourites is done
	if(m_FavLookup.m_Active && m_FavLookup.m_HostLookup.m_Job.Status() == CJob::STATE_DONE)
	{
		// check if favourite has not been removed in the meanwhile
		if(m_FavLookup.m_FavoriteIndex != -1)
		{
			if(m_FavLookup.m_HostLookup.m_Job.Result() == 0)
			{
				CFavoriteServer *pEntry = FindFavoriteByAddr(m_FavLookup.m_HostLookup.m_Addr, 0);
				if(pEntry)
				{
					// address is already in the list -> acquire hostname if existing entry lacks it and drop multiple address entry
					if(pEntry->m_State != FAVSTATE_HOST)
					{
						str_copy(pEntry->m_aHostname, m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_aHostname, sizeof(pEntry->m_aHostname));
						pEntry->m_State = FAVSTATE_HOST;
						dbg_msg("test", "fav aquired hostname, %s", m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_aHostname);
					}
					RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
					dbg_msg("test", "fav removed multiple entry");
				}
				else
				{
					// address wasn't in the list yet -> add it (optional check if hostname matches given address -> drop entry on fail)
					if(m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State == FAVSTATE_LOOKUP ||
						net_addr_comp(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, &m_FavLookup.m_HostLookup.m_Addr) == 0)
					{
						m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_Addr = m_FavLookup.m_HostLookup.m_Addr;
						m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State = FAVSTATE_HOST;
						CServerEntry *pEntry = Find(m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_Addr);
						if(pEntry)
							pEntry->m_Info.m_Favorite = 1;
						dbg_msg("test", "fav added, %s", m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_aHostname);
					}
					else
					{
						RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
						dbg_msg("test", "fav removed entry that failed hostname-address check");
					}
				}
			}
			else
			{
				// hostname lookup failed
				if(m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State == FAVSTATE_LOOKUP)
				{
					m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State = FAVSTATE_INVALID;
					dbg_msg("test", "fav invalid, %s", m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_aHostname);
				}
				else
				{
					RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
					dbg_msg("test", "fav removed invalid check-based entry");
				}
			}
		}
		m_FavLookup.m_Active = false;
	}

	// add hostname lookup for favourites
	if(m_FavLookup.m_LookupCount > 0 && !m_FavLookup.m_Active)
	{
		for(int i = 0; i < m_NumFavoriteServers; i++)
		{
			if(m_aFavoriteServers[i].m_State <= FAVSTATE_LOOKUPCHECK)
			{
				m_pEngine->HostLookup(&m_FavLookup.m_HostLookup, m_aFavoriteServers[i].m_aHostname, m_pNetClient->NetType());
				m_FavLookup.m_FavoriteIndex = i;
				--m_FavLookup.m_LookupCount;
				m_FavLookup.m_Active = true;
				break;
			}
		}
	}
}

CServerBrowser::CFavoriteServer *CServerBrowser::FindFavoriteByAddr(const NETADDR &Addr, int *Index)
{
	for(int i = 0; i < m_NumFavoriteServers; i++)
	{
		if(net_addr_comp(&Addr, &m_aFavoriteServers[i].m_Addr) == 0)
		{
			if(Index)
				*Index = i;
			return &m_aFavoriteServers[i];
		}
	}

	return 0;
}

CServerBrowser::CFavoriteServer *CServerBrowser::FindFavoriteByHostname(const char *pHostname, int *Index)
{
	for(int i = 0; i < m_NumFavoriteServers; i++)
	{
		if(str_comp(pHostname, m_aFavoriteServers[i].m_aHostname) == 0)
		{
			if(Index)
				*Index = i;
			return &m_aFavoriteServers[i];
		}
	}
	
	return 0;
}

void CServerBrowser::RemoveFavoriteEntry(int Index)
{
	mem_move(&m_aFavoriteServers[Index], &m_aFavoriteServers[Index+1], sizeof(CFavoriteServer)*(m_NumFavoriteServers-(Index+1)));
	m_NumFavoriteServers--;
}

void CServerBrowser::AddFavoriteEx(const char *pHostname, const NETADDR *pAddr, bool DoCheck)
{
	if(m_NumFavoriteServers == MAX_FAVORITES || FindFavoriteByHostname(pHostname, 0))
		return;

	// check if hostname is a net address string
	if(net_addr_from_str(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, pHostname) == 0)
	{
		// make sure that we don't already have the server in our list
		if(FindFavoriteByAddr(m_aFavoriteServers[m_NumFavoriteServers].m_Addr, 0) != 0)
			return;

		// check if hostname does not match given address
		if(DoCheck && net_addr_comp(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, pAddr) != 0)
			return;

		// add the server to the list
		m_aFavoriteServers[m_NumFavoriteServers].m_State = FAVSTATE_ADDR;
		CServerEntry *pEntry = Find(m_aFavoriteServers[m_NumFavoriteServers].m_Addr);
		if(pEntry)
			pEntry->m_Info.m_Favorite = 1;
	}
	else
	{
		// prepare for hostname lookup
		if(DoCheck)
		{
			m_aFavoriteServers[m_NumFavoriteServers].m_State = FAVSTATE_LOOKUPCHECK;
			m_aFavoriteServers[m_NumFavoriteServers].m_Addr = *pAddr;
		}
		else
			m_aFavoriteServers[m_NumFavoriteServers].m_State = FAVSTATE_LOOKUP;
		++m_FavLookup.m_LookupCount;
	}

	str_copy(m_aFavoriteServers[m_NumFavoriteServers].m_aHostname, pHostname, sizeof(m_aFavoriteServers[m_NumFavoriteServers].m_aHostname));
	++m_NumFavoriteServers;

	if(g_Config.m_Debug)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "added fav '%s' (%s)", pHostname);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", aBuf);
	}

	// refresh servers in all filters where favorites are filtered
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		CServerFilter *pFilter = &m_lFilters[i];
		if(pFilter->m_SortHash&FILTER_FAVORITE)
			pFilter->Sort();
	}
}

void CServerBrowser::RemoveFavoriteEx(const char *pHostname, const NETADDR *pAddr)
{
	// find favorite entry
	int Index = 0;
	CFavoriteServer *pFavEntry = FindFavoriteByHostname(pHostname, &Index);
	if(pFavEntry == 0 && pAddr)
		pFavEntry = FindFavoriteByAddr(*pAddr, &Index);
	if(pFavEntry)
	{
		if(pFavEntry->m_State >= FAVSTATE_ADDR)
		{
			// invalidate favorite state for server entry
			CServerEntry *pEntry = Find(pFavEntry->m_Addr);
			if(pEntry)
 				pEntry->m_Info.m_Favorite = 0;
		}
		else if(pFavEntry->m_State <= FAVSTATE_LOOKUPCHECK && m_FavLookup.m_FavoriteIndex == Index)
		{
			// skip result on favorite hostname lookup 
			m_FavLookup.m_FavoriteIndex = -1;
		}
		
		// remove favorite
		RemoveFavoriteEntry(Index);
		if(m_FavLookup.m_FavoriteIndex > Index)
			--m_FavLookup.m_FavoriteIndex;
	}

	// refresh servers in all filters where favorites are filtered
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		CServerFilter *pFilter = &m_lFilters[i];
		if(pFilter->m_SortHash&FILTER_FAVORITE)
			pFilter->Sort();
	}
}


int CServerBrowser::LoadingProgression() const
{
	if(m_NumServers == 0)
		return 0;

	int Servers = m_NumServers;
	int Loaded = m_NumServers-m_NumRequests;
	return 100.0f * Loaded/Servers;
}


void CServerBrowser::ConAddFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CServerBrowser *pSelf = static_cast<CServerBrowser *>(pUserData);
	pSelf->AddFavoriteEx(pResult->GetString(0), 0, false);
}

void CServerBrowser::ConRemoveFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CServerBrowser *pSelf = static_cast<CServerBrowser *>(pUserData);
	pSelf->RemoveFavoriteEx(pResult->GetString(0), 0);
}

void CServerBrowser::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CServerBrowser *pSelf = (CServerBrowser *)pUserData;

	char aBuffer[256];
	for(int i = 0; i < pSelf->m_NumFavoriteServers; i++)
	{
		str_format(aBuffer, sizeof(aBuffer), "add_favorite %s", pSelf->m_aFavoriteServers[i].m_aHostname);
		pConfig->WriteLine(aBuffer);
	}
}
