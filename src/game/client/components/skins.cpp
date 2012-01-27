/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include "skins.h"

char *const gs_apSkinVariables[NUM_SKINPARTS] = {g_Config.m_PlayerSkinBody, g_Config.m_PlayerSkinTattoo, g_Config.m_PlayerSkinDecoration,
													g_Config.m_PlayerSkinHands, g_Config.m_PlayerSkinFeet, g_Config.m_PlayerSkinEyes};
int *const gs_apUCCVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerUseCustomColorBody, &g_Config.m_PlayerUseCustomColorTattoo, &g_Config.m_PlayerUseCustomColorDecoration,
													&g_Config.m_PlayerUseCustomColorHands, &g_Config.m_PlayerUseCustomColorFeet, &g_Config.m_PlayerUseCustomColorEyes};
int *const gs_apColorVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerColorBody, &g_Config.m_PlayerColorTattoo, &g_Config.m_PlayerColorDecoration,
													&g_Config.m_PlayerColorHands, &g_Config.m_PlayerColorFeet, &g_Config.m_PlayerColorEyes};

static const char *const gs_apFolders[NUM_SKINPARTS] = {"bodies", "tattoos", "decoration",
														"hands", "feet", "eyes"};

int CSkins::SkinPartScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CSkins *pSelf = (CSkins *)pUser;
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/%s/%s", gs_apFolders[pSelf->m_ScanningPart], pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load skin from %s", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return 0;
	}

	CSkinPart Part;
	Part.m_OrgTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);


	unsigned char *d = (unsigned char *)Info.m_pData;
	int Pitch = Info.m_Width*4;

	// dig out blood color
	if(pSelf->m_ScanningPart == SKINPART_BODY)
	{
		int PartX = Info.m_Width/2;
		int PartY = 0;
		int PartWidth = Info.m_Width/2;
		int PartHeight = Info.m_Height/2;

		int aColors[3] = {0};
		for(int y = PartY; y < PartY+PartHeight; y++)
			for(int x = PartX; x < PartX+PartWidth; x++)
			{
				if(d[y*Pitch+x*4+3] > 128)
				{
					aColors[0] += d[y*Pitch+x*4+0];
					aColors[1] += d[y*Pitch+x*4+1];
					aColors[2] += d[y*Pitch+x*4+2];
				}
			}

		Part.m_BloodColor = normalize(vec3(aColors[0], aColors[1], aColors[2]));
	}

	// create colorless version
	int Step = Info.m_Format == CImageInfo::FORMAT_RGBA ? 4 : 3;

	// make the texture gray scale
	for(int i = 0; i < Info.m_Width*Info.m_Height; i++)
	{
		int v = (d[i*Step]+d[i*Step+1]+d[i*Step+2])/3;
		d[i*Step] = v;
		d[i*Step+1] = v;
		d[i*Step+2] = v;
	}

	Part.m_ColorTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	mem_free(Info.m_pData);

	// set skin part data
	str_copy(Part.m_aName, pName, min((int)sizeof(Part.m_aName),l-3));
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load skin part %s", Part.m_aName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	pSelf->m_aaSkinParts[pSelf->m_ScanningPart].add(Part);

	return 0;
}

