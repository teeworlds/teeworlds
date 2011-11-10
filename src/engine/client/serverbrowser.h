/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_H
#define ENGINE_CLIENT_SERVERBROWSER_H

#include <base/tl/array.h>
#include <engine/serverbrowser.h>

class CServerBrowser : public IServerBrowser
{
public:
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

	enum
	{
		MAX_FAVORITES=256
	};

	class CServerFilter
	{
	public:
		enum
		{
			FILTER_EMPTY=16,
			FILTER_FULL=32,
			FILTER_SPECTATORS=64,
			FILTER_FRIENDS=128,
			FILTER_PW=256,
			FILTER_COMPAT_VERSION=1024,
			FILTER_PURE=2048,
			FILTER_PURE_MAP=4096,
			FILTER_GAMETYPE_STRICT=8192,
			FILTER_COUNTRY=16384,
			FILTER_PING=32768,
		};

		CServerBrowser *pServerBrowser;

		int m_SortHash;
		char m_aGametype[32];
		char m_aServerAddress[16];
		int m_Ping;
		int m_Country;

		int m_NumSortedServers;
		int m_NumSortedServersCapacity;

		int *m_pSortedServerlist;

		CServerEntry **m_ppServerlist;

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

	void AddFilter(int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress);
		
	CServerBrowser();

	// interface functions
	void Refresh(int Type);
	bool IsRefreshing() const;
	bool IsRefreshingMasters() const;
	int LoadingProgression() const;

	int NumServers() const { return m_NumServers; }

	int NumSortedServers(int Index) const { return m_lFilters[Index].m_NumSortedServers; }
	const CServerInfo *SortedGet(int FilterIndex, int Index) const;

	bool IsFavorite(const NETADDR &Addr) const;
	void AddFavorite(const NETADDR &Addr);
	void RemoveFavorite(const NETADDR &Addr);

	//
	void Update(bool ForceResort);
	void Set(const NETADDR &Addr, int Type, int Token, const CServerInfo *pInfo);
	void Request(const NETADDR &Addr) const;

	void SetBaseInfo(class CNetClient *pClient, const char *pNetVersion);

private:
	CNetClient *m_pNetClient;
	IMasterServer *m_pMasterServer;
	class IConsole *m_pConsole;
	class IFriends *m_pFriends;
	char m_aNetVersion[128];

	CHeap m_ServerlistHeap;

	NETADDR m_aFavoriteServers[MAX_FAVORITES];
	int m_NumFavoriteServers;

	CServerEntry *m_aServerlistIp[256]; // ip hash list

	CServerEntry *m_pFirstReqServer; // request list
	CServerEntry *m_pLastReqServer;
	int m_NumRequests;

	int m_NeedRefresh;

	int m_NumServers;
	int m_NumServerCapacity;

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

	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);
};

#endif
