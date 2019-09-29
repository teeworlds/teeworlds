/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <algorithm> // sort  TODO: remove this

#include <base/math.h>

#include <engine/shared/config.h>
#include <engine/client/contacts.h>
#include <engine/serverbrowser.h>

#include "serverbrowser_entry.h"
#include "serverbrowser_filter.h"


class SortWrap
{
	typedef bool (CServerBrowserFilter::CServerFilter::*SortFunc)(int, int) const;
	SortFunc m_pfnSort;
	CServerBrowserFilter::CServerFilter *m_pThis;
public:
	SortWrap(CServerBrowserFilter::CServerFilter *t, SortFunc f) : m_pfnSort(f), m_pThis(t) {}
	bool operator()(int a, int b) { return (g_Config.m_BrSortOrder ? (m_pThis->*m_pfnSort)(b, a) : (m_pThis->*m_pfnSort)(a, b)); }
};

//	CServerFilter
CServerBrowserFilter::CServerFilter::CServerFilter()
{
	m_pServerBrowserFilter = 0;

	m_FilterInfo.m_SortHash = 0;
	m_FilterInfo.m_Ping = 0;
	m_FilterInfo.m_Country = 0;
	m_FilterInfo.m_ServerLevel = 0;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		m_FilterInfo.m_aGametype[i][0] = 0;
	m_FilterInfo.m_aAddress[0] = 0;

	m_NumSortedPlayers = 0;
	m_NumSortedServers = 0;
	m_SortedServersCapacity = 0;

	m_pSortedServerlist = 0;
}

CServerBrowserFilter::CServerFilter::~CServerFilter()
{
	if(m_pSortedServerlist)
		mem_free(m_pSortedServerlist);
}

CServerBrowserFilter::CServerFilter& CServerBrowserFilter::CServerFilter::operator=(const CServerBrowserFilter::CServerFilter& Other)
{
	if(&Other != this)
	{
		m_pServerBrowserFilter = Other.m_pServerBrowserFilter;
		m_FilterInfo.m_SortHash = Other.m_FilterInfo.m_SortHash;
		m_FilterInfo.m_Ping = Other.m_FilterInfo.m_Ping;
		m_FilterInfo.m_Country = Other.m_FilterInfo.m_Country;
		m_FilterInfo.m_ServerLevel = Other.m_FilterInfo.m_ServerLevel;
		for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		{
			if(Other.m_FilterInfo.m_aGametype[i][0])
				str_copy(m_FilterInfo.m_aGametype[i], Other.m_FilterInfo.m_aGametype[i], sizeof(m_FilterInfo.m_aGametype[i]));
			else
				m_FilterInfo.m_aGametype[i][0] = 0;
		}
		str_copy(m_FilterInfo.m_aAddress, Other.m_FilterInfo.m_aAddress, sizeof(m_FilterInfo.m_aAddress));

		m_NumSortedPlayers = Other.m_NumSortedPlayers;
		m_NumSortedServers = Other.m_NumSortedServers;
		m_SortedServersCapacity = Other.m_SortedServersCapacity;

		m_pSortedServerlist = (int *)mem_alloc(m_SortedServersCapacity * sizeof(int), 1);
		for(int i = 0; i < m_SortedServersCapacity; ++i)
			m_pSortedServerlist[i] = Other.m_pSortedServerlist[i];
	}
	return *this;
}

