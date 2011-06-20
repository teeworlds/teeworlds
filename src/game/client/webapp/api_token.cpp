#include <game/client/webapp.h>

#include "api_token.h"

int CWebApiToken::GetApiToken(void *pUserData)
{
	CParam *pData = (CParam*)pUserData;
	CClientWebapp *pWebapp = (CClientWebapp*)pData->m_pWebapp;
	char aUsername[32];
	char aPassword[32];
	str_copy(aUsername, pData->m_aUsername, sizeof(aUsername));
	str_copy(aPassword, pData->m_aPassword, sizeof(aPassword));
	delete pData;
	
	if(!pWebapp->Connect())
		return 0;
	
	char *pReceived = 0;
	char aData[128];
	char aBuf[512];
	str_format(aData, sizeof(aData), "username=%s&password=%s", aUsername, aPassword);
	str_format(aBuf, sizeof(aBuf), CClientWebapp::POST, pWebapp->ApiPath(), "anonclient/get_token/", pWebapp->ServerIP(), str_length(aData), aData);
	int Size = pWebapp->SendAndReceive(aBuf, &pReceived);
	pWebapp->Disconnect();
	
	if(Size < 0)
	{
		dbg_msg("webapp", "error: %d (api_token)", Size);
		COut *pOut = new COut(WEB_API_TOKEN);
		str_copy(pOut->m_aApiToken, "false", sizeof(pOut->m_aApiToken));
		pWebapp->AddOutput(pOut);
		return 0;
	}

	COut *pOut = new COut(WEB_API_TOKEN);
	str_copy(pOut->m_aApiToken, pReceived+1, 25);

	mem_free(pReceived);
	
	pWebapp->AddOutput(pOut);
	return 1;
}
