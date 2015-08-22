#include <engine/console.h>
#include <engine/shared/config.h>

#include "econ.h"
#include "netban.h"


int CEcon::NewClientCallback(int ClientID, void *pUser)
{
	CEcon *pThis = (CEcon *)pUser;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pThis->m_NetConsole.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "client accepted. cid=%d addr=%s'", ClientID, aAddrStr);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "econ", aBuf);

	pThis->m_aClients[ClientID].m_State = CClient::STATE_CONNECTED;
	pThis->m_aClients[ClientID].m_TimeConnected = time_get();
	pThis->m_aClients[ClientID].m_AuthTries = 0;

	pThis->m_NetConsole.Send(ClientID, "Enter password:");
	return 0;
}

int CEcon::DelClientCallback(int ClientID, const char *pReason, void *pUser)
{
	CEcon *pThis = (CEcon *)pUser;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pThis->m_NetConsole.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "client dropped. cid=%d addr=%s reason='%s'", ClientID, aAddrStr, pReason);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "econ", aBuf);

	pThis->m_aClients[ClientID].m_State = CClient::STATE_EMPTY;
	return 0;
}

void CEcon::SendLineCB(const char *pLine, void *pUserData)
{
	static_cast<CEcon *>(pUserData)->Send(-1, pLine);
}

void CEcon::ConchainEconOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 1)
	{
		CEcon *pThis = static_cast<CEcon *>(pUserData);
		pThis->Console()->SetPrintOutputLevel(pThis->m_PrintCBIndex, pResult->GetInteger(0));
	}
}

void CEcon::ConLogout(IConsole::IResult *pResult, void *pUserData)
{
	CEcon *pThis = static_cast<CEcon *>(pUserData);

	if(pThis->m_UserClientID >= 0 && pThis->m_UserClientID < NET_MAX_CONSOLE_CLIENTS && pThis->m_aClients[pThis->m_UserClientID].m_State != CClient::STATE_EMPTY)
		pThis->m_NetConsole.Drop(pThis->m_UserClientID, "Logout");
}

void CEcon::Init(IConsole *pConsole, CNetBan *pNetBan)
{
	m_pConsole = pConsole;

	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
		m_aClients[i].m_State = CClient::STATE_EMPTY;

	m_Ready = false;
	m_UserClientID = -1;

	if(g_Config.m_EcPort == 0 || g_Config.m_EcPassword[0] == 0)
		return;

	NETADDR BindAddr;
	if(g_Config.m_EcBindaddr[0] && net_host_lookup(g_Config.m_EcBindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// got bindaddr
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = g_Config.m_EcPort;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = g_Config.m_EcPort;
	}

	if(m_NetConsole.Open(BindAddr, pNetBan, 0))
	{
		m_NetConsole.SetCallbacks(NewClientCallback, DelClientCallback, this);
		m_Ready = true;
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "bound to %s:%d", g_Config.m_EcBindaddr, g_Config.m_EcPort);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"econ", aBuf);

		Console()->Chain("ec_output_level", ConchainEconOutputLevelUpdate, this);
		m_PrintCBIndex = Console()->RegisterPrintCallback(g_Config.m_EcOutputLevel, SendLineCB, this);

		Console()->Register("logout", "", CFGFLAG_ECON, ConLogout, this, "Logout of econ");
	}
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,"econ", "couldn't open socket. port might already be in use");
}

void CEcon::Update()
{
	if(!m_Ready)
		return;

	m_NetConsole.Update();

	char aBuf[NET_MAX_PACKETSIZE];
	int ClientID;

	while(m_NetConsole.Recv(aBuf, (int)(sizeof(aBuf))-1, &ClientID))
	{
		dbg_assert(m_aClients[ClientID].m_State != CClient::STATE_EMPTY, "got message from empty slot");
		if(m_aClients[ClientID].m_State == CClient::STATE_CONNECTED)
		{
			if(str_comp(aBuf, g_Config.m_EcPassword) == 0)
			{
				m_aClients[ClientID].m_State = CClient::STATE_AUTHED;
				m_NetConsole.Send(ClientID, "Authentication successful. External console access granted.");

				str_format(aBuf, sizeof(aBuf), "cid=%d authed", ClientID);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "econ", aBuf);
			}
			else
			{
				m_aClients[ClientID].m_AuthTries++;
				char aMsg[128];
				str_format(aMsg, sizeof(aMsg), "Wrong password %d/%d.", m_aClients[ClientID].m_AuthTries, MAX_AUTH_TRIES);
				m_NetConsole.Send(ClientID, aMsg);
				if(m_aClients[ClientID].m_AuthTries >= MAX_AUTH_TRIES)
				{
					if(g_Config.m_EcBantime)
						m_NetConsole.NetBan()->BanAddr(m_NetConsole.ClientAddr(ClientID), g_Config.m_EcBantime*60, "Too many authentication tries");
					m_NetConsole.Drop(ClientID, "Too many authentication tries");
				}
			}
		}
		else if(m_aClients[ClientID].m_State == CClient::STATE_AUTHED)
		{
			char aFormatted[256];
			str_format(aFormatted, sizeof(aFormatted), "cid=%d cmd='%s'", ClientID, aBuf);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aFormatted);
			m_UserClientID = ClientID;
			Console()->ExecuteLine(aBuf);
			m_UserClientID = -1;
		}
	}

	for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; ++i)
	{
		if(m_aClients[i].m_State == CClient::STATE_CONNECTED &&
			time_get() > m_aClients[i].m_TimeConnected + g_Config.m_EcAuthTimeout * time_freq())
			m_NetConsole.Drop(i, "authentication timeout");
	}
}

void CEcon::Send(int ClientID, const char *pLine)
{
	if(!m_Ready)
		return;

	if(ClientID == -1)
	{
		for(int i = 0; i < NET_MAX_CONSOLE_CLIENTS; i++)
		{
			if(m_aClients[i].m_State == CClient::STATE_AUTHED)
				m_NetConsole.Send(i, pLine);
		}
	}
	else if(ClientID >= 0 && ClientID < NET_MAX_CONSOLE_CLIENTS && m_aClients[ClientID].m_State == CClient::STATE_AUTHED)
		m_NetConsole.Send(ClientID, pLine);
}

void CEcon::Shutdown()
{
	if(!m_Ready)
		return;

	m_NetConsole.Close();
}
