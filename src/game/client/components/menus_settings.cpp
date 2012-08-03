/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/components/sounds.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

CMenusKeyBinder CMenus::m_Binder;

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		if(Event.m_Flags&IInput::FLAG_PRESS)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;
		}
		return true;
	}

	return false;
}

static int const gs_aSelectionParts[6] = {SELECTION_BODY, SELECTION_TATTOO, SELECTION_DECORATION,
											SELECTION_HANDS, SELECTION_FEET, SELECTION_EYES};

int CMenus::DoButton_Customize(const void *pID, int Texture, int SpriteID, const CUIRect *pRect, float ImageRatio)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds);

	RenderTools()->DrawUIRect(pRect, vec4(1.0f, 1.0f, 1.0f, 0.5f+(*pFade/Seconds)*0.25f), CUI::CORNER_ALL, 10.0f);
	Graphics()->TextureSet(Texture);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteID);
	float Height = pRect->w/ImageRatio;
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y+(pRect->h-Height)/2, pRect->w, Height);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pID, "", 0, pRect);
}

void CMenus::SaveSkinfile()
{
	char aFilename[256];
	str_format(aFilename, sizeof(aFilename), "skins/%s.skn", m_aSaveSkinName);
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;

	static const char *const apParts[6] = {"body", "tattoo", "decoration",
											"hands", "feet", "eyes"};
	static const char *const apComponents[4] = {"hue", "sat", "lgt", "alp"};

	char aBuf[256];
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		if(!gs_apSkinVariables[p][0])
			continue;

		str_format(aBuf, sizeof(aBuf), "%s.filename := %s", apParts[p], gs_apSkinVariables[p]);
		WriteLineSkinfile(File, aBuf);

		str_format(aBuf, sizeof(aBuf), "%s.custom_colors := %s", apParts[p], *gs_apUCCVariables[p]?"true":"false");
		WriteLineSkinfile(File, aBuf);

		if(*gs_apUCCVariables[p])
		{
			for(int c = 0; c < 3; c++)
			{
				int Val = (*gs_apColorVariables[p] >> (2-c)*8) & 0xff;
				str_format(aBuf, sizeof(aBuf), "%s.%s := %d", apParts[p], apComponents[c], Val);
				WriteLineSkinfile(File, aBuf);
			}
			if(p == SKINPART_TATTOO)
			{
				int Val = (*gs_apColorVariables[p] >> 24) & 0xff;
				str_format(aBuf, sizeof(aBuf), "%s.%s := %d", apParts[p], apComponents[3], Val);
				WriteLineSkinfile(File, aBuf);
			}
		}

		if(p != NUM_SKINPARTS-1)
			WriteLineSkinfile(File, "");
	}

	io_close(File);
}

void CMenus::WriteLineSkinfile(IOHANDLE File, const char *pLine)
{
	io_write(File, pLine, str_length(pLine));
	io_write_newline(File);
}

