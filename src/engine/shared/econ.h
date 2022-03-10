/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_ECON_H
#define ENGINE_SHARED_ECON_H

#include "network.h"


class CEcon
{
	enum
	{
		MAX_AUTH_TRIES=3,
	};

	class CClient
	{
	public:
		enum
		{
			STATE_EMPTY=0,
			STATE_CONNECTED,
			STATE_AUTHED,
		};

		int m_State;
		int64 m_TimeConnected;
		int m_AuthTries;
	};
	CClient m_aClients[NET_MAX_CONSOLE_CLIENTS];

	CConfig *m_pConfig;
	IConsole *m_pConsole;
	CNetBan *m_pNetBan;
	CNetConsole m_NetConsole;

	bool m_Ready;
	int64 m_LastOpenTry;
	int m_PrintCBIndex;
	int m_UserClientID;

	void SetDefaultValues();

	static void SendLineCB(const char *pLine, void *pUserData, bool Highlighted);
	static void ConchainEconOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainEconLingerUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConLogout(IConsole::IResult *pResult, void *pUserData);

	static int NewClientCallback(int ClientID, void *pUser);
	static int DelClientCallback(int ClientID, const char *pReason, void *pUser);

public:
	IConsole *Console() { return m_pConsole; }

	void Init(CConfig *pConfig, IConsole *pConsole, class CNetBan *pNetBan);
	bool Open();
	void Update();
	void Send(int ClientID, const char *pLine);
	void Shutdown();
};

#endif
