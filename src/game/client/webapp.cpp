/* CClientWebapp Class by Sushi and Redix*/
#include <engine/shared/config.h>

#include "gameclient.h"
#include "webapp.h"

const char CClientWebapp::POST[] = "POST %s/%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s";

CClientWebapp::CClientWebapp(CGameClient *pGameClient)
: CWebapp(pGameClient->Storage(), g_Config.m_ClWebappIp),
  m_pClient(pGameClient)
{
	m_ApiTokenError = false;
	m_ApiTokenRequested = false;
}

CClientWebapp::~CClientWebapp() {}

void CClientWebapp::Tick()
{
	int Jobs = UpdateJobs();
	if(Jobs > 0)
		dbg_msg("webapp", "Removed %d jobs", Jobs);
	
	// TODO: add event listener (server and client)
	lock_wait(m_OutputLock);
	IDataOut *pNext = 0;
	for(IDataOut *pItem = m_pFirst; pItem; pItem = pNext)
	{
		int Type = pItem->m_Type;
		if(Type == WEB_API_TOKEN)
		{
			CWebApiToken::COut *pData = (CWebApiToken::COut*)pItem;
			if(str_length(pData->m_aApiToken) != 24)
				m_ApiTokenError = true;
			else
			{
				m_ApiTokenError = false;
				str_copy(g_Config.m_ClApiToken, pData->m_aApiToken, sizeof(g_Config.m_ClApiToken));
			}
			m_ApiTokenRequested = false;
		}
		pNext = pItem->m_pNext;
		delete pItem;
	}
	m_pFirst = 0;
	m_pLast = 0;
	lock_release(m_OutputLock);
}

const char* CClientWebapp::ServerIP()
{
	return g_Config.m_ClWebappIp;
}

const char* CClientWebapp::ApiPath()
{
	return g_Config.m_ClApiPath;
}