void CMenus::RenderHSLPicker(CUIRect Picker)
{
	CUIRect Label, Button;
	bool Modified = false;

	int ConfigColor = -1;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		if(!(m_TeePartsColorSelection & gs_aSelectionParts[p]))
			continue;
		int Val = (*gs_apColorVariables[p])&0xffffff;
		if(ConfigColor != -1 && ConfigColor != Val)
		{
			ConfigColor = -1;
			break;
		}
		ConfigColor = Val;
	}

	bool UseAlpha = m_TeePartsColorSelection & SELECTION_TATTOO;

	int Hue, Sat, Lgt, Alp;
	if(ConfigColor != -1)
	{
		Hue = (ConfigColor>>16)&0xff;
		Sat = (ConfigColor>>8)&0xff;
		Lgt = ConfigColor&0xff;
	}
	else
	{
		Hue = -1;
		Sat = -1;
		Lgt = -1;
	}
	if(UseAlpha)
		Alp = (g_Config.m_PlayerColorTattoo>>24)&0xff;
	else
		Alp = -1;

	// Hue/Lgt picker :
	{
		Picker.VSplitLeft(256.0f, &Picker, 0);
		Picker.HSplitTop(128.0f, &Picker, 0);

		// picker
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_HLPICKER].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem(Picker.x, Picker.y, Picker.w, Picker.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		/*Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		for(int i = 0; i < 256; i++)
		{
			for(int j = 0; j < 256-DARKEST_COLOR_LGT; j++)
			{
				int H = i;
				int L = 255-j;
				vec3 rgb = HslToRgb(vec3(H/255.0f, Sat/255.0f, L/255.0f));
				Graphics()->SetColor(rgb.r, rgb.g, rgb.b, 1.0f);
				IGraphics::CQuadItem QuadItem(Picker.x+i, Picker.y+j, 1.0f, 1.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
		}
		Graphics()->QuadsEnd();*/

		// marker
		vec2 Marker = vec2(Hue*UI()->Scale(), max(0.0f, 127-Lgt/2.0f)*UI()->Scale());
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		IGraphics::CQuadItem aMarker[2];
		aMarker[0] = IGraphics::CQuadItem(Picker.x+Marker.x, Picker.y+Marker.y - 5.0f*UI()->PixelSize(), UI()->PixelSize(), 11.0f*UI()->PixelSize());
		aMarker[1] = IGraphics::CQuadItem(Picker.x+Marker.x - 5.0f*UI()->PixelSize(), Picker.y+Marker.y, 11.0f*UI()->PixelSize(), UI()->PixelSize());
		Graphics()->QuadsDrawTL(aMarker, 2);
		Graphics()->QuadsEnd();

		// logic
		int X, Y;
		static int s_HLPicker;
		int Logic = UI()->DoPickerLogic(&s_HLPicker, &Picker, &X, &Y);
		if(Logic)
		{
			Hue = X;
			Lgt = 255 - Y*2;
			Modified = true;
		}
	}

	// H/S/L/A sliders :
	{
		static float const aPos[4] = {130.0f, 147.0f, 164.0f, 181.0f};
		const char *const apNames[4] = {Localize("Hue:"), Localize("Sat:"), Localize("Lgt:"), Localize("Alp:")};
		int *const apVars[4] = {&Hue, &Sat, &Lgt, &Alp};
		static int s_aButtons[12];

		int NumBars = UseAlpha ? 4 : 3;
		for(int i = 0; i < NumBars; i++)
		{
			CUIRect Bar;
			// label
			Picker.HSplitTop(aPos[i], 0, &Label);
			Label.HSplitTop(15.0f, &Label, 0);
			Label.VSplitLeft(30.0f, &Label, &Button);
			UI()->DoLabelScaled(&Label, apNames[i], 12.0f, -1);
			// button <
			Button.VSplitLeft(20.0f, &Button, &Bar);
			if(DoButton_Menu(&s_aButtons[i*3], "<", 0, &Button, 5.0f, 0.0f, CUI::CORNER_TL|CUI::CORNER_BL))
			{
				*apVars[i] = max(0, *apVars[i]-1);
				Modified = true;
			}
			// bar
			Bar.VSplitLeft(256/2, &Bar, &Button);
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			for(int v = 0; v < 256/2; v++)
			{
				int Val = v*2;
				vec3 rgb;
				float Dark = DARKEST_COLOR_LGT/255.0f;
				if(i == 0)
					rgb = HslToRgb(vec3(Val/255.0f, 1.0f, 0.5f));
				else if(i == 1)
					rgb = HslToRgb(vec3(Hue/255.0f, Val/255.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
				else if(i == 2)
					rgb = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark+Val/255.0f*(1.0f-Dark)));
				else
					rgb = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
				if(i == 3)
					Graphics()->SetColor(rgb.r, rgb.g, rgb.b, Val/255.0f);
				else
					Graphics()->SetColor(rgb.r, rgb.g, rgb.b, 1.0f);
				IGraphics::CQuadItem QuadItem(Bar.x+v*UI()->Scale(), Bar.y, 1.0f*UI()->Scale(), Bar.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
			// bar marker
			Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Bar.x + min(127.0f, *apVars[i]/2.0f)*UI()->Scale(), Bar.y, UI()->PixelSize(), Bar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			// button >
			Button.VSplitLeft(20.0f, &Button, &Label);
			if(DoButton_Menu(&s_aButtons[i*3+1], ">", 0, &Button, 5.0f, 0.0f, CUI::CORNER_TR|CUI::CORNER_BR))
			{
				*apVars[i] = min(255, *apVars[i]+1);
				Modified = true;
			}
			// label value
			char aBuf[16];
			str_format(aBuf, sizeof(aBuf), "%d", *apVars[i]);
			UI()->DoLabelScaled(&Label, aBuf, 12.0f, -1);
			// logic
			int X;
			int Logic = UI()->DoPickerLogic(&s_aButtons[i*3+2], &Bar, &X, 0);
			if(Logic)
			{
				*apVars[i] = X*2;
				Modified = true;
			}
		}
	}

	if(Modified)
	{
		int NewVal = (Hue << 16) + (Sat << 8) + Lgt;
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if(m_TeePartsColorSelection & gs_aSelectionParts[p])
				*gs_apColorVariables[p] = NewVal;
		}
		if(UseAlpha)
			g_Config.m_PlayerColorTattoo = (Alp << 24) + NewVal;
	}
}

void CMenus::RenderSkinSelection(CUIRect MainView)
{
	static bool s_InitSkinlist = true;
	static sorted_array<const CSkins::CSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	UiDoListboxStart(&s_InitSkinlist, &MainView, 50.0f, Localize("Skins"), "", s_paSkinList.size(), 4, -1, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkins::CSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], false);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				if(s->m_aUseCustomColors[p])
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_ColorTexture;
					Info.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(s->m_aPartColors[p], p==SKINPART_TATTOO);
				}
				else
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_OrgTexture;
					Info.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	static int OldSelected = -1;
	if(NewSelected != -1)
	{
		if(NewSelected != OldSelected)
		{
			const CSkins::CSkin *s = s_paSkinList[NewSelected];
			mem_copy(g_Config.m_PlayerSkin, s->m_aName, sizeof(g_Config.m_PlayerSkin));
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				mem_copy(gs_apSkinVariables[p], s->m_apParts[p]->m_aName, 24);
				*gs_apUCCVariables[p] = s->m_aUseCustomColors[p];
				*gs_apColorVariables[p] = s->m_aPartColors[p];
			}
		}
		m_TeePartSelection = NO_SELECTION;
	}
	OldSelected = NewSelected;
}

