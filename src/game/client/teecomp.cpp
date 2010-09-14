#include <base/math.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include "teecomp.h"

vec3 CTeecompUtils::GetTeamColor(int ForTeam, int LocalTeam, int Color1, int Color2, int Method)
{
	vec3 c1((Color1>>16)/255.0f, ((Color1>>8)&0xff)/255.0f, (Color1&0xff)/255.0f);
	vec3 c2((Color2>>16)/255.0f, ((Color2>>8)&0xff)/255.0f, (Color2&0xff)/255.0f);

	// Team based Colors or spectating
	if(!Method || LocalTeam == -1)
	{
		if(ForTeam == 0)
			return c1;
		return c2;
	}

	// Enemy based Colors
	if(ForTeam == LocalTeam)
		return c1;
	return c2;
}

bool CTeecompUtils::GetForcedSkinName(int ForTeam, int LocalTeam, const char*& pSkinName)
{
	// Team based Colors or spectating
	if(!g_Config.m_TcForcedSkinsMethod || LocalTeam == -1)
	{
		if(ForTeam == 0)
		{
			pSkinName = g_Config.m_TcForcedSkin1;
			return g_Config.m_TcForceSkinTeam1;
		}
		pSkinName = g_Config.m_TcForcedSkin2;
		return g_Config.m_TcForceSkinTeam2;
	}

	// Enemy based Colors
	if(ForTeam == LocalTeam)
	{
		pSkinName = g_Config.m_TcForcedSkin1;
		return g_Config.m_TcForceSkinTeam1;
	}
	pSkinName = g_Config.m_TcForcedSkin2;
	return g_Config.m_TcForceSkinTeam2;
}

bool CTeecompUtils::GetForceDmColors(int ForTeam, int LocalTeam)
{
	if(!g_Config.m_TcColoredTeesMethod || LocalTeam == -1)
	{
		if(ForTeam == 0)
			return g_Config.m_TcDmColorsTeam1;
		return g_Config.m_TcDmColorsTeam2;
	}

	if(ForTeam == LocalTeam)
		return g_Config.m_TcDmColorsTeam1;
	return g_Config.m_TcDmColorsTeam2;
}

void CTeecompUtils::ResetConfig()
{
	#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) g_Config.m_##Name = Def;
	#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) str_copy(g_Config.m_##Name, Def, Len);
	#include "../teecomp_vars.h"
	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR
}

static vec3 RgbToHsl(vec3 rgb)
{
	float r = rgb.r;
	float g = rgb.g;
	float b = rgb.b;

	float vMin = min(min(r, g), b);
	float vMax = max(max(r, g), b);
	float dMax = vMax - vMin;

	float h = 0.0f;
	float s = 0.0f;
	float l = (vMax + vMin) / 2.0f;

	if(dMax == 0.0f)
	{
		h = 0.0f;
		s = 0.0f;
	}
	else
	{
		if(l < 0.5f)
			s = dMax / (vMax + vMin);
		else
			s = dMax / (2 - vMax - vMin);

		float dR = (((vMax - r) / 6.0f) + (dMax / 2.0f)) / dMax;
		float dG = (((vMax - g) / 6.0f) + (dMax / 2.0f)) / dMax;
		float dB = (((vMax - b) / 6.0f) + (dMax / 2.0f)) / dMax;

		if(r == vMax)
			h = dB - dG;
		else if(g == vMax)
			h = (1.0f/3.0f) + dR - dB;
		else if(b == vMax)
			h = (2.0f/3.0f) + dG - dR;

		if(h < 0.0f)
			h += 1.0f;
		if(h > 1.0f)
			h -= 1.0f;
	}

	return vec3(h*360, s, l);
}

const char* CTeecompUtils::RgbToName(int rgb)
{
	vec3 rgb_v((rgb>>16)/255.0f, ((rgb>>8)&0xff)/255.0f, (rgb&0xff)/255.0f);
	vec3 hsl = RgbToHsl(rgb_v);

	if(hsl.l < 0.2f)
		return "Black";
	if(hsl.l > 0.9f)
		return "White";
	if(hsl.s < 0.1f)
		return "Gray";
	if(hsl.h < 20)
		return "Red";
	if(hsl.h < 45)
		return "Orange";
	if(hsl.h < 70)
		return "Yellow";
	if(hsl.h < 155)
		return "Green";
	if(hsl.h < 260)
		return "Blue";
	if(hsl.h < 335)
		return "Purple";
	return "Red";
}

const char* CTeecompUtils::TeamColorToName(int rgb)
{
	vec3 rgb_v((rgb>>16)/255.0f, ((rgb>>8)&0xff)/255.0f, (rgb&0xff)/255.0f);
	vec3 hsl = RgbToHsl(rgb_v);

	if(hsl.l < 0.2f)
		return "black";
	if(hsl.l > 0.9f)
		return "white";
	if(hsl.s < 0.1f)
		return "gray";
	if(hsl.h < 20)
		return "red";
	if(hsl.h < 45)
		return "orange";
	if(hsl.h < 70)
		return "yellow";
	if(hsl.h < 155)
		return "green";
	if(hsl.h < 260)
		return "blue";
	if(hsl.h < 335)
		return "purple";
	return "red";
}
