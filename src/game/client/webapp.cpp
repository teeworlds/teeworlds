/* CClientWebapp Class by Sushi and Redix*/
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/external/json/reader.h>

#include <game/http/response.h>

#include "gameclient.h"
#include "webapp.h"

CClientWebapp::CClientWebapp(CGameClient *pGameClient)
: IWebapp(pGameClient->Storage()),
  m_pClient(pGameClient)
{
	m_ApiTokenError = false;
	m_ApiTokenRequested = false;
}

void CClientWebapp::OnResponse(CHttpConnection *pCon)
{
	int Type = pCon->Type();
	CResponse *pResponse = pCon->Response();
	bool Error = pCon->Error() || pResponse->StatusCode() != 200;

	Json::Value JsonData;
	Json::Reader Reader;
	bool Json = false;
	if(!pCon->Error() && !pResponse->IsFile())
		Json = Reader.parse(pResponse->GetBody(), pResponse->GetBody()+pResponse->Size(), JsonData);

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
	else if(Type == WEB_SERVER_LIST)
	{
		if(!Error && Json)
		{
			for(int i = 0; i < JsonData.size(); i++)
			{
				NETADDR ServerAddress;
				if(m_pClient->Client()->CheckHost(JsonData[i].asCString(), &ServerAddress))
					m_pClient->ServerBrowser()->AddTeerace(ServerAddress);
			}
		}
	}
}
