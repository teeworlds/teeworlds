/* CClientWebapp Class by Sushi and Redix*/
#ifndef GAME_CLIENT_WEBAPP_H
#define GAME_CLIENT_WEBAPP_H

class CClientWebapp
{
	class CGameClient *m_pClient;

	//void RegisterFields(class CRequest *pRequest, bool Api) { }

	void CheckHost(const char* pAddr);
	static void CheckHostThread(void *pUser);

public:
	CClientWebapp(class CGameClient *pGameClient);
	virtual ~CClientWebapp();
	
	static void OnApiToken(class IResponse *pResponse, bool Error, void *pUserData);
	static void OnServerList(class IResponse *pResponse, bool Error, void *pUserData);

	// api token vars
	bool m_ApiTokenRequested;
	bool m_ApiTokenError;
};

#endif
