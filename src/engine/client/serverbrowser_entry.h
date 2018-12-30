/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SERVERBROWSER_ENTRY_H
#define ENGINE_CLIENT_SERVERBROWSER_ENTRY_H

class CServerEntry
{
public:
	enum
	{
		STATE_INVALID=0,
		STATE_PENDING,
		STATE_READY,
	};

	NETADDR m_Addr;
	int64 m_RequestTime;
	int m_InfoState;
	int m_CurrentToken;	// the token is to keep server refresh separated from each other
	int m_TrackID;
	class CServerInfo m_Info;

	CServerEntry *m_pNextIp; // ip hashed list

	CServerEntry *m_pPrevReq; // request list
	CServerEntry *m_pNextReq;
};

#endif
