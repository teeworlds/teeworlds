/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVERBROWSER_H
#define ENGINE_SERVERBROWSER_H

#include <engine/shared/protocol.h>

#include "kernel.h"

/*
	Structure: CServerInfo
*/
class CServerInfo
{
public:
	/*
		Structure: CInfoClient
	*/
	class CClient
	{
	public:
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Score;
		bool m_Player;

		int m_FriendState;
	};

	//int m_SortedIndex;
	int m_ServerIndex;

	NETADDR m_NetAddr;

	int m_QuickSearchHit;
	int m_FriendState;

	int m_MaxClients;
	int m_NumClients;
	int m_MaxPlayers;
	int m_NumPlayers;
	int m_Flags;
	int m_ServerLevel;
	int m_Favorite;
	int m_Latency; // in ms
	char m_aGameType[16];
	char m_aName[64];
	char m_aHostname[128];
	char m_aMap[32];
	char m_aVersion[32];
	char m_aAddress[NETADDR_MAXSTRSIZE];
	CClient m_aClients[MAX_CLIENTS];
};

class IServerBrowser : public IInterface
{
	MACRO_INTERFACE("serverbrowser", 0)
public:

	/* Constants: Server Browser Sorting
		SORT_NAME - Sort by name.
		SORT_PING - Sort by ping.
		SORT_MAP - Sort by map
		SORT_GAMETYPE - Sort by game type. DM, TDM etc.
		SORT_NUMPLAYERS - Sort after how many players there are on the server.
	*/
	enum{
		SORT_NAME = 0,
		SORT_PING,
		SORT_MAP,
		SORT_GAMETYPE,
		SORT_NUMPLAYERS,

		QUICK_SERVERNAME=1,
		QUICK_PLAYER=2,
		QUICK_MAPNAME=4,

		TYPE_INTERNET = 0,
		TYPE_LAN = 1,

		FLAG_PASSWORD	=1,
		FLAG_PURE		=2,
		FLAG_PUREMAP	=4,

		FILTER_EMPTY=32,
		FILTER_FULL=64,
		FILTER_SPECTATORS=128,
		FILTER_FRIENDS=256,
		FILTER_PW=512,
		FILTER_FAVORITE=1024,
		FILTER_COMPAT_VERSION=2048,
		FILTER_PURE=4096,
		FILTER_PURE_MAP=8192,
		FILTER_GAMETYPE_STRICT=16384,
		FILTER_COUNTRY=32768,
		FILTER_PING=65536,
	};

	virtual void Refresh(int Type) = 0;
	virtual bool IsRefreshing() const = 0;
	virtual bool IsRefreshingMasters() const = 0;
	virtual int LoadingProgression() const = 0;

	virtual int NumServers() const = 0;
	virtual int NumPlayers() const = 0;

	virtual int NumSortedServers(int Index) const = 0;
	virtual int NumSortedPlayers(int Index) const = 0;
	virtual const CServerInfo *SortedGet(int FilterIndex, int Index) const = 0;
	virtual const void *GetID(int FilterIndex, int Index) const = 0;

	virtual bool IsFavorite(const NETADDR &Addr) = 0;	// todo: remove this
	virtual void AddFavorite(const CServerInfo *pEntry) = 0;
	virtual void RemoveFavorite(const CServerInfo *pEntry) = 0;

	virtual int AddFilter(int Flag, int Ping, int Country, const char* pGametype, const char* pServerAddress) = 0;
	virtual void SetFilter(int Index, int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress) = 0;
	virtual void GetFilter(int Index, int *pSortHash, int *pPing, int *pCountry, char* pGametype, char* pServerAddress) = 0;
	virtual void RemoveFilter(int Index) = 0;
};

#endif
