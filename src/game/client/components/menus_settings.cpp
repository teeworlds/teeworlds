/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/color.h>
#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/external/json-parser/json.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/components/maplayers.h>
#include <game/client/components/sounds.h>
#include <game/client/components/stats.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		int TriggeringEvent = (Event.m_Key == KEY_MOUSE_1) ? IInput::FLAG_PRESS : IInput::FLAG_RELEASE;
		if(Event.m_Flags&TriggeringEvent) // delay to RELEASE to support composed binds
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;

			int Mask = CBinds::GetModifierMask(Input()); // always > 0
			m_Modifier = 0;
			while(!(Mask&1)) // this computes a log2, we take the first modifier flag in mask.
			{
				Mask >>= 1;
				m_Modifier++;
			}
			// prevent from adding e.g. a control modifier to lctrl
			if(CBinds::ModifierMatchesKey(m_Modifier, Event.m_Key))
				m_Modifier = 0;
		}
		return true;
	}

	return false;
}

void CMenus::RenderHSLPicker(CUIRect MainView)
{
	CUIRect Label, Button, Picker, Sliders;

	// background
	float Spacing = 2.0f;

	if(!(*CSkins::ms_apUCCVariables[m_TeePartSelected]))
		return;

	MainView.HSplitTop(Spacing, 0, &MainView);

	bool Modified = false;
	bool UseAlpha = m_TeePartSelected == SKINPART_MARKING;
	int Color = *CSkins::ms_apColorVariables[m_TeePartSelected];

	int Hue, Sat, Lgt, Alp;
	Hue = (Color>>16)&0xff;
	Sat = (Color>>8)&0xff;
	Lgt = Color&0xff;
	if(UseAlpha)
		Alp = (Color>>24)&0xff;


	MainView.VSplitMid(&Picker, &Sliders, 5.0f);
	float PickerSize = minimum(Picker.w, Picker.h) - 20.0f;
	Picker.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	float Dark = CSkins::DARKEST_COLOR_LGT/255.0f;
	IGraphics::CColorVertex ColorArray[4];

	// Hue/Lgt picker :
	{
		Picker.VMargin((Picker.w-PickerSize)/2.0f, &Picker);
		Picker.HMargin((Picker.h-PickerSize)/2.0f, &Picker);

		// picker
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();

		// base: grey - hue
		vec3 c = HslToRgb(vec3(Hue/255.0f, 0.0f, 0.5f));
		ColorArray[0] = IGraphics::CColorVertex(0, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 1.0f, 0.5f));
		ColorArray[1] = IGraphics::CColorVertex(1, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 1.0f, 0.5f));
		ColorArray[2] = IGraphics::CColorVertex(2, c.r, c.g, c.b, 1.0f);
		c = HslToRgb(vec3(Hue/255.0f, 0.0f, 0.5f));
		ColorArray[3] = IGraphics::CColorVertex(3, c.r, c.g, c.b, 1.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem QuadItem(Picker.x, Picker.y, Picker.w, Picker.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);

		// white blending
		ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[2] = IGraphics::CColorVertex(2, 1.0f, 1.0f, 1.0f, 1.0f);
		ColorArray[3] = IGraphics::CColorVertex(3, 1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem WhiteGradient(Picker.x, Picker.y + Picker.h*(1-2*Dark)/((1-Dark)*2), Picker.w, Picker.h/((1-Dark)*2));
		Graphics()->QuadsDrawTL(&WhiteGradient, 1);

		// black blending
		ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 1.0f-2*Dark);
		ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 1.0f-2*Dark);
		ColorArray[2] = IGraphics::CColorVertex(2, 0.0f, 0.0f, 0.0f, 0.0f);
		ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 0.0f);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem BlackGradient(Picker.x, Picker.y, Picker.w, Picker.h*(1-2*Dark)/((1-Dark)*2));
		Graphics()->QuadsDrawTL(&BlackGradient, 1);

		Graphics()->QuadsEnd();

		// marker
		vec2 Marker = vec2(Sat/255.0f*PickerSize, Lgt/255.0f*PickerSize);
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		IGraphics::CQuadItem aMarker[2];
		aMarker[0] = IGraphics::CQuadItem(Picker.x+Marker.x, Picker.y+Marker.y - 5.0f*UI()->PixelSize(), UI()->PixelSize(), 11.0f*UI()->PixelSize());
		aMarker[1] = IGraphics::CQuadItem(Picker.x+Marker.x - 5.0f*UI()->PixelSize(), Picker.y+Marker.y, 11.0f*UI()->PixelSize(), UI()->PixelSize());
		Graphics()->QuadsDrawTL(aMarker, 2);
		Graphics()->QuadsEnd();

		// logic
		float X, Y;
		static int s_HLPicker;
		if(UI()->DoPickerLogic(&s_HLPicker, &Picker, &X, &Y))
		{
			Sat = (int)(255.0f*X/Picker.w);
			Lgt = (int)(255.0f*Y/Picker.h);
			Modified = true;
		}
	}

	// H/S/L/A sliders :
	{
		int NumBars = UseAlpha ? 4 : 3;
		const char *const apNames[4] = {Localize("Hue:"), Localize("Sat:"), Localize("Lgt:"), Localize("Alp:")};
		int *const apVars[4] = {&Hue, &Sat, &Lgt, &Alp};
		static CButtonContainer s_aButtons[12];
		float SectionHeight = 40.0f;
		float SliderHeight = 16.0f;
		static const float s_aColorIndices[7][3] = {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
													{0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

		for(int i = 0; i < NumBars; i++)
		{
			CUIRect Bar, Section;

			Sliders.HSplitTop(SectionHeight, &Section, &Sliders);
			Sliders.HSplitTop(Spacing, 0, &Sliders);
			Section.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

			Section.HSplitTop(SectionHeight-SliderHeight, &Label, &Section);

			// label
			Label.VSplitMid(&Label, &Button, 0.0f);
			Label.y += 4.0f;
			UI()->DoLabel(&Label, apNames[i], SliderHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

			// value label
			char aBuf[16];
			str_format(aBuf, sizeof(aBuf), "%d", *apVars[i]);
			Button.y += 4.0f;
			UI()->DoLabel(&Button, aBuf, SliderHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

			// button <
			Section.VSplitLeft(SliderHeight, &Button, &Bar);
			if(DoButton_Menu(&s_aButtons[i*3], "<", 0, &Button, 0, CUIRect::CORNER_TL|CUIRect::CORNER_BL))
			{
				*apVars[i] = maximum(0, *apVars[i]-1);
				Modified = true;
			}

			// bar
			Bar.VSplitLeft(Section.w-SliderHeight*2, &Bar, &Button);
			int NumQuads = 1;
			if(i == 0)
				NumQuads = 6;
			else if(i == 2)
				NumQuads = 2;
			else
				NumQuads = 1;

			float Length = Bar.w/NumQuads;
			float Offset = Length;
			vec4 ColorL, ColorR;

			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			for(int j = 0; j < NumQuads; ++j)
			{
				switch(i)
				{
				case 0: // Hue
					{
						ColorL = vec4(s_aColorIndices[j][0], s_aColorIndices[j][1], s_aColorIndices[j][2], 1.0f);
						ColorR = vec4(s_aColorIndices[j+1][0], s_aColorIndices[j+1][1], s_aColorIndices[j+1][2], 1.0f);
					}
					break;
				case 1: // Sat
					{
						vec3 c = HslToRgb(vec3(Hue/255.0f, 0.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorL = vec4(c.r, c.g, c.b, 1.0f);
						c = HslToRgb(vec3(Hue/255.0f, 1.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorR = vec4(c.r, c.g, c.b, 1.0f);
					}
					break;
				case 2: // Lgt
					{
						if(j == 0)
						{
							// Dark - 0.5f
							vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark));
							ColorL = vec4(c.r, c.g, c.b, 1.0f);
							c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 0.5f));
							ColorR = vec4(c.r, c.g, c.b, 1.0f);
							Length = Offset = Bar.w - Bar.w/((1-Dark)*2);
						}
						else
						{
							// 0.5f - 0.0f
							vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 0.5f));
							ColorL = vec4(c.r, c.g, c.b, 1.0f);
							c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, 1.0f));
							ColorR = vec4(c.r, c.g, c.b, 1.0f);
							Length = Bar.w/((1-Dark)*2);
						}
					}
					break;
				default: // Alpha
					{
						vec3 c = HslToRgb(vec3(Hue/255.0f, Sat/255.0f, Dark+Lgt/255.0f*(1.0f-Dark)));
						ColorL = vec4(c.r, c.g, c.b, 0.0f);
						ColorR = vec4(c.r, c.g, c.b, 1.0f);
					}
				}

				ColorArray[0] = IGraphics::CColorVertex(0, ColorL.r, ColorL.g, ColorL.b, ColorL.a);
				ColorArray[1] = IGraphics::CColorVertex(1, ColorR.r, ColorR.g, ColorR.b, ColorR.a);
				ColorArray[2] = IGraphics::CColorVertex(2, ColorR.r, ColorR.g, ColorR.b, ColorR.a);
				ColorArray[3] = IGraphics::CColorVertex(3, ColorL.r, ColorL.g, ColorL.b, ColorL.a);
				Graphics()->SetColorVertex(ColorArray, 4);
				IGraphics::CQuadItem QuadItem(Bar.x+Offset*j, Bar.y, Length, Bar.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}

			// bar marker
			Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Bar.x + minimum(Bar.w, *apVars[i]/255.0f*Bar.w), Bar.y, UI()->PixelSize(), Bar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			// button >
			Button.VSplitLeft(SliderHeight, &Button, &Label);
			if(DoButton_Menu(&s_aButtons[i*3+1], ">", 0, &Button, 0, CUIRect::CORNER_TR|CUIRect::CORNER_BR))
			{
				*apVars[i] = minimum(255, *apVars[i]+1);
				Modified = true;
			}

			// logic
			float X;
			if(UI()->DoPickerLogic(&s_aButtons[i*3+2], &Bar, &X, 0))
			{
				*apVars[i] = X*255.0f/Bar.w;
				Modified = true;
			}
		}
	}

	if(Modified)
	{
		int NewVal = (Hue << 16) + (Sat << 8) + Lgt;
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			if(m_TeePartSelected == p)
				*CSkins::ms_apColorVariables[p] = NewVal;
		}
		if(UseAlpha)
			Config()->m_PlayerColorMarking = (Alp << 24) + NewVal;
		m_SkinModified = true;
	}
}

