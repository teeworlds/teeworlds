#ifndef GAME_HTTP_RESPONSE_H
#define GAME_HTTP_RESPONSE_H

#include "http_base.h"

class CResponse : public IHttpBase
{
	char *m_pData;
	int m_HeaderSize;
	int m_Size;
	int m_StatusCode;

	int ParseHeader();

public:
	CResponse() : m_pData(0), m_HeaderSize(-1), m_Size(0), m_StatusCode(0) {}
	~CResponse();

	int Write(char *pData, int Size);
	bool Finish();

	char *GetBody() { return m_Finish ? m_pData + m_HeaderSize : 0; }
	int Size() { return m_Finish ? m_Size - m_HeaderSize : -1; }
	int StatusCode() { return m_Finish ? m_StatusCode : -1; }
};

#endif
