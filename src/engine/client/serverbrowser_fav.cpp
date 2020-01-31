/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/memheap.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/engine.h>
#include <engine/contacts.h>
#include <engine/masterserver.h>

#include <mastersrv/mastersrv.h>

#include "serverbrowser_fav.h"


CServerBrowserFavorites::CServerBrowserFavorites()
{
	m_NumFavoriteServers = 0;
	m_FavLookup.m_LookupCount = 0;
	m_FavLookup.m_Active = false;
}

void CServerBrowserFavorites::Init(CNetClient *pNetClient, IConsole *pConsole, IEngine *pEngine, IConfig *pConfig)
{
	m_pNetClient = pNetClient;
	m_pConfig = pConfig;
	m_pConsole = pConsole;
	m_pEngine = pEngine;
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	m_pConsole->Register("add_favorite", "s?s", CFGFLAG_CLIENT, ConAddFavorite, this, "Add a server (optionally with password) as a favorite. Also updates password of existing favorite.");
	m_pConsole->Register("remove_favorite", "s", CFGFLAG_CLIENT, ConRemoveFavorite, this, "Remove a server from favorites");
}

bool CServerBrowserFavorites::AddFavoriteEx(const char *pHostname, const NETADDR *pAddr, bool DoCheck, const char *pPassword)
{
	if(m_NumFavoriteServers == MAX_FAVORITES)
		return false;

	CFavoriteServer *pExistingFavorite = FindFavoriteByHostname(pHostname, 0);
	if(pExistingFavorite)
	{
		if(pPassword)
			str_copy(pExistingFavorite->m_aPassword, pPassword, sizeof(pExistingFavorite->m_aPassword));
		else
			pExistingFavorite->m_aPassword[0] = '\0';
		return false;
	}

	bool Result = false;
	// check if hostname is a net address string
	if(net_addr_from_str(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, pHostname) == 0)
	{
		// make sure that we don't already have the server in our list
		if(FindFavoriteByAddr(m_aFavoriteServers[m_NumFavoriteServers].m_Addr, 0) != 0)
			return false;

		// check if hostname does not match given address
		if(DoCheck && net_addr_comp(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, pAddr) != 0)
			return false;

		// add the server to the list
		m_aFavoriteServers[m_NumFavoriteServers].m_State = FAVSTATE_ADDR;
		Result = true;
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

	if(pPassword)
		str_copy(m_aFavoriteServers[m_NumFavoriteServers].m_aPassword, pPassword, sizeof(m_aFavoriteServers[m_NumFavoriteServers].m_aPassword));
	else
		m_aFavoriteServers[m_NumFavoriteServers].m_aPassword[0] = '\0';

	++m_NumFavoriteServers;

	if(m_pConfig->Values()->m_Debug)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "added fav '%s'", pHostname);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", aBuf);
	}

	return Result;
}

CServerBrowserFavorites::CFavoriteServer *CServerBrowserFavorites::FindFavoriteByAddr(const NETADDR &Addr, int *Index)
{
	for(int i = 0; i < m_NumFavoriteServers; i++)
	{
		if(m_aFavoriteServers[i].m_State >= FAVSTATE_ADDR && net_addr_comp(&Addr, &m_aFavoriteServers[i].m_Addr) == 0)
		{
			if(Index)
				*Index = i;
			return &m_aFavoriteServers[i];
		}
	}

	return 0;
}

CServerBrowserFavorites::CFavoriteServer *CServerBrowserFavorites::FindFavoriteByHostname(const char *pHostname, int *Index)
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

void CServerBrowserFavorites::RemoveFavoriteEntry(int Index)
{
	mem_move(&m_aFavoriteServers[Index], &m_aFavoriteServers[Index+1], sizeof(CFavoriteServer)*(m_NumFavoriteServers-(Index+1)));
	m_NumFavoriteServers--;
}

bool CServerBrowserFavorites::RemoveFavoriteEx(const char *pHostname, const NETADDR *pAddr)
{
	bool Result = false;

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
			Result = true;
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

	return Result;
}

const NETADDR *CServerBrowserFavorites::UpdateFavorites()
{
	NETADDR *pResult = 0;

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
					}
					RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
				}
				else
				{
					// address wasn't in the list yet -> add it (optional check if hostname matches given address -> drop entry on fail)
					if(m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State == FAVSTATE_LOOKUP ||
						net_addr_comp(&m_aFavoriteServers[m_NumFavoriteServers].m_Addr, &m_FavLookup.m_HostLookup.m_Addr) == 0)
					{
						m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_Addr = m_FavLookup.m_HostLookup.m_Addr;
						m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State = FAVSTATE_HOST;
						pResult = &m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_Addr;
					}
					else
					{
						RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
					}
				}
			}
			else
			{
				// hostname lookup failed
				if(m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State == FAVSTATE_LOOKUP)
				{
					m_aFavoriteServers[m_FavLookup.m_FavoriteIndex].m_State = FAVSTATE_INVALID;
				}
				else
				{
					RemoveFavoriteEntry(m_FavLookup.m_FavoriteIndex);
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

	return pResult;
}

void CServerBrowserFavorites::ConAddFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CServerBrowserFavorites *pSelf = static_cast<CServerBrowserFavorites *>(pUserData);
	pSelf->AddFavoriteEx(pResult->GetString(0), 0, false, pResult->NumArguments() > 1 ? pResult->GetString(1) : 0);
}

void CServerBrowserFavorites::ConRemoveFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CServerBrowserFavorites *pSelf = static_cast<CServerBrowserFavorites *>(pUserData);
	pSelf->RemoveFavoriteEx(pResult->GetString(0), 0);
}

void CServerBrowserFavorites::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CServerBrowserFavorites *pSelf = (CServerBrowserFavorites *)pUserData;

	char aBuffer[320];
	for(int i = 0; i < pSelf->m_NumFavoriteServers; i++)
	{
		if(pSelf->m_aFavoriteServers[i].m_aPassword[0])
			str_format(aBuffer, sizeof(aBuffer), "add_favorite \"%s\" \"%s\"", pSelf->m_aFavoriteServers[i].m_aHostname, pSelf->m_aFavoriteServers[i].m_aPassword);
		else
			str_format(aBuffer, sizeof(aBuffer), "add_favorite \"%s\"", pSelf->m_aFavoriteServers[i].m_aHostname);
		pConfig->WriteLine(aBuffer);
	}
}
