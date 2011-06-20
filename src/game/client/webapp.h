/* CClientWebapp Class by Sushi and Redix*/
#ifndef GAME_CLIENT_WEBAPP_H
#define GAME_CLIENT_WEBAPP_H

#include <game/webapp.h>

#include "webapp/api_token.h"

class CClientWebapp : public CWebapp
{
private:
	class CGameClient *m_pClient;

public:
	static const char POST[];

	CClientWebapp(class CGameClient *pGameClient);
	virtual ~CClientWebapp();

	void Tick();

	const char *ServerIP();
	const char *ApiPath();

	// api token vars
	bool m_ApiTokenRequested;
	bool m_ApiTokenError;
};

#endif
