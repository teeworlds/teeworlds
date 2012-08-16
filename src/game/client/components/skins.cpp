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
int *const gs_apMirroredVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerMirroredBody, &g_Config.m_PlayerMirroredTattoo, &g_Config.m_PlayerMirroredDecoration,
													0, 0, &g_Config.m_PlayerMirroredEyes};
int *const gs_apUCCVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerUseCustomColorBody, &g_Config.m_PlayerUseCustomColorTattoo, &g_Config.m_PlayerUseCustomColorDecoration,
													&g_Config.m_PlayerUseCustomColorHands, &g_Config.m_PlayerUseCustomColorFeet, &g_Config.m_PlayerUseCustomColorEyes};
int *const gs_apColorVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerColorBody, &g_Config.m_PlayerColorTattoo, &g_Config.m_PlayerColorDecoration,
													&g_Config.m_PlayerColorHands, &g_Config.m_PlayerColorFeet, &g_Config.m_PlayerColorEyes};
int *const gs_apRedColorVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerRedColorBody, &g_Config.m_PlayerRedColorTattoo, &g_Config.m_PlayerRedColorDecoration,
													&g_Config.m_PlayerRedColorHands, &g_Config.m_PlayerRedColorFeet, &g_Config.m_PlayerRedColorEyes};
int *const gs_apBlueColorVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerBlueColorBody, &g_Config.m_PlayerBlueColorTattoo, &g_Config.m_PlayerBlueColorDecoration,
													&g_Config.m_PlayerBlueColorHands, &g_Config.m_PlayerBlueColorFeet, &g_Config.m_PlayerBlueColorEyes};

static const char *const gs_apFolders[NUM_SKINPARTS] = {"bodies", "tattoos", "decoration",
														"hands", "feet", "eyes"};

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
		Storage()->ListDirectory(IStorage::TYPE_ALL, aBuf, SkinPartScanCb, this);

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
		m_DummySkin.m_aMirrored[p] = 0;
		m_DummySkin.m_aUseCustomColors[p] = 0;
		m_DummySkin.m_aPartColors[p] = IsUsingAlpha(p) ? (255<<24)+65408 : 65408;
	}

	// load skins
	m_aSkins.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins", SkinScanCb, this);

	// add dummy skin
	if(!m_aSkins.size())
		m_aSkins.add(m_DummySkin);
}

void CSkins::SkinScan(const char *pName)
{
	CSkin Skin = m_DummySkin;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/%s", pName);
	IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return;
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
			int SkinPart = FindSkinPart(Part, pValue);
			if(SkinPart < 0)
				continue;
			Skin.m_apParts[Part] = GetSkinPart(Part, SkinPart);
		}
		else if(str_comp(pVariable, "mirrored") == 0)
		{
			if(str_comp(pValue, "true") == 0)
				Skin.m_aMirrored[Part] = 1;
			else if(str_comp(pValue, "false") == 0)
				Skin.m_aMirrored[Part] = 0;
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
			if(IsUsingAlpha(Part) && Component == 3)
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
	int l = str_length(pName);
	str_copy(Skin.m_aName, pName, min((int)sizeof(Skin.m_aName),l-3));
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load skin %s", Skin.m_aName);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	int SkinID = Find(Skin.m_aName);
	if(SkinID == -1)
		m_aSkins.add(Skin);
	else
		m_aSkins[SkinID] = Skin;
}

void CSkins::SkinPartScan(const char *pName, int p)
{
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/%s/%s", gs_apFolders[p], pName);
	CImageInfo Info;
	if(!Graphics()->LoadPNG(&Info, aBuf, IStorage::TYPE_ALL))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load skin from %s", pName);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return;
	}

	CSkinPart Part;
	Part.m_OrgTexture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);

	unsigned char *d = (unsigned char *)Info.m_pData;
	int Pitch = Info.m_Width*4;

	// dig out blood color
	if(p == SKINPART_BODY)
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

	Part.m_ColorTexture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	mem_free(Info.m_pData);

	// set skin part data
	int l = str_length(pName);
	str_copy(Part.m_aName, pName, min((int)sizeof(Part.m_aName),l-3));
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load skin part %s", Part.m_aName);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	int PartID = Find(Part.m_aName);
	if(PartID == -1)
		m_aaSkinParts[p].add(Part);
	else
		m_aaSkinParts[p][PartID] = Part;
}