void CMenus::RenderSkinPartSelection(CUIRect MainView)
{
	static bool s_InitSkinPartList = true;
	static sorted_array<const CSkins::CSkinPart *> s_paList[6];
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinPartList)
	{
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			s_paList[p].clear();
			for(int i = 0; i < m_pClient->m_pSkins->NumSkinPart(p); ++i)
			{
				const CSkins::CSkinPart *s = m_pClient->m_pSkins->GetSkinPart(p, i);
				// no special skins
				if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
					continue;
				s_paList[p].add(s);
			}
		}
		s_InitSkinPartList = false;
	}

	int p = -1;
	for(int j = 0; j < NUM_SKINPARTS; j++)
	{
		if(m_TeePartSelection == gs_aSelectionParts[j])
			p = j;
	}
	if(p < 0)
		return;

	const char *const s_apTitles[6] = {Localize("Bodies"), Localize("Tattoos"), Localize("Decoration"),
											Localize("Hands"), Localize("Feet"), Localize("Eyes")};

	UiDoListboxStart(&s_InitSkinPartList, &MainView, 50.0f, s_apTitles[p], "", s_paList[p].size(), 4, -1, s_ScrollValue);

	for(int i = 0; i < s_paList[p].size(); ++i)
	{
		const CSkins::CSkinPart *s = s_paList[p][i];
		if(s == 0)
			continue;

		CListboxItem Item = UiDoListboxNextItem(&s_paList[p][i], false);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int j = 0; j < NUM_SKINPARTS; j++)
			{
				int SkinPart = m_pClient->m_pSkins->FindSkinPart(j, gs_apSkinVariables[j]);
				const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(j, SkinPart);
				if(*gs_apUCCVariables[j])
				{
					if(p == j)
						Info.m_aTextures[j] = s->m_ColorTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_ColorTexture;
					Info.m_aColors[j] = m_pClient->m_pSkins->GetColorV4(*gs_apColorVariables[j], j==SKINPART_TATTOO);
				}
				else
				{
					if(p == j)
						Info.m_aTextures[j] = s->m_OrgTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_OrgTexture;
					Info.m_aColors[j] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	static int OldSelected = -1;
	if(NewSelected != -1)
	{
		if(NewSelected != OldSelected)
		{
			const CSkins::CSkinPart *s = s_paList[p][NewSelected];
			mem_copy(gs_apSkinVariables[p], s->m_aName, 24);
		}
		m_TeePartSelection = NO_SELECTION;
	}
	OldSelected = NewSelected;
}

class CLanguage
{
public:
	CLanguage() {}
	CLanguage(const char *n, const char *f, int Code) : m_Name(n), m_FileName(f), m_CountryCode(Code) {}

	string m_Name;
	string m_FileName;
	int m_CountryCode;

	bool operator<(const CLanguage &Other) { return m_Name < Other.m_Name; }
};

void LoadLanguageIndexfile(IStorage *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	IOHANDLE File = pStorage->OpenFile("languages/index.txt", IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}

	char aOrigin[128];
	char aReplacement[128];
	CLineReader LineReader;
	LineReader.Init(File);
	char *pLine;
	while((pLine = LineReader.Get()))
	{
		if(!str_length(pLine) || pLine[0] == '#') // skip empty lines and comments
			continue;

		str_copy(aOrigin, pLine, sizeof(aOrigin));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			(void)LineReader.Get();
			continue;
		}
		str_copy(aReplacement, pLine+3, sizeof(aReplacement));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			continue;
		}

		char aFileName[128];
		str_format(aFileName, sizeof(aFileName), "languages/%s.txt", aOrigin);
		pLanguages->add(CLanguage(aReplacement, aFileName, str_toint(pLine+3)));
	}
	io_close(File);
}

