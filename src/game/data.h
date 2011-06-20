#ifndef GAME_SERVER_WEBAPP_DATA_H
#define GAME_SERVER_WEBAPP_DATA_H

enum
{
	/*// server
	WEB_USER_AUTH = 0,
	WEB_USER_RANK,
	WEB_USER_TOP,
	WEB_PING_PING,
	WEB_MAP_LIST,
	WEB_MAP_DOWNLOADED,
	WEB_RUN,*/

	// client
	WEB_API_TOKEN = 0,
	
	UPLOAD_DEMO = 0,
	UPLOAD_GHOST
};

class IDataIn
{
public:
	virtual ~IDataIn() {}
	class CWebapp *m_pWebapp;
};

class IDataOut
{
public:
	virtual ~IDataOut() {}
	IDataOut *m_pNext;
	int m_Type;
};
	
#endif
