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

int CMenus::DoButton_Customize(const void *pID, IGraphics::CTextureHandle Texture, int SpriteID, const CUIRect *pRect, float ImageRatio)
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

void CMenus::RenderHSLPicker(CUIRect MainView)
{
	// check if we need the color stuff
	bool Checked = false;
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		if(m_TeePartsSelected&(1<<i))
		{
			if(*gs_apUCCVariables[i])
			{
				Checked = true;
				break;
			}
		}
	}

	CUIRect Label;

	// background
	float Spacing = 2.0f;
	float ButtonHeight = 20.0f;
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Coloration"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	// dont do all the rest if not needed
	if(!Checked)
	{
		// render background of color selection here anyway
		MainView.HSplitTop(Spacing, 0, &MainView);
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		return;
	}

	bool Modified = false;

	int ConfigColor = -1;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		if(!(m_TeePartsSelected&(1<<p)))
			continue;
		int Val = (*gs_apColorVariables[p])&0xffffff;
		if(ConfigColor != -1 && ConfigColor != Val)
		{
			ConfigColor = -1;
			break;
		}
		ConfigColor = Val;
	}

	CUIRect Button, Picker;
	MainView.HSplitTop(Spacing, 0, &MainView);

	bool UseAlpha = (m_TeePartsSelected & SELECTION_TATTOO) && *gs_apUCCVariables[SKINPART_TATTOO];

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

	MainView.HSplitTop(UseAlpha ? 186.0f : 208.0f, &Picker, &MainView);
	RenderTools()->DrawUIRect(&Picker, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// Hue/Lgt picker :
	{
		Picker.VMargin((Picker.w-256.0f)/2.0f, &Picker);
		Picker.HMargin((Picker.h-128.0f)/2.0f, &Picker);
		//Picker.VSplitLeft(256.0f, &Picker, 0);
		//Picker.HSplitTop(128.0f, &Picker, 0);

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
		Graphics()->TextureClear();
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

	MainView.HSplitTop(Spacing, 0, &MainView);

	// H/S/L/A sliders :
	{
		int NumBars = UseAlpha ? 4 : 3;
		const char *const apNames[4] = {Localize("Hue:"), Localize("Sat:"), Localize("Lgt:"), Localize("Alp:")};
		int *const apVars[4] = {&Hue, &Sat, &Lgt, &Alp};
		static int s_aButtons[12];

		for(int i = 0; i < NumBars; i++)
		{
			CUIRect Bar;
			// label
			MainView.HSplitTop(ButtonHeight, &Label, &MainView);
			MainView.HSplitTop(Spacing, 0, &MainView);
			RenderTools()->DrawUIRect(&Label, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

			Label.VSplitLeft((Label.w-168.0f)/2.0f, &Label, &Button);
			Label.y += 2.0f;
			UI()->DoLabelScaled(&Label, apNames[i], ButtonHeight*ms_FontmodHeight*0.8f, 0);

			// button <
			Button.VSplitLeft(Button.h, &Button, &Bar);
			if(DoButton_Menu(&s_aButtons[i*3], "<", 0, &Button, 5.0f, 0.0f, CUI::CORNER_TL|CUI::CORNER_BL))
			{
				*apVars[i] = max(0, *apVars[i]-1);
				Modified = true;
			}

			// bar
			Bar.VSplitLeft(256.0f/2.0f, &Bar, &Button);
			Graphics()->TextureClear();
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
			Button.VSplitLeft(Button.h, &Button, &Label);
			if(DoButton_Menu(&s_aButtons[i*3+1], ">", 0, &Button, 5.0f, 0.0f, CUI::CORNER_TR|CUI::CORNER_BR))
			{
				*apVars[i] = min(255, *apVars[i]+1);
				Modified = true;
			}

			// label value
			char aBuf[16];
			str_format(aBuf, sizeof(aBuf), "%d", *apVars[i]);
			Label.y += 2.0f;
			UI()->DoLabelScaled(&Label, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, 0);
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
			if(m_TeePartsSelected&(1<<p))
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

	int OldSelected = -1;
	UiDoListboxHeader(&MainView, Localize("Skins"), 20.0f, 2.0f);
	UiDoListboxStart(&s_InitSkinlist, 50.0f, 0, s_paSkinList.size(), 12, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkins::CSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, g_Config.m_PlayerSkin))
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
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

			Info.m_Size = 40.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
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
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		if(m_TeePartsSelected == 1<<i)
			p = i;
	}
	if(p < 0)
	{
		// fake list layout for better looks
		CUIRect Label;
		float Spacing = 2.0f;
		float ButtonHeight = 20.0f;
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		MainView.HSplitTop(ButtonHeight, &Label, &MainView);
		Label.y += 2.0f;
		UI()->DoLabel(&Label, Localize("Selection"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

		MainView.HSplitTop(Spacing, 0, &MainView);
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		return;
	}

	int OldSelected = -1;
	UiDoListboxHeader(&MainView, Localize("Selection"), 20.0f, 2.0f);
	UiDoListboxStart(&s_InitSkinPartList, 50.0f, 0, s_paList[p].size(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paList[p].size(); ++i)
	{
		const CSkins::CSkinPart *s = s_paList[p][i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, gs_apSkinVariables[p]))
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paList[p][i], OldSelected == i);
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
			Info.m_Size = 40.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2), m_TeePartsSelected, true);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(NewSelected != -1)
	{
		if(NewSelected != OldSelected)
		{
			const CSkins::CSkinPart *s = s_paList[p][NewSelected];
			mem_copy(gs_apSkinVariables[p], s->m_aName, 24);
		}
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

	UiDoListboxHeader(&MainView, Localize("Language"), 20.0f, 2.0f);
	UiDoListboxStart(&s_LanguageList, 20.0f, 0, s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

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
			Item.m_Rect.y += 2.0f;
			if(!str_comp(s_Languages[s_SelectedLanguage].m_Name, r.front().m_Name))
			{
				TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
				TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
				UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, Item.m_Rect.h*ms_FontmodHeight*0.8f, -1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			}
			else
 				UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, Item.m_Rect.h*ms_FontmodHeight*0.8f, -1);
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
	CUIRect Label, Button, Game, Client, BottomView;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render game menu backgrounds
	int NumOptions = g_Config.m_ClNameplates ? 8 : 5;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Game, &MainView);
	RenderTools()->DrawUIRect(&Game, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render client menu background
	NumOptions = 2;
	if(g_Config.m_ClAutoDemoRecord) NumOptions += 1;
	if(g_Config.m_ClAutoScreenshot) NumOptions += 1;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Client, &MainView);
	RenderTools()->DrawUIRect(&Client, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render game menu
	Game.HSplitTop(ButtonHeight, &Label, &Game);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Game"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	Game.HSplitTop(Spacing, 0, &Game);
	Game.HSplitTop(ButtonHeight, &Button, &Game);
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

	Game.HSplitTop(Spacing, 0, &Game);
	Game.HSplitTop(ButtonHeight, &Button, &Game);
	static int s_AutoswitchWeapons = 0;
	if(DoButton_CheckBox(&s_AutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
		g_Config.m_ClAutoswitchWeapons ^= 1;

	Game.HSplitTop(Spacing, 0, &Game);
	Game.HSplitTop(ButtonHeight, &Button, &Game);
	static int s_Showhud = 0;
	if(DoButton_CheckBox(&s_Showhud, Localize("Show ingame HUD"), g_Config.m_ClShowhud, &Button))
		g_Config.m_ClShowhud ^= 1;

	Game.HSplitTop(Spacing, 0, &Game);
	Game.HSplitTop(ButtonHeight, &Button, &Game);
	static int s_Friendchat = 0;
	if(DoButton_CheckBox(&s_Friendchat, Localize("Show only chat messages from friends"), g_Config.m_ClShowChatFriends, &Button))
		g_Config.m_ClShowChatFriends ^= 1;

	Game.HSplitTop(Spacing, 0, &Game);
	Game.HSplitTop(ButtonHeight, &Button, &Game);
	static int s_Nameplates = 0;
	if(DoButton_CheckBox(&s_Nameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
		g_Config.m_ClNameplates ^= 1;

	if(g_Config.m_ClNameplates)
	{
		Game.HSplitTop(Spacing, 0, &Game);
		Game.HSplitTop(ButtonHeight, &Button, &Game);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_NameplatesAlways = 0;
		if(DoButton_CheckBox(&s_NameplatesAlways, Localize("Always show name plates"), g_Config.m_ClNameplatesAlways, &Button))
			g_Config.m_ClNameplatesAlways ^= 1;

		Game.HSplitTop(Spacing, 0, &Game);
		Game.HSplitTop(ButtonHeight, &Button, &Game);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClNameplatesSize, &g_Config.m_ClNameplatesSize, &Button, Localize("Size"), 100.0f, 0, 100);

		Game.HSplitTop(Spacing, 0, &Game);
		Game.HSplitTop(ButtonHeight, &Button, &Game);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_NameplatesTeamcolors = 0;
		if(DoButton_CheckBox(&s_NameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
			g_Config.m_ClNameplatesTeamcolors ^= 1;
	}

	// render client menu
	Client.HSplitTop(ButtonHeight, &Label, &Client);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Client"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	static int s_AutoDemoRecord = 0;
	if(DoButton_CheckBox(&s_AutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
		g_Config.m_ClAutoDemoRecord ^= 1;

	if(g_Config.m_ClAutoDemoRecord)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClAutoDemoMax, &g_Config.m_ClAutoDemoMax, &Button, Localize("Max"), 100.0f, 0, 1000, true);
	}

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	static int s_AutoScreenshot = 0;
	if(DoButton_CheckBox(&s_AutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
		g_Config.m_ClAutoScreenshot ^= 1;

	if(g_Config.m_ClAutoScreenshot)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		DoScrollbarOption(&g_Config.m_ClAutoScreenshotMax, &g_Config.m_ClAutoScreenshotMax, &Button, Localize("Max"), 100.0f, 0, 1000, true);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	// render language selection
	RenderLanguageSelection(MainView);

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static int s_ResetButton=0;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		g_Config.m_ClMouseFollowfactor = 60;
		g_Config.m_ClMouseMaxDistance = 1000;
		g_Config.m_ClMouseDeadzone = 300;
		g_Config.m_ClAutoswitchWeapons = 1;
		g_Config.m_ClShowhud = 1;
		g_Config.m_ClShowChatFriends = 0;
		g_Config.m_ClNameplates = 1;
		g_Config.m_ClNameplatesAlways = 1;
		g_Config.m_ClNameplatesSize = 50;
		g_Config.m_ClNameplatesTeamcolors = 1;
		g_Config.m_ClAutoDemoRecord = 0;
		g_Config.m_ClAutoDemoMax = 10;
		g_Config.m_ClAutoScreenshot = 0;
		g_Config.m_ClAutoScreenshotMax = 10;
	}
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CUIRect Button, Left, Right, TopView, Label;

	// render game menu backgrounds
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = 2.0f*ButtonHeight+Spacing;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitBottom(80.0f, &MainView, 0); // now we have the total rect for the settings
	MainView.HSplitTop(BackgroundHeight, &TopView, &MainView);
	RenderTools()->DrawUIRect(&TopView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render game menu
	TopView.HSplitTop(ButtonHeight, &Label, &TopView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Personal"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	// split menu
	TopView.HSplitTop(Spacing, 0, &TopView);
	TopView.VSplitMid(&Left, &Right);
	Left.VSplitRight(1.5f, &Left, 0);
	Right.VSplitLeft(1.5f, 0, &Right);

	// left menu
	Left.HSplitTop(ButtonHeight, &Button, &Left);
	static float s_OffsetName = 0.0f;
	DoEditBoxOption(g_Config.m_PlayerName, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), &Button, Localize("Name"),  100.0f, &s_OffsetName);

	// right menu
	Right.HSplitTop(ButtonHeight, &Button, &Right);
	static float s_OffsetClan = 0.0f;
	DoEditBoxOption(g_Config.m_PlayerClan, g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), &Button, Localize("Clan"),  100.0f, &s_OffsetClan);

	// country flag selector
	MainView.HSplitTop(10.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	UiDoListboxHeader(&MainView, Localize("Country"), 20.0f, 2.0f);
	UiDoListboxStart(&s_ScrollValue, 40.0f, 0, m_pClient->m_pCountryFlags->Num(), 18, OldSelected, s_ScrollValue);

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
			if(i == OldSelected)
			{
				TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
				TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			}
			else
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		g_Config.m_PlayerCountry = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
	}
}

void CMenus::RenderSettingsTeeBasic(CUIRect MainView)
{
	RenderSkinSelection(MainView); // yes thats all here ^^
}

void CMenus::RenderSettingsTeeCustom(CUIRect MainView)
{
	CUIRect Label, Patterns, Button, Left, Right;

	// render skin preview background
	float SpacingH = 2.0f;
	float SpacingW = 3.0f;
	float ButtonHeight = 20.0f;
	float BoxSize = 297.0f;
	float BackgroundHeight = (ButtonHeight+SpacingH)*3.0f+BoxSize;

	MainView.HSplitTop(BackgroundHeight, &MainView, 0);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Customize"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	// skin part selection
	static const char *const apParts[6] = {Localize("Body"), Localize("Tattoo"), Localize("Decoration"),
											Localize("Hands"), Localize("Feet"), Localize("Eyes")};

	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.HSplitTop(ButtonHeight, &Patterns, &MainView);
	RenderTools()->DrawUIRect(&Patterns, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	float ButtonWidth = (Patterns.w/6.0f)-(SpacingW*5.0)/6.0f;

	static int s_PatternButtons[6] = {0};
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		Patterns.VSplitLeft(ButtonWidth, &Button, &Patterns);
		if(DoButton_MenuTabTop(&s_PatternButtons[i], apParts[i], m_TeePartsSelected&(1<<i), &Button))
		{
			if(Input()->KeyPressed(KEY_RCTRL) || Input()->KeyPressed(KEY_LCTRL))
				m_TeePartsSelected ^= 1<<i;
			else
				m_TeePartsSelected = 1<<i;
		}
		Patterns.VSplitLeft(SpacingW, 0, &Patterns);
	}

	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.HSplitTop(ButtonHeight, &Patterns, &MainView);
	static int s_PatternUCCBoxes[6] = {0};
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		Patterns.VSplitLeft(ButtonWidth, &Button, &Patterns);
		if(DoButton_CheckBox(&s_PatternUCCBoxes[i], Localize("Custom Color"), *gs_apUCCVariables[i], &Button))
		{
			*gs_apUCCVariables[i] ^= 1;
		}
		Patterns.VSplitLeft(SpacingW, 0, &Patterns);
	}

	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.VSplitMid(&Left, &Right);
	Left.VSplitRight(SpacingW/2.0f, &Left, 0);
	Right.VSplitLeft(SpacingW/2.0f, 0, &Right);

	// part selection
	RenderSkinPartSelection(Left);

	// HSL picker
	RenderHSLPicker(Right);
}

void CMenus::RenderSettingsTee(CUIRect MainView)
{
	static bool s_CustomSkinMenu = false;

	CUIRect Button, Label, BottomView, Preview;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render skin preview background
	float SpacingH = 2.0f;
	float SpacingW = 3.0f;
	float ButtonHeight = 20.0f;
	float SkinHeight = 45.0f;
	float BackgroundHeight = ButtonHeight+SpacingH+SkinHeight;
	if(!s_CustomSkinMenu)
		BackgroundHeight = (ButtonHeight+SpacingH)*2.0f+SkinHeight;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Preview, &MainView);
	RenderTools()->DrawUIRect(&Preview, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	Preview.HSplitTop(ButtonHeight, &Label, &Preview);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Skin"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	// Preview
	{
		CUIRect Left, Right;
		Preview.HSplitTop(SpacingH, 0, &Preview);
		Preview.HSplitTop(SkinHeight, &Left, &Preview);

		// split the menu in 2 parts
		Left.VSplitMid(&Left, &Right);
		Left.VSplitRight(SpacingW/2.0f, &Left, 0);
		Right.VSplitLeft(SpacingW/2.0f, 0, &Right);

		// handle left
		RenderTools()->DrawUIRect(&Left, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Left.VSplitMid(&Label, &Left);
		Label.y += 14.0f;
		UI()->DoLabelScaled(&Label, Localize("Normal:"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

		RenderTools()->DrawUIRect(&Left, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		CTeeRenderInfo OwnSkinInfo;
		OwnSkinInfo.m_Size = 40.0f;
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
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Left.x+Left.w/2.0f, Left.y+Left.h/2.0f+2.0f));

		// handle right (team skins)
		RenderTools()->DrawUIRect(&Right, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Right.VSplitLeft(Right.w/3.0f+SpacingW/2.0f, &Label, &Right);
		Label.y += 14.0f;
		UI()->DoLabelScaled(&Label, Localize("Team:"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

		Right.VSplitMid(&Left, &Right);
		Left.VSplitRight(SpacingW/2.0f, &Left, 0);
		Right.VSplitLeft(SpacingW/2.0f, 0, &Right);

		RenderTools()->DrawUIRect(&Left, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(*gs_apUCCVariables[p], *gs_apColorVariables[p], TEAM_RED, p);
			OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_TATTOO);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Left.x+Left.w/2.0f, Left.y+Left.h/2.0f+2.0f));

		RenderTools()->DrawUIRect(&Right, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(*gs_apUCCVariables[p], *gs_apColorVariables[p], TEAM_BLUE, p);
			OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_TATTOO);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Right.x+Right.w/2.0f, Right.y+Right.h/2.0f+2.0f));
	}

	if(!s_CustomSkinMenu)
	{
		Preview.HSplitTop(SpacingH, 0, &Preview);
		RenderTools()->DrawUIRect(&Preview, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Preview.y += 2.0f;
		UI()->DoLabel(&Preview, g_Config.m_PlayerSkin, ButtonHeight*ms_FontmodHeight*0.8f, 0);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	if(s_CustomSkinMenu)
		RenderSettingsTeeCustom(MainView);
	else
		RenderSettingsTeeBasic(MainView);

	// bottom button
	float ButtonWidth = (BottomView.w/6.0f)-(SpacingW*5.0)/6.0f;
	float BackgroundWidth = s_CustomSkinMenu ? ButtonWidth*2.0f+SpacingW : ButtonWidth;

	BottomView.VSplitRight(BackgroundWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	if(s_CustomSkinMenu)
	{
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static int s_CustomSkinSaveButton=0;
		if(DoButton_Menu(&s_CustomSkinSaveButton, Localize("Save"), 0, &Button))
			m_Popup = POPUP_SAVE_SKIN;
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);
	}

	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static int s_CustomSwitchButton=0;
	if(DoButton_Menu(&s_CustomSwitchButton, s_CustomSkinMenu ? Localize("Basic") : Localize("Custom"), 0, &Button))
	{
		if(s_CustomSkinMenu)
			s_CustomSkinMenu = false;
		else
			s_CustomSkinMenu = true;
	}
}

//typedef void (*pfnAssignFuncCallback)(CConfiguration *pConfig, int Value);

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	MainView.HSplitTop(20.0f, 0, &MainView);

	// cut view
	CUIRect BottomView, Button;
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	float HeaderHeight = 20.0f;

	static int s_MovementDropdown = 0;
	float Split = DoDropdownMenu(&s_MovementDropdown, &MainView, Localize("Movement"), HeaderHeight, RenderSettingsControlsMovement);

	MainView.HSplitTop(Split+10.0f, 0, &MainView);
	static int s_WeaponDropdown = 0;
	Split = DoDropdownMenu(&s_WeaponDropdown, &MainView, Localize("Weapon"), HeaderHeight, RenderSettingsControlsWeapon);

	MainView.HSplitTop(Split+10.0f, 0, &MainView);
	static int s_VotingDropdown = 0;
	Split = DoDropdownMenu(&s_VotingDropdown, &MainView, Localize("Voting"), HeaderHeight, RenderSettingsControlsVoting);

	MainView.HSplitTop(Split+10.0f, 0, &MainView);
	static int s_ChatDropdown = 0;
	Split = DoDropdownMenu(&s_ChatDropdown, &MainView, Localize("Chat"), HeaderHeight, RenderSettingsControlsChat);

	MainView.HSplitTop(Split+10.0f, 0, &MainView);
	static int s_MiscDropdown = 0;
	Split = DoDropdownMenu(&s_MiscDropdown, &MainView, Localize("Misc"), HeaderHeight, RenderSettingsControlsMisc);

	// reset button
	float Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static int s_ResetButton=0;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
		m_pClient->m_pBinds->SetDefaults();
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	char aBuf[128];
	bool CheckSettings = false;

	static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static int s_GfxBorderless = g_Config.m_GfxBorderless;
	static int s_GfxFullscreen = g_Config.m_GfxFullscreen;
	static int s_GfxVsync = g_Config.m_GfxVsync;
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;

	CUIRect Label, Button, Screen, Texture, BottomView;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render screen menu background
	int NumOptions = g_Config.m_GfxFullscreen ? 3 : 4;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Screen, &MainView);
	RenderTools()->DrawUIRect(&Screen, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render textures menu background
	NumOptions = 3;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Texture, &MainView);
	RenderTools()->DrawUIRect(&Texture, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render screen menu
	Screen.HSplitTop(ButtonHeight, &Label, &Screen);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Screen"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	Screen.HSplitTop(Spacing, 0, &Screen);
	Screen.HSplitTop(ButtonHeight, &Button, &Screen);
	static int s_ButtonGfxFullscreen = 0;
	if(DoButton_CheckBox(&s_ButtonGfxFullscreen, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		g_Config.m_GfxFullscreen ^= 1;
		if(g_Config.m_GfxFullscreen && g_Config.m_GfxBorderless)
			g_Config.m_GfxBorderless = 0;
		CheckSettings = true;
	}

	if(!g_Config.m_GfxFullscreen)
	{
		Screen.HSplitTop(Spacing, 0, &Screen);
		Screen.HSplitTop(ButtonHeight, &Button, &Screen);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonGfxBorderless = 0;
		if(DoButton_CheckBox(&s_ButtonGfxBorderless, Localize("Borderless window"), g_Config.m_GfxBorderless, &Button))
		{
			g_Config.m_GfxBorderless ^= 1;
			if(g_Config.m_GfxBorderless && g_Config.m_GfxFullscreen)
				g_Config.m_GfxFullscreen = 0;
			CheckSettings = true;
		}
	}

	Screen.HSplitTop(Spacing, 0, &Screen);
	Screen.HSplitTop(ButtonHeight, &Button, &Screen);
	static int s_ButtonGfxVsync = 0;
	if(DoButton_CheckBox(&s_ButtonGfxVsync, Localize("V-Sync"), g_Config.m_GfxVsync, &Button))
	{
		g_Config.m_GfxVsync ^= 1;
		CheckSettings = true;
	}

	// FSAA button
	{
		Screen.HSplitTop(Spacing, 0, &Screen);
		Screen.HSplitTop(ButtonHeight, &Button, &Screen);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Anti Aliasing"));
		Text.y += 2.0f;
		UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, -1);

		Button.VSplitLeft(70.0f, &Button, 0);
		str_format(aBuf, sizeof(aBuf), "%dx", g_Config.m_GfxFsaaSamples);
		static int s_ButtonGfxFsaaSamples = 0;
		if(DoButton_Menu(&s_ButtonGfxFsaaSamples, aBuf, 0, &Button))
		{
			if(!g_Config.m_GfxFsaaSamples)
				g_Config.m_GfxFsaaSamples = 2;
			else if(g_Config.m_GfxFsaaSamples == 16)
				g_Config.m_GfxFsaaSamples = 0;
			else
				g_Config.m_GfxFsaaSamples *= 2;
			CheckSettings = true;
		}
	}

	// render texture menu
	Texture.HSplitTop(ButtonHeight, &Label, &Texture);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Texture"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxTextureQuality = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureQuality, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxTextureCompression = 0;
	if(DoButton_CheckBox(&s_ButtonGfxTextureCompression, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	static int s_ButtonGfxHighDetail = 0;
	if(DoButton_CheckBox(&s_ButtonGfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;

	// render screen modes
	MainView.HSplitTop(10.0f, 0, &MainView);

	// display mode list
	{
		// custom list header
		NumOptions = 2;

		CUIRect Header, Button;
		MainView.HSplitTop(ButtonHeight*(float)(NumOptions+1)+Spacing*(float)(NumOptions+1), &Header, 0);
		RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_T, 5.0f);

		// draw header
		MainView.HSplitTop(ButtonHeight, &Header, &MainView);
		Header.y += 2.0f;
		UI()->DoLabel(&Header, Localize("Resolutions"), Header.h*ms_FontmodHeight*0.8f, 0);

		// supported modes button
		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		static int s_GfxDisplayAllModes = 0;
		if(DoButton_CheckBox(&s_GfxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
		{
			g_Config.m_GfxDisplayAllModes ^= 1;
			m_NumModes = Graphics()->GetVideoModes(m_aModes, MAX_RESOLUTIONS);
			UpdateVideoFormats();

			bool Found = false;
			for(int i = 0; i < m_NumVideoFormats; i++)
			{
				int G = gcd(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
				if(m_aVideoFormats[i].m_WidthValue == g_Config.m_GfxScreenWidth/G && m_aVideoFormats[i].m_HeightValue == g_Config.m_GfxScreenHeight/G)
				{
					m_CurrentVideoFormat = i;
					Found = true;
					break;
				}

			}

			if(!Found)
				m_CurrentVideoFormat = 0;

			UpdatedFilteredVideoModes();
		}

		// format changer
		{
			MainView.HSplitTop(Spacing, 0, &MainView);
			MainView.HSplitTop(ButtonHeight, &Button, &MainView);
			RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			CUIRect Text, Value, Unit;
			Button.VSplitLeft(Button.w/3.0f, &Text, &Button);
			Button.VSplitMid(&Value, &Unit);

			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "%s:", Localize("Format"));
			Text.y += 2.0f;
			UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, 0);

			Unit.y += 2.0f;
			if((float)m_aVideoFormats[m_CurrentVideoFormat].m_WidthValue/(float)m_aVideoFormats[m_CurrentVideoFormat].m_HeightValue >= 1.55f)
				UI()->DoLabel(&Unit, Localize("Wide"), Unit.h*ms_FontmodHeight*0.8f, 0);
			else
				UI()->DoLabel(&Unit, Localize("Letterbox"), Unit.h*ms_FontmodHeight*0.8f, 0);

			str_format(aBuf, sizeof(aBuf), "%d:%d", m_aVideoFormats[m_CurrentVideoFormat].m_WidthValue, m_aVideoFormats[m_CurrentVideoFormat].m_HeightValue);
			static int s_VideoFormatButton = 0;
			if(DoButton_Menu(&s_VideoFormatButton, aBuf, 0, &Value))
			{
				m_CurrentVideoFormat++;
				if(m_CurrentVideoFormat == m_NumVideoFormats)
					m_CurrentVideoFormat = 0;

				UpdatedFilteredVideoModes();
			}
		}

		MainView.HSplitTop(Spacing, 0, &MainView);

		static float s_ScrollValue = 0;
		static int s_DisplayModeList = 0;
		int OldSelected = -1;
		int G = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
		str_format(aBuf, sizeof(aBuf), "%s: %dx%d (%d:%d)", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxScreenWidth/G, s_GfxScreenHeight/G);
		UiDoListboxStart(&s_DisplayModeList, 20.0f, aBuf, m_lFilteredVideoModes.size(), 1, OldSelected, s_ScrollValue, &MainView);

		for(int i = 0; i < m_lFilteredVideoModes.size(); ++i)
		{
			if(g_Config.m_GfxScreenWidth == m_lFilteredVideoModes[i].m_Width &&
				g_Config.m_GfxScreenHeight == m_lFilteredVideoModes[i].m_Height)
			{
				OldSelected = i;
			}

			CListboxItem Item = UiDoListboxNextItem(&m_lFilteredVideoModes[i], OldSelected == i);
			if(Item.m_Visible)
			{
				int G = gcd(m_lFilteredVideoModes[i].m_Width, m_lFilteredVideoModes[i].m_Height);
				str_format(aBuf, sizeof(aBuf), " %dx%d", m_lFilteredVideoModes[i].m_Width, m_lFilteredVideoModes[i].m_Height);
				if(i == OldSelected)
				{
					TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
					TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
					Item.m_Rect.y += 2.0f;
					UI()->DoLabelScaled(&Item.m_Rect, aBuf, Item.m_Rect.h*ms_FontmodHeight*0.8f, 0);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
				}
				else
				{
					Item.m_Rect.y += 2.0f;
					UI()->DoLabelScaled(&Item.m_Rect, aBuf, Item.m_Rect.h*ms_FontmodHeight*0.8f, 0);
				}
			}
		}

		const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
		if(OldSelected != NewSelected)
		{
			g_Config.m_GfxScreenWidth = m_lFilteredVideoModes[NewSelected].m_Width;
			g_Config.m_GfxScreenHeight = m_lFilteredVideoModes[NewSelected].m_Height;
			CheckSettings = true;
		}
	}

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static int s_ResetButton=0;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		g_Config.m_GfxScreenWidth = Graphics()->GetDesktopScreenWidth();
		g_Config.m_GfxScreenHeight = Graphics()->GetDesktopScreenHeight();
		g_Config.m_GfxBorderless = 0;
		g_Config.m_GfxFullscreen = 1;
		g_Config.m_GfxVsync = 1;
		g_Config.m_GfxFsaaSamples = 0;
		g_Config.m_GfxTextureQuality = 1;
		g_Config.m_GfxTextureCompression = 0;
		g_Config.m_GfxHighDetail = 1;

		if(g_Config.m_GfxDisplayAllModes)
		{
			g_Config.m_GfxDisplayAllModes = 0;
			m_NumModes = Graphics()->GetVideoModes(m_aModes, MAX_RESOLUTIONS);
			UpdateVideoFormats();

			bool Found = false;
			for(int i = 0; i < m_NumVideoFormats; i++)
			{
				int G = gcd(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
				if(m_aVideoFormats[i].m_WidthValue == g_Config.m_GfxScreenWidth/G && m_aVideoFormats[i].m_HeightValue == g_Config.m_GfxScreenHeight/G)
				{
					m_CurrentVideoFormat = i;
					Found = true;
					break;
				}

			}

			if(!Found)
				m_CurrentVideoFormat = 0;

			UpdatedFilteredVideoModes();
		}

		CheckSettings = true;
	}

	// check if the new settings require a restart
	if(CheckSettings)
	{
		if(s_GfxScreenWidth == g_Config.m_GfxScreenWidth &&
			s_GfxScreenHeight == g_Config.m_GfxScreenHeight &&
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
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Label, Button, Sound, Detail, BottomView;

	// render sound menu background
	int NumOptions = g_Config.m_SndEnable ? 3 : 1;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Sound, &MainView);
	RenderTools()->DrawUIRect(&Sound, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// render detail menu background
	if(g_Config.m_SndEnable)
	{
		BackgroundHeight = 2.0f*ButtonHeight+Spacing;

		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(BackgroundHeight, &Detail, &MainView);
		RenderTools()->DrawUIRect(&Detail, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
	}

	static int s_SndEnable = g_Config.m_SndEnable;
	static int s_SndRate = g_Config.m_SndRate;

	// render sound menu
	Sound.HSplitTop(ButtonHeight, &Label, &Sound);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, Localize("Sound"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

	Sound.HSplitTop(Spacing, 0, &Sound);
	Sound.HSplitTop(ButtonHeight, &Button, &Sound);
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

	if(g_Config.m_SndEnable)
	{
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
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

		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		static int s_ButtonSndNonactiveMute = 0;
		if(DoButton_CheckBox(&s_ButtonSndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
			g_Config.m_SndNonactiveMute ^= 1;

		// render detail menu
		Detail.HSplitTop(ButtonHeight, &Label, &Detail);
		Label.y += 2.0f;
		UI()->DoLabel(&Label, Localize("Detail"), ButtonHeight*ms_FontmodHeight*0.8f, 0);

		// split menu
		CUIRect Left, Right;
		Detail.HSplitTop(Spacing, 0, &Detail);
		Detail.VSplitMid(&Left, &Right);
		Left.VSplitRight(1.5f, &Left, 0);
		Right.VSplitLeft(1.5f, 0, &Right);

		// sample rate thingy
		{
			Left.HSplitTop(ButtonHeight, &Button, &Left);

			RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			CUIRect Text, Value, Unit;
			Button.VSplitLeft(Button.w/3.0f, &Text, &Button);
			Button.VSplitMid(&Value, &Unit);

			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "%s:", Localize("Sample rate"));
			Text.y += 2.0f;
			UI()->DoLabel(&Text, aBuf, Text.h*ms_FontmodHeight*0.8f, 0);

			Unit.y += 2.0f;
			UI()->DoLabel(&Unit, "kHz", Unit.h*ms_FontmodHeight*0.8f, 0);

			if(g_Config.m_SndRate != 48000 && g_Config.m_SndRate != 44100)
				g_Config.m_SndRate = 48000;
			if(g_Config.m_SndRate == 48000)
				str_copy(aBuf, "48.0", sizeof(aBuf));
			else
				str_copy(aBuf, "44.1", sizeof(aBuf));
			static int s_SampleRateButton = 0;
			if(DoButton_Menu(&s_SampleRateButton, aBuf, 0, &Value))
			{
				if(g_Config.m_SndRate == 48000)
					g_Config.m_SndRate = 44100;
				else
					g_Config.m_SndRate = 48000;
			}

			m_NeedRestartSound = !s_SndEnable || s_SndRate != g_Config.m_SndRate;
		}

		Right.HSplitTop(ButtonHeight, &Button, &Right);
		DoScrollbarOption(&g_Config.m_SndVolume, &g_Config.m_SndVolume, &Button, Localize("Volume"), 110.0f, 0, 100);
	}

	// reset button
	MainView.HSplitBottom(60.0f, 0, &BottomView);

	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static int s_ResetButton=0;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		g_Config.m_SndEnable = 1;
		if(!g_Config.m_SndMusic)
		{
			g_Config.m_SndMusic = 1;
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		}
		g_Config.m_SndNonactiveMute = 0;
		g_Config.m_SndRate = 48000;
		g_Config.m_SndVolume = 100;
	}
}

void CMenus::RenderSettings(CUIRect MainView)
{
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

	MainView.HSplitBottom(60.0f, 0, &MainView);

	// reset warning
	if(m_NeedRestartGraphics || m_NeedRestartSound)
	{
		// background
		CUIRect RestartWarning;
		MainView.HSplitTop(25.0f, &RestartWarning, 0);
		RestartWarning.VMargin(140.0f, &RestartWarning);
		RenderTools()->DrawUIRect(&RestartWarning, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		// text
		TextRender()->TextColor(0.973f, 0.863f, 0.207, 1.0f);
		RestartWarning.y += 2.0f;
		UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), RestartWarning.h*ms_FontmodHeight*0.75f, 0);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
