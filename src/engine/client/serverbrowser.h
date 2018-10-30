/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_H
#define ENGINE_CLIENT_SERVERBROWSER_H

#include <engine/serverbrowser.h>
#include "serverbrowser_entry.h"
#include "serverbrowser_fav.h"
#include "serverbrowser_filter.h"

class CServerBrowser : public IServerBrowser
{
public:
	enum
	{
		SET_MASTER_ADD=1,
		SET_FAV_ADD,
		SET_TOKEN,
	};
		
	CServerBrowser();
	void Init(class CNetClient *pClient, const char *pNetVersion);
	void Set(const NETADDR &Addr, int SetType, int Token, const CServerInfo *pInfo);
	void Update(bool ForceResort);	

	// interface functions
	void SetType(int Type);
	void Refresh(int RefreshFlags);
	bool IsRefreshing() const { return m_pFirstReqServer != 0; }
	bool IsRefreshingMasters() const { return m_pMasterServer->IsRefreshing(); }
	int LoadingProgression() const;

	int NumServers() const { return m_aServerlist[m_ActServerlistType].m_NumServers; }
	int NumPlayers() const { return m_aServerlist[m_ActServerlistType].m_NumPlayers; }

	int NumSortedServers(int Index) const { return m_ServerBrowserFilter.GetNumSortedServers(Index); }
	int NumSortedPlayers(int Index) const { return m_ServerBrowserFilter.GetNumSortedPlayers(Index); }
	const CServerInfo *SortedGet(int FilterIndex, int Index) const { return &m_aServerlist[m_ActServerlistType].m_ppServerlist[m_ServerBrowserFilter.GetIndex(FilterIndex, Index)]->m_Info; };
	const void *GetID(int FilterIndex, int Index) const { return m_ServerBrowserFilter.GetID(FilterIndex, Index); };

	bool IsFavorite(const NETADDR &Addr) { return m_ServerBrowserFavorites.FindFavoriteByAddr(Addr, 0) != 0; }
	void AddFavorite(const CServerInfo *pEntry);
	void RemoveFavorite(const CServerInfo *pEntry);

	int AddFilter(const CServerFilterInfo *pFilterInfo) { return m_ServerBrowserFilter.AddFilter(pFilterInfo); };
	void SetFilter(int Index, const CServerFilterInfo *pFilterInfo) { m_ServerBrowserFilter.SetFilter(Index, pFilterInfo); }; 
	void GetFilter(int Index, CServerFilterInfo *pFilterInfo) { m_ServerBrowserFilter.GetFilter(Index, pFilterInfo); };
	void RemoveFilter(int Index) { m_ServerBrowserFilter.RemoveFilter(Index); };

	static void CBFTrackPacket(int TrackID, void *pUser);

private:
	class CNetClient *m_pNetClient;
	class IConsole *m_pConsole;
	class IMasterServer *m_pMasterServer;
		
	class CServerBrowserFavorites m_ServerBrowserFavorites;
	class CServerBrowserFilter m_ServerBrowserFilter;

	// serverlist
	int m_ActServerlistType;
	class CServerlist
	{
	public:
		class CHeap m_ServerlistHeap;

		int m_NumPlayers;
		int m_NumServers;
		int m_NumServerCapacity;
	
		CServerEntry *m_aServerlistIp[256]; // ip hash list
		CServerEntry **m_ppServerlist;

		void Clear();
	} m_aServerlist[NUM_TYPES];

	CServerEntry *m_pFirstReqServer; // request list
	CServerEntry *m_pLastReqServer;
	int m_NumRequests;

	int m_NeedRefresh;

	// the token is to keep server refresh separated from each other
	int m_CurrentLanToken;

	int m_RefreshFlags;
	int64 m_BroadcastTime;

	CServerEntry *Add(int ServerlistType, const NETADDR &Addr);
	CServerEntry *Find(int ServerlistType, const NETADDR &Addr);
	void QueueRequest(CServerEntry *pEntry);
	void RemoveRequest(CServerEntry *pEntry);
	void RequestImpl(const NETADDR &Addr, CServerEntry *pEntry);
	void SetInfo(int ServerlistType, CServerEntry *pEntry, const CServerInfo &Info);
};

#endif