void CMenus::RenderSkinSelection(CUIRect MainView)
{
	static float s_LastSelectionTime = -10.0f;
	static sorted_array<const CSkins::CSkin *> s_paSkinList;
	static CListBox s_ListBox;
	if(m_RefreshSkinSelector)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
			// no special skins
			if((s->m_Flags&CSkins::SKINFLAG_SPECIAL) == 0 && s_ListBox.FilterMatches(s->m_aName))
			{
				s_paSkinList.add(s);
			}
		}
		m_RefreshSkinSelector = false;
	}

	m_pSelectedSkin = 0;
	int OldSelected = -1;
	s_ListBox.DoHeader(&MainView, Localize("Skins"), UI()->GetListHeaderHeight());
	m_RefreshSkinSelector = s_ListBox.DoFilter();
	s_ListBox.DoStart(60.0f, s_paSkinList.size(), 10, 1, OldSelected);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CSkins::CSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, Config()->m_PlayerSkin))
		{
			m_pSelectedSkin = s;
			OldSelected = i;
		}

		CListboxItem Item = s_ListBox.DoNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int p = 0; p < NUM_SKINPARTS; p++)
			{
				if(s->m_aUseCustomColors[p])
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_ColorTexture;
					Info.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(s->m_aPartColors[p], p==SKINPART_MARKING);
				}
				else
				{
					Info.m_aTextures[p] = s->m_apParts[p]->m_OrgTexture;
					Info.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}

			Info.m_Size = 50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top

			{
				// interactive tee: tee is happy to be selected
				int TeeEmote = (Item.m_Selected && s_LastSelectionTime + 0.75f > Client()->LocalTime()) ? EMOTE_HAPPY : EMOTE_NORMAL;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, TeeEmote, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));
			}

			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			UI()->DoLabelSelected(&Label, s->m_aName, Item.m_Selected, 10.0f, TEXTALIGN_CENTER);
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(NewSelected != -1 && NewSelected != OldSelected)
	{
		s_LastSelectionTime = Client()->LocalTime();
		m_pSelectedSkin = s_paSkinList[NewSelected];
		mem_copy(Config()->m_PlayerSkin, m_pSelectedSkin->m_aName, sizeof(Config()->m_PlayerSkin));
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			mem_copy(CSkins::ms_apSkinVariables[p], m_pSelectedSkin->m_apParts[p]->m_aName, MAX_SKIN_ARRAY_SIZE);
			*CSkins::ms_apUCCVariables[p] = m_pSelectedSkin->m_aUseCustomColors[p];
			*CSkins::ms_apColorVariables[p] = m_pSelectedSkin->m_aPartColors[p];
		}
		m_SkinModified = true;
	}
	OldSelected = NewSelected;
}

void CMenus::RenderSkinPartSelection(CUIRect MainView)
{
	static bool s_InitSkinPartList = true;
	static sorted_array<const CSkins::CSkinPart *> s_paList[6];
	static CListBox s_ListBox;
	if(s_InitSkinPartList)
	{
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			s_paList[p].clear();
			for(int i = 0; i < m_pClient->m_pSkins->NumSkinPart(p); ++i)
			{
				const CSkins::CSkinPart *s = m_pClient->m_pSkins->GetSkinPart(p, i);
				// no special skins
				if((s->m_Flags&CSkins::SKINFLAG_SPECIAL) == 0 && s_ListBox.FilterMatches(s->m_aName))
				{
					s_paList[p].add(s);
				}
			}
		}
		s_InitSkinPartList = false;
	}

	static int OldSelected = -1;
	s_ListBox.DoBegin(&MainView);
	s_InitSkinPartList = s_ListBox.DoFilter();
	s_ListBox.DoStart(60.0f, s_paList[m_TeePartSelected].size(), 5, 1, OldSelected);

	for(int i = 0; i < s_paList[m_TeePartSelected].size(); ++i)
	{
		const CSkins::CSkinPart *s = s_paList[m_TeePartSelected][i];
		if(s == 0)
			continue;
		if(!str_comp(s->m_aName, CSkins::ms_apSkinVariables[m_TeePartSelected]))
			OldSelected = i;

		CListboxItem Item = s_ListBox.DoNextItem(&s_paList[m_TeePartSelected][i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			for(int j = 0; j < NUM_SKINPARTS; j++)
			{
				int SkinPart = m_pClient->m_pSkins->FindSkinPart(j, CSkins::ms_apSkinVariables[j], false);
				const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(j, SkinPart);
				if(*CSkins::ms_apUCCVariables[j])
				{
					if(m_TeePartSelected == j)
						Info.m_aTextures[j] = s->m_ColorTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_ColorTexture;
					Info.m_aColors[j] = m_pClient->m_pSkins->GetColorV4(*CSkins::ms_apColorVariables[j], j==SKINPART_MARKING);
				}
				else
				{
					if(m_TeePartSelected == j)
						Info.m_aTextures[j] = s->m_OrgTexture;
					else
						Info.m_aTextures[j] = pSkinPart->m_OrgTexture;
					Info.m_aColors[j] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
			Info.m_Size = 50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			const vec2 TeePos(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2);

			if(m_TeePartSelected == SKINPART_HANDS)
			{
				RenderTools()->RenderTeeHand(&Info, TeePos, vec2(1.0f, 0.0f), -pi*0.5f, vec2(18, 0));
			}
			int TeePartEmote = EMOTE_NORMAL;
			if(m_TeePartSelected == SKINPART_EYES)
			{
				float LocalTime = Client()->LocalTime();
				TeePartEmote = (int)(LocalTime * 0.5f) % NUM_EMOTES;
			}
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, TeePartEmote, vec2(1.0f, 0.0f), TeePos);

			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			UI()->DoLabelSelected(&Label, s->m_aName, Item.m_Selected, 10.0f, TEXTALIGN_CENTER);
		}
	}

	const int NewSelected = s_ListBox.DoEnd();
	if(NewSelected != -1 && NewSelected != OldSelected)
	{
		const CSkins::CSkinPart *s = s_paList[m_TeePartSelected][NewSelected];
		mem_copy(CSkins::ms_apSkinVariables[m_TeePartSelected], s->m_aName, MAX_SKIN_ARRAY_SIZE);
		Config()->m_PlayerSkin[0] = 0;
		m_SkinModified = true;
	}
	OldSelected = NewSelected;
}

void CMenus::RenderSkinPartPalette(CUIRect MainView)
{
	if(!*CSkins::ms_apUCCVariables[m_TeePartSelected])
		return; // color selection not open

	float ButtonHeight = 20.0f;
	float Width = MainView.w;
	float Margin = 5.0f;
	CUIRect Button;

	// palette
	MainView.HSplitBottom(ButtonHeight/2.0f, &MainView, 0);
	MainView.HSplitBottom(ButtonHeight+2*Margin, &MainView, &MainView);
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		MainView.VSplitLeft(Width/NUM_SKINPARTS, &Button, &MainView);

		// no palette if color is unused for this skin parts
		static int s_aColorPalettes[NUM_SKINPARTS];
		if(*CSkins::ms_apUCCVariables[p])
		{
			float HMargin = (Button.w-(ButtonHeight+2*Margin))/2.0f;
			Button.VSplitLeft(HMargin, 0, &Button);
			Button.VSplitRight(HMargin, &Button, 0);

			vec4 PartColor = m_pClient->m_pSkins->GetColorV4(*CSkins::ms_apColorVariables[p], p==SKINPART_MARKING);

			bool Hovered = UI()->HotItem() == &s_aColorPalettes[p];
			bool Clicked = UI()->DoButtonLogic(&s_aColorPalettes[p], &Button);
			bool Selected = m_TeePartSelected == p;
			if(Selected)
			{
				CUIRect Underline = {Button.x, Button.y + ButtonHeight + 2*Margin + 2.0f, Button.w, 1.0f};
				Underline.Draw(vec4(1.0f, 1.0f, 1.0f, 1.5f), 0.0f, CUIRect::CORNER_NONE);
			}
			Button.Draw((Hovered ? vec4(1.0f, 1.0f, 1.0f, 0.25f) : vec4(0.0f, 0.0f, 0.0f, 0.25f)));
			Button.Margin(Margin, &Button);
			Button.Draw(PartColor, 3.0f);
			if(Clicked && p != m_TeePartSelected)
			{
				int& TeePartSelectedColor = *CSkins::ms_apColorVariables[m_TeePartSelected];
				TeePartSelectedColor = *CSkins::ms_apColorVariables[p];
				TeePartSelectedColor -= ((TeePartSelectedColor>>24)&0xff)<<24; // remove any alpha
				if(m_TeePartSelected == SKINPART_MARKING)
					TeePartSelectedColor += 0xff<<24; // force full alpha
			}
		}
	}
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


int CMenus::ThemeScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pSuffix = str_endswith(pName, ".map");
	if(IsDir || !pSuffix)
		return 0;
	char aFullName[128];
	char aThemeName[128];
	str_truncate(aFullName, sizeof(aFullName), pName, pSuffix - pName);

	bool IsDay = false;
	bool IsNight = false;
	if((pSuffix = str_endswith(aFullName, "_day")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsDay = true;
	}
	else if((pSuffix = str_endswith(aFullName, "_night")))
	{
		str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - aFullName);
		IsNight = true;
	}
	else
		str_copy(aThemeName, aFullName, sizeof(aThemeName));

	// "none" and "auto" are reserved, disallowed for maps
	if(str_comp(aThemeName, "none") == 0 || str_comp(aThemeName, "auto") == 0)
		return 0;

	// try to edit an existing theme
	for(int i = 0; i < pSelf->m_lThemes.size(); i++)
	{
		if(str_comp(pSelf->m_lThemes[i].m_Name, aThemeName) == 0)
		{
			if(IsDay)
				pSelf->m_lThemes[i].m_HasDay = true;
			if(IsNight)
				pSelf->m_lThemes[i].m_HasNight = true;
			return 0;
		}
	}

	// make new theme
	CTheme Theme(aThemeName, IsDay, IsNight);
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "added theme %s from ui/themes/%s", aThemeName, pName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	pSelf->m_lThemes.add(Theme);
	return 0;
}

