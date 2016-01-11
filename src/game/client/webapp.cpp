/* CClientWebapp Class by Sushi and Redix*/
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/shared/http.h>
#include <engine/external/json-parser/json.h>

#include "gameclient.h"
#include "webapp.h"

static LOCK gs_CheckHostLock = 0;

struct CCheckHostData
{
	CGameClient *m_pClient;
	char m_aAddr[128];
};

CClientWebapp::CClientWebapp()
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

void CClientWebapp::OnApiToken(IResponse *pResponse, bool Error, void *pUserData)
{
	CBufferResponse *pRes = (CBufferResponse*)pResponse;
	CClientWebapp *pWebapp = (CClientWebapp*) pUserData;
	bool Success = !Error && pRes->StatusCode() == 200;

	if(!Success || pRes->Size() != 24+2 || str_comp(pRes->GetBody(), "false") == 0)
		pWebapp->m_ApiTokenError = true;
	else
	{
		pWebapp->m_ApiTokenError = false;
		str_copy(g_Config.m_WaApiToken, pRes->GetBody()+1, 24+1);
	}
	pWebapp->m_ApiTokenRequested = false;
}

void CClientWebapp::OnServerList(IResponse *pResponse, bool Error, void *pUserData)
{
	CBufferResponse *pRes = (CBufferResponse*)pResponse;
	CGameClient *pClient = (CGameClient*) pUserData;
	bool Success = !Error && pRes->StatusCode() == 200;
	if(!Success)
		return;

	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];

	json_value *pJsonData = json_parse_ex(&JsonSettings, pRes->GetBody(), pRes->Size(), aError);
	if(!pJsonData)
		dbg_msg("json", aError);
	else
	{
		if(pJsonData->type == json_array)
		{
			for(unsigned i = 0; i < pJsonData->u.array.length; i++)
			{
				json_value *pJsonSrv = pJsonData->u.array.values[i];
				if (pJsonSrv && pJsonSrv->type == json_string)
				{
					CCheckHostData *pTmp = new CCheckHostData();
					pTmp->m_pClient = pClient;
					str_copy(pTmp->m_aAddr, pJsonSrv->u.string.ptr, sizeof(pTmp->m_aAddr));

					void *LoadThread = thread_create(CheckHostThread, pTmp);
					thread_detach(LoadThread);
				}
			}
		}
		json_value_free(pJsonData);
	}
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
