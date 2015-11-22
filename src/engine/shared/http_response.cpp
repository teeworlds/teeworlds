#include <stdio.h>

#include <base/system.h>

#include "http.h"

// TODO: download to file
// TODO: support for chunked transfer-encoding?
CResponse::CResponse() : m_pData(0), m_HeaderSize(-1), m_BufferSize(0), m_Size(0), m_StatusCode(0), m_ContentLength(-1) { }

CResponse::~CResponse()
{
	if(m_pData)
		mem_free(m_pData);
}

// TODO: rework this
int CResponse::ParseHeader()
{
	char *pData = m_pData;
	char *pEnd = (char*)str_find(pData, "\r\n\r\n");
	if(!pEnd)
		return 0;
	*(pEnd+2) = 0;

	if(sscanf(pData, "HTTP/%*d.%*d %d %*s\r\n", &this->m_StatusCode) != 1)
		return -1;
	pData = (char*)str_find(pData, "\r\n") + 2;

	while(*pData)
	{
		char *pKey = (char*)str_find(pData, ": ");
		if(!pKey) break;
		char *pVal = (char*)str_find(pKey+2, "\r\n");
		if(!pVal) break;
		
		*pKey = 0;
		*pVal = 0;
		AddField(pData, pKey+2);
		pData = pVal+2;
	}
	
	int HeaderSize = (pEnd-m_pData)+4;
	const char *pLen = GetField("Content-Length");
	if(!pLen)
		return -1;
	m_ContentLength = str_toint(pLen);
	if(!ResizeBuffer(HeaderSize + m_ContentLength))
		return -1;
	
	m_HeaderSize = HeaderSize;
	dbg_msg("http/response", "header finished");
	return 1;
}

bool CResponse::ResizeBuffer(int NeededSize)
{
	dbg_msg("http/response", "resizing buffer: %d -> %d", m_BufferSize, NeededSize);
	if(NeededSize < m_BufferSize || NeededSize <= 0)
		return false;
	else if(NeededSize == m_BufferSize)
		return true;
	m_BufferSize = NeededSize;
	
	if(m_pData)
	{
		char *pTmp = (char *)mem_alloc(m_BufferSize+1, 1);
		mem_copy(pTmp, m_pData, m_Size);
		mem_free(m_pData);
		m_pData = pTmp;
	}
	else
		m_pData = (char *)mem_alloc(m_BufferSize+1, 1);
	
	// terminate for string functions
	m_pData[m_BufferSize] = 0;
	return true;
}

bool CResponse::Write(char *pData, int Size)
{
	if(Size <= 0 || IsFinalized())
		return false;
	
	dbg_msg("http/response", "writing data: %d (%d of %d used)", Size, m_Size, m_BufferSize);
	
	if(m_Size + Size > m_BufferSize)
		ResizeBuffer(m_Size + Size);
	
	mem_copy(m_pData+m_Size, pData, Size);
	m_Size += Size;

	if(m_HeaderSize <= 0)
	{
		if(ParseHeader() == -1)
			return false;
	}
	return true;
}

bool CResponse::IsComplete()
{
	return m_HeaderSize > 0 && m_ContentLength > 0 && m_Size == m_HeaderSize + m_ContentLength;
}

bool CResponse::Finalize()
{
	dbg_msg("http/response", "finishing (header: %d, size: %d, buffer: %d)", m_HeaderSize, m_Size, m_BufferSize);
	if(IsFinalized() || !IsComplete())
		return false;
	IHttpBase::Finalize();
	return true;
}
