/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_FAV_H
#define ENGINE_CLIENT_SERVERBROWSER_FAV_H

#include <engine/serverbrowser.h>
#include "serverbrowser_entry.h"
#include "serverbrowser_filter.h"

class CServerBrowserFavorites
{
public:
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
		char m_aPassword[sizeof(g_Config.m_Password)];
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

	class CNetClient *m_pNetClient;
	class IConsole *m_pConsole;
	class IEngine *m_pEngine;

	CServerBrowserFavorites();
	void Init(class CNetClient *pNetClient, class IConsole *pConsole, class IEngine *pEngine, class IConfig *pConfig);
	
	bool AddFavoriteEx(const char *pHostname, const NETADDR *pAddr, bool DoCheck, const char *pPassword = 0);
	CFavoriteServer *FindFavoriteByAddr(const NETADDR &Addr, int *Index);
	CFavoriteServer *FindFavoriteByHostname(const char *pHostname, int *Index);
	void RemoveFavoriteEntry(int Index);
	bool RemoveFavoriteEx(const char *pHostname, const NETADDR *Addr);
	const NETADDR *UpdateFavorites();

	static void ConAddFavorite(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveFavorite(IConsole::IResult *pResult, void *pUserData);
	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);
};

#endif