int CMenus::ThemeIconScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	const char *pSuffix = str_endswith(pName, ".png");
	if(IsDir || !pSuffix)
		return 0;

	char aThemeName[128];
	str_truncate(aThemeName, sizeof(aThemeName), pName, pSuffix - pName);

	// save icon for an existing theme
	for(sorted_array<CTheme>::range r = pSelf->m_lThemes.all(); !r.empty(); r.pop_front()) // bit slow but whatever
	{
		if(str_comp(r.front().m_Name, aThemeName) == 0 || (!r.front().m_Name[0] && str_comp(aThemeName, "none") == 0))
		{
			char aBuf[IO_MAX_PATH_LENGTH];
			str_format(aBuf, sizeof(aBuf), "ui/themes/%s", pName);
			CImageInfo Info;
			if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
			{
				str_format(aBuf, sizeof(aBuf), "failed to load theme icon from %s", pName);
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
				return 0;
			}
			str_format(aBuf, sizeof(aBuf), "loaded theme icon %s", pName);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);

			r.front().m_IconTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
			mem_free(Info.m_pData);
			return 0;
		}
	}
	return 0; // no existing theme
}

void LoadLanguageIndexfile(IStorage *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	// read file data into buffer
	const char *pFilename = "languages/index.json";
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);

	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
		return;
	}

	// extract data
	const json_value &rStart = (*pJsonData)["language indices"];
	if(rStart.type == json_array)
	{
		for(unsigned i = 0; i < rStart.u.array.length; ++i)
		{
			char aFileName[128];
			str_format(aFileName, sizeof(aFileName), "languages/%s.json", (const char *)rStart[i]["file"]);
			pLanguages->add(CLanguage((const char *)rStart[i]["name"], aFileName, (json_int_t)rStart[i]["code"]));
		}
	}

	// clean up
	json_value_free(pJsonData);
}

void CMenus::RenderLanguageSelection(CUIRect MainView, bool Header)
{
	static int s_SelectedLanguage = -1;
	static sorted_array<CLanguage> s_Languages;
	static CListBox s_ListBox;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
		{
			if(str_comp(s_Languages[i].m_FileName, Config()->m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
		}
	}

	int OldSelected = s_SelectedLanguage;

	if(Header)
		s_ListBox.DoHeader(&MainView, Localize("Language"), UI()->GetListHeaderHeight());
	bool IsActive = m_ActiveListBox == ACTLB_LANG;
	s_ListBox.DoStart(20.0f, s_Languages.size(), 1, 3, s_SelectedLanguage, Header?0:&MainView, Header, &IsActive);

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = s_ListBox.DoNextItem(&r.front(), s_SelectedLanguage != -1 && !str_comp(s_Languages[s_SelectedLanguage].m_Name, r.front().m_Name), &IsActive);
		if(IsActive)
			m_ActiveListBox = ACTLB_LANG;

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_pCountryFlags->Render(r.front().m_CountryCode, &Color, Rect.x, Rect.y, Rect.w, Rect.h, true);
			UI()->DoLabelSelected(&Item.m_Rect, r.front().m_Name, Item.m_Selected, Item.m_Rect.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);
		}
	}

	s_SelectedLanguage = s_ListBox.DoEnd();

	if(OldSelected != s_SelectedLanguage)
	{
		m_ActiveListBox = ACTLB_LANG;
		str_copy(Config()->m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(Config()->m_ClLanguagefile));
		TextRender()->SetFontLanguageVariant(Config()->m_ClLanguagefile);
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());
	}
}

