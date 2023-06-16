/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include "compression.h"
#include "packer.h"

void CPacker::Reset()
{
	m_Error = false;
	m_pCurrent = m_aBuffer;
	m_pEnd = m_pCurrent + PACKER_BUFFER_SIZE;
}

void CPacker::AddInt(int Integer)
{
	if(m_Error)
		return;

	unsigned char *pNext = CVariableInt::Pack(m_pCurrent, Integer, RemainingSize());
	if(!pNext)
	{
		m_Error = true;
		return;
	}
	m_pCurrent = pNext;
}

void CPacker::AddString(const char *pStr, int Limit)
{
	if(m_Error)
		return;

	if(Limit <= 0)
		Limit = PACKER_BUFFER_SIZE;

	while(*pStr && Limit != 0)
	{
		int Codepoint = str_utf8_decode(&pStr);
		if(Codepoint == -1)
			Codepoint = 0xfffd; // Unicode replacement character.

		char aEncoded[4];
		const int Length = str_utf8_encode(aEncoded, Codepoint);
		if(Limit < Length)
			break;

		// Ensure space for the null termination.
		if(RemainingSize() < Length + 1)
		{
			m_Error = true;
			break;
		}
		mem_copy(m_pCurrent, aEncoded, Length);
		m_pCurrent += Length;
		Limit -= Length;
	}
	*m_pCurrent++ = 0;
}

void CPacker::AddRaw(const void *pData, int Size)
{
	if(m_Error)
		return;

	if(Size <= 0 || Size >= RemainingSize())
	{
		m_Error = true;
		return;
	}

	mem_copy(m_pCurrent, pData, Size);
	m_pCurrent += Size;
}


void CUnpacker::Reset(const void *pData, int Size)
{
	m_Error = false;
	m_pStart = (const unsigned char *)pData;
	m_pEnd = m_pStart + Size;
	m_pCurrent = m_pStart;
}

int CUnpacker::GetInt()
{
	if(m_Error)
		return 0;

	if(RemainingSize() <= 0)
	{
		m_Error = true;
		return 0;
	}

	int Integer;
	const unsigned char *pNext = CVariableInt::Unpack(m_pCurrent, &Integer, RemainingSize());
	if(!pNext)
	{
		m_Error = true;
		return 0;
	}
	m_pCurrent = pNext;
	return Integer;
}

int CUnpacker::GetIntOrDefault(int Default)
{
	if(m_Error)
		return 0;

	if(RemainingSize() == 0)
		return Default;

	return GetInt();
}

const char *CUnpacker::GetString(int SanitizeType)
{
	if(m_Error)
		return "";

	if(RemainingSize() <= 0)
	{
		m_Error = true;
		return "";
	}

	// Ensure string is null terminated.
	char *pStr = (char *)m_pCurrent;
	while(*m_pCurrent)
	{
		m_pCurrent++;
		if(m_pCurrent == m_pEnd)
		{
			m_Error = true;
			return "";
		}
	}
	m_pCurrent++;

	// sanitize all strings
	if(SanitizeType & SANITIZE)
		str_sanitize(pStr);
	else if(SanitizeType & SANITIZE_CC)
		str_sanitize_cc(pStr);

	if(SanitizeType & SKIP_START_WHITESPACES)
		return str_utf8_skip_whitespaces(pStr);

	return pStr;
}

const unsigned char *CUnpacker::GetRaw(int Size)
{
	if(m_Error)
		return 0;

	// check for nasty sizes
	if(Size <= 0 || Size > RemainingSize())
	{
		m_Error = true;
		return 0;
	}

	// "unpack" the data
	const unsigned char *pPtr = m_pCurrent;
	m_pCurrent += Size;
	return pPtr;
}
