// copyright (c) 2007 magnus auvinen, see licence.txt for more info

#include "localization.h"
#include <base/tl/algorithm.h>

#include <engine/shared/linereader.h>
#include <engine/console.h>

const char *Localize(const char *pStr)
{
	const char *pNewStr = g_Localization.FindString(str_quickhash(pStr));
	return pNewStr ? pNewStr : pStr;
}

CLocConstString::CLocConstString(const char *pStr)
{
	m_pDefaultStr = pStr;
	m_Hash = str_quickhash(m_pDefaultStr);
	m_Version = -1;
}

void CLocConstString::Reload()
{
	m_Version = g_Localization.Version();
	const char *pNewStr = g_Localization.FindString(m_Hash);
	m_pCurrentStr = pNewStr;
	if(!m_pCurrentStr)
		m_pCurrentStr = m_pDefaultStr;
}

CLocalizationDatabase::CLocalizationDatabase()
{
	m_CurrentVersion = 0;
}

void CLocalizationDatabase::AddString(const char *pOrgStr, const char *pNewStr)
{
	CString s;
	s.m_Hash = str_quickhash(pOrgStr);
	s.m_Replacement = pNewStr;
	m_Strings.add(s);
}

bool CLocalizationDatabase::Load(const char *pFilename, IConsole *pConsole)
{
	// empty string means unload
	if(pFilename[0] == 0)
	{
		m_Strings.clear();
		m_CurrentVersion = 0;
		return true;
	}
	
	FILE *IoHandle = io_open(pFilename, IOFLAG_READ);
	if(!IoHandle)
		return false;
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded '%s'", pFilename);
	pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
	m_Strings.clear();
	
	CLineReader LineReader;
	LineReader.Init(IoHandle);
	char *pLine;
	while((pLine = LineReader.Get()) != 0)
	{
		if(!str_length(pLine))
			continue;
			
		if(pLine[0] == '#') // skip comments
			continue;
			
		char *pReplacement = LineReader.Get();
		if(!pReplacement)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of file");
			break;
		}
		
		if(pReplacement[0] != '=' || pReplacement[1] != '=' || pReplacement[2] != ' ')
		{
			str_format(aBuf, sizeof(aBuf), "malform replacement line for '%s'", pLine);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			continue;
		}

		pReplacement += 3;
		AddString(pLine, pReplacement);
	}
	
	m_CurrentVersion++;
	return true;
}

const char *CLocalizationDatabase::FindString(unsigned Hash)
{
	CString String;
	String.m_Hash = Hash;
	sorted_array<CString>::range r = ::find_binary(m_Strings.all(), String);
	if(r.empty())
		return 0;
	return r.front().m_Replacement;
}

CLocalizationDatabase g_Localization;
