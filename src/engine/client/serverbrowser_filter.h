/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_FILTER_H
#define ENGINE_CLIENT_SERVERBROWSER_FILTER_H

#include <base/tl/array.h>

class CServerBrowserFilter
{
public:
	enum
	{
		RESORT_FLAG_FORCE=1,
		RESORT_FLAG_FAV=2,
	};

	class CServerFilter
	{
	public:
		CServerBrowserFilter *m_pServerBrowserFilter;
		IConfig *Config() const { return m_pServerBrowserFilter->m_pConfig; }

		// filter settings
		CServerFilterInfo m_FilterInfo;
		
		// stats
		int m_NumSortedPlayers;
		int m_NumSortedServers;
		int *m_pSortedServerlist;
		int m_SortedServersCapacity;
		
		CServerFilter();
		~CServerFilter();
		CServerFilter& operator=(const CServerFilter& Other);

		void Filter();
		int GetSortHash() const;
		void Sort();

		// sorting criterions
		bool SortCompareName(int Index1, int Index2) const;
		bool SortCompareMap(int Index1, int Index2) const;
		bool SortComparePing(int Index1, int Index2) const;
		bool SortCompareGametype(int Index1, int Index2) const;
		bool SortCompareNumPlayers(int Index1, int Index2) const;
		bool SortCompareNumRealPlayers(int Index1, int Index2) const;
		bool SortCompareNumClients(int Index1, int Index2) const;
		bool SortCompareNumRealClients(int Index1, int Index2) const;
	};
	IConfig *Config() { return m_pConfig; }

	//
	void Init(class IConfig *pConfig, class IFriends *pFriends, const char *pNetVersion);
	void Clear();
	void Sort(class CServerEntry **ppServerlist, int NumServers, int ResortFlags);

	// filter
	int AddFilter(const class CServerFilterInfo *pFilterInfo);
	void GetFilter(int Index, class CServerFilterInfo *pFilterInfo) const;
	void RemoveFilter(int Index);
	void SetFilter(int Index, const class CServerFilterInfo *pFilterInfo);
	
	// stats
	const void *GetID(int FilterIndex, int Index) const { return &m_lFilters[FilterIndex].m_pSortedServerlist[Index]; }
	int GetIndex(int FilterIndex, int Index) const { return m_lFilters[FilterIndex].m_pSortedServerlist[Index]; }
	int GetNumSortedServers(int FilterIndex) const { return m_lFilters[FilterIndex].m_NumSortedServers; }
	int GetNumSortedPlayers(int FilterIndex) const { return m_lFilters[FilterIndex].m_NumSortedPlayers; }

private:
	class IConfig *m_pConfig;
	class IFriends *m_pFriends;
	char m_aNetVersion[128];
	array<CServerFilter> m_lFilters;

	// get updated on sort
	class CServerEntry **m_ppServerlist;
	int m_NumServers;
};

#endif