void CMenus::RenderThemeSelection(CUIRect MainView, bool Header)
{
	static CListBox s_ListBox;

	if(m_lThemes.size() == 0) // not loaded yet
	{
		if(!Config()->m_ClShowMenuMap)
			str_copy(Config()->m_ClMenuMap, "", sizeof(Config()->m_ClMenuMap)); // cl_menu_map otherwise resets to default on loading
		m_lThemes.add(CTheme("", false, false)); // no theme
		m_lThemes.add(CTheme("auto", false, false)); // auto theme
		Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/themes", ThemeScan, (CMenus*)this);
		Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/themes", ThemeIconScan, (CMenus*)this);
	}

	int SelectedTheme = -1;
	for(int i = 0; i < m_lThemes.size(); i++)
		if(str_comp(m_lThemes[i].m_Name, Config()->m_ClMenuMap) == 0)
		{
			SelectedTheme = i;
			break;
		}
	const int OldSelected = SelectedTheme;

	if(Header)
		s_ListBox.DoHeader(&MainView, Localize("Theme"), UI()->GetListHeaderHeight());

	bool IsActive = m_ActiveListBox == ACTLB_THEME;
	s_ListBox.DoStart(20.0f, m_lThemes.size(), 1, 3, SelectedTheme, Header?0:&MainView, Header, &IsActive);

	for(sorted_array<CTheme>::range r = m_lThemes.all(); !r.empty(); r.pop_front())
	{
		const CTheme& Theme = r.front();
		CListboxItem Item = s_ListBox.DoNextItem(&Theme, SelectedTheme == (&Theme-m_lThemes.base_ptr()), &IsActive);
		if(IsActive)
			m_ActiveListBox = ACTLB_THEME;

		if(!Item.m_Visible)
			continue;

		CUIRect Icon;
		Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Icon, &Item.m_Rect);

		// draw icon if it exists
		if(Theme.m_IconTexture.IsValid())
		{
			Icon.VMargin(6.0f, &Icon);
			Icon.HMargin(3.0f, &Icon);
			Graphics()->TextureSet(Theme.m_IconTexture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		char aName[128];
		if(!Theme.m_Name[0])
			str_copy(aName, "(none)", sizeof(aName));
		else if(str_comp(Theme.m_Name, "auto") == 0)
			str_copy(aName, "(automatic)", sizeof(aName));
		else if(Theme.m_HasDay && Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s", Theme.m_Name.cstr());
		else if(Theme.m_HasDay && !Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (day)", Theme.m_Name.cstr());
		else if(!Theme.m_HasDay && Theme.m_HasNight)
			str_format(aName, sizeof(aName), "%s (night)", Theme.m_Name.cstr());
		else // generic
			str_format(aName, sizeof(aName), "%s", Theme.m_Name.cstr());

		UI()->DoLabelSelected(&Item.m_Rect, aName, Item.m_Selected, Item.m_Rect.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);
	}

	SelectedTheme = s_ListBox.DoEnd();

	if(OldSelected != SelectedTheme)
	{
		m_ActiveListBox = ACTLB_THEME;
		str_copy(Config()->m_ClMenuMap, m_lThemes[SelectedTheme].m_Name, sizeof(Config()->m_ClMenuMap));
		Config()->m_ClShowMenuMap = m_lThemes[SelectedTheme].m_Name[0] ? 1 : 0;
		m_pClient->m_pMapLayersBackGround->BackgroundMapUpdate();
	}
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	CUIRect Label, Button, Game, Client, BottomView, Background;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render game menu backgrounds
	int NumOptions = maximum(Config()->m_ClNameplates ? 6 : 3, Config()->m_ClShowsocial ? 6 : 5);
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	Background.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, this->Client()->State() == IClient::STATE_OFFLINE ? CUIRect::CORNER_ALL : CUIRect::CORNER_B);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Game, &MainView);
	Game.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// render client menu background
	NumOptions = 4;
	if(Config()->m_ClAutoDemoRecord) NumOptions += 1;
	if(Config()->m_ClAutoScreenshot) NumOptions += 1;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Client, &MainView);
	Client.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect GameLeft, GameRight;
	// render game menu
	Game.HSplitTop(ButtonHeight, &Label, &Game);
	UI()->DoLabel(&Label, Localize("Game"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	Game.VSplitMid(&GameLeft, &GameRight, Spacing);

	// left side
	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);

	// TODO: make space for camera settings
	CUIRect CheckBoxLeft, CheckBoxRight;
	Button.VSplitMid(&CheckBoxLeft, &CheckBoxRight);
	if(DoButton_CheckBox(&Config()->m_ClDynamicCamera, Localize("Dynamic Camera"), Config()->m_ClDynamicCamera, &CheckBoxLeft))
	{
		if(Config()->m_ClDynamicCamera)
		{
			Config()->m_ClDynamicCamera = 0;
			// force to defaults when using the GUI
			Config()->m_ClMouseMaxDistanceStatic = 400;
			// Config()->m_ClMouseFollowfactor = 0;
			// Config()->m_ClMouseDeadzone = 0;
		}
		else
		{
			Config()->m_ClDynamicCamera = 1;
			// force to defaults when using the GUI
			Config()->m_ClMouseMaxDistanceDynamic = 1000;
			Config()->m_ClMouseFollowfactor = 60;
			Config()->m_ClMouseDeadzone = 300;
		}
	}

	if(DoButton_CheckBox(&Config()->m_ClCameraSmoothness, Localize("Smooth Camera"), Config()->m_ClCameraSmoothness, &CheckBoxRight))
	{
		if(Config()->m_ClCameraSmoothness)
		{
			Config()->m_ClCameraSmoothness = 0;
		}
		else
		{
			Config()->m_ClCameraSmoothness = 50;
			Config()->m_ClCameraStabilizing = 50;
		}
	}

	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
	if(DoButton_CheckBox(&Config()->m_ClAutoswitchWeapons, Localize("Switch weapon on pickup"), Config()->m_ClAutoswitchWeapons, &Button))
		Config()->m_ClAutoswitchWeapons ^= 1;

	GameLeft.HSplitTop(Spacing, 0, &GameLeft);
	GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
	if(DoButton_CheckBox(&Config()->m_ClNameplates, Localize("Show name plates"), Config()->m_ClNameplates, &Button))
		Config()->m_ClNameplates ^= 1;

	if(Config()->m_ClNameplates)
	{
		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_ClNameplatesAlways, Localize("Always show name plates"), Config()->m_ClNameplatesAlways, &Button))
			Config()->m_ClNameplatesAlways ^= 1;

		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		UI()->DoScrollbarOption(&Config()->m_ClNameplatesSize, &Config()->m_ClNameplatesSize, &Button, Localize("Size"), 0, 100);

		GameLeft.HSplitTop(Spacing, 0, &GameLeft);
		GameLeft.HSplitTop(ButtonHeight, &Button, &GameLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_ClNameplatesTeamcolors, Localize("Use team colors for name plates"), Config()->m_ClNameplatesTeamcolors, &Button))
			Config()->m_ClNameplatesTeamcolors ^= 1;
	}

	// right side
	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	CUIRect CheckBoxShowHud, CheckBoxHideScore;
	Button.VSplitMid(&CheckBoxShowHud, &CheckBoxHideScore);

	if(DoButton_CheckBox(&Config()->m_ClShowhud, Localize("Show ingame HUD"), Config()->m_ClShowhud, &CheckBoxShowHud))
		Config()->m_ClShowhud ^= 1;

	if(DoButton_CheckBox(&Config()->m_ClHideSelfScore, Localize("Hide player's score"), Config()->m_ClHideSelfScore, &CheckBoxHideScore))
		Config()->m_ClHideSelfScore ^= 1;

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	if(DoButton_CheckBox(&Config()->m_ClShowUserId, Localize("Show user IDs"), Config()->m_ClShowUserId, &Button))
		Config()->m_ClShowUserId ^= 1;

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	if(DoButton_CheckBox(&Config()->m_ClShowsocial, Localize("Show social"), Config()->m_ClShowsocial, &Button))
		Config()->m_ClShowsocial ^= 1;

	// show chat messages button
	if(Config()->m_ClShowsocial)
	{
		GameRight.HSplitTop(Spacing, 0, &GameRight);
		GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
		const char *apLabels[] = { Localize("everyone", "Show chat messages from"), Localize("friends only", "Show chat messages from"), Localize("no one", "Show chat messages from") };
		UI()->DoScrollbarOptionLabeled(&Config()->m_ClFilterchat, &Config()->m_ClFilterchat, &Button, Localize("Show chat messages from"), apLabels, sizeof(apLabels)/sizeof(char *));
	}

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	if(DoButton_CheckBox(&Config()->m_ClColoredBroadcast, Localize("Enable colored server broadcasts"), Config()->m_ClColoredBroadcast, &Button))
		Config()->m_ClColoredBroadcast ^= 1;

	GameRight.HSplitTop(Spacing, 0, &GameRight);
	GameRight.HSplitTop(ButtonHeight, &Button, &GameRight);
	if(DoButton_CheckBox(&Config()->m_ClDisableWhisper, Localize("Disable whisper feature"), Config()->m_ClDisableWhisper, &Button))
		Config()->m_ClDisableWhisper ^= 1;

	// render client menu
	Client.HSplitTop(ButtonHeight, &Label, &Client);
	UI()->DoLabel(&Label, Localize("Client"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	if(DoButton_CheckBox(&Config()->m_ClSkipStartMenu, Localize("Skip the main menu"), Config()->m_ClSkipStartMenu, &Button))
		Config()->m_ClSkipStartMenu ^= 1;

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	UI()->DoScrollbarOption(&Config()->m_ClMenuAlpha, &Config()->m_ClMenuAlpha, &Button, Localize("Menu background opacity"), 0, 75);

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	if(DoButton_CheckBox(&Config()->m_ClAutoDemoRecord, Localize("Automatically record demos"), Config()->m_ClAutoDemoRecord, &Button))
		Config()->m_ClAutoDemoRecord ^= 1;

	if(Config()->m_ClAutoDemoRecord)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		UI()->DoScrollbarOption(&Config()->m_ClAutoDemoMax, &Config()->m_ClAutoDemoMax, &Button, Localize("Max"), 0, 1000, &LogarithmicScrollbarScale, true);
	}

	Client.HSplitTop(Spacing, 0, &Client);
	Client.HSplitTop(ButtonHeight, &Button, &Client);
	if(DoButton_CheckBox(&Config()->m_ClAutoScreenshot, Localize("Automatically take game over screenshot"), Config()->m_ClAutoScreenshot, &Button))
		Config()->m_ClAutoScreenshot ^= 1;

	if(Config()->m_ClAutoScreenshot)
	{
		Client.HSplitTop(Spacing, 0, &Client);
		Client.HSplitTop(ButtonHeight, &Button, &Client);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		UI()->DoScrollbarOption(&Config()->m_ClAutoScreenshotMax, &Config()->m_ClAutoScreenshotMax, &Button, Localize("Max"), 0, 1000, &LogarithmicScrollbarScale, true);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	// render language and theme selection
	CUIRect LanguageView, ThemeView;
	MainView.VSplitMid(&LanguageView, &ThemeView, 2.0f);
	RenderLanguageSelection(LanguageView);
	RenderThemeSelection(ThemeView);

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset general settings"), Localize("Are you sure that you want to reset the general settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsGeneral);
	}
}

void CMenus::RenderSettingsTeeBasic(CUIRect MainView)
{
	RenderSkinSelection(MainView); // yes thats all here ^^
}