int CSkins::SkinScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CSkins *pSelf = (CSkins *)pUser;
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".skn") != 0)
		return 0;

	CSkin Skin = pSelf->m_DummySkin;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/%s", pName);
	IOHANDLE File = pSelf->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return 0;
	CLineReader LineReader;
	LineReader.Init(File);

	while(1)
	{
		char *pLine = LineReader.Get();
		if(!pLine)
			break;

		char aBuffer[1024];
		str_copy(aBuffer, pLine, sizeof(aBuffer));
		char *pStr = aBuffer;

		pStr = str_skip_whitespaces(pStr);
		char *pVariable = pStr;
		pStr = str_skip_to_whitespace(pStr);
		if(!pStr[0])
			continue;
		pStr[0] = 0;
		pStr++;
		pStr = str_skip_whitespaces(pStr);
		if(pStr[0] != ':' || pStr[1] != '=')
			continue;
		pStr += 2;
		if(!pStr[0])
			continue;
		pStr = str_skip_whitespaces(pStr);
		char *pValue = pStr;

		static const char *const apParts[6] = {"body.", "tattoo.", "decoration.",
												"hands.", "feet.", "eyes."};
		int Part = -1;
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if(str_comp_num(pVariable, apParts[p], str_length(apParts[p])) == 0)
				Part = p;
		}
		if(Part < 0)
			continue;
		pVariable += str_length(apParts[Part]);
		if(str_comp(pVariable, "filename") == 0)
		{
			int SkinPart = pSelf->FindSkinPart(Part, pValue);
			if(SkinPart < 0)
				continue;
			Skin.m_apParts[Part] = pSelf->GetSkinPart(Part, SkinPart);
		}
		else if(str_comp(pVariable, "custom_colors") == 0)
		{
			if(str_comp(pValue, "true") == 0)
				Skin.m_aUseCustomColors[Part] = 1;
			else if(str_comp(pValue, "false") == 0)
				Skin.m_aUseCustomColors[Part] = 0;
		}
		else
		{
			static const char *const apComponents[4] = {"hue", "sat", "lgt", "alp"};
			int Component = -1;
			for(int i = 0; i < 4; i++)
			{
				if(str_comp(pVariable, apComponents[i]) == 0)
					Component = i;
			}
			if(Component < 0)
				continue;
			if(Part != SKINPART_TATTOO && Component == 3)
				continue;
			int OldVal = Skin.m_aPartColors[Part];
			int Val = str_toint(pValue);
			if(Component == 0)
				Skin.m_aPartColors[Part] = (OldVal&0xFF00FFFF) | (Val << 16);
			else if(Component == 1)
				Skin.m_aPartColors[Part] = (OldVal&0xFFFF00FF) | (Val << 8);
			else if(Component == 2)
				Skin.m_aPartColors[Part] = (OldVal&0xFFFFFF00) | Val;
			else if(Component == 3)
				Skin.m_aPartColors[Part] = (OldVal&0x00FFFFFF) | (Val << 24);
		}
	}

	io_close(File);

	// set skin data
	str_copy(Skin.m_aName, pName, min((int)sizeof(Skin.m_aName),l-3));
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load skin %s", Skin.m_aName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	pSelf->m_aSkins.add(Skin);

	return 0;
}


void CSkins::OnInit()
{
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		m_aaSkinParts[p].clear();

		// add none part
		if(p == SKINPART_TATTOO || p == SKINPART_DECORATION)
		{
			CSkinPart NoneSkinPart;
			str_copy(NoneSkinPart.m_aName, "", sizeof(NoneSkinPart.m_aName));
			NoneSkinPart.m_OrgTexture = -1;
			NoneSkinPart.m_ColorTexture = -1;
			NoneSkinPart.m_BloodColor = vec3(1.0f, 1.0f, 1.0f);
			m_aaSkinParts[p].add(NoneSkinPart);
		}

		// load skin parts
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "skins/%s", gs_apFolders[p]);
		m_ScanningPart = p;
		Storage()->ListDirectory(IStorage::TYPE_ALL, aBuf, SkinPartScan, this);

		// add dummy skin part
		if(!m_aaSkinParts[p].size())
		{
			CSkinPart DummySkinPart;
			str_copy(DummySkinPart.m_aName, "dummy", sizeof(DummySkinPart.m_aName));
			DummySkinPart.m_OrgTexture = -1;
			DummySkinPart.m_ColorTexture = -1;
			DummySkinPart.m_BloodColor = vec3(1.0f, 1.0f, 1.0f);
			m_aaSkinParts[p].add(DummySkinPart);
		}
	}

	// create dummy skin
	str_copy(m_DummySkin.m_aName, "dummy", sizeof(m_DummySkin.m_aName));
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		int Default;
		if(p == SKINPART_TATTOO || p == SKINPART_DECORATION)
			Default = FindSkinPart(p, "");
		else
			Default = FindSkinPart(p, "standard");
		if(Default < 0)
			Default = 0;
		m_DummySkin.m_apParts[p] = GetSkinPart(p, Default);
		m_DummySkin.m_aPartColors[p] = p==SKINPART_TATTOO ? (255<<24)+65408 : 65408;
		m_DummySkin.m_aUseCustomColors[p] = 0;
	}

	// load skins
	m_aSkins.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins", SkinScan, this);

	// add dummy skin
	if(!m_aSkins.size())
		m_aSkins.add(m_DummySkin);
}