void CMenus::RenderLanguageSelection(CUIRect MainView)
{
	static int s_LanguageList = 0;
	static int s_SelectedLanguage = 0;
	static sorted_array<CLanguage> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

	UiDoListboxStart(&s_LanguageList , &MainView, 24.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_pCountryFlags->Render(r.front().m_CountryCode, &Color, Rect.x, Rect.y, Rect.w, Rect.h);
			Item.m_Rect.HSplitTop(2.0f, 0, &Item.m_Rect);
 			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
		}
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
	}
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	char aBuf[128];
	CUIRect Label, Button, Left, Right, Game, Client, Language;
	MainView.HSplitTop(150.0f, &Game, &Client);
	Client.HSplitTop(130.0f, &Client, &Language);

	// game
	{
		// headline
		Game.HSplitTop(30.0f, &Label, &Game);
		UI()->DoLabelScaled(&Label, Localize("Game"), 20.0f, -1);
		Game.Margin(5.0f, &Game);
		Game.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// dynamic camera
		Left.HSplitTop(20.0f, &Button, &Left);
		static int s_DynamicCameraButton = 0;
		if(DoButton_CheckBox(&s_DynamicCameraButton, Localize("Dynamic Camera"), g_Config.m_ClMouseDeadzone != 0, &Button))
		{
			if(g_Config.m_ClMouseDeadzone)
			{
				g_Config.m_ClMouseFollowfactor = 0;
				g_Config.m_ClMouseMaxDistance = 400;
				g_Config.m_ClMouseDeadzone = 0;
			}
			else
			{
				g_Config.m_ClMouseFollowfactor = 60;
				g_Config.m_ClMouseMaxDistance = 1000;
				g_Config.m_ClMouseDeadzone = 300;
			}
		}

		// weapon pickup
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static int s_AutoswitchWeapons = 0;
		if(DoButton_CheckBox(&s_AutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		// show hud
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static int s_Showhud = 0;
		if(DoButton_CheckBox(&s_Showhud, Localize("Show ingame HUD"), g_Config.m_ClShowhud, &Button))
			g_Config.m_ClShowhud ^= 1;

		// chat messages
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static int s_Friendchat = 0;
		if(DoButton_CheckBox(&s_Friendchat, Localize("Show only chat messages from friends"), g_Config.m_ClShowChatFriends, &Button))
			g_Config.m_ClShowChatFriends ^= 1;

		// name plates
		Right.HSplitTop(20.0f, &Button, &Right);
		static int s_Nameplates = 0;
		if(DoButton_CheckBox(&s_Nameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
			g_Config.m_ClNameplates ^= 1;

		if(g_Config.m_ClNameplates)
		{
			Right.HSplitTop(2.5f, 0, &Right);
			Right.VSplitLeft(30.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			static int s_NameplatesAlways = 0;
			if(DoButton_CheckBox(&s_NameplatesAlways, Localize("Always show name plates"), g_Config.m_ClNameplatesAlways, &Button))
				g_Config.m_ClNameplatesAlways ^= 1;

			Right.HSplitTop(2.5f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Name plates size"), g_Config.m_ClNameplatesSize);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClNameplatesSize = (int)(DoScrollbarH(&g_Config.m_ClNameplatesSize, &Button, g_Config.m_ClNameplatesSize/100.0f)*100.0f+0.1f);

			Right.HSplitTop(20.0f, &Button, &Right);
			static int s_NameplatesTeamcolors = 0;
			if(DoButton_CheckBox(&s_NameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
				g_Config.m_ClNameplatesTeamcolors ^= 1;
		}
	}

	// client
	{
		// headline
		Client.HSplitTop(30.0f, &Label, &Client);
		UI()->DoLabelScaled(&Label, Localize("Client"), 20.0f, -1);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// auto demo settings
		{
			Left.HSplitTop(20.0f, &Button, &Left);
			static int s_AutoDemoRecord = 0;
			if(DoButton_CheckBox(&s_AutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
				g_Config.m_ClAutoDemoRecord ^= 1;

			Right.HSplitTop(20.0f, &Button, &Right);
			static int s_AutoScreenshot = 0;
			if(DoButton_CheckBox(&s_AutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
				g_Config.m_ClAutoScreenshot ^= 1;

			Left.HSplitTop(10.0f, 0, &Left);
			Left.VSplitLeft(20.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			char aBuf[64];
			if(g_Config.m_ClAutoDemoMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max demos"), g_Config.m_ClAutoDemoMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max demos"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClAutoDemoMax = static_cast<int>(DoScrollbarH(&g_Config.m_ClAutoDemoMax, &Button, g_Config.m_ClAutoDemoMax/1000.0f)*1000.0f+0.1f);

			Right.HSplitTop(10.0f, 0, &Right);
			Right.VSplitLeft(20.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClAutoScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Right.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			g_Config.m_ClAutoScreenshotMax = static_cast<int>(DoScrollbarH(&g_Config.m_ClAutoScreenshotMax, &Button, g_Config.m_ClAutoScreenshotMax/1000.0f)*1000.0f+0.1f);
		}
	}

	// language selection
	static int s_LanguageList = 0;
	static int s_SelectedLanguage = 0;
	static sorted_array<CLanguage> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

	UiDoListboxStart(&s_LanguageList , &Language, 24.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			Graphics()->TextureSet(m_pClient->m_pCountryFlags->GetByCountryCode(r.front().m_CountryCode)->m_Texture);
 			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Rect.x, Rect.y, Rect.w, Rect.h);
 			Graphics()->QuadsDrawTL(&QuadItem, 1);
 			Graphics()->QuadsEnd();
			Item.m_Rect.HSplitTop(2.0f, 0, &Item.m_Rect);
 			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
		}
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
	}
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CUIRect Button, Label;
	MainView.HSplitTop(10.0f, 0, &MainView);

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName = 0.0f;
	DoEditBox(g_Config.m_PlayerName, &Button, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName);

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan = 0.0f;
	DoEditBox(g_Config.m_PlayerClan, &Button, g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan);

	// country flag selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	UiDoListboxStart(&s_ScrollValue, &MainView, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
		if(pEntry->m_CountryCode == g_Config.m_PlayerCountry)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&pEntry->m_CountryCode, OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h*2;
			Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
			Graphics()->TextureSet(pEntry->m_Texture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		g_Config.m_PlayerCountry = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
	}
}

void CMenus::RenderSettingsTee(CUIRect MainView)
{
	CUIRect Button, Label, EditBox;
	CUIRect YourSkin, Preview, CustomizeSkin, Left, Mid, SaveSkin, Selection, UCC, Picker;
	MainView.VMargin(20.0f, &MainView);
	MainView.HMargin(10.0f, &MainView);
	MainView.VSplitLeft(420.0f, 0, &Selection);
	MainView.HSplitTop(65.0f, &YourSkin, &CustomizeSkin);

	// Preview :
	{
		YourSkin.VSplitLeft(80.0f, &YourSkin, &Button);
		YourSkin.HSplitTop(18.0f, 0, &Label);
		UI()->DoLabelScaled(&Label, Localize("Your skin:"), 14.0f, -1);

		float tw = TextRender()->TextWidth(0, 14.0f, g_Config.m_PlayerSkin, -1);
		Button.VSplitLeft(16.0f + 50.0f+tw, &Button, 0);
		Button.HSplitTop(12.0f + 40.0f, &Button, 0);
		static int s_SkinButton = 0;
		if(DoButton_Menu(&s_SkinButton, "", 0, &Button))
		{
			m_TeePartSelection = SELECTION_SKIN;
			m_TeePartsColorSelection = NO_SELECTION;
		}

		Button.VSplitLeft(24.0f, 0, &Preview);
		Preview.HSplitTop(30.0f, 0, &Preview);
		CTeeRenderInfo OwnSkinInfo;
		OwnSkinInfo.m_Size = 50.0f*UI()->Scale();
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, gs_apSkinVariables[p]);
			const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
			if(*gs_apUCCVariables[p])
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(*gs_apColorVariables[p], p==SKINPART_TATTOO);
			}
			else
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				OwnSkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Preview.x, Preview.y));

		Button.VSplitLeft(50.0f, 0, &Label);
		Label.HSplitTop(18.0f, 0, &Label);
		UI()->DoLabelScaled(&Label, g_Config.m_PlayerSkin, 14.0f, -1);

		if(m_TeePartSelection == NO_SELECTION)
		{
			// draw tee with red team color
			Button.VSplitLeft(150.0f, 0, &Preview);
			Preview.VSplitLeft(100.0f, &Label, &Preview);
			Label.HSplitTop(18.0f, 0, &Label);
			UI()->DoLabelScaled(&Label, Localize("Red team:"), 14.0f, -1);

			Preview.HSplitTop(30.0f, 0, &Preview);
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				int TeamColor = m_pClient->m_pSkins->GetTeamColor(*gs_apUCCVariables[p], *gs_apColorVariables[p], TEAM_RED, p);
				OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_TATTOO);
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Preview.x, Preview.y));

			// draw tee with blue team color
			Button.VSplitLeft(300.0f, 0, &Preview);
			Preview.VSplitLeft(100.0f, &Label, &Preview);
			Label.HSplitTop(18.0f, 0, &Label);
			UI()->DoLabelScaled(&Label, Localize("Blue team:"), 14.0f, -1);

			Preview.HSplitTop(30.0f, 0, &Preview);
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				int TeamColor = m_pClient->m_pSkins->GetTeamColor(*gs_apUCCVariables[p], *gs_apColorVariables[p], TEAM_BLUE, p);
				OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_TATTOO);
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Preview.x, Preview.y));
		}
	}

	// Selection :
	{
		if(m_TeePartSelection == SELECTION_SKIN)
			RenderSkinSelection(Selection);
		else if(m_TeePartSelection != NO_SELECTION)
			RenderSkinPartSelection(Selection);
	}

	// Skin Customization :
	{
		CustomizeSkin.HSplitTop(20.0f, &Button, &Left);
		static int s_CustomizeSkinButton = 0;
		if(DoButton_CheckBox(&s_CustomizeSkinButton, Localize("Customize skin"), g_Config.m_ClCustomizeSkin, &Button))
		{
			g_Config.m_ClCustomizeSkin ^= 1;
			if(!g_Config.m_ClCustomizeSkin)
			{
				m_TeePartSelection = NO_SELECTION;
				m_TeePartsColorSelection = NO_SELECTION;
			}
		}
	}

	if(!g_Config.m_ClCustomizeSkin)
		return;

	// Parts Customization :
	{
		Left.HSplitTop(10.0f, 0, &Left);
		Left.VSplitLeft(15.0f, 0, &Left);
		Left.HSplitTop(260.0f, &Left, &SaveSkin);
		Left.VSplitLeft(180.0f, &Left, &Mid);
		Mid.VSplitLeft(10.0f, 0, &Mid);
		Mid.VSplitLeft(165.0f, &Mid, &UCC);

		const char *const apNames[6] = {Localize("Body:"), Localize("Tattoo:"), Localize("Decoration:"),
										Localize("Hands:"), Localize("Feet:"), Localize("Eyes:")};
		CUIRect *const apColumns[6] = {&Left, &Left, &Left,
										&Mid, &Mid, &Mid};
		static int const s_aSprites[6] = {SPRITE_TEE_BODY, SPRITE_TEE_TATTOO, SPRITE_TEE_DECORATION,
											SPRITE_TEE_HAND, SPRITE_TEE_FOOT, SPRITE_TEE_EYES_NORMAL};
		static int s_aButtons[6];

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int Tex = g_pData->m_aImages[IMAGE_NO_SKINPART].m_Id;
			int Sprite = -1;
			if(gs_apSkinVariables[p][0])
			{
				int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, gs_apSkinVariables[p]);
				const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
				Tex = pSkinPart->m_OrgTexture;
				Sprite = s_aSprites[p];
			}

			CUIRect ColorSelection;
			apColumns[p]->HSplitTop(80.0f, &ColorSelection, apColumns[p]);
			ColorSelection.HMargin(5.0f, &ColorSelection);
			if(m_TeePartsColorSelection & gs_aSelectionParts[p])
				RenderTools()->DrawUIRect(&ColorSelection, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			ColorSelection.HSplitTop(5.0f, 0, &ColorSelection);
			ColorSelection.VSplitRight(70.0f, &Label, &Button);
			Label.HSplitTop(20.0f, 0, &Label);
			Label.VSplitLeft(5.0f, 0, &Label);
			UI()->DoLabelScaled(&Label, apNames[p], 14.0f, -1);
			Button.HSplitTop(60.0f, &Button, 0);
			Button.VSplitLeft(60.0f, &Button, 0);
			float ImageRatio = p==SKINPART_EYES?2.0f:1.0f;
			if(DoButton_Customize(&s_aButtons[p], Tex, Sprite, &Button, ImageRatio))
			{
				m_TeePartSelection = gs_aSelectionParts[p];
				m_TeePartsColorSelection = gs_aSelectionParts[p];
			}
			if(UI()->DoColorSelectionLogic(&ColorSelection, &Button))
			{
				static int s_InitialSelectedPart = -1;
				int AddSelection = NO_SELECTION;
				if(Input()->KeyPressed(KEY_RSHIFT) || Input()->KeyPressed(KEY_LSHIFT))
				{
					if(s_InitialSelectedPart != -1)
					{
						int Min = p < s_InitialSelectedPart ? p : s_InitialSelectedPart;
						int Max = p > s_InitialSelectedPart ? p : s_InitialSelectedPart;
						for(int p2 = Min; p2 <= Max; p2++)
							AddSelection |= gs_aSelectionParts[p2];
					}
				}
				else
				{
					AddSelection = gs_aSelectionParts[p];
					s_InitialSelectedPart = p;
				}
				if(Input()->KeyPressed(KEY_RCTRL) || Input()->KeyPressed(KEY_LCTRL))
				{
					if(m_TeePartsColorSelection & gs_aSelectionParts[p])
						m_TeePartsColorSelection &= ~AddSelection;
					else
						m_TeePartsColorSelection |= AddSelection;
				}
				else
					m_TeePartsColorSelection = AddSelection;
				m_TeePartSelection = NO_SELECTION;
			}
		}
	}

	// Save Skin :
	{
		SaveSkin.HSplitTop(20.0f, &SaveSkin, 0);
		SaveSkin.VSplitLeft(160.0f, &EditBox, &Button);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(80.0f, &Button, 0);
		static float s_OffsetSkin = 0.0f;
		static int s_EditBox;
		DoEditBox(&s_EditBox, &EditBox, m_aSaveSkinName, sizeof(m_aSaveSkinName), 14.0f, &s_OffsetSkin);
		static int s_SaveButton;
		if(DoButton_Menu(&s_SaveButton, Localize("Save skin"), 0, &Button) || (UI()->LastActiveItem() == &s_EditBox && m_EnterPressed))
		{
			if(m_aSaveSkinName[0])
				m_Popup = POPUP_SAVE_SKIN;
		}
	}

	if(m_TeePartsColorSelection == NO_SELECTION || m_TeePartSelection != NO_SELECTION)
		return;

	bool None = true;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		if((m_TeePartsColorSelection & gs_aSelectionParts[p]) && gs_apSkinVariables[p][0])
			None = false;
	}
	if(None)
		return;

	int CheckedUCC = 0;

	// Use Custom Color :
	{
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if((m_TeePartsColorSelection & gs_aSelectionParts[p]) && *gs_apUCCVariables[p])
				CheckedUCC = 1;
		}

		UCC.VSplitLeft(40.0f, 0, &UCC);
		UCC.HSplitTop(5.0f, 0, &UCC);
		UCC.HSplitTop(20.0f, &Button, &Picker);
		static int s_UCCButton = 0;
		if(DoButton_CheckBox(&s_UCCButton, Localize("Custom colors"), CheckedUCC, &Button))
		{
			CheckedUCC ^= 1;
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				if(m_TeePartsColorSelection & gs_aSelectionParts[p])
					*gs_apUCCVariables[p] = CheckedUCC;
			}
		}
	}

	if(!CheckedUCC)
		return;

	// HSL Picker :
	{
		Picker.HSplitTop(10.0f, 0, &Picker);
		Picker.VSplitLeft(20.0f, 0, &Picker);
		RenderHSLPicker(Picker);
	}
}

