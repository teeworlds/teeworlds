/* Webapp Class by Sushi and Redix */
#ifndef GAME_HTTP_CON_H
#define GAME_HTTP_CON_H

#include <base/system.h>

class CHttpConnection
{
	enum
	{
		STATE_NONE = 0,
		STATE_CONNECT,
		STATE_WAIT,
		STATE_SEND,
		STATE_RECV,
		STATE_DONE,
		STATE_ERROR
	};

	NETSOCKET m_Socket;
	NETADDR m_Addr;
	int m_State;
	int m_Type;

	int64 m_LastActionTime;

	class CResponse *m_pResponse;
	class CRequest *m_pRequest;
	class CWebData *m_pUserData;

	int SetState(int State, const char *pMsg = 0);
	
public:
	CHttpConnection();
	~CHttpConnection();
	
	bool Create(NETADDR Addr, int Type);
	int Update();
	void CloseFiles();
	void Close();

	bool Error() { return m_State == STATE_ERROR; }
	int Type() { return m_Type; }
	CRequest *Request() { return m_pRequest; }
	CResponse *Response() { return m_pResponse; }
	CWebData *UserData() { return m_pUserData; }

	void SetRequest(CRequest *pRequest) { m_pRequest = pRequest; }
	void SetResponse(CResponse *pResponse) { m_pResponse = pResponse; }
	void SetUserData(CWebData *pUserData) { m_pUserData = pUserData; }
};

#endif
