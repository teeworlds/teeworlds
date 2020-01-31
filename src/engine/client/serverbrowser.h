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
	int GetType() { return m_ActServerlistType; };
	void SetType(int Type);
	void Refresh(int RefreshFlags);
	bool IsRefreshing() const { return m_pFirstReqServer != 0; }
	bool IsRefreshingMasters() const { return m_pMasterServer->IsRefreshing(); }
	int LoadingProgression() const;

	int NumServers() const { return m_aServerlist[m_ActServerlistType].m_NumServers; }
	int NumPlayers() const { return m_aServerlist[m_ActServerlistType].m_NumPlayers; }
	int NumClients() const { return m_aServerlist[m_ActServerlistType].m_NumClients; }
	const CServerInfo *Get(int Index) const { return &m_aServerlist[m_ActServerlistType].m_ppServerlist[Index]->m_Info; };

	int NumSortedServers(int FilterIndex) const { return m_ServerBrowserFilter.GetNumSortedServers(FilterIndex); }
	int NumSortedPlayers(int FilterIndex) const { return m_ServerBrowserFilter.GetNumSortedPlayers(FilterIndex); }
	const CServerInfo *SortedGet(int FilterIndex, int Index) const { return &m_aServerlist[m_ActServerlistType].m_ppServerlist[m_ServerBrowserFilter.GetIndex(FilterIndex, Index)]->m_Info; };
	const void *GetID(int FilterIndex, int Index) const { return m_ServerBrowserFilter.GetID(FilterIndex, Index); };

	void AddFavorite(const CServerInfo *pInfo);
	void RemoveFavorite(const CServerInfo *pInfo);
	void UpdateFavoriteState(CServerInfo *pInfo);
	void SetFavoritePassword(const char *pAddress, const char *pPassword);
	const char *GetFavoritePassword(const char *pAddress);

	int AddFilter(const CServerFilterInfo *pFilterInfo) { return m_ServerBrowserFilter.AddFilter(pFilterInfo); };
	void SetFilter(int Index, const CServerFilterInfo *pFilterInfo) { m_ServerBrowserFilter.SetFilter(Index, pFilterInfo); };
	void GetFilter(int Index, CServerFilterInfo *pFilterInfo) { m_ServerBrowserFilter.GetFilter(Index, pFilterInfo); };
	void RemoveFilter(int Index) { m_ServerBrowserFilter.RemoveFilter(Index); };

	static void CBFTrackPacket(int TrackID, void *pUser);
	
	void LoadServerlist();
	void SaveServerlist();

private:
	class CNetClient *m_pNetClient;
	class IConfig *m_pConfig;
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	class IMasterServer *m_pMasterServer;
		
	class CServerBrowserFavorites m_ServerBrowserFavorites;
	class CServerBrowserFilter m_ServerBrowserFilter;

	class IConfig *Config() const { return m_pConfig; }
	class IConsole *Console() const { return m_pConsole; }
	class IStorage *Storage() const { return m_pStorage; }

	// serverlist
	int m_ActServerlistType;
	class CServerlist
	{
	public:
		class CHeap m_ServerlistHeap;

		int m_NumClients;
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
	int64 m_MasterRefreshTime;

	CServerEntry *Add(int ServerlistType, const NETADDR &Addr);
	CServerEntry *Find(int ServerlistType, const NETADDR &Addr);
	void QueueRequest(CServerEntry *pEntry);
	void RemoveRequest(CServerEntry *pEntry);
	void RequestImpl(const NETADDR &Addr, CServerEntry *pEntry);
	void SetInfo(int ServerlistType, CServerEntry *pEntry, const CServerInfo &Info);
};

#endif
