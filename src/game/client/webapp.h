/* CClientWebapp Class by Sushi and Redix*/
#ifndef GAME_CLIENT_WEBAPP_H
#define GAME_CLIENT_WEBAPP_H

#include <game/http/request.h>
#include <game/data.h>
#include <game/webapp.h>

class CClientWebapp : public IWebapp
{
	class CGameClient *m_pClient;

	void RegisterFields(class CRequest *pRequest, bool Api) {}
	void OnResponse(class CHttpConnection *pCon);

public:
	CClientWebapp(class CGameClient *pGameClient);
	virtual ~CClientWebapp() {}

	// api token vars
	bool m_ApiTokenRequested;
	bool m_ApiTokenError;
};

#endif