int CSkins::SkinScanCb(const char *pName, int IsDir, int DirType, void *pUser)
{
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".skn") != 0)
		return 0;

	CSkins *pSelf = (CSkins *)pUser;
	pSelf->SkinScan(pName);

	return 0;
}

int CSkins::SkinPartScanCb(const char *pName, int IsDir, int DirType, void *pUser)
{
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	CSkins *pSelf = (CSkins *)pUser;
	pSelf->SkinPartScan(pName, pSelf->m_ScanningPart);

	return 0;
}

int CSkins::Num() const
{
	return m_aSkins.size();
}

int CSkins::NumSkinPart(int Part) const
{
	return m_aaSkinParts[Part].size();
}

const CSkins::CSkin *CSkins::Get(int Index) const
{
	return &m_aSkins[max(0, Index%m_aSkins.size())];
}

int CSkins::Find(const char *pName) const
{
	for(int i = 0; i < m_aSkins.size(); i++)
	{
		if(str_comp(m_aSkins[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

const CSkins::CSkinPart *CSkins::GetSkinPart(int Part, int Index) const
{
	int Size = m_aaSkinParts[Part].size();
	return &m_aaSkinParts[Part][max(0, Index%Size)];
}

int CSkins::FindSkinPart(int Part, const char *pName) const
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

vec4 CSkins::GetColorV4(int v, int Part) const
{
	vec3 r = GetColorV3(v);
	float Alpha = IsUsingAlpha(Part) ? ((v>>24)&0xff)/255.0f : 1.0f;
	return vec4(r.r, r.g, r.b, Alpha);
}

void CSkins::SelectSkin(const char *pName) const
{
	const CSkin *s = Get(Find(pName));

	mem_copy(g_Config.m_PlayerSkin, pName, sizeof(g_Config.m_PlayerSkin));
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		mem_copy(gs_apSkinVariables[p], s->m_apParts[p]->m_aName, 24);
		if(IsMirrorable(p))
			*gs_apMirroredVariables[p] = s->m_aMirrored[p];
		*gs_apUCCVariables[p] = s->m_aUseCustomColors[p];
		*gs_apColorVariables[p] = s->m_aPartColors[p];
	}
}

int CSkins::GetTeamColor(int UseCustomColors, int PartHue, int PartAlp, int Team, int Part) const
{
	if(Team == TEAM_SPECTATORS)
		return 12895054;

	if(!UseCustomColors)
	{
		if(Team == TEAM_RED)
			PartHue = RED_TEAM_HUE;
		else
			PartHue = BLUE_TEAM_HUE;
	}
	else
	{
		PartHue %= 256;

		if(Team == TEAM_RED && absolute(PartHue-RED_TEAM_HUE) > 32)
			PartHue = RED_TEAM_HUE;
		if(Team == TEAM_BLUE && absolute(PartHue-BLUE_TEAM_HUE) > 32)
			PartHue = BLUE_TEAM_HUE;

		if(PartHue < 0)
			PartHue += 256;
	}

	if(IsUsingAlpha(Part))
		return (PartAlp<<24) + (PartHue<<16) + (TEAM_SAT<<8) + TEAM_LGT;
	else
		return (PartHue<<16) + (TEAM_SAT<<8) + TEAM_LGT;
}

bool CSkins::IsMirrorable(int Part) const
{
	return gs_apMirroredVariables[Part] != 0;
}

bool CSkins::IsUsingAlpha(int Part) const
{
	return Part == SKINPART_TATTOO;
}
