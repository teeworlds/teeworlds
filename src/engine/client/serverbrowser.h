/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_H
#define ENGINE_CLIENT_SERVERBROWSER_H

#include <base/tl/array.h>
#include <engine/serverbrowser.h>

class CServerBrowser : public IServerBrowser
{
public:
	enum
	{
		SET_MASTER_ADD=1,
		SET_FAV_ADD,
		SET_TOKEN,
	};

	class CServerEntry
	{
	public:
		NETADDR m_Addr;
		int64 m_RequestTime;
		int m_GotInfo;
		CServerInfo m_Info;

		CServerEntry *m_pNextIp; // ip hashed list

		CServerEntry *m_pPrevReq; // request list
		CServerEntry *m_pNextReq;
	};

	class CServerFilter
	{
	public:
		CServerBrowser *m_pServerBrowser;

		int m_SortHash;
		char m_aGametype[32];
		char m_aServerAddress[16];
		int m_Ping;
		int m_Country;

		int m_NumSortedServers;
		int m_NumSortedServersCapacity;

		int m_NumPlayers;

		int *m_pSortedServerlist;

		~CServerFilter();

		// sorting criterions
		bool SortCompareName(int Index1, int Index2) const;
		bool SortCompareMap(int Index1, int Index2) const;
		bool SortComparePing(int Index1, int Index2) const;
		bool SortCompareGametype(int Index1, int Index2) const;
		bool SortCompareNumPlayers(int Index1, int Index2) const;
		bool SortCompareNumClients(int Index1, int Index2) const;

		void Filter();
		void Sort();
		int SortHash() const;
	};

	array<CServerFilter> m_lFilters;

	int AddFilter(int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress);
	void SetFilter(int Index, int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress);
	void GetFilter(int Index, int *pSortHash, int *pPing, int *pCountry, char* pGametype, char* pServerAddress);
	void RemoveFilter(int Index);
		
	CServerBrowser();

	// interface functions
	void Refresh(int Type);
	bool IsRefreshing() const { return m_pFirstReqServer != 0; }
	bool IsRefreshingMasters() const { return m_pMasterServer->IsRefreshing(); }
	int LoadingProgression() const;

	int NumServers() const { return m_NumServers; }
	int NumPlayers() const { return m_NumPlayers; }

	int NumSortedServers(int Index) const { return m_lFilters[Index].m_NumSortedServers; }
	int NumSortedPlayers(int Index) const { return m_lFilters[Index].m_NumPlayers; }
	const CServerInfo *SortedGet(int FilterIndex, int Index) const;
	const void *GetID(int FilterIndex, int Index) const;

	bool IsFavorite(const NETADDR &Addr) { return FindFavoriteByAddr(Addr, 0) != 0; }
	void AddFavorite(const CServerInfo *pEntry) { AddFavoriteEx(pEntry->m_aHostname, &pEntry->m_NetAddr, true); }
	void RemoveFavorite(const CServerInfo *pEntry) { RemoveFavoriteEx(pEntry->m_aHostname, &pEntry->m_NetAddr); }

	//
	void Update(bool ForceResort);
	void Set(const NETADDR &Addr, int Type, int Token, const CServerInfo *pInfo);
	void Request(const NETADDR &Addr) const;

	void SetBaseInfo(class CNetClient *pClient, const char *pNetVersion);

private:
	CNetClient *m_pNetClient;
	IMasterServer *m_pMasterServer;
	class IConsole *m_pConsole;
	class IEngine *m_pEngine;
	class IFriends *m_pFriends;
	char m_aNetVersion[128];

	CHeap m_ServerlistHeap;

	// favourite
	enum
	{
		FAVSTATE_LOOKUP=0,
		FAVSTATE_LOOKUPCHECK,
		FAVSTATE_INVALID,
		FAVSTATE_ADDR,
		FAVSTATE_HOST,

		MAX_FAVORITES=256,
	};

	struct CFavoriteServer
	{
		char m_aHostname[128];
		NETADDR m_Addr;
		int m_State;
	} m_aFavoriteServers[MAX_FAVORITES];

	int m_NumFavoriteServers;

	struct CFavoriteLookup
	{
		class CHostLookup m_HostLookup;
		int m_FavoriteIndex;
		int m_LookupCount;
		bool m_Active;
	} m_FavLookup;

	void UpdateFavorites();
	CFavoriteServer *FindFavoriteByAddr(const NETADDR &Addr, int *Index);
	CFavoriteServer *FindFavoriteByHostname(const char *pHostname, int *Index);
	void RemoveFavoriteEntry(int Index);
	void AddFavoriteEx(const char *pHostname, const NETADDR *pAddr, bool DoCheck);
	void RemoveFavoriteEx(const char *pHostname, const NETADDR *Addr);

	//
	CServerEntry *m_aServerlistIp[256]; // ip hash list

	CServerEntry **m_ppServerlist;

	CServerEntry *m_pFirstReqServer; // request list
	CServerEntry *m_pLastReqServer;
	int m_NumRequests;

	int m_NeedRefresh;

	int m_NumServers;
	int m_NumServerCapacity;

	int m_NumPlayers;

	// the token is to keep server refresh separated from each other
	int m_CurrentToken;

	int m_ServerlistType;
	int64 m_BroadcastTime;

	CServerEntry *Find(const NETADDR &Addr);
	CServerEntry *Add(const NETADDR &Addr);

	void RemoveRequest(CServerEntry *pEntry);
	void QueueRequest(CServerEntry *pEntry);

	void RequestImpl(const NETADDR &Addr, CServerEntry *pEntry) const;

	void SetInfo(CServerEntry *pEntry, const CServerInfo &Info);

	static void ConAddFavorite(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveFavorite(IConsole::IResult *pResult, void *pUserData);
	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);
};

#endif
