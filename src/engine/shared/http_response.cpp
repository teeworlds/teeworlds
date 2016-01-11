#include <base/system.h>
#include <base/math.h>

#include "http.h"

// TODO: download to file
IResponse::IResponse() : m_LastWasValue(false), m_Complete(false), m_Close(false), m_Size(0)
{
	mem_zero(&m_ParserSettings, sizeof(m_ParserSettings));
	m_ParserSettings.on_header_field = OnHeaderField;
	m_ParserSettings.on_header_value = OnHeaderValue;
	m_ParserSettings.on_message_complete = OnMessageComplete;

	http_parser_init(&m_Parser, HTTP_RESPONSE);
	m_Parser.data = this;

	mem_zero(&m_CurField, sizeof(m_CurField));
}

IResponse::~IResponse() { }

int IResponse::OnHeaderField(http_parser *pParser, const char *pData, size_t Len)
{
	IResponse *pSelf = (IResponse*)pParser->data;
	if(pSelf->m_LastWasValue)
	{
		pSelf->AddField(pSelf->m_CurField);
		mem_zero(&pSelf->m_CurField, sizeof(pSelf->m_CurField));
		pSelf->m_LastWasValue = false;
	}
	unsigned MinLen = str_length(pSelf->m_CurField.m_aKey) + Len + 1;
	str_append(pSelf->m_CurField.m_aKey, pData, min((unsigned)sizeof(pSelf->m_CurField.m_aKey), MinLen));
	return 0;
}

int IResponse::OnHeaderValue(http_parser *pParser, const char *pData, size_t Len)
{
	IResponse *pSelf = (IResponse*)pParser->data;
	unsigned MinLen = str_length(pSelf->m_CurField.m_aValue) + Len + 1;
	str_append(pSelf->m_CurField.m_aValue, pData, min((unsigned)sizeof(pSelf->m_CurField.m_aValue), MinLen));
	pSelf->m_LastWasValue = true;
	return 0;
}

int IResponse::OnMessageComplete(http_parser *pParser)
{
	IResponse *pSelf = (IResponse*)pParser->data;
	pSelf->m_Complete = true;
	if(http_should_keep_alive(pParser) == 0)
		pSelf->m_Close = true;
	return 0;
}

bool IResponse::Write(char *pData, int Size)
{
	if(Size < 0 || IsFinalized())
		return false;
	size_t Parsed = http_parser_execute(&m_Parser, &m_ParserSettings, pData, Size);
	return Parsed == (size_t)Size;
}

bool IResponse::Finalize()
{
	//dbg_msg("http/response", "finishing (size: %d, buffer: %d)", m_Size, m_BufferSize);
	if(IsFinalized() || !m_Complete)
		return false;
	IHttpBase::Finalize();
	return true;
}

CBufferResponse::CBufferResponse() : IResponse(), m_pData(0), m_BufferSize(0)
{
	m_ParserSettings.on_body = OnBody;
	m_ParserSettings.on_headers_complete = OnHeadersComplete;
}

CBufferResponse::~CBufferResponse()
{
	if (m_pData)
		mem_free(m_pData);
}

int CBufferResponse::OnBody(http_parser *pParser, const char *pData, size_t Len)
{
	CBufferResponse *pSelf = (CBufferResponse*)pParser->data;
	if (pSelf->m_Size + Len > pSelf->m_BufferSize)
		pSelf->ResizeBuffer(pSelf->m_Size + Len);
	mem_copy(pSelf->m_pData + pSelf->m_Size, pData, Len);
	pSelf->m_Size += Len;
	return 0;
}

int CBufferResponse::OnHeadersComplete(http_parser *pParser)
{
	CBufferResponse *pSelf = (CBufferResponse*)pParser->data;
	const char *pLen = pSelf->GetField("Content-Length");
	if (pLen)
		pSelf->ResizeBuffer(str_toint(pLen));
	return 0;
}

bool CBufferResponse::ResizeBuffer(int NeededSize)
{
	if (NeededSize < m_BufferSize || NeededSize <= 0)
		return false;
	else if (NeededSize == m_BufferSize)
		return true;
	m_BufferSize = NeededSize;

	if (m_pData)
	{
		char *pTmp = (char *)mem_alloc(m_BufferSize, 1);
		mem_copy(pTmp, m_pData, m_Size);
		mem_free(m_pData);
		m_pData = pTmp;
	}
	else
		m_pData = (char *)mem_alloc(m_BufferSize, 1);
	return true;
}

CFileResponse::CFileResponse(IOHANDLE File) : IResponse(), m_File(File)
{
	m_ParserSettings.on_body = OnBody;
}

CFileResponse::~CFileResponse()
{
	io_close(m_File);
}

int CFileResponse::OnBody(http_parser *pParser, const char *pData, size_t Len)
{
	CFileResponse *pSelf = (CFileResponse*)pParser->data;
	io_write(pSelf->m_File, pData, Len);
	pSelf->m_Size += Len;
	return 0;
}
