#ifndef GAME_HTTP_REQUEST_H
#define GAME_HTTP_REQUEST_H

#include "http_base.h"

class CRequest : public IHttpBase
{
	enum
	{
		STATE_NONE = 0,
		STATE_HEADER,
		STATE_BODY,
		STATE_FILE_START,
		STATE_FILE,
		STATE_FILE_END
	};

	char m_aURI[256];
	int m_Method;
	int m_State;

	char m_aHeader[1024*2];
	char *m_pBody;
	int m_BodySize;

	char m_aUploadHeader[256];
	char m_aUploadFooter[256];

	char *m_pCur;
	char *m_pEnd;

	int64 m_StartTime;

	void GenerateHeader();

public:
	enum
	{
		HTTP_GET = 0,
		HTTP_POST,
		HTTP_PUT
	};

	CRequest(const char *pHost, const char *pURI, int Method);
	~CRequest();

	const char *GetURI() { return m_aURI; }

	bool Finish();
	bool SetBody(const char *pData, int Size, const char *pContentType = "application/json");
	void SetFile(IOHANDLE File, const char *pFilename, const char *pUploadName);
	int GetData(char *pBuf, int MaxSize);
	void MoveCursor(int Bytes);

	void SetStartTime(int64 StartTime) { m_StartTime = StartTime; }
	int64 StartTime() { return m_StartTime; }
};

#endif