typedef void (*pfnAssignFuncCallback)(CConfiguration *pConfig, int Value);

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
{
	{ "Move left", "+left", 0},		// Localize - these strings are localized within CLocConstString
	{ "Move right", "+right", 0 },
	{ "Jump", "+jump", 0 },
	{ "Fire", "+fire", 0 },
	{ "Hook", "+hook", 0 },
	{ "Hammer", "+weapon1", 0 },
	{ "Pistol", "+weapon2", 0 },
	{ "Shotgun", "+weapon3", 0 },
	{ "Grenade", "+weapon4", 0 },
	{ "Laser", "+weapon5", 0 },
	{ "Next weapon", "+nextweapon", 0 },
	{ "Prev. weapon", "+prevweapon", 0 },
	{ "Vote yes", "vote yes", 0 },
	{ "Vote no", "vote no", 0 },
	{ "Chat", "chat all", 0 },
	{ "Team chat", "chat team", 0 },
	{ "Show chat", "+show_chat", 0 },
	{ "Emoticon", "+emote", 0 },
	{ "Spectator mode", "+spectate", 0 },
	{ "Spectate next", "spectate_next", 0 },
	{ "Spectate previous", "spectate_previous", 0 },
	{ "Console", "toggle_local_console", 0 },
	{ "Remote console", "toggle_remote_console", 0 },
	{ "Screenshot", "screenshot", 0 },
	{ "Scoreboard", "+scoreboard", 0 },
	{ "Respawn", "kill", 0 },
	{ "Ready", "ready_change", 0 },
};