int CSkins::Num()
{
	return m_aSkins.size();
}

int CSkins::NumSkinPart(int Part)
{
	return m_aaSkinParts[Part].size();
}

const CSkins::CSkin *CSkins::Get(int Index)
{
	return &m_aSkins[max(0, Index%m_aSkins.size())];
}

int CSkins::Find(const char *pName)
{
	for(int i = 0; i < m_aSkins.size(); i++)
	{
		if(str_comp(m_aSkins[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

const CSkins::CSkinPart *CSkins::GetSkinPart(int Part, int Index)
{
	int Size = m_aaSkinParts[Part].size();
	return &m_aaSkinParts[Part][max(0, Index%Size)];
}

int CSkins::FindSkinPart(int Part, const char *pName)
{
	for(int i = 0; i < m_aaSkinParts[Part].size(); i++)
	{
		if(str_comp(m_aaSkinParts[Part][i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

vec3 CSkins::GetColorV3(int v) const
{
	float Dark = DARKEST_COLOR_LGT/255.0f;
	return HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, Dark+(v&0xff)/255.0f*(1.0f-Dark)));
}

vec4 CSkins::GetColorV4(int v, bool UseAlpha) const
{
	vec3 r = GetColorV3(v);
	float Alpha = UseAlpha ? ((v>>24)&0xff)/255.0f : 1.0f;
	return vec4(r.r, r.g, r.b, Alpha);
}

int CSkins::GetTeamColor(int UseCustomColors, int PartColor, int Team, int Part) const
{
	static const int s_aTeamColors[3] = {12895054, 65387, 10223467};
	if(!UseCustomColors)
	{
		int ColorVal = s_aTeamColors[Team+1];
		if(Part == SKINPART_TATTOO)
			ColorVal |= 0xff000000;
		return ColorVal;
	}

	/*blue128:  128/PI*ARCSIN(COS((PI*(x+10)/128)))+182 // (decoration, tattoo, hands)
	blue64:  64/PI*ARCSIN(COS((PI*(x-76)/64)))+172 // (body, feet, eyes)
	red128:   Mod((128/PI*ARCSIN(COS((PI*(x-105)/128)))+297),256)
	red64:    Mod((64/PI*ARCSIN(COS((PI*(x-56)/64)))+280),256)*/

	int MinSat = 160;
	int MaxSat = 255;
	float Dark = DARKEST_COLOR_LGT/255.0f;
	int MinLgt = Dark + 64*(1.0f-Dark);
	int MaxLgt = Dark + 191*(1.0f-Dark);

	int Hue = (PartColor>>16)&0xff;
	int Sat = (PartColor>>8)&0xff;
	int Lgt = PartColor&0xff;

	int NewHue;
	if(Team == TEAM_RED)
	{
		if(Part == SKINPART_TATTOO || Part == SKINPART_DECORATION || Part == SKINPART_HANDS)
			NewHue = (int)(128.0f/pi*asinf(cosf(pi/128.0f*(Hue-105.0f))) + 297.0f) % 256;
		else
			NewHue = (int)(64.0f/pi*asinf(cosf(pi/64.0f*(Hue-56.0f))) + 280.0f) % 256;
	}
	else
		NewHue = 64.0f/pi*asinf(cosf(pi/64.0f*(Hue-76.0f))) + 172.0f;

	int NewSat = clamp(Sat, MinSat, MaxSat);
	int NewLgt = clamp(Lgt, MinLgt, MaxLgt);
	int ColorVal = (NewHue<<16) + (NewSat<<8) + NewLgt;

	if(Part == SKINPART_TATTOO)
		ColorVal += PartColor&0xff000000;

	return ColorVal;
}