void CMenus::RenderSettingsTeeCustom(CUIRect MainView)
{
	CUIRect Label, Patterns, Button, Left, Right, Picker, Palette;

	// render skin preview background
	float SpacingH = 2.0f;
	float SpacingW = 3.0f;
	float ButtonHeight = 20.0f;

	MainView.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	UI()->DoLabel(&Label, Localize("Customize"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	// skin part selection
	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.HSplitTop(ButtonHeight, &Patterns, &MainView);
	Patterns.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	float ButtonWidth = (Patterns.w/6.0f)-(SpacingW*5.0)/6.0f;

	static CButtonContainer s_aPatternButtons[6];
	for(int i = 0; i < NUM_SKINPARTS; i++)
	{
		Patterns.VSplitLeft(ButtonWidth, &Button, &Patterns);
		if(DoButton_MenuTabTop(&s_aPatternButtons[i], Localize(CSkins::ms_apSkinPartNames[i], "skins"), m_TeePartSelected==i, &Button))
		{
			m_TeePartSelected = i;
		}
		Patterns.VSplitLeft(SpacingW, 0, &Patterns);
	}

	MainView.HSplitTop(SpacingH, 0, &MainView);
	MainView.VSplitMid(&Left, &Right, SpacingW);

	// part selection
	RenderSkinPartSelection(Left);

	// use custom color checkbox
	Right.HSplitTop(ButtonHeight, &Button, &Right);
	static bool s_CustomColors;
	s_CustomColors = *CSkins::ms_apUCCVariables[m_TeePartSelected] == 1;
	if(DoButton_CheckBox(&s_CustomColors, Localize("Custom colors"), s_CustomColors, &Button))
	{
		*CSkins::ms_apUCCVariables[m_TeePartSelected] = s_CustomColors ? 0 : 1;
		m_SkinModified = true;
	}

	// HSL picker
	Right.HSplitBottom(45.0f, &Picker, &Palette);
	if(s_CustomColors)
	{
		Right.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		RenderHSLPicker(Picker);
		RenderSkinPartPalette(Palette);
	}
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	static bool s_CustomSkinMenu = false;
	static int s_PlayerCountry = 0;
	static char s_aPlayerName[MAX_NAME_ARRAY_SIZE] = {0};
	static char s_aPlayerClan[MAX_CLAN_ARRAY_SIZE] = {0};

	if(m_pClient->m_IdentityState < 0)
	{
		s_PlayerCountry = Config()->m_PlayerCountry;
		str_copy(s_aPlayerName, Config()->m_PlayerName, sizeof(s_aPlayerName));
		str_copy(s_aPlayerClan, Config()->m_PlayerClan, sizeof(s_aPlayerClan));
		m_pClient->m_IdentityState = 0;
	}

	CUIRect Button, Label, TopView, BottomView, Background, Left, Right;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render skin preview background
	const float SpacingH = 2.0f;
	const float SpacingW = 3.0f;
	const float ButtonHeight = 20.0f;
	const float SkinHeight = 50.0f;
	const float BackgroundHeight = (ButtonHeight+SpacingH) + SkinHeight*2;
	const vec2 MousePosition = vec2(UI()->MouseX(), UI()->MouseY());

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	Background.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, Client()->State() == IClient::STATE_OFFLINE ? CUIRect::CORNER_ALL : CUIRect::CORNER_B);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &TopView, &MainView);
	TopView.VSplitMid(&Left, &Right, 3.0f);
	Left.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
	Right.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	Left.HSplitTop(ButtonHeight, &Label, &Left);
	UI()->DoLabel(&Label, Localize("Tee"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	// Preview
	{
		CUIRect Top, Bottom, TeeLeft, TeeRight;
		Left.HSplitTop(SpacingH, 0, &Left);
		Left.HSplitTop(SkinHeight * 2, &Top, &Left);

		// split the menu in 2 parts
		Top.HSplitMid(&Top, &Bottom, SpacingH);

		// handle left

		// validate skin parts for solo mode
		CTeeRenderInfo OwnSkinInfo;
		OwnSkinInfo.m_Size = 50.0f;

		char aSkinParts[NUM_SKINPARTS][MAX_SKIN_ARRAY_SIZE];
		char* apSkinPartsPtr[NUM_SKINPARTS];
		int aUCCVars[NUM_SKINPARTS];
		int aColorVars[NUM_SKINPARTS];
		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			str_copy(aSkinParts[p], CSkins::ms_apSkinVariables[p], MAX_SKIN_ARRAY_SIZE);
			apSkinPartsPtr[p] = aSkinParts[p];
			aUCCVars[p] = *CSkins::ms_apUCCVariables[p];
			aColorVars[p] = *CSkins::ms_apColorVariables[p];
		}

		m_pClient->m_pSkins->ValidateSkinParts(apSkinPartsPtr, aUCCVars, aColorVars, 0);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, apSkinPartsPtr[p], false);
			const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
			if(aUCCVars[p])
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				OwnSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(aColorVars[p], p==SKINPART_MARKING);
			}
			else
			{
				OwnSkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				OwnSkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// draw preview
		Top.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		Top.VSplitLeft(Top.w/3.0f+SpacingW/2.0f, &Label, &Top);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Normal:"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

		Top.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		{
			// interactive tee: tee looking towards cursor, and it is happy when you touch it
			vec2 TeePosition = vec2(Top.x+Top.w/2.0f, Top.y+Top.h/2.0f+6.0f);
			vec2 DeltaPosition = MousePosition - TeePosition;
			float Distance = length(DeltaPosition);
			vec2 TeeDirection = Distance < 20.0f ? normalize(vec2(DeltaPosition.x, maximum(DeltaPosition.y, 0.5f))) : normalize(DeltaPosition);
			int TeeEmote = Distance < 20.0f ? EMOTE_HAPPY : EMOTE_NORMAL;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, TeeEmote, TeeDirection, TeePosition);
			if(Distance < 20.0f && UI()->MouseButtonClicked(0))
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_PLAYER_SPAWN, 0);
		}

		// handle right (team skins)

		// validate skin parts for team game mode
		CTeeRenderInfo TeamSkinInfo = OwnSkinInfo;

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			str_copy(aSkinParts[p], CSkins::ms_apSkinVariables[p], MAX_SKIN_ARRAY_SIZE);
			apSkinPartsPtr[p] = aSkinParts[p];
			aUCCVars[p] = *CSkins::ms_apUCCVariables[p];
			aColorVars[p] = *CSkins::ms_apColorVariables[p];
		}

		m_pClient->m_pSkins->ValidateSkinParts(apSkinPartsPtr, aUCCVars, aColorVars, GAMEFLAG_TEAMS);

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int SkinPart = m_pClient->m_pSkins->FindSkinPart(p, apSkinPartsPtr[p], false);
			const CSkins::CSkinPart *pSkinPart = m_pClient->m_pSkins->GetSkinPart(p, SkinPart);
			if(aUCCVars[p])
			{
				TeamSkinInfo.m_aTextures[p] = pSkinPart->m_ColorTexture;
				TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(aColorVars[p], p==SKINPART_MARKING);
			}
			else
			{
				TeamSkinInfo.m_aTextures[p] = pSkinPart->m_OrgTexture;
				TeamSkinInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}

		// draw preview
		Bottom.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		Bottom.VSplitLeft(Bottom.w/3.0f+SpacingW/2.0f, &Label, &Bottom);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Team:"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

		Bottom.VSplitMid(&TeeLeft, &TeeRight, SpacingW);

		TeeLeft.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(aUCCVars[p], aColorVars[p], TEAM_RED, p);
			TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_MARKING);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeamSkinInfo, 0, vec2(1, 0), vec2(TeeLeft.x+TeeLeft.w/2.0f, TeeLeft.y+TeeLeft.h/2.0f+6.0f));

		TeeRight.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		for(int p = 0; p < NUM_SKINPARTS; p++)
		{
			int TeamColor = m_pClient->m_pSkins->GetTeamColor(aUCCVars[p], aColorVars[p], TEAM_BLUE, p);
			TeamSkinInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(TeamColor, p==SKINPART_MARKING);
		}
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeamSkinInfo, 0, vec2(-1, 0), vec2(TeeRight.x+TeeRight.w/2.0f, TeeRight.y+TeeRight.h/2.0f+6.0f));
	}

	Right.HSplitTop(ButtonHeight, &Label, &Right);
	UI()->DoLabel(&Label, Localize("Personal"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	// Personal
	{
		CUIRect Top, Bottom, Name, Clan, Flag;
		Right.HSplitTop(SpacingH, 0, &Right);
		Right.HSplitMid(&Top, &Bottom, SpacingH);
		Top.HSplitMid(&Name, &Clan, SpacingH);

		// player name
		Name.HSplitTop(ButtonHeight, &Button, &Name);
		static CLineInput s_NameInput(Config()->m_PlayerName, sizeof(Config()->m_PlayerName), MAX_NAME_LENGTH);
		UI()->DoEditBoxOption(&s_NameInput, &Button, Localize("Name"), 100.0f);

		// player clan
		Clan.HSplitTop(ButtonHeight, &Button, &Clan);
		static CLineInput s_ClanInput(Config()->m_PlayerClan, sizeof(Config()->m_PlayerClan), MAX_CLAN_LENGTH);
		UI()->DoEditBoxOption(&s_ClanInput, &Button, Localize("Clan"), 100.0f);

		// country selector
		Bottom.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		Bottom.VSplitLeft(100.0f, &Label, &Button);
		Label.y += 17.0f;
		UI()->DoLabel(&Label, Localize("Flag:"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

		Button.w = (SkinHeight - 20.0f) * 2 + 20.0f;
		Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		if(UI()->MouseHovered(&Button))
			Button.Draw(vec4(0.5f, 0.5f, 0.5f, 0.25f));
		
		Button.Margin(10.0f, &Flag);
		vec4 Color = vec4(1,1,1,1);
		m_pClient->m_pCountryFlags->Render(Config()->m_PlayerCountry, &Color, Flag.x, Flag.y, Flag.w, Flag.h);

		if(UI()->DoButtonLogic(&Config()->m_PlayerCountry, &Button))
			PopupCountry(Config()->m_PlayerCountry, &CMenus::PopupConfirmPlayerCountry);
	}

	MainView.HSplitTop(10.0f, 0, &MainView);

	if(s_CustomSkinMenu)
		RenderSettingsTeeCustom(MainView);
	else
		RenderSettingsTeeBasic(MainView);

	// bottom button
	float ButtonWidth = (BottomView.w/6.0f)-(SpacingW*5.0)/6.0f;
	int NumButtons = 1;
	if(s_CustomSkinMenu)
		NumButtons = 3;
	else if(m_pSelectedSkin && (m_pSelectedSkin->m_Flags&CSkins::SKINFLAG_STANDARD) == 0)
		NumButtons = 2;
	float BackgroundWidth = ButtonWidth*NumButtons+SpacingW*(NumButtons-1);

	BottomView.VSplitRight(BackgroundWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	if(s_CustomSkinMenu)
	{
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_CustomSkinSaveButton;
		if(DoButton_Menu(&s_CustomSkinSaveButton, Localize("Save"), 0, &Button))
		{
			m_SkinNameInput.SetCursorOffset(m_SkinNameInput.GetLength());
			m_SkinNameInput.SetSelection(0, m_SkinNameInput.GetLength());
			UI()->SetActiveItem(&m_SkinNameInput);
			m_Popup = POPUP_SAVE_SKIN;
		}
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);

		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_RandomizeSkinButton;
		if(DoButton_Menu(&s_RandomizeSkinButton, Localize("Randomize"), 0, &Button))
		{
			m_pClient->m_pSkins->RandomizeSkin();
			Config()->m_PlayerSkin[0] = 0;
			m_SkinModified = true;
		}
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);
	}
	else if(m_pSelectedSkin && (m_pSelectedSkin->m_Flags&CSkins::SKINFLAG_STANDARD) == 0)
	{
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_CustomSkinDeleteButton;
		if(DoButton_Menu(&s_CustomSkinDeleteButton, Localize("Delete"), 0, &Button))
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), Localize("Are you sure that you want to delete the skin '%s'?"), m_pSelectedSkin->m_aName);
			PopupConfirm(Localize("Delete skin"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDeleteSkin);
		}
		BottomView.VSplitLeft(SpacingW, 0, &BottomView);
	}

	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static CButtonContainer s_CustomSwitchButton;
	if(DoButton_Menu(&s_CustomSwitchButton, s_CustomSkinMenu ? Localize("Basic") : Localize("Custom"), 0, &Button))
	{
		s_CustomSkinMenu = !s_CustomSkinMenu;
		if(s_CustomSkinMenu && m_pSelectedSkin)
		{
			if(m_pSelectedSkin->m_Flags&CSkins::SKINFLAG_STANDARD)
			{
				m_SkinNameInput.Set("copy_");
				m_SkinNameInput.Append(m_pSelectedSkin->m_aName);
			}
			else
				m_SkinNameInput.Set(m_pSelectedSkin->m_aName);
		}
	}

	// check if the new settings require a server reload
	m_NeedRestartPlayer = !(
		s_PlayerCountry == Config()->m_PlayerCountry &&
		!str_comp(s_aPlayerClan, Config()->m_PlayerClan) &&
		!str_comp(s_aPlayerName, Config()->m_PlayerName)
	);
	m_pClient->m_IdentityState = m_NeedRestartPlayer ? 1 : 0;
}

