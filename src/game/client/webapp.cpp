/* CClientWebapp Class by Sushi and Redix*/
#include <base/tl/array.h>

#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/external/json-parser/json.h>

#include <game/http/response.h>

#include "gameclient.h"
#include "webapp.h"

static LOCK gs_CheckHostLock = 0;

struct CCheckHostData
{
	CGameClient *m_pClient;
	char m_aAddr[128];
};

CClientWebapp::CClientWebapp(CGameClient *pGameClient)
: IWebapp(pGameClient->Storage()),
  m_pClient(pGameClient)
{
	if(gs_CheckHostLock == 0)
		gs_CheckHostLock = lock_create();

	m_ApiTokenError = false;
	m_ApiTokenRequested = false;
}

CClientWebapp::~CClientWebapp()
{
	lock_wait(gs_CheckHostLock);
	lock_release(gs_CheckHostLock);
}

void CClientWebapp::OnResponse(CHttpConnection *pCon)
{
	int Type = pCon->Type();
	CResponse *pResponse = pCon->Response();
	bool Error = pCon->Error() || pResponse->StatusCode() != 200 || pResponse->IsFile();
	
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];

	// TODO: add event listener (server and client)
	if(Type == WEB_API_TOKEN)
	{
		// TODO: better error messages
		if(Error || str_comp(pResponse->GetBody(), "false") == 0 || pResponse->Size() != 24+2)
			m_ApiTokenError = true;
		else
		{
			m_ApiTokenError = false;
			str_copy(g_Config.m_WaApiToken, pResponse->GetBody()+1, 24+1);
		}
		m_ApiTokenRequested = false;
	}
	else if(Type == WEB_SERVER_LIST && !Error)
	{
		json_value *pJsonData = json_parse_ex(&JsonSettings, pResponse->GetBody(), pResponse->Size(), aError);
		if(!pJsonData)
			m_pClient->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "json", aError);
		else
		{
			if(pJsonData->type == json_array)
			{
				for(unsigned i = 0; i < pJsonData->u.array.length; i++)
				{
					json_value *pJsonSrv = pJsonData->u.array.values[i];
					if(pJsonSrv && pJsonSrv->type == json_string)
						CheckHost(pJsonSrv->u.string.ptr);
				}
			}
			json_value_free(pJsonData);
		}
	}
}

void CClientWebapp::CheckHost(const char* pAddr)
{
	CCheckHostData *pTmp = new CCheckHostData();
	pTmp->m_pClient = m_pClient;
	str_copy(pTmp->m_aAddr, pAddr, sizeof(pTmp->m_aAddr));

	void *LoadThread = thread_create(CheckHostThread, pTmp);
	thread_detach(LoadThread);
}

void CClientWebapp::CheckHostThread(void *pUser)
{
	lock_wait(gs_CheckHostLock);

	CCheckHostData *pData = (CCheckHostData*)pUser;

	NETADDR ServerAddress;
	if(net_host_lookup(pData->m_aAddr, &ServerAddress, NETTYPE_IPV4) == 0)
		pData->m_pClient->ServerBrowser()->AddTeerace(ServerAddress);

	delete pData;

	lock_release(gs_CheckHostLock);
}