void CServerBrowserFilter::CServerFilter::Filter()
{
	int NumServers = m_pServerBrowserFilter->m_NumServers;
	m_NumSortedServers = 0;
	m_NumSortedPlayers = 0;

	// allocate the sorted list
	if(m_SortedServersCapacity < NumServers)
	{
		if(m_pSortedServerlist)
			mem_free(m_pSortedServerlist);
		m_SortedServersCapacity = max(1000, NumServers+NumServers/2);
		m_pSortedServerlist = (int *)mem_alloc(m_SortedServersCapacity*sizeof(int), 1);
	}

	// filter the servers
	for(int i = 0; i < NumServers; i++)
	{
		int Filtered = 0;

		int RelevantClientCount = (m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) ? m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumPlayers : m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumClients;
		if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS)
		{
			RelevantClientCount -= m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumBotPlayers;
			if(!(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS))
				RelevantClientCount -= m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumBotSpectators;
		}

		if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY && RelevantClientCount == 0)
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL && ((m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS && m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumPlayers == m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_MaxPlayers) ||
				m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumClients == m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_MaxClients))
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PW && m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_Flags&IServerBrowser::FLAG_PASSWORD)
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FAVORITE && !m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_Favorite)
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE && !(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_Flags&IServerBrowser::FLAG_PURE))
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP &&  !(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_Flags&IServerBrowser::FLAG_PUREMAP))
			Filtered = 1;
		else if(m_FilterInfo.m_Ping < m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_Latency)
			Filtered = 1;
		else if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION && str_comp_num(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aVersion, m_pServerBrowserFilter->m_aNetVersion, 3) != 0)
			Filtered = 1;
		else if(m_FilterInfo.m_aAddress[0] && !str_find_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aAddress, m_FilterInfo.m_aAddress))
			Filtered = 1;
		else if(m_FilterInfo.m_ServerLevel & (1 << m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_ServerLevel))
			Filtered = 1;
		else
		{
			if(m_FilterInfo.m_aGametype[0][0])
			{
				Filtered = 1;
				for(int Index = 0; Index < CServerFilterInfo::MAX_GAMETYPES; ++Index)
				{
					if(!m_FilterInfo.m_aGametype[Index][0])
						break;
					if(!str_comp_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aGameType, m_FilterInfo.m_aGametype[Index]))
					{
						Filtered = 0;
						break;
					}
				}
			}

			if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY)
			{
				Filtered = 1;
				// match against player country
				for(int p = 0; p < m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_Country == m_FilterInfo.m_Country)
					{
						Filtered = 0;
						break;
					}
				}
			}

			if(!Filtered && g_Config.m_BrFilterString[0] != 0)
			{
				int MatchFound = 0;

				m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_QuickSearchHit = 0;

				// match against server name
				if(str_find_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aName, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_SERVERNAME;
				}

				// match against players
				for(int p = 0; p < m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(str_find_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_aName, g_Config.m_BrFilterString) ||
						str_find_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan, g_Config.m_BrFilterString))
					{
						MatchFound = 1;
						m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_PLAYER;
						break;
					}
				}

				// match against map
				if(str_find_nocase(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aMap, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_MAPNAME;
				}

				if(!MatchFound)
					Filtered = 1;
			}
		}

		if(Filtered == 0)
		{
			// check for friend
			m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_FriendState = CContactInfo::CONTACT_NO;
			for(int p = 0; p < m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_NumClients; p++)
			{
				m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState = m_pServerBrowserFilter->m_pFriends->GetFriendState(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_aName,
					m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan);
				m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_FriendState = max(m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_FriendState, m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState);
			}

			if(!(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS) || m_pServerBrowserFilter->m_ppServerlist[i]->m_Info.m_FriendState != CContactInfo::CONTACT_NO)
			{
				m_pSortedServerlist[m_NumSortedServers++] = i;
				m_NumSortedPlayers += RelevantClientCount;
			}
		}
	}
}

int CServerBrowserFilter::CServerFilter::GetSortHash() const
{
	int i = g_Config.m_BrSort&0x7;
	i |= g_Config.m_BrSortOrder<<3;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS) i |= 1<<4;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_EMPTY) i |= 1<<5;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FULL) i |= 1<<6;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) i |= 1<<7;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FRIENDS) i |= 1<<8;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PW) i |= 1<<9;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FAVORITE) i |= 1<<10;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_COMPAT_VERSION) i |= 1<<11;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE) i |= 1<<12;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_PURE_MAP) i |= 1<<13;
	if(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_COUNTRY) i |= 1<<14;
	return i;
}

