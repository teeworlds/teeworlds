#include <modapi/compatibility.h>
#include <modapi/server/message.h>

#include <engine/server.h>

#include <game/server/gamecontext.h>

CModAPI_Msg::CModAPI_Msg(CGameContext* pGameServer) :
	m_pGameServer(pGameServer)
{
	
}

IServer* CModAPI_Msg::Server()
{
	return m_pGameServer->Server();
}

CGameContext* CModAPI_Msg::GameServer()
{
	return m_pGameServer;
}