/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Laser");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");
	Localize("Screenshot");Localize("Scoreboard");Localize("Respawn");Localize("Ready");
*/

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View)
{
	for (int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(135.0f, &Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
		int OldId = Key.m_KeyId;
		int NewId = DoKeyReader((void *)&gs_aKeys[i].m_Name, &Button, OldId);
		if(NewId != OldId)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_pBinds->Bind(OldId, "");
			if(NewId != 0)
				m_pClient->m_pBinds->Bind(NewId, gs_aKeys[i].m_pCommand);
		}
		View.HSplitTop(5.0f, 0, &View);
	}
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	CUIRect MovementSettings, WeaponSettings, VotingSettings, ChatSettings, MiscSettings, ResetButton;
	MainView.VSplitMid(&MovementSettings, &VotingSettings);

	// movement settings
	{
		MovementSettings.VMargin(5.0f, &MovementSettings);
		MovementSettings.HSplitTop(MainView.h/3+32.0f, &MovementSettings, &WeaponSettings);
		RenderTools()->DrawUIRect(&MovementSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		MovementSettings.Margin(10.0f, &MovementSettings);

		TextRender()->Text(0, MovementSettings.x, MovementSettings.y, 14.0f*UI()->Scale(), Localize("Movement"), -1);

		MovementSettings.HSplitTop(14.0f+7.0f, 0, &MovementSettings);

		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(135.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Mouse sens."), 14.0f*UI()->Scale(), -1);
			Button.HMargin(2.0f, &Button);
			static int s_InpMousesense = 0;
			g_Config.m_InpMousesens = (int)(DoScrollbarH(&s_InpMousesense, &Button, (g_Config.m_InpMousesens-5)/500.0f)*500.0f)+5;
			//*key.key = ui_do_key_reader(key.key, &Button, *key.key);
			MovementSettings.HSplitTop(10.0f, 0, &MovementSettings);
		}

		UiDoGetButtons(0, 5, MovementSettings);

	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(10.0f, 0, &WeaponSettings);
		WeaponSettings.HSplitTop(MainView.h/3+52.0f, &WeaponSettings, &ResetButton);
		RenderTools()->DrawUIRect(&WeaponSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		WeaponSettings.Margin(10.0f, &WeaponSettings);

		TextRender()->Text(0, WeaponSettings.x, WeaponSettings.y, 14.0f*UI()->Scale(), Localize("Weapon"), -1);

		WeaponSettings.HSplitTop(14.0f+7.0f, 0, &WeaponSettings);
		UiDoGetButtons(5, 12, WeaponSettings);
	}

	// defaults
	{
		ResetButton.HSplitTop(10.0f, 0, &ResetButton);
		RenderTools()->DrawUIRect(&ResetButton, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		ResetButton.HMargin(17.0f, &ResetButton);
		ResetButton.VMargin(30.0f, &ResetButton);
		ResetButton.HSplitTop(20.0f, &ResetButton, 0);
		static int s_DefaultButton = 0;
		if(DoButton_Menu((void*)&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
			m_pClient->m_pBinds->SetDefaults();
	}

	// voting settings
	{
		VotingSettings.VMargin(5.0f, &VotingSettings);
		VotingSettings.HSplitTop(MainView.h/3-73.0f, &VotingSettings, &ChatSettings);
		RenderTools()->DrawUIRect(&VotingSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		VotingSettings.Margin(10.0f, &VotingSettings);

		TextRender()->Text(0, VotingSettings.x, VotingSettings.y, 14.0f*UI()->Scale(), Localize("Voting"), -1);

		VotingSettings.HSplitTop(14.0f+7.0f, 0, &VotingSettings);
		UiDoGetButtons(12, 14, VotingSettings);
	}

	// chat settings
	{
		ChatSettings.HSplitTop(10.0f, 0, &ChatSettings);
		ChatSettings.HSplitTop(MainView.h/3-48.0f, &ChatSettings, &MiscSettings);
		RenderTools()->DrawUIRect(&ChatSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		ChatSettings.Margin(10.0f, &ChatSettings);

		TextRender()->Text(0, ChatSettings.x, ChatSettings.y, 14.0f*UI()->Scale(), Localize("Chat"), -1);

		ChatSettings.HSplitTop(14.0f+7.0f, 0, &ChatSettings);
		UiDoGetButtons(14, 17, ChatSettings);
	}

	// misc settings
	{
		MiscSettings.HSplitTop(10.0f, 0, &MiscSettings);
		RenderTools()->DrawUIRect(&MiscSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		MiscSettings.Margin(10.0f, &MiscSettings);

		TextRender()->Text(0, MiscSettings.x, MiscSettings.y, 14.0f*UI()->Scale(), Localize("Miscellaneous"), -1);

		MiscSettings.HSplitTop(14.0f+7.0f, 0, &MiscSettings);
		UiDoGetButtons(17, 26, MiscSettings);
	}

}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CUIRect Button;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);
	static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static int s_GfxColorDepth = g_Config.m_GfxColorDepth;
	static int s_GfxBorderless = g_Config.m_GfxBorderless;
	static int s_GfxFullscreen = g_Config.m_GfxFullscreen;
	static int s_GfxVsync = g_Config.m_GfxVsync;
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;

	CUIRect ModeList;
	MainView.VSplitLeft(300.0f, &MainView, &ModeList);

	// draw allmodes switch
	ModeList.HSplitTop(20, &Button, &ModeList);
	static int s_GfxDisplayAllModes = 0;
	if(DoButton_CheckBox(&s_GfxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
	{
		g_Config.m_GfxDisplayAllModes ^= 1;
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);
	}

	// display mode list
	static float s_ScrollValue = 0;
	static int s_DisplayModeList = 0;
	int OldSelected = -1;
	int G = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
	str_format(aBuf, sizeof(aBuf), "%s: %dx%d %d bit (%d:%d)", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxColorDepth, s_GfxScreenWidth/G, s_GfxScreenHeight/G);
	UiDoListboxStart(&s_DisplayModeList , &ModeList, 24.0f, Localize("Display Modes"), aBuf, s_NumNodes, 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red+s_aModes[i].m_Green+s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_Width &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_Height)
		{
			OldSelected = i;
		}

		CListboxItem Item = UiDoListboxNextItem(&s_aModes[i], OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(s_aModes[i].m_Width, s_aModes[i].m_Height);
			str_format(aBuf, sizeof(aBuf), " %dx%d %d bit (%d:%d)", s_aModes[i].m_Width, s_aModes[i].m_Height, Depth, s_aModes[i].m_Width/G, s_aModes[i].m_Height/G);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red+s_aModes[NewSelected].m_Green+s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_Height;
		CheckSettings = true;
	}

	// switches
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxBorderless = 0;
	if(DoButton_CheckBox(&s_ButtonGfxBorderless, Localize("Borderless window"), g_Config.m_GfxBorderless, &Button))
	{
		g_Config.m_GfxBorderless ^= 1;
		if(g_Config.m_GfxBorderless && g_Config.m_GfxFullscreen)
			g_Config.m_GfxFullscreen = 0;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxFullscreen = 0;
	if(DoButton_CheckBox(&s_ButtonGfxFullscreen, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		g_Config.m_GfxFullscreen ^= 1;
		if(g_Config.m_GfxFullscreen && g_Config.m_GfxBorderless)
			g_Config.m_GfxBorderless = 0;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxVsync = 0;
	if(DoButton_CheckBox(&s_ButtonGfxVsync, Localize("V-Sync"), g_Config.m_GfxVsync, &Button))
	{
		g_Config.m_GfxVsync ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxFsaaSamples = 0;
	if(DoButton_CheckBox_Number(&s_ButtonGfxFsaaSamples, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button))
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples+1)%17;
		CheckSettings = true;
	}

	MainView.HSplitTop(40.0f, &Button, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxTextureQuality = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureQuality, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxTextureCompression = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureCompression, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonGfxHighDetail = 0;
	if(DoButton_CheckBox(&s_ButtonGfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;

	// check if the new settings require a restart
	if(CheckSettings)
	{
		if(s_GfxScreenWidth == g_Config.m_GfxScreenWidth &&
			s_GfxScreenHeight == g_Config.m_GfxScreenHeight &&
			s_GfxColorDepth == g_Config.m_GfxColorDepth &&
			s_GfxBorderless == g_Config.m_GfxBorderless &&
			s_GfxFullscreen == g_Config.m_GfxFullscreen &&
			s_GfxVsync == g_Config.m_GfxVsync &&
			s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
			s_GfxTextureQuality == g_Config.m_GfxTextureQuality &&
			s_GfxTextureCompression == g_Config.m_GfxTextureCompression)
			m_NeedRestartGraphics = false;
		else
			m_NeedRestartGraphics = true;
	}

	//

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, -1);

	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht."),
		Localize("Alpha")};
	int *pColorSlider[4] = {&g_Config.m_UiColorHue, &g_Config.m_UiColorSat, &g_Config.m_UiColorLht, &g_Config.m_UiColorAlpha};
	static int s_aColorSliders[4] = {0};
	for(int s = 0; s < 4; s++)
	{
		CUIRect Text;
		MainView.HSplitTop(19.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);
		//Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = (*pColorSlider[s]) / 255.0f;
		k = DoScrollbarH(&s_aColorSliders[s], &Button, k);
		*pColorSlider[s] = (int)(k*255.0f);
		UI()->DoLabelScaled(&Text, paLabels[s], 15.0f, -1);
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Button;
	MainView.VSplitMid(&MainView, 0);
	static int s_SndEnable = g_Config.m_SndEnable;
	static int s_SndRate = g_Config.m_SndRate;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonSndEnable = 0;
	if(DoButton_CheckBox(&s_ButtonSndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		if(g_Config.m_SndEnable)
		{
			if(g_Config.m_SndMusic)
				m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		}
		else
			m_pClient->m_pSounds->Stop(SOUND_MENU);
		m_NeedRestartSound = g_Config.m_SndEnable && (!s_SndEnable || s_SndRate != g_Config.m_SndRate);
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonSndMusic = 0;
	if(DoButton_CheckBox(&s_ButtonSndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
	{
		g_Config.m_SndMusic ^= 1;
		if(Client()->State() == IClient::STATE_OFFLINE)
		{
			if(g_Config.m_SndMusic)
				m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
			else
				m_pClient->m_pSounds->Stop(SOUND_MENU);
		}
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static int s_ButtonSndNonactiveMute = 0;
	if(DoButton_CheckBox(&s_ButtonSndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	// sample rate box
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_SndRate);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoLabelScaled(&Button, Localize("Sample rate"), 14.0f, -1);
		Button.VSplitLeft(190.0f, 0, &Button);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_SndRate, &Button, aBuf, sizeof(aBuf), 14.0f, &Offset);
		g_Config.m_SndRate = max(1, str_toint(aBuf));
		m_NeedRestartSound = !s_SndEnable || s_SndRate != g_Config.m_SndRate;
	}

	// volume slider
	{
		CUIRect Button, Label;
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(190.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sound volume"), 14.0f, -1);
		g_Config.m_SndVolume = (int)(DoScrollbarH(&g_Config.m_SndVolume, &Button, g_Config.m_SndVolume/100.0f)*100.0f);
		MainView.HSplitTop(20.0f, 0, &MainView);
	}
}

void CMenus::RenderSettings(CUIRect MainView)
{
	CUIRect RestartWarning;
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	MainView.Margin(10.0f, &MainView);

	// handle which page should be rendered
	if(g_Config.m_UiSettingsPage == SETTINGS_GENERAL)
		RenderSettingsGeneral(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_PLAYER)
		RenderSettingsPlayer(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_TEE)
		RenderSettingsTee(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_CONTROLS)
		RenderSettingsControls(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_GRAPHICS)
		RenderSettingsGraphics(MainView);
	else if(g_Config.m_UiSettingsPage == SETTINGS_SOUND)
		RenderSettingsSound(MainView);

	// reset warning
	if(m_NeedRestartGraphics || m_NeedRestartSound)
		UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 15.0f, -1);
}
