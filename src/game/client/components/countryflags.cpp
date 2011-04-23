/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/linereader.h>

#include "countryflags.h"


void CCountryFlags::LoadCountryflagsIndexfile()
{
	IOHANDLE File = Storage()->OpenFile("countryflags/index.txt", IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", "couldn't open index file");
		return;
	}

	char aOrigin[128];
	CLineReader LineReader;
	LineReader.Init(File);
	char *pLine;
	while((pLine = LineReader.Get()))
	{
		if(!str_length(pLine) || pLine[0] == '#') // skip empty lines and comments
			continue;

		str_copy(aOrigin, pLine, sizeof(aOrigin));
		char *pReplacement = LineReader.Get();
		if(!pReplacement)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", "unexpected end of index file");
			break;
		}

		if(pReplacement[0] != '=' || pReplacement[1] != '=' || pReplacement[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aBuf);
			continue;
		}

		// load the graphic file
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "countryflags/%s.png", aOrigin);
		CImageInfo Info;
		if(!Graphics()->LoadPNG(&Info, aBuf, IStorage::TYPE_ALL))
		{
			char aMsg[128];
			str_format(aMsg, sizeof(aMsg), "failed to load '%s'", aBuf);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aMsg);
			continue;
		}

		// add entry
		CCountryFlag CountryFlag;
		CountryFlag.m_CountryCode = str_toint(pReplacement+3);
		CountryFlag.m_Texture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
		mem_free(Info.m_pData);
		str_format(aBuf, sizeof(aBuf), "loaded country flag '%s'", aOrigin);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aBuf);
		m_aCountryFlags.add(CountryFlag);
	}
	io_close(File);
}

void CCountryFlags::OnInit()
{
	// load country flags
	m_aCountryFlags.clear();
	LoadCountryflagsIndexfile();
	if(!m_aCountryFlags.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "countryflags", "failed to load country flags. folder='countryflags/'");
		CCountryFlag DummyEntry;
		DummyEntry.m_CountryCode = -1;
		DummyEntry.m_Texture = -1;
		m_aCountryFlags.add(DummyEntry);
	}
}

int CCountryFlags::Num() const
{
	return m_aCountryFlags.size();
}

const CCountryFlags::CCountryFlag *CCountryFlags::Get(int Index) const
{
	return &m_aCountryFlags[max(0, Index%m_aCountryFlags.size())];
}

int CCountryFlags::Find(int CountryCode) const
{
	for(int i = 0; i < m_aCountryFlags.size(); ++i)
	{
		if(m_aCountryFlags[i].m_CountryCode == CountryCode)
			return i;
	}
	return -1;
}