void CMenus::PopupConfirmDeleteSkin()
{
	if(m_pSelectedSkin)
	{
		char aBuf[IO_MAX_PATH_LENGTH];
		str_format(aBuf, sizeof(aBuf), "skins/%s.json", m_pSelectedSkin->m_aName);
		if(Storage()->RemoveFile(aBuf, IStorage::TYPE_SAVE))
		{
			m_pClient->m_pSkins->RemoveSkin(m_pSelectedSkin);
			m_RefreshSkinSelector = true;
			m_pSelectedSkin = 0;
		}
		else
			PopupMessage(Localize("Error"), Localize("Unable to delete the skin"), Localize("Ok"));
	}
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	// cut view
	CUIRect BottomView, Button, Background;
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	Background.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, Client()->State() == IClient::STATE_OFFLINE ? CUIRect::CORNER_ALL : CUIRect::CORNER_B);
	MainView.HSplitTop(20.0f, 0, &MainView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	const float HeaderHeight = 20.0f;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0, 0);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ClipBgColor = vec4(0,0,0,0);
	ScrollParams.m_ScrollUnit = 60.0f; // inconsistent margin, 3 category header per scroll
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);
	MainView.y += ScrollOffset.y;

	CUIRect LastExpandRect;
	static bool s_MouseActive = true;
	float Split = DoIndependentDropdownMenu(&s_MouseActive, &MainView, Localize("Mouse"), HeaderHeight, &CMenus::RenderSettingsControlsMouse, &s_MouseActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_JoystickActive = m_pClient->Input()->NumJoysticks() > 0; // hide by default if no joystick found
	Split = DoIndependentDropdownMenu(&s_JoystickActive, &MainView, Localize("Joystick"), HeaderHeight, &CMenus::RenderSettingsControlsJoystick, &s_JoystickActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_MovementActive = true;
	Split = DoIndependentDropdownMenu(&s_MovementActive, &MainView, Localize("Movement"), HeaderHeight, &CMenus::RenderSettingsControlsMovement, &s_MovementActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_WeaponActive = true;
	Split = DoIndependentDropdownMenu(&s_WeaponActive, &MainView, Localize("Weapon"), HeaderHeight, &CMenus::RenderSettingsControlsWeapon, &s_WeaponActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_VotingActive = true;
	Split = DoIndependentDropdownMenu(&s_VotingActive, &MainView, Localize("Voting"), HeaderHeight, &CMenus::RenderSettingsControlsVoting, &s_VotingActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_ChatActive = true;
	Split = DoIndependentDropdownMenu(&s_ChatActive, &MainView, Localize("Chat"), HeaderHeight, &CMenus::RenderSettingsControlsChat, &s_ChatActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_ScoreboardActive = true;
	Split = DoIndependentDropdownMenu(&s_ScoreboardActive, &MainView, Localize("Scoreboard"), HeaderHeight, &CMenus::RenderSettingsControlsScoreboard, &s_ScoreboardActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);
	static bool s_MiscActive = true;
	Split = DoIndependentDropdownMenu(&s_MiscActive, &MainView, Localize("Misc"), HeaderHeight, &CMenus::RenderSettingsControlsMisc, &s_MiscActive);

	MainView.HSplitTop(Split+10.0f, &LastExpandRect, &MainView);
	s_ScrollRegion.AddRect(LastExpandRect);

	s_ScrollRegion.End();

	// reset button
	float Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset controls"), Localize("Are you sure that you want to reset the controls to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsControls);
	}
}

float CMenus::RenderSettingsControlsStats(CUIRect View)
{
	static char s_aCheckboxIds[NUM_TC_STATS];
	const float RowHeight = 20.0f;
	CUIRect Button;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 0, Localize("Frags"), Config()->m_ClStatboardInfos & TC_STATS_FRAGS, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_FRAGS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 1, Localize("Deaths"), Config()->m_ClStatboardInfos & TC_STATS_DEATHS, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_DEATHS;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 2, Localize("Suicides"), Config()->m_ClStatboardInfos & TC_STATS_SUICIDES, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_SUICIDES;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 3, Localize("Ratio"), Config()->m_ClStatboardInfos & TC_STATS_RATIO, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_RATIO;
	UI()->DoTooltip(s_aCheckboxIds + 3, &Button, Localize("The ratio of frags to deaths."));

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 4, Localize("Net score"), Config()->m_ClStatboardInfos & TC_STATS_NET, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_NET;
	UI()->DoTooltip(s_aCheckboxIds + 4, &Button, Localize("The number of frags minus the number of deaths."));

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 5, Localize("Frags per minute"), Config()->m_ClStatboardInfos & TC_STATS_FPM, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_FPM;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 6, Localize("Current spree"), Config()->m_ClStatboardInfos & TC_STATS_SPREE, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_SPREE;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 7, Localize("Best spree"), Config()->m_ClStatboardInfos & TC_STATS_BESTSPREE, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_BESTSPREE;

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 8, Localize("Weapons stats"), Config()->m_ClStatboardInfos & TC_STATS_WEAPS, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_WEAPS;
	UI()->DoTooltip(s_aCheckboxIds + 8, &Button, Localize("The proportion of frags gotten with each weapon."));

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 9, Localize("Flag grabs"), Config()->m_ClStatboardInfos & TC_STATS_FLAGGRABS, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_FLAGGRABS;
	UI()->DoTooltip(s_aCheckboxIds + 9, &Button, Localize("The number of times that the flag was touched in CTF (1 point)."));

	View.HSplitTop(RowHeight, &Button, &View);
	if(DoButton_CheckBox(s_aCheckboxIds + 10, Localize("Flag captures"), Config()->m_ClStatboardInfos & TC_STATS_FLAGCAPTURES, &Button))
		Config()->m_ClStatboardInfos ^= TC_STATS_FLAGCAPTURES;
	UI()->DoTooltip(s_aCheckboxIds + 10, &Button, Localize("The number of times that the flag was captured in CTF (100 points)."));

	return NUM_TC_STATS * RowHeight;
}

bool CMenus::DoResolutionList(CUIRect* pRect, CListBox* pListBox,
							  const sorted_array<CVideoMode>& lModes)
{
	int OldSelected = -1;
	char aBuf[32];

	float HiDPIScale = Graphics()->ScreenHiDPIScale();

	pListBox->DoStart(20.0f, lModes.size(), 1, 3, OldSelected, pRect);

	for(int i = 0; i < lModes.size(); ++i)
	{
		if(Config()->m_GfxScreenWidth == lModes[i].m_Width &&
			Config()->m_GfxScreenHeight == lModes[i].m_Height)
		{
			OldSelected = i;
		}

		CListboxItem Item = pListBox->DoNextItem(&lModes[i], OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(lModes[i].m_Width, lModes[i].m_Height);
			str_format(aBuf, sizeof(aBuf), "%dx%d (%d:%d)",
					   (int)(lModes[i].m_Width * HiDPIScale),
					   (int)(lModes[i].m_Height * HiDPIScale),
					   lModes[i].m_Width/G,
					   lModes[i].m_Height/G);
			UI()->DoLabelSelected(&Item.m_Rect, aBuf, Item.m_Selected, Item.m_Rect.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);
		}
	}

	const int NewSelected = pListBox->DoEnd();
	if(OldSelected != NewSelected)
	{
		Config()->m_GfxScreenWidth = lModes[NewSelected].m_Width;
		Config()->m_GfxScreenHeight = lModes[NewSelected].m_Height;
		return true;
	}
	return false;
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	bool CheckFullscreen = false;
	#ifdef CONF_PLATFORM_MACOSX
	CheckFullscreen = true;
	#endif

	static const int s_GfxFullscreen = Config()->m_GfxFullscreen;
	static const int s_GfxScreenWidth = Config()->m_GfxScreenWidth;
	static const int s_GfxScreenHeight = Config()->m_GfxScreenHeight;
	static const int s_GfxFsaaSamples = Config()->m_GfxFsaaSamples;
	static const int s_GfxTextureQuality = Config()->m_GfxTextureQuality;
	static const int s_GfxTextureCompression = Config()->m_GfxTextureCompression;

	CUIRect Label, Button, ScreenLeft, ScreenRight, Texture, BottomView, Background;

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	BottomView.HSplitTop(20.f, 0, &BottomView);

	// render screen menu background
	int NumOptions = 3;
	if(Graphics()->GetNumScreens() > 1 && !Config()->m_GfxFullscreen)
		++NumOptions;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	Background.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, Client()->State() == IClient::STATE_OFFLINE ? CUIRect::CORNER_ALL : CUIRect::CORNER_B);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &ScreenLeft, &MainView);
	ScreenLeft.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// render textures menu background
	NumOptions = 3;
	BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Texture, &MainView);
	Texture.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// render screen menu
	ScreenLeft.HSplitTop(ButtonHeight, &Label, &ScreenLeft);
	UI()->DoLabel(&Label, Localize("Screen"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	ScreenLeft.VSplitMid(&ScreenLeft, &ScreenRight, Spacing);

	ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
	ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
	if(DoButton_CheckBox(&Config()->m_GfxFullscreen, Localize("Fullscreen"), Config()->m_GfxFullscreen, &Button))
		m_CheckVideoSettings |= !Client()->ToggleFullscreen();

	if(!Config()->m_GfxFullscreen)
	{
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_GfxBorderless, Localize("Borderless window"), Config()->m_GfxBorderless, &Button))
			Client()->ToggleWindowBordered();
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		char aBuf[64];
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(100.0f-25.0f, &Text, &Button); // make button appear centered with FSAA
		str_format(aBuf, sizeof(aBuf), Localize("Screen:"));
		UI()->DoLabel(&Text, aBuf, Text.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);

		Button.VSplitLeft(120.0f, &Button, 0);
		str_format(aBuf, sizeof(aBuf), "#%d  (%dx%d)", Config()->m_GfxScreen+1, Graphics()->DesktopWidth(), Graphics()->DesktopHeight());
		static CButtonContainer s_ButtonScreenId;
		if(DoButton_Menu(&s_ButtonScreenId, aBuf, 0, &Button))
		{
			Client()->SwitchWindowScreen((Config()->m_GfxScreen + 1) % Graphics()->GetNumScreens());
		}
	}

	// FSAA button
	{
		ScreenLeft.HSplitTop(Spacing, 0, &ScreenLeft);
		ScreenLeft.HSplitTop(ButtonHeight, &Button, &ScreenLeft);
		Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
		CUIRect Text;
		Button.VSplitLeft(ButtonHeight+5.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Anti Aliasing"));
		UI()->DoLabel(&Text, aBuf, Text.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);

		Button.VSplitLeft(70.0f, &Button, 0);
		str_format(aBuf, sizeof(aBuf), "%dx", Config()->m_GfxFsaaSamples);
		static CButtonContainer s_ButtonGfxFsaaSamples;
		if(DoButton_Menu(&s_ButtonGfxFsaaSamples, aBuf, 0, &Button))
		{
			if(!Config()->m_GfxFsaaSamples)
				Config()->m_GfxFsaaSamples = 2;
			else if(Config()->m_GfxFsaaSamples == 16)
				Config()->m_GfxFsaaSamples = 0;
			else
				Config()->m_GfxFsaaSamples *= 2;
			m_CheckVideoSettings = true;
		}
	}

	ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
	ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);
	if(DoButton_CheckBox(&Config()->m_GfxVsync, Localize("V-Sync"), Config()->m_GfxVsync, &Button))
		Client()->ToggleWindowVSync();

	ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
	ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);

	// TODO: greyed out checkbox (not clickable)
	if(!Config()->m_GfxVsync)
	{
		if(DoButton_CheckBox(&Config()->m_GfxLimitFps, Localize("Limit Fps"), Config()->m_GfxLimitFps, &Button))
			Config()->m_GfxLimitFps ^= 1;

		if(Config()->m_GfxLimitFps > 0)
		{
			ScreenRight.HSplitTop(Spacing, 0, &ScreenRight);
			ScreenRight.HSplitTop(ButtonHeight, &Button, &ScreenRight);
			UI()->DoScrollbarOption(&Config()->m_GfxMaxFps, &Config()->m_GfxMaxFps,
							  &Button, Localize("Max fps"), 30, 300);
		}
	}

	// render texture menu
	Texture.HSplitTop(ButtonHeight, &Label, &Texture);
	UI()->DoLabel(&Label, Localize("Texture"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	if(DoButton_CheckBox(&Config()->m_GfxTextureQuality, Localize("Quality Textures"), Config()->m_GfxTextureQuality, &Button))
	{
		Config()->m_GfxTextureQuality ^= 1;
		m_CheckVideoSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	if(DoButton_CheckBox(&Config()->m_GfxTextureCompression, Localize("Texture Compression"), Config()->m_GfxTextureCompression, &Button))
	{
		Config()->m_GfxTextureCompression ^= 1;
		m_CheckVideoSettings = true;
	}

	Texture.HSplitTop(Spacing, 0, &Texture);
	Texture.HSplitTop(ButtonHeight, &Button, &Texture);
	if(DoButton_CheckBox(&Config()->m_GfxHighDetail, Localize("High Detail"), Config()->m_GfxHighDetail, &Button))
		Config()->m_GfxHighDetail ^= 1;

	// render screen modes
	MainView.HSplitTop(10.0f, 0, &MainView);

	// display mode list
	{
		// custom list header
		NumOptions = 1;

		CUIRect Header, Button;
		MainView.HSplitTop(ButtonHeight*(float)(NumOptions+1)+Spacing*(float)(NumOptions+1), &Header, 0);
		Header.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f, CUIRect::CORNER_T);

		// draw header
		MainView.HSplitTop(ButtonHeight, &Header, &MainView);
		UI()->DoLabel(&Header, Localize("Resolution"), Header.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		CUIRect HeaderLeft, HeaderRight;
		Button.VSplitMid(&HeaderLeft, &HeaderRight, 3.0f);

		HeaderLeft.Draw(vec4(0.30f, 0.4f, 1.0f, 0.5f), 5.0f, CUIRect::CORNER_T);
		HeaderRight.Draw(vec4(0.0f, 0.0f, 0.0f, 0.5f), 5.0f, CUIRect::CORNER_T);

		UI()->DoLabel(&HeaderLeft, Localize("Recommended"), HeaderLeft.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);
		UI()->DoLabel(&HeaderRight, Localize("Other"), HeaderRight.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

		MainView.HSplitTop(Spacing, 0, &MainView);
		CUIRect ListRec, ListOth;
		MainView.VSplitMid(&ListRec, &ListOth, 3.0f);

		ListRec.HSplitBottom(ButtonHeight, &ListRec, &Button);
		ListRec.HSplitBottom(Spacing, &ListRec, 0);
		Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.5f), 5.0f, CUIRect::CORNER_B);
		int g = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
		const float HiDPIScale = Graphics()->ScreenHiDPIScale();
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Current: %dx%d (%d:%d)"), (int)(s_GfxScreenWidth*HiDPIScale), (int)(s_GfxScreenHeight*HiDPIScale), s_GfxScreenWidth/g, s_GfxScreenHeight/g);
		UI()->DoLabel(&Button, aBuf, Button.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

		static int s_LastScreen = Config()->m_GfxScreen;
		if(s_LastScreen != Config()->m_GfxScreen)
		{
			UpdatedFilteredVideoModes();
			s_LastScreen = Config()->m_GfxScreen;
		}
		static CListBox s_RecListBox;
		static CListBox s_OthListBox;
		m_CheckVideoSettings |= DoResolutionList(&ListRec, &s_RecListBox, m_lRecommendedVideoModes);
		m_CheckVideoSettings |= DoResolutionList(&ListOth, &s_OthListBox, m_lOtherVideoModes);
	}

	// reset button
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset graphics settings"), Localize("Are you sure that you want to reset the graphics settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsGraphics);
	}

	// check if the new settings require a restart
	if(m_CheckVideoSettings)
	{
		m_NeedRestartGraphics =
			s_GfxScreenWidth != Config()->m_GfxScreenWidth ||
			s_GfxScreenHeight != Config()->m_GfxScreenHeight ||
			s_GfxFsaaSamples != Config()->m_GfxFsaaSamples ||
			s_GfxTextureQuality != Config()->m_GfxTextureQuality ||
			s_GfxTextureCompression != Config()->m_GfxTextureCompression ||
			(CheckFullscreen && s_GfxFullscreen != Config()->m_GfxFullscreen);
		m_CheckVideoSettings = false;
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Label, Button, Sound, Detail, BottomView, Background;

	// render sound menu background
	int NumOptions = Config()->m_SndEnable ? 3 : 2;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;
	float TotalHeight = BackgroundHeight;
	if(Config()->m_SndEnable)
		TotalHeight += 10.0f+2.0f*ButtonHeight+Spacing;

	MainView.HSplitBottom(MainView.h-TotalHeight-20.0f, &MainView, &BottomView);
	if(this->Client()->State() == IClient::STATE_ONLINE)
		Background = MainView;
	else
		MainView.HSplitTop(20.0f, 0, &Background);
	Background.Draw(vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), 5.0f, Client()->State() == IClient::STATE_OFFLINE ? CUIRect::CORNER_ALL : CUIRect::CORNER_B);
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &Sound, &MainView);
	Sound.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	// render detail menu background
	if(Config()->m_SndEnable)
	{
		BackgroundHeight = 2.0f*ButtonHeight+Spacing;

		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(BackgroundHeight, &Detail, &MainView);
		Detail.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
	}

	static int s_SndInit = Config()->m_SndInit;
	static int s_SndRate = Config()->m_SndRate;

	// render sound menu
	Sound.HSplitTop(ButtonHeight, &Label, &Sound);
	UI()->DoLabel(&Label, Localize("Sound"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	Sound.HSplitTop(Spacing, 0, &Sound);
	CUIRect UseSoundButton;
	Sound.HSplitTop(ButtonHeight, &UseSoundButton, &Sound);

	if(Config()->m_SndEnable)
	{
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_SndMusic, Localize("Play background music"), Config()->m_SndMusic, &Button))
		{
			Config()->m_SndMusic ^= 1;
			UpdateMusicState();
		}

		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_SndNonactiveMute, Localize("Mute when not active"), Config()->m_SndNonactiveMute, &Button))
			Config()->m_SndNonactiveMute ^= 1;

		// render detail menu
		Detail.HSplitTop(ButtonHeight, &Label, &Detail);
		UI()->DoLabel(&Label, Localize("Detail"), ButtonHeight*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

		// split menu
		CUIRect Left, Right;
		Detail.HSplitTop(Spacing, 0, &Detail);
		Detail.VSplitMid(&Left, &Right, 3.0f);

		// sample rate thingy
		{
			Left.HSplitTop(ButtonHeight, &Button, &Left);

			Button.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));
			CUIRect Text, Value, Unit;
			Button.VSplitLeft(Button.w/3.0f, &Text, &Button);
			Button.VSplitMid(&Value, &Unit);

			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "%s:", Localize("Sample rate"));
			UI()->DoLabel(&Text, aBuf, Text.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);
			UI()->DoLabel(&Unit, "kHz", Unit.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

			if(Config()->m_SndRate != 48000 && Config()->m_SndRate != 44100)
				Config()->m_SndRate = 48000;
			if(Config()->m_SndRate == 48000)
				str_copy(aBuf, "48.0", sizeof(aBuf));
			else
				str_copy(aBuf, "44.1", sizeof(aBuf));
			static CButtonContainer s_SampleRateButton;
			if(DoButton_Menu(&s_SampleRateButton, aBuf, 0, &Value))
			{
				if(Config()->m_SndRate == 48000)
					Config()->m_SndRate = 44100;
				else
					Config()->m_SndRate = 48000;
			}

			m_NeedRestartSound = Config()->m_SndInit && (!s_SndInit || s_SndRate != Config()->m_SndRate);
		}

		Right.HSplitTop(ButtonHeight, &Button, &Right);
		UI()->DoScrollbarOption(&Config()->m_SndVolume, &Config()->m_SndVolume, &Button, Localize("Volume"), 0, 100, &LogarithmicScrollbarScale);
	}
	else
	{
		Sound.HSplitTop(Spacing, 0, &Sound);
		Sound.HSplitTop(ButtonHeight, &Button, &Sound);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		if(DoButton_CheckBox(&Config()->m_SndInit, Localize("Load the sound system"), Config()->m_SndInit, &Button))
		{
			Config()->m_SndInit ^= 1;
			m_NeedRestartSound = Config()->m_SndInit && (!s_SndInit || s_SndRate != Config()->m_SndRate);
		}
	}

	if(DoButton_CheckBox(&Config()->m_SndEnable, Localize("Use sounds"), Config()->m_SndEnable, &UseSoundButton))
	{
		Config()->m_SndEnable ^= 1;
		if(Config()->m_SndEnable)
		{
			Config()->m_SndInit = 1;
		}
		UpdateMusicState();
	}

	// reset button
	BottomView.HSplitBottom(60.0f, 0, &BottomView);

	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;

	BottomView.VSplitRight(ButtonWidth, 0, &BottomView);
	RenderBackgroundShadow(&BottomView, true);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	Button = BottomView;
	static CButtonContainer s_ResetButton;
	if(DoButton_Menu(&s_ResetButton, Localize("Reset"), 0, &Button))
	{
		PopupConfirm(Localize("Reset sound settings"), Localize("Are you sure that you want to reset the sound settings to their defaults?"),
			Localize("Reset"), Localize("Cancel"), &CMenus::ResetSettingsSound);
	}
}

