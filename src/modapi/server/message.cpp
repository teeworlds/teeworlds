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

CModAPI_Msg_Broadcast::CModAPI_Msg_Broadcast(CGameContext* pGameServer, int Alternative) :
	CModAPI_Msg(pGameServer), m_Alternative(Alternative)
{
	
}

void CModAPI_Msg_Broadcast::Send(int ClientID, const char* pMessage)
{
	if(GameServer()->m_apPlayers[ClientID])
	{
		if(Server()->GetClientProtocolCompatibility(ClientID, MODAPI_COMPATIBILITY_BROADCAST))
		{
			CNetMsg_ModAPI_Sv_Broadcast Msg;
			Msg.m_pMessage = pMessage;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
		}
		else if(m_Alternative == MODAPIALT_BROADCAST_CHAT)
		{
			CNetMsg_Sv_Chat Msg;
			Msg.m_Team = 0;
			Msg.m_ClientID = -1;
			Msg.m_pMessage = pMessage;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
		}
		else if(m_Alternative == MODAPIALT_BROADCAST_MOTD)
		{
			CNetMsg_Sv_Motd Msg;
			Msg.m_pMessage = pMessage;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientID);
		}
	}
}
