/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "packer.h"
#include "compression.h"
#include "config.h"

void CPacker::Reset()
{
	m_Error = 0;
	m_pCurrent = m_aBuffer;
	m_pEnd = m_pCurrent + PACKER_BUFFER_SIZE;
}

void CPacker::AddInt(int i)
{
	if(m_Error)
		return;

	unsigned char *pNext = CVariableInt::Pack(m_pCurrent, i, m_pEnd - m_pCurrent);
	if(!pNext)
	{
		m_Error = 1;
		return;
	}
	m_pCurrent = pNext;
}

void CPacker::AddString(const char *pStr, int Limit)
{
	if(m_Error)
		return;

	if(Limit <= 0)
	{
		Limit = PACKER_BUFFER_SIZE;
	}
	while(*pStr && Limit != 0)
	{
		int Codepoint = str_utf8_decode(&pStr);
		if(Codepoint == -1)
		{
			Codepoint = 0xfffd; // Unicode replacement character.
		}
		char aGarbage[4];
		int Length = str_utf8_encode(aGarbage, Codepoint);
		if(Limit < Length)
		{
			break;
		}
		// Ensure space for the null termination.
		if(m_pEnd - m_pCurrent < Length + 1)
		{
			m_Error = 1;
			break;
		}
		Length = str_utf8_encode((char *)m_pCurrent, Codepoint);
		m_pCurrent += Length;
		Limit -= Length;
	}
	*m_pCurrent++ = 0;
}

void CPacker::AddRaw(const void *pData, int Size)
{
	if(m_Error)
		return;

	if(m_pCurrent+Size >= m_pEnd)
	{
		m_Error = 1;
		return;
	}

	const unsigned char *pSrc = (const unsigned char *)pData;
	while(Size)
	{
		*m_pCurrent++ = *pSrc++;
		Size--;
	}
}


void CUnpacker::Reset(const void *pData, int Size)
{
	m_Error = 0;
	m_pStart = (const unsigned char *)pData;
	m_pEnd = m_pStart + Size;
	m_pCurrent = m_pStart;
}

int CUnpacker::GetInt()
{
	if(m_Error)
		return 0;

	if(m_pCurrent >= m_pEnd)
	{
		m_Error = 1;
		return 0;
	}

	int i;
	const unsigned char *pNext = CVariableInt::Unpack(m_pCurrent, &i, m_pEnd - m_pCurrent);
	if(!pNext)
	{
		m_Error = 1;
		return 0;
	}
	m_pCurrent = pNext;
	return i;
}

int CUnpacker::GetIntOrDefault(int Default)
{
	if(m_Error)
	{
		return 0;
	}
	if(m_pCurrent == m_pEnd)
	{
		return Default;
	}
	return GetInt();
}

const char *CUnpacker::GetString(int SanitizeType)
{
	if(m_Error || m_pCurrent >= m_pEnd)
		return "";

	char *pPtr = (char *)m_pCurrent;
	while(*m_pCurrent) // skip the string
	{
		m_pCurrent++;
		if(m_pCurrent == m_pEnd)
		{
			m_Error = 1;;
			return "";
		}
	}
	m_pCurrent++;

	// sanitize all strings
	if(SanitizeType&SANITIZE)
		str_sanitize(pPtr);
	else if(SanitizeType&SANITIZE_CC)
		str_sanitize_cc(pPtr);
	return SanitizeType&SKIP_START_WHITESPACES ? str_utf8_skip_whitespaces(pPtr) : pPtr;
}

const unsigned char *CUnpacker::GetRaw(int Size)
{
	const unsigned char *pPtr = m_pCurrent;
	if(m_Error)
		return 0;

	// check for nasty sizes
	if(Size < 0 || m_pCurrent+Size > m_pEnd)
	{
		m_Error = 1;
		return 0;
	}

	// "unpack" the data
	m_pCurrent += Size;
	return pPtr;
}
