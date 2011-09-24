#include <base/math.h>
#include <stdio.h>

#include "response.h"

CResponse::~CResponse()
{
	if(m_pData)
		mem_free(m_pData);
}

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

		CHttpField Tmp;
		str_copy(Tmp.m_aKey, pData, min((int)sizeof(Tmp.m_aKey), (int)(pKey-pData+1)));
		str_copy(Tmp.m_aValue, pKey+2, min((int)sizeof(Tmp.m_aValue), (int)(pVal-pKey-1)));
		AddField(Tmp);
		pData = pVal+2;
	}

	if(!GetField("Content-Length"))
		return -1;

	m_HeaderSize = (pEnd-m_pData)+4;
	if(m_File)
		io_write(m_File, m_pData+m_HeaderSize, m_Size-m_HeaderSize);
	return 1;
}

int CResponse::Write(char *pData, int Size)
{
	if(Size <= 0 || m_Finish)
		return 0;

	if(m_File && m_HeaderSize > 0)
	{
		io_write(m_File, pData, Size);
	}
	else
	{
		if(m_pData)
		{
			char *pTmp = (char *)mem_alloc(m_Size + Size+1, 1);
			mem_copy(pTmp, m_pData, m_Size);
			mem_free(m_pData);
			m_pData = pTmp;
		}
		else
			m_pData = (char *)mem_alloc(Size+1, 1);
		mem_copy(m_pData+m_Size, pData, Size);
		m_pData[m_Size+Size] = 0;
	}
	m_Size += Size;

	if(m_HeaderSize <= 0)
	{
		if(ParseHeader() == -1)
			return -1;
	}
	return 1;
}

bool CResponse::Finish()
{
	if(m_HeaderSize <= 0 || m_Size - m_HeaderSize != str_toint(GetField("Content-Length")))
		return false;
	CloseFiles();
	m_Finish = true;
	return true;
}