void CMenus::ResetSettingsGeneral()
{
	Config()->m_ClDynamicCamera = 0;
	Config()->m_ClMouseMaxDistanceStatic = 400;
	Config()->m_ClMouseMaxDistanceDynamic = 1000;
	Config()->m_ClMouseFollowfactor = 60;
	Config()->m_ClMouseDeadzone = 300;
	Config()->m_ClAutoswitchWeapons = 1;
	Config()->m_ClShowhud = 1;
	Config()->m_ClFilterchat = 0;
	Config()->m_ClNameplates = 1;
	Config()->m_ClNameplatesAlways = 1;
	Config()->m_ClNameplatesSize = 50;
	Config()->m_ClNameplatesTeamcolors = 1;
	Config()->m_ClAutoDemoRecord = 0;
	Config()->m_ClAutoDemoMax = 10;
	Config()->m_ClAutoScreenshot = 0;
	Config()->m_ClAutoScreenshotMax = 10;
}

void CMenus::ResetSettingsControls()
{
	m_pClient->m_pBinds->SetDefaults();
}

void CMenus::ResetSettingsGraphics()
{
	if(Config()->m_GfxScreen)
		Client()->SwitchWindowScreen(0);

	Config()->m_GfxScreenWidth = Graphics()->DesktopWidth();
	Config()->m_GfxScreenHeight = Graphics()->DesktopHeight();
	Config()->m_GfxBorderless = 0;
	Config()->m_GfxFullscreen = 1;
	Config()->m_GfxVsync = 1;
	Config()->m_GfxFsaaSamples = 0;
	Config()->m_GfxTextureQuality = 1;
	Config()->m_GfxTextureCompression = 0;
	Config()->m_GfxHighDetail = 1;

	if(Config()->m_GfxDisplayAllModes)
	{
		Config()->m_GfxDisplayAllModes = 0;
		UpdateVideoModeSettings();
	}

	m_CheckVideoSettings = true;
}

