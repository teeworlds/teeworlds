#ifndef GAME_SERVER_WEBAPP_API_TOKEN_H
#define GAME_SERVER_WEBAPP_API_TOKEN_H

#include <game/data.h>

class CWebApiToken
{
public:
	class CParam : public IDataIn
	{
	public:
		char m_aUsername[64];
		char m_aPassword[64];
	};
	
	class COut : public IDataOut
	{
	public:
		COut(int Type) { m_Type = Type; }
		char m_aApiToken[32];
	};
	
	static int GetApiToken(void *pUserData);
};

#endif