void CServerBrowserFilter::CServerFilter::Sort()
{
	// create filtered list
	Filter();

	// sort
	switch(g_Config.m_BrSort)
	{
	case IServerBrowser::SORT_NAME:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowserFilter::CServerFilter::SortCompareName));
		break;
	case IServerBrowser::SORT_PING:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowserFilter::CServerFilter::SortComparePing));
		break;
	case IServerBrowser::SORT_MAP:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowserFilter::CServerFilter::SortCompareMap));
		break;
	case IServerBrowser::SORT_NUMPLAYERS:
		if(!(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_BOTS))
			std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this,
						(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) ? &CServerBrowserFilter::CServerFilter::SortCompareNumPlayers : &CServerBrowserFilter::CServerFilter::SortCompareNumClients));
		else
			std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this,
						(m_FilterInfo.m_SortHash&IServerBrowser::FILTER_SPECTATORS) ? &CServerBrowserFilter::CServerFilter::SortCompareNumRealPlayers : &CServerBrowserFilter::CServerFilter::SortCompareNumRealClients));
		break;
	case IServerBrowser::SORT_GAMETYPE:
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowserFilter::CServerFilter::SortCompareGametype));
	}

	m_FilterInfo.m_SortHash = GetSortHash();
}

bool CServerBrowserFilter::CServerFilter::SortCompareName(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	//	make sure empty entries are listed last
	return (a->m_InfoState == CServerEntry::STATE_READY && b->m_InfoState == CServerEntry::STATE_READY) || (a->m_InfoState != CServerEntry::STATE_READY && b->m_InfoState != CServerEntry::STATE_READY) ? str_comp_nocase(a->m_Info.m_aName, b->m_Info.m_aName) < 0 :
			a->m_InfoState == CServerEntry::STATE_READY;
}

