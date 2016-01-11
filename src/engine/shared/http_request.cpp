#include <base/system.h>
#include <base/math.h>

#include "http.h"

IRequest::IRequest(int Method, const char *pURI, int State) : m_Method(Method), m_State(State), m_pCur(0), m_pEnd(0)
{
	AddField("Connection", "Keep-Alive");
	str_copy(m_aURI, pURI, sizeof(m_aURI));
}

IRequest::~IRequest() { }

int IRequest::AddToHeader(char *pCur, const char *pData, int Size)
{
	Size = min((int)(m_aHeader + sizeof(m_aHeader) - pCur), Size);
	mem_copy(pCur, pData, Size);
	return Size;
}

int IRequest::GenerateHeader()
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

CBufferRequest::CBufferRequest(int Method, const char *pURI) : IRequest(Method, pURI, STATE_HEADER), m_pBody(0), m_BodySize(0) { }

CBufferRequest::~CBufferRequest()
{
	if(m_pBody)
		mem_free(m_pBody);
}

bool CBufferRequest::Finalize()
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
	return true;
}

bool CBufferRequest::SetBody(const char *pData, int Size, const char *pContentType)
{
	if(Size <= 0 || m_pBody || IsFinalized() || m_Method == HTTP_GET)
		return false;

	AddField("Content-Type", pContentType);
	m_BodySize = Size;
	m_pBody = (char *)mem_alloc(m_BodySize, 1);
	mem_copy(m_pBody, pData, m_BodySize);
	return true;
}

int CBufferRequest::GetData(char *pBuf, int MaxSize)
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

CFileRequest::CFileRequest(const char *pURI) : IRequest(HTTP_POST, pURI, STATE_HEADER), m_File(0) { }

CFileRequest::~CFileRequest()
{
	if(m_File)
		io_close(m_File);
}

const char *CFileRequest::GetFilename(const char *pFilename) const
{
	const char *pShort = pFilename;
	for(const char *pCur = pShort; *pCur; pCur++)
	{
		if (*pCur == '/' || *pCur == '\\')
			pShort = pCur + 1;
	}
	return pShort;
}

bool CFileRequest::Finalize()
{
	if(IsFinalized() || !m_File)
		return false;
	AddField("Content-Length", io_length(m_File) + str_length(m_aUploadHeader) + str_length(m_aUploadFooter));
	int HeaderSize = GenerateHeader();
	m_pCur = m_aHeader;
	m_pEnd = m_aHeader + HeaderSize;
	IHttpBase::Finalize();
	return true;
}

bool CFileRequest::SetFile(IOHANDLE File, const char *pFilename, const char *pUploadName)
{
	if(!File || m_File || IsFinalized())
		return false;

	m_File = File;
	str_format(m_aUploadHeader, sizeof(m_aUploadHeader),
		"--frontier\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n",
		pUploadName, GetFilename(pFilename));
	str_copy(m_aUploadFooter, "\r\n--frontier--\r\n", sizeof(m_aUploadFooter));
	AddField("Content-Type", "multipart/form-data; boundary=frontier");
	return true;
}

int CFileRequest::GetData(char *pBuf, int MaxSize)
{
	if(!IsFinalized())
		return -1;

	if(m_pCur >= m_pEnd)
	{
		if(m_State == STATE_HEADER)
		{
			m_pCur = m_aUploadHeader;
			m_pEnd = m_pCur + str_length(m_aUploadHeader);
			m_State = STATE_FILE_HEADER;
		}
		else if(m_State == STATE_FILE_HEADER)
		{
			m_State = STATE_FILE_BODY;
		}
		else if(m_State == STATE_FILE_FOOTER)
		{
			return 0;
		}
	}

	if(m_State == STATE_FILE_BODY)
	{
		int Size = io_read(m_File, pBuf, MaxSize);
		if (Size > 0)
			return Size;

		io_close(m_File);
		m_File = 0;
		m_pCur = m_aUploadFooter;
		m_pEnd = m_pCur + str_length(m_aUploadFooter);
		m_State = STATE_FILE_FOOTER;
	}
	
	if(m_State != STATE_FILE_BODY)
	{
		int Size = min((int)(m_pEnd - m_pCur), MaxSize);
		mem_copy(pBuf, m_pCur, Size);
		m_pCur += Size;
		return Size;
	}
	return -1;
}