void CMenus::ResetSettingsSound()
{
	Config()->m_SndEnable = 1;
	Config()->m_SndInit = 1;
	Config()->m_SndMusic = 1;
	Config()->m_SndNonactiveMute = 0;
	Config()->m_SndRate = 48000;
	Config()->m_SndVolume = 100;
	UpdateMusicState();
}

void CMenus::PopupConfirmPlayerCountry()
{
	if(m_PopupCountrySelection != -2)
		Config()->m_PlayerCountry = m_PopupCountrySelection;
}

void CMenus::RenderSettings(CUIRect MainView)
{
	// handle which page should be rendered
	if(Config()->m_UiSettingsPage == SETTINGS_GENERAL)
		RenderSettingsGeneral(MainView);
	else if(Config()->m_UiSettingsPage == SETTINGS_PLAYER)
		RenderSettingsPlayer(MainView);
	else if(Config()->m_UiSettingsPage == SETTINGS_TBD) // TODO: replace removed tee page to something else	
		Config()->m_UiSettingsPage = SETTINGS_PLAYER; // TODO: remove this
	else if(Config()->m_UiSettingsPage == SETTINGS_CONTROLS)
		RenderSettingsControls(MainView);
	else if(Config()->m_UiSettingsPage == SETTINGS_GRAPHICS)
		RenderSettingsGraphics(MainView);
	else if(Config()->m_UiSettingsPage == SETTINGS_SOUND)
		RenderSettingsSound(MainView);

	MainView.HSplitBottom(32.0f, 0, &MainView);

	// reset warning
	bool NeedReconnect = (m_NeedRestartPlayer || (m_SkinModified && m_pClient->m_LastSkinChangeTime + 6.0f > Client()->LocalTime())) && this->Client()->State() == IClient::STATE_ONLINE;
	// backwards compatibility
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	const char *pVersion = str_startswith(CurrentServerInfo.m_aVersion, "0.7.");
	bool NeedRestartTee = pVersion && (pVersion[0] == '1' || pVersion[0] == '2') && pVersion[1] == 0;

	if(m_NeedRestartGraphics || m_NeedRestartSound || NeedReconnect || NeedRestartTee)
	{
		// background
		CUIRect RestartWarning;
		MainView.HSplitTop(25.0f, &RestartWarning, 0);
		RestartWarning.VMargin(Config()->m_UiWideview ? 210.0f : 140.0f, &RestartWarning);
		RestartWarning.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f));

		// text
		TextRender()->TextColor(0.973f, 0.863f, 0.207f, 1.0f);
		if(m_NeedRestartGraphics || m_NeedRestartSound)
			UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), RestartWarning.h*CUI::ms_FontmodHeight*0.75f, TEXTALIGN_MC);
		else if(Client()->State() == IClient::STATE_ONLINE)
		{
			if(m_NeedRestartPlayer || NeedRestartTee)
				UI()->DoLabel(&RestartWarning, Localize("You must reconnect to change identity."), RestartWarning.h*CUI::ms_FontmodHeight*0.75f, TEXTALIGN_MC);
			else if(m_SkinModified)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), Localize("You have to wait %1.0f seconds to change identity."), m_pClient->m_LastSkinChangeTime+6.5f - Client()->LocalTime());
				UI()->DoLabel(&RestartWarning, aBuf, RestartWarning.h*CUI::ms_FontmodHeight*0.75f, TEXTALIGN_MC);
			}
		}
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
	}

	RenderBackButton(MainView);
}