bool CServerBrowserFilter::CServerFilter::SortCompareMap(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	int Result = str_comp_nocase(a->m_Info.m_aMap, b->m_Info.m_aMap);
	return Result < 0 || (Result == 0 && (a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && !(b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

bool CServerBrowserFilter::CServerFilter::SortComparePing(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return a->m_Info.m_Latency < b->m_Info.m_Latency ||
		(a->m_Info.m_Latency == b->m_Info.m_Latency && (a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && !(b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

bool CServerBrowserFilter::CServerFilter::SortCompareGametype(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return str_comp_nocase(a->m_Info.m_aGameType, b->m_Info.m_aGameType) < 0;
}

bool CServerBrowserFilter::CServerFilter::SortCompareNumPlayers(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return a->m_Info.m_NumPlayers < b->m_Info.m_NumPlayers ||
		(a->m_Info.m_NumPlayers == b->m_Info.m_NumPlayers && !(a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && (b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

bool CServerBrowserFilter::CServerFilter::SortCompareNumRealPlayers(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return (a->m_Info.m_NumPlayers - a->m_Info.m_NumBotPlayers) < (b->m_Info.m_NumPlayers - b->m_Info.m_NumBotPlayers) ||
		((a->m_Info.m_NumPlayers - a->m_Info.m_NumBotPlayers) == (b->m_Info.m_NumPlayers - b->m_Info.m_NumBotPlayers) && !(a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && (b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

bool CServerBrowserFilter::CServerFilter::SortCompareNumClients(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return a->m_Info.m_NumClients < b->m_Info.m_NumClients ||
		(a->m_Info.m_NumClients == b->m_Info.m_NumClients && !(a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && (b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

bool CServerBrowserFilter::CServerFilter::SortCompareNumRealClients(int Index1, int Index2) const
{
	CServerEntry *a = m_pServerBrowserFilter->m_ppServerlist[Index1];
	CServerEntry *b = m_pServerBrowserFilter->m_ppServerlist[Index2];
	return (a->m_Info.m_NumClients - a->m_Info.m_NumBotPlayers - a->m_Info.m_NumBotSpectators) < (b->m_Info.m_NumClients - b->m_Info.m_NumBotPlayers - b->m_Info.m_NumBotSpectators) ||
		((a->m_Info.m_NumClients - a->m_Info.m_NumBotPlayers - a->m_Info.m_NumBotSpectators) == (b->m_Info.m_NumClients - b->m_Info.m_NumBotPlayers - b->m_Info.m_NumBotSpectators) && !(a->m_Info.m_Flags&IServerBrowser::FLAG_PURE) && (b->m_Info.m_Flags&IServerBrowser::FLAG_PURE));
}

//	CServerBrowserFilter
void CServerBrowserFilter::Init(IFriends *pFriends, const char *pNetVersion)
{
	m_pFriends = pFriends;
	str_copy(m_aNetVersion, pNetVersion, sizeof(m_aNetVersion));
}

void CServerBrowserFilter::Clear()
{
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		m_lFilters[i].m_NumSortedServers = 0;
		m_lFilters[i].m_NumSortedPlayers = 0;
	}
}

void CServerBrowserFilter::Sort(CServerEntry **ppServerlist, int NumServers, int ResortFlags)
{
	m_ppServerlist = ppServerlist;
	m_NumServers = NumServers;
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		// check if we need to resort
		CServerFilter *pFilter = &m_lFilters[i];
		if((ResortFlags&RESORT_FLAG_FORCE) || ((ResortFlags&RESORT_FLAG_FAV) && pFilter->m_FilterInfo.m_SortHash&IServerBrowser::FILTER_FAVORITE) || pFilter->m_FilterInfo.m_SortHash != pFilter->GetSortHash())
			pFilter->Sort();
	}
}

int CServerBrowserFilter::AddFilter(const CServerFilterInfo *pFilterInfo)
{
	CServerFilter Filter;
	Filter.m_FilterInfo.m_SortHash = pFilterInfo->m_SortHash;
	Filter.m_FilterInfo.m_Ping = pFilterInfo->m_Ping;
	Filter.m_FilterInfo.m_Country = pFilterInfo->m_Country;
	Filter.m_FilterInfo.m_ServerLevel = pFilterInfo->m_ServerLevel;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		str_copy(Filter.m_FilterInfo.m_aGametype[i], pFilterInfo->m_aGametype[i], sizeof(Filter.m_FilterInfo.m_aGametype[i]));
	str_copy(Filter.m_FilterInfo.m_aAddress, pFilterInfo->m_aAddress, sizeof(Filter.m_FilterInfo.m_aAddress));
	Filter.m_pSortedServerlist = 0;
	Filter.m_NumSortedPlayers = 0;
	Filter.m_NumSortedServers = 0;
	Filter.m_SortedServersCapacity = 0;
	Filter.m_pServerBrowserFilter = this;
	m_lFilters.add(Filter);

	return m_lFilters.size()-1;
}

void CServerBrowserFilter::GetFilter(int Index, CServerFilterInfo *pFilterInfo) const
{
	const CServerFilter *pFilter = &m_lFilters[Index];
	pFilterInfo->m_SortHash = pFilter->m_FilterInfo.m_SortHash;
	pFilterInfo->m_Ping = pFilter->m_FilterInfo.m_Ping;
	pFilterInfo->m_Country = pFilter->m_FilterInfo.m_Country;
	pFilterInfo->m_ServerLevel = pFilter->m_FilterInfo.m_ServerLevel;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		str_copy(pFilterInfo->m_aGametype[i], pFilter->m_FilterInfo.m_aGametype[i], sizeof(pFilterInfo->m_aGametype[i]));
	str_copy(pFilterInfo->m_aAddress, pFilter->m_FilterInfo.m_aAddress, sizeof(pFilterInfo->m_aAddress));
}

void CServerBrowserFilter::SetFilter(int Index, const CServerFilterInfo *pFilterInfo)
{
	CServerFilter *pFilter = &m_lFilters[Index];
	pFilter->m_FilterInfo.m_SortHash = pFilterInfo->m_SortHash;
	pFilter->m_FilterInfo.m_Ping = pFilterInfo->m_Ping;
	pFilter->m_FilterInfo.m_Country = pFilterInfo->m_Country;
	pFilter->m_FilterInfo.m_ServerLevel = pFilterInfo->m_ServerLevel;
	for(int i = 0; i < CServerFilterInfo::MAX_GAMETYPES; ++i)
		str_copy(pFilter->m_FilterInfo.m_aGametype[i], pFilterInfo->m_aGametype[i], sizeof(pFilter->m_FilterInfo.m_aGametype[i]));
	str_copy(pFilter->m_FilterInfo.m_aAddress, pFilterInfo->m_aAddress, sizeof(pFilter->m_FilterInfo.m_aAddress));

	pFilter->Sort();
}

void CServerBrowserFilter::RemoveFilter(int Index)
{
	m_lFilters.remove_index(Index);
}