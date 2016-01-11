#include <base/system.h>
#include <base/math.h>

#include "http.h"

// TODO: file upload
CRequest::CRequest(int Method, const char *pURI)
	: m_Method(Method), m_State(STATE_HEADER), m_pBody(0), m_BodySize(0), m_pCur(0), m_pEnd(0)
{
	AddField("Connection", "Keep-Alive");
	str_copy(m_aURI, pURI, sizeof(m_aURI));
}

CRequest::~CRequest()
{
	if(m_pBody)
		mem_free(m_pBody);
}

int CRequest::AddToHeader(char *pCur, const char *pData, int Size)
{
	Size = min((int)(m_aHeader + sizeof(m_aHeader) - pCur), Size);
	mem_copy(pCur, pData, Size);
	return Size;
}

int CRequest::GenerateHeader()
{
	char aBuf[512];
	char *pCur = m_aHeader;
	
	const char *pMethod = "GET"; // default
	if(m_Method == HTTP_POST)
		pMethod = "POST";
	else if(m_Method == HTTP_PUT)
		pMethod = "PUT";
	
	str_format(aBuf, sizeof(aBuf), "%s %s%s HTTP/1.1\r\n", pMethod, m_aURI[0] == '/' ? "" : "/", m_aURI);
	pCur += AddToHeader(pCur, aBuf, str_length(aBuf));

	for(int i = 0; i < m_FieldNum; i++)
	{
		str_format(aBuf, sizeof(aBuf), "%s: %s\r\n", m_aFields[i].m_aKey, m_aFields[i].m_aValue);
		pCur += AddToHeader(pCur, aBuf, str_length(aBuf));
	}

	pCur += AddToHeader(pCur, "\r\n", 2);
	return pCur - m_aHeader;
}

bool CRequest::Finalize()
{
	if(IsFinalized())
		return false;
	if(m_Method != HTTP_GET)
	{
		if(!m_pBody)
			return false;
		AddField("Content-Length", m_BodySize);
	}
	int HeaderSize = GenerateHeader();
	m_pCur = m_aHeader;
	m_pEnd = m_aHeader + HeaderSize;
	IHttpBase::Finalize();
	//dbg_msg("http/request", "finished (header: %d, body: %d)", HeaderSize, m_BodySize);
	return true;
}

bool CRequest::InitBody(int Size, const char *pContentType)
{
	if(Size <= 0 || m_pBody || IsFinalized() || m_Method == HTTP_GET)
		return false;

	AddField("Content-Type", pContentType);
	m_BodySize = Size;
	m_pBody = (char *)mem_alloc(m_BodySize, 1);
	return true;
}

bool CRequest::SetBody(const char *pData, int Size, const char *pContentType)
{
	if(!InitBody(Size, pContentType))
		return false;
	
	mem_copy(m_pBody, pData, m_BodySize);
	return true;
}

const char *CRequest::GetFilename(const char *pFilename) const
{
	const char *pShort = pFilename;
	for(const char *pCur = pShort; *pCur; pCur++)
	{
		if(*pCur == '/' || *pCur == '\\')
			pShort = pCur+1;
	}
	return pShort;
}

int CRequest::GetData(char *pBuf, int MaxSize)
{
	if(!IsFinalized())
		return -1;

	if(m_pCur >= m_pEnd)
	{
		if(m_State == STATE_HEADER)
		{
			//dbg_msg("http/request", "sent header");
			if(!m_pBody)
				return 0;
			m_pCur = m_pBody;
			m_pEnd = m_pBody + m_BodySize;
			m_State = STATE_BODY;
		}
		else if(m_State == STATE_BODY)
		{
			//dbg_msg("http/request", "sent body");
			return 0;
		}
	}

	int Size = min((int)(m_pEnd-m_pCur), MaxSize);
	mem_copy(pBuf, m_pCur, Size);
	m_pCur += Size;
	return Size;
}
