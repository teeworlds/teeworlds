/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
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

		int CountryCode = str_toint(pReplacement+3);
		if(CountryCode < CODE_LB || CountryCode > CODE_UB)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "country code '%i' not within valid code range [%i..%i]", CountryCode, CODE_LB, CODE_UB);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aBuf);
			continue;
		}

		// load the graphic file
		char aBuf[128];
		CImageInfo Info;
		if(g_Config.m_ClLoadCountryFlags)
		{
			str_format(aBuf, sizeof(aBuf), "countryflags/%s.png", aOrigin);
			if(!Graphics()->LoadPNG(&Info, aBuf, IStorage::TYPE_ALL))
			{
				char aMsg[128];
				str_format(aMsg, sizeof(aMsg), "failed to load '%s'", aBuf);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aMsg);
				continue;
			}
		}

		// add entry
		CCountryFlag CountryFlag;
		CountryFlag.m_CountryCode = CountryCode;
		str_copy(CountryFlag.m_aCountryCodeString, aOrigin, sizeof(CountryFlag.m_aCountryCodeString));
		if(g_Config.m_ClLoadCountryFlags)
		{
			CountryFlag.m_Texture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
			mem_free(Info.m_pData);
		}
		else
			CountryFlag.m_Texture = -1;
		if(g_Config.m_Debug)
		{
			str_format(aBuf, sizeof(aBuf), "loaded country flag '%s'", aOrigin);
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "countryflags", aBuf);
		}
		m_aCountryFlags.add_unsorted(CountryFlag);
	}
	io_close(File);
	m_aCountryFlags.sort_range();

	// find index of default item
	int DefaultIndex = 0, Index = 0;
	for(sorted_array<CCountryFlag>::range r = m_aCountryFlags.all(); !r.empty(); r.pop_front(), ++Index)
		if(r.front().m_CountryCode == -1)
		{
			DefaultIndex = Index;
			break;
		}
	
	// init LUT
	if(DefaultIndex != 0)
		for(int i = 0; i < CODE_RANGE; ++i)
			m_CodeIndexLUT[i] = DefaultIndex;
	else
		mem_zero(m_CodeIndexLUT, sizeof(m_CodeIndexLUT));
	for(int i = 0; i < m_aCountryFlags.size(); ++i)
		m_CodeIndexLUT[max(0, (m_aCountryFlags[i].m_CountryCode-CODE_LB)%CODE_RANGE)] = i;
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
		mem_zero(DummyEntry.m_aCountryCodeString, sizeof(DummyEntry.m_aCountryCodeString));
		m_aCountryFlags.add(DummyEntry);
	}
}

int CCountryFlags::Num() const
{
	return m_aCountryFlags.size();
}

const CCountryFlags::CCountryFlag *CCountryFlags::GetByCountryCode(int CountryCode) const
{
	return GetByIndex(m_CodeIndexLUT[max(0, (CountryCode-CODE_LB)%CODE_RANGE)]);
}

const CCountryFlags::CCountryFlag *CCountryFlags::GetByIndex(int Index) const
{
	return &m_aCountryFlags[max(0, Index%m_aCountryFlags.size())];
}

void CCountryFlags::Render(int CountryCode, const vec4 *pColor, float x, float y, float w, float h)
{
	const CCountryFlag *pFlag = GetByCountryCode(CountryCode);
	if(pFlag->m_Texture != -1)
	{
		Graphics()->TextureSet(pFlag->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(pColor->r, pColor->g, pColor->b, pColor->a);
		IGraphics::CQuadItem QuadItem(x, y, w, h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	else
	{
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, 10.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = w;
		TextRender()->TextEx(&Cursor, pFlag->m_aCountryCodeString, -1);
	}
}
