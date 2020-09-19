/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/contacts.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/version.h>
#include <generated/protocol.h>

#include <generated/client_data.h>
#include <game/client/components/binds.h>
#include <game/client/components/camera.h>
#include <game/client/components/console.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <mastersrv/mastersrv.h>

#include "maplayers.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"

float CMenus::ms_ButtonHeight = 25.0f;
float CMenus::ms_ListheaderHeight = 17.0f;
float CMenus::ms_FontmodHeight = 0.8f;

CMenus *CMenus::CUIElementBase::m_pMenus = 0;
CRenderTools *CMenus::CUIElementBase::m_pRenderTools = 0;
CUI *CMenus::CUIElementBase::m_pUI = 0;
IInput *CMenus::CUIElementBase::m_pInput = 0;
IClient *CMenus::CUIElementBase::m_pClient = 0;

CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_ActivePage = PAGE_INTERNET;
	m_GamePage = PAGE_GAME;

	m_SidebarTab = 0;
	m_SidebarActive = true;
	m_ShowServerDetails = true;
	m_LastBrowserType = -1;
	m_AddressSelection = 0;
	for(int Type = 0; Type < IServerBrowser::NUM_TYPES; Type++)
	{
		m_aSelectedFilters[Type] = -2;
		m_aSelectedServers[Type] = -1;
	}

	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_NeedRestartPlayer = false;
	m_TeePartSelected = SKINPART_BODY;
	m_aSaveSkinName[0] = 0;
	m_RefreshSkinSelector = true;
	m_pSelectedSkin = 0;
	m_MenuActive = true;
	m_SeekBarActivatedTime = 0;
	m_SeekBarActive = true;
	m_SkinModified = false;
	m_KeyReaderWasActive = false;
	m_KeyReaderIsActive = false;

	SetMenuPage(PAGE_START);
	m_MenuPageOld = PAGE_START;

	m_PopupActive = false;

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_TabPressed = false;
	m_DeletePressed = false;
	m_UpArrowPressed = false;
	m_DownArrowPressed = false;

	m_LastInput = time_get();

	m_CursorActive = false;
	m_PrevCursorActive = false;

	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCallvoteReason[0] = 0;
	m_aFilterString[0] = 0;

	m_ActiveListBox = ACTLB_NONE;
	m_DemoPlayerState = DEMOPLAYER_NONE;
}

float CMenus::CButtonContainer::GetFade(bool Checked, float Seconds)
{
	if(m_pUI->HotItem() == GetID() || Checked)
	{
		m_FadeStartTime = m_pClient->LocalTime();
		return 1.0f;
	}

	return max(0.0f, m_FadeStartTime -  m_pClient->LocalTime() + Seconds)/Seconds;
}

void CMenus::DoIcon(int ImageId, int SpriteId, const CUIRect *pRect, const vec4 *pColor)
{
	Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);

	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	if(pColor)
	{
		Graphics()->SetColor(pColor->r*pColor->a, pColor->g*pColor->a, pColor->b*pColor->a, pColor->a);
	}
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

bool CMenus::DoButton_Toggle(const void *pID, bool Checked, const CUIRect *pRect, bool Active)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
	{
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	}
	RenderTools()->SelectSprite(Checked ? SPRITE_GUIBUTTON_ON : SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(UI()->HotItem() == pID && Active)
	{
		RenderTools()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active && UI()->DoButtonLogic(pID, pRect);
}

bool CMenus::DoButton_Menu(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, vec4 ColorHot, bool TextFade)
{
	const float FadeVal = pBC->GetFade(Checked);

	CUIRect Text = *pRect;
	RenderTools()->DrawUIRect(pRect, mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), ColorHot, FadeVal), Corners, Rounding);

	if(pImageName)
	{
		CUIRect Image;
		pRect->VSplitRight(pRect->h*4.0f, &Text, &Image); // always correct ratio for image

		// render image
		const CMenuImage *pImage = FindMenuImage(pImageName);
		if(pImage)
		{

			Graphics()->TextureSet(pImage->m_GreyTexture);
			Graphics()->WrapClamp();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();

			if(FadeVal > 0.0f)
			{
				Graphics()->TextureSet(pImage->m_OrgTexture);
				Graphics()->WrapClamp();
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f*FadeVal, 1.0f*FadeVal, 1.0f*FadeVal, FadeVal);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			Graphics()->WrapNormal();
		}
	}

	Text.HMargin(pRect->h >= 20.0f ? 2.0f : 1.0f, &Text);
	Text.HMargin((Text.h*FontFactor)/2.0f, &Text);
	if(TextFade)
	{
		TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
		TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	}
	UI()->DoLabel(&Text, pText, Text.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	if(TextFade)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
	}
	return UI()->DoButtonLogic(pBC->GetID(), pRect);
}

void CMenus::DoButton_KeySelect(CButtonContainer *pBC, const char *pText, const CUIRect *pRect)
{
	const float FadeVal = pBC->GetFade();

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label;
	pRect->HMargin(1.0f, &Label);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Label, pText, Label.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
}

bool CMenus::DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, float Alpha, float FontAlpha, int Corners, float Rounding, float FontFactor)
{
	const float ActualAlpha = UI()->MouseHovered(pRect) ? 1.0f : Alpha;
	const float FadeVal = pBC->GetFade(Checked)*FontAlpha;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, Config()->m_ClMenuAlpha/100.0f*ActualAlpha+FadeVal*0.5f), Corners, Rounding);

	CUIRect Label;
	pRect->HMargin(pRect->h >= 20.0f ? 2.0f : 1.0f, &Label);
	Label.HMargin((Label.h*FontFactor)/2.0f, &Label);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, FontAlpha);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f*FontAlpha);
	UI()->DoLabel(&Label, pText, Label.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
	return UI()->DoButtonLogic(pBC->GetID(), pRect);
}

bool CMenus::DoButton_GridHeader(const void *pID, const char *pText, bool Checked, CUI::EAlignment Align, const CUIRect *pRect)
{
	if(Checked)
	{
		RenderTools()->DrawUIRect(pRect, vec4(0.9f, 0.9f, 0.9f, 0.5f), CUI::CORNER_ALL, 5.0f);
		TextRender()->TextColor(CUI::ms_HighlightTextColor);
		TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
	}
	else if(UI()->HotItem() == pID)
	{
		RenderTools()->DrawUIRect(pRect, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f);
	}

	CUIRect Label;
	pRect->VMargin(2.0f, &Label);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, pText, pRect->h*ms_FontmodHeight*0.8f, Align);

	if(Checked)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
	}

	return UI()->DoButtonLogic(pID, pRect);
}

bool CMenus::DoButton_CheckBox(const void *pID, const char *pText, bool Checked, const CUIRect *pRect, bool Locked)
{
	if(Locked)
	{
		TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
	}

	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Checkbox = *pRect;
	CUIRect Label = *pRect;
	Checkbox.w = Checkbox.h;
	Label.x += Checkbox.w;
	Label.w -= Checkbox.w;
	Label.VSplitLeft(5.0f, 0, &Label);

	Checkbox.Margin(2.0f, &Checkbox);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	if(Locked)
		Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	else
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	if(UI()->HotItem() == pID)
	{
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_HOVER);
		IGraphics::CQuadItem QuadItem(Checkbox.x, Checkbox.y, Checkbox.w, Checkbox.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	RenderTools()->SelectSprite(Checked ? SPRITE_MENU_CHECKBOX_ACTIVE : SPRITE_MENU_CHECKBOX_INACTIVE);
	IGraphics::CQuadItem QuadItem(Checkbox.x, Checkbox.y, Checkbox.w, Checkbox.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	Label.y += 1.0f; // lame fix
	UI()->DoLabel(&Label, pText, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	if(Locked)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		return false;
	}
	return UI()->DoButtonLogic(pID, pRect);
}

bool CMenus::DoButton_SpriteID(CButtonContainer *pBC, int ImageID, int SpriteID, bool Checked, const CUIRect *pRect, int Corners, float Rounding, bool Fade)
{
	const float FadeVal = Fade ? pBC->GetFade(Checked) : 0.0f;

	CUIRect Icon = *pRect;

	if(FadeVal > 0.0f || !pBC->IsCleanBackground())
	{
		RenderTools()->DrawUIRect(pRect, vec4(0.0f + FadeVal, 0.0f + FadeVal, 0.0f + FadeVal, 0.25f + FadeVal * 0.5f), Corners, Rounding);
	}

	if(!pBC->IsCleanBackground())
	{
		if(Icon.w > Icon.h)
			Icon.VMargin((Icon.w - Icon.h) / 2, &Icon);
		else if(Icon.w < Icon.h)
			Icon.HMargin((Icon.h - Icon.w) / 2, &Icon);
		Icon.Margin(2.0f, &Icon);
	}

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pBC->GetID(), pRect);
}

bool CMenus::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden, int Corners)
{
	bool Inside = UI()->MouseHovered(pRect);
	bool Changed = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;

	if(UI()->LastActiveItem() == pID)
	{
		static float s_ScrollStart = 0.0f;

		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;

		if(Inside && UI()->MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = UI()->MouseX();
			int MxRel = (int)(UI()->MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(TextRender()->TextWidth(0, FontSize, pStr, i, -1.0f) - *pOffset > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!UI()->MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(UI()->MouseX() < pRect->x && s_ScrollStart-UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
			else if(UI()->MouseX() > pRect->x+pRect->w && UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
		}
		else if(!Inside && UI()->MouseButton(0))
		{
			s_AtIndex = min(s_AtIndex, str_length(pStr));
			s_DoScroll = false;
			UI()->SetActiveItem(0);
			UI()->ClearLastActiveItem();
		}

		if(UI()->LastActiveItem() == pID && UI()->Enabled())
		{
			for(int i = 0; i < Input()->NumEvents(); i++)
			{
				Len = str_length(pStr);
				int NumChars = Len;
				Changed |= CLineInput::Manipulate(Input()->GetEvent(i), pStr, StrSize, StrSize, &Len, &s_AtIndex, &NumChars, Input());
			}
		}
	}

	bool JustGotActive = false;

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
		{
			s_AtIndex = min(s_AtIndex, str_length(pStr));
			s_DoScroll = false;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pID)
				JustGotActive = true;
			UI()->SetActiveItem(pID);
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	vec4 Color;
	if(UI()->LastActiveItem() == pID)
		Color = vec4(0.25f, 0.25f, 0.25f, 0.25f);
	else if(Inside)
		Color = vec4(0.5f, 0.5f, 0.5f, 0.25f);
	else
		Color = vec4(0.0f, 0.0f, 0.0f, 0.25f);
	RenderTools()->DrawUIRect(&Textbox, Color, Corners, 5.0f);
	Textbox.Margin(2.0f, &Textbox);

	const char *pDisplayStr = pStr;
	char aStars[128];

	if(Hidden)
	{
		unsigned s = str_length(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(UI()->LastActiveItem() == pID && !JustGotActive && (UpdateOffset || Input()->NumEvents()))
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex, -1.0f);
		if(w-*pOffset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1, -1.0f);
			do
			{
				*pOffset += min(wt-*pOffset-Textbox.w, Textbox.w/3);
			}
			while(w-*pOffset > Textbox.w);
		}
		else if(w-*pOffset < 0.0f)
		{
			// move to the right
			do
			{
				*pOffset = max(0.0f, *pOffset-Textbox.w/3);
			}
			while(w-*pOffset < 0.0f);
		}
	}
	UI()->ClipEnable(pRect);
	Textbox.x -= *pOffset;

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, CUI::ALIGN_LEFT);

	// render the cursor
	if(UI()->LastActiveItem() == pID && !JustGotActive)
	{
		// set cursor active
		m_CursorActive = true;

		if((2*time_get()/time_freq()) % 2)	// make it blink
		{
			float TextWidth = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex, -1.0f);
			Textbox = *pRect;
			Textbox.VSplitLeft(2.0f, 0, &Textbox);
			Textbox.x += TextWidth - *pOffset - TextRender()->TextWidth(0, FontSize, "|", -1, -1.0f)/2;
			UI()->DoLabel(&Textbox, "|", FontSize, CUI::ALIGN_LEFT);
		}
	}
	UI()->ClipDisable();

	return Changed;
}

void CMenus::DoEditBoxOption(void *pID, char *pOption, int OptionLength, const CUIRect *pRect, const char *pStr,  float VSplitVal, float *pOffset, bool Hidden)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, EditBox;
	pRect->VSplitLeft(VSplitVal, &Label, &EditBox);

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pStr);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, aBuf, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	DoEditBox(pID, &EditBox, pOption, OptionLength, pRect->h*ms_FontmodHeight*0.8f, pOffset, Hidden);
}

void CMenus::DoScrollbarOption(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, IScrollbarScale *pScale, bool Infinite)
{
	int Value = *pOption;
	if(Infinite)
	{
		Min += 1;
		Max += 1;
		if(Value == 0)
			Value = Max;
	}

	char aBufMax[128];
	str_format(aBufMax, sizeof(aBufMax), "%s: %i", pStr, Max);
	char aBuf[128];
	if(!Infinite || Value != Max)
		str_format(aBuf, sizeof(aBuf), "%s: %i", pStr, Value);
	else
		str_format(aBuf, sizeof(aBuf), "%s: \xe2\x88\x9e", pStr);

	float FontSize = pRect->h*ms_FontmodHeight*0.8f;
	float VSplitVal = max(TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f), TextRender()->TextWidth(0, FontSize, aBufMax, -1, -1.0f));

	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(pRect->h+10.0f+VSplitVal, &Label, &ScrollBar);
	Label.VSplitLeft(Label.h+5.0f, 0, &Label);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_LEFT);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, Min, Max)), Min, Max);
	if(Infinite && Value == Max)
		Value = 0;

	*pOption = Value;
}

void CMenus::DoScrollbarOptionLabeled(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, const char* aLabels[], int Num, IScrollbarScale *pScale)
{
	int Value = clamp(*pOption, 0, Num - 1);
	const int Max = Num - 1;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s: %s", pStr, aLabels[Value]);

	float FontSize = pRect->h*ms_FontmodHeight*0.8f;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(5.0f, 0, &Label);
	Label.VSplitRight(60.0f, &Label, &ScrollBar);

	Label.y += 2.0f;
	UI()->DoLabel(&Label, aBuf, FontSize, CUI::ALIGN_LEFT);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	Value = pScale->ToAbsolute(DoScrollbarH(pID, &ScrollBar, pScale->ToRelative(Value, 0, Max)), 0, Max);

	*pOption = clamp(Value, 0, Max);
}

float CMenus::DoIndependentDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, bool *pActive)
{
	CUIRect View = *pRect;
	CUIRect Header, Label;

	View.HSplitTop(HeaderHeight, &Header, &View);

	// background
	RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), *pActive ? CUI::CORNER_T : CUI::CORNER_ALL, 5.0f);

	// render icon
	CUIRect Button;
	Header.VSplitLeft(Header.h, &Button, 0);
	Button.Margin(2.0f, &Button);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	if(UI()->MouseHovered(&Header))
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	else
		Graphics()->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	RenderTools()->SelectSprite(*pActive ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED);
	IGraphics::CQuadItem QuadItem(Button.x, Button.y, Button.w, Button.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->QuadsEnd();

	// label
	Label = Header;
	Label.y += 2.0f;
	UI()->DoLabel(&Label, pStr, Header.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	if(UI()->DoButtonLogic(pID, &Header))
		*pActive ^= 1;

	// render content of expanded menu
	if(*pActive)
		return HeaderHeight + (this->*pfnCallback)(View);

	return HeaderHeight;
}

void CMenus::DoInfoBox(const CUIRect *pRect, const char *pLabel, const char *pValue)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, Value;
	pRect->VSplitMid(&Label, &Value, 2.0f);

	RenderTools()->DrawUIRect(&Value, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pLabel);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, aBuf, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	Value.y += 2.0f;
	UI()->DoLabel(&Value, pValue, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
}

float CMenus::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	// layout
	CUIRect Handle;
	pRect->HSplitTop(min(pRect->h/8.0f, 33.0f), &Handle, 0);
	Handle.y += (pRect->h-Handle.h)*Current;
	Handle.VMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);

	// logic
	static float s_OffsetY;
	const bool InsideHandle = UI()->MouseHovered(&Handle);
	const bool InsideRail = UI()->MouseHovered(&Rail);
	float ReturnValue = Current;
	bool Grabbed = false; // whether to apply the offset

	if(UI()->CheckActiveItem(pID))
	{
		if(UI()->MouseButton(0))
			Grabbed = true;
		else
			UI()->SetActiveItem(0);
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			s_OffsetY = UI()->MouseY()-Handle.y;
			UI()->SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(UI()->MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetY = Handle.h * 0.5f;
		UI()->SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		UI()->SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->y;
		const float Max = pRect->h-Handle.h;
		const float Cur = UI()->MouseY()-s_OffsetY;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	RenderTools()->DrawRoundRect(&Rail, vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.w/2.0f);

	vec4 Color;
	if(Grabbed)
		Color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	else if(InsideHandle)
		Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else
		Color = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	RenderTools()->DrawRoundRect(&Handle, Color, Handle.w/2.0f);

	return ReturnValue;
}

float CMenus::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	// layout
	CUIRect Handle;
	pRect->VSplitLeft(max(min(pRect->w/8.0f, 33.0f), pRect->h), &Handle, 0);
	Handle.x += (pRect->w-Handle.w)*clamp(Current, 0.0f, 1.0f);
	Handle.HMargin(5.0f, &Handle);

	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);

	// logic
	static float s_OffsetX;
	const bool InsideHandle = UI()->MouseHovered(&Handle);
	const bool InsideRail = UI()->MouseHovered(&Rail);
	float ReturnValue = Current;
	bool Grabbed = false; // whether to apply the offset

	if(UI()->CheckActiveItem(pID))
	{
		if(UI()->MouseButton(0))
			Grabbed = true;
		else
			UI()->SetActiveItem(0);
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			s_OffsetX = UI()->MouseX()-Handle.x;
			UI()->SetActiveItem(pID);
			Grabbed = true;
		}
	}
	else if(UI()->MouseButtonClicked(0) && !InsideHandle && InsideRail)
	{
		s_OffsetX = Handle.w * 0.5f;
		UI()->SetActiveItem(pID);
		Grabbed = true;
	}

	if(InsideHandle)
	{
		UI()->SetHotItem(pID);
	}

	if(Grabbed)
	{
		const float Min = pRect->x;
		const float Max = pRect->w-Handle.w;
		const float Cur = UI()->MouseX()-s_OffsetX;
		ReturnValue = clamp((Cur-Min)/Max, 0.0f, 1.0f);
	}

	// render
	RenderTools()->DrawRoundRect(&Rail, vec4(1.0f, 1.0f, 1.0f, 0.25f), Rail.h/2.0f);

	vec4 Color;
	if(Grabbed)
		Color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
	else if(InsideHandle)
		Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	else
		Color = vec4(0.8f, 0.8f, 0.8f, 1.0f);
	RenderTools()->DrawRoundRect(&Handle, Color, Handle.h/2.0f);

	return ReturnValue;
}

void CMenus::DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active)
{
	CUIRect Handle;
	pRect->VSplitLeft(7, &Handle, 0); // Slider size

	Handle.x += (pRect->w-Handle.w)*Current;

	// render
	CUIRect Rail;
	pRect->HMargin(4.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1.0f, 1.0f, 1.0f, Active ? 0.25f : 0.125f), CUI::CORNER_ALL, Rail.h/2.0f);

	CUIRect ToleranceArea = Rail;
	ToleranceArea.w *= Tolerance;
	ToleranceArea.x += (Rail.w-ToleranceArea.w)/2.0f;
	vec4 ToleranceColor = Active ? vec4(0.8f, 0.35f, 0.35f, 1.0f) : vec4(0.7f, 0.5f, 0.5f, 1.0f);
	RenderTools()->DrawUIRect(&ToleranceArea, ToleranceColor, CUI::CORNER_ALL, ToleranceArea.h/2.0f);

	CUIRect Slider = Handle;
	Slider.HMargin(4.0f, &Slider);
	vec4 SliderColor = Active ? vec4(0.95f, 0.95f, 0.95f, 1.0f) : vec4(0.8f, 0.8f, 0.8f, 1.0f);
	RenderTools()->DrawUIRect(&Slider, SliderColor, CUI::CORNER_ALL, Slider.h/2.0f);
}

int CMenus::DoKeyReader(CButtonContainer *pBC, const CUIRect *pRect, int Key, int Modifier, int* NewModifier)
{
	// process
	static const void *pGrabbedID = 0;
	static bool MouseReleased = true;
	static int ButtonUsed = 0;
	const bool Hovered = UI()->MouseHovered(pRect);
	int NewKey = Key;
	*NewModifier = Modifier;

	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && pGrabbedID == pBC->GetID())
		MouseReleased = true;

	if(UI()->CheckActiveItem(pBC->GetID()))
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
			{
				NewKey = m_Binder.m_Key.m_Key;
				*NewModifier = m_Binder.m_Modifier;
			}
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pBC->GetID();
		}

		if(ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Hovered)
				NewKey = 0;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pBC->GetID())
	{
		if(MouseReleased)
		{
			if(UI()->MouseButton(0))
			{
				m_Binder.m_TakeKey = true;
				m_Binder.m_GotKey = false;
				UI()->SetActiveItem(pBC->GetID());
				ButtonUsed = 0;
			}

			if(UI()->MouseButton(1))
			{
				UI()->SetActiveItem(pBC->GetID());
				ButtonUsed = 1;
			}
		}
	}

	if(Hovered)
		UI()->SetHotItem(pBC->GetID());

	// draw
	if(UI()->CheckActiveItem(pBC->GetID()) && ButtonUsed == 0)
	{
		DoButton_KeySelect(pBC, "???", pRect);
		m_KeyReaderIsActive = true;
	}
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pBC, "", pRect);
		else
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s%s", CBinds::GetModifierName(*NewModifier), Input()->KeyName(Key));
			DoButton_KeySelect(pBC, aBuf, pRect);
		}
	}
	return NewKey;
}

void CMenus::RenderMenubar(CUIRect Rect)
{
	CUIRect Box;
	CUIRect Button;
	Rect.HSplitTop(60.0f, &Box, &Rect);
	const float InactiveAlpha = 0.25f;

	m_ActivePage = m_MenuPage;
	int NewPage = -1;

	if(Client()->State() != IClient::STATE_OFFLINE)
		m_ActivePage = m_GamePage;

	if(Client()->State() == IClient::STATE_ONLINE)
	{
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w / 6.0f) - (Spacing*5.0) / 6.0f;
		float Alpha = 1.0f;
		if(m_GamePage == PAGE_SETTINGS)
			Alpha = InactiveAlpha;

		// render header backgrounds
		CUIRect Left, Right;
		Box.VSplitLeft(ButtonWidth*4.0f + Spacing*3.0f, &Left, 0);
		Box.VSplitRight(ButtonWidth*1.5f + Spacing, 0, &Right);
		RenderBackgroundShadow(&Left, false);
		RenderBackgroundShadow(&Right, false);

		Left.HSplitBottom(25.0f, 0, &Left);
		Right.HSplitBottom(25.0f, 0, &Right);

		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_GameButton;
		if(DoButton_MenuTabTop(&s_GameButton, Localize("Game"), m_ActivePage == PAGE_GAME, &Button, Alpha, Alpha) || CheckHotKey(KEY_G))
			NewPage = PAGE_GAME;

		Left.VSplitLeft(Spacing, 0, &Left); // little space
		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_PlayersButton;
		if(DoButton_MenuTabTop(&s_PlayersButton, Localize("Players"), m_ActivePage == PAGE_PLAYERS, &Button, Alpha, Alpha) || CheckHotKey(KEY_P))
			NewPage = PAGE_PLAYERS;

		Left.VSplitLeft(Spacing, 0, &Left); // little space
		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_ServerInfoButton;
		if(DoButton_MenuTabTop(&s_ServerInfoButton, Localize("Server info"), m_ActivePage == PAGE_SERVER_INFO, &Button, Alpha, Alpha) || CheckHotKey(KEY_I))
			NewPage = PAGE_SERVER_INFO;

		Left.VSplitLeft(Spacing, 0, &Left); // little space
		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_CallVoteButton;
		if(DoButton_MenuTabTop(&s_CallVoteButton, Localize("Call vote"), m_ActivePage == PAGE_CALLVOTE, &Button, Alpha, Alpha) || CheckHotKey(KEY_V))
			NewPage = PAGE_CALLVOTE;

		Right.VSplitRight(ButtonWidth, &Right, &Button);
		static CButtonContainer s_SettingsButton;
		if(DoButton_MenuTabTop(&s_SettingsButton, Localize("Settings"), m_GamePage == PAGE_SETTINGS, &Button) || CheckHotKey(KEY_S))
			NewPage = PAGE_SETTINGS;

		Right.VSplitRight(Spacing, &Right, 0); // little space
		Right.VSplitRight(ButtonWidth/2.0f, &Right, &Button);
		static CButtonContainer s_ServerBrowserButton;
		if(DoButton_SpriteID(&s_ServerBrowserButton, IMAGE_BROWSER, UI()->MouseHovered(&Button) || m_GamePage == PAGE_INTERNET || m_GamePage == PAGE_LAN ? SPRITE_BROWSER_B : SPRITE_BROWSER_A,
			m_GamePage == PAGE_INTERNET || m_GamePage == PAGE_LAN, &Button) || CheckHotKey(KEY_B))
		{
			NewPage = ServerBrowser()->GetType() == IServerBrowser::TYPE_INTERNET ? PAGE_INTERNET : PAGE_LAN;
		}

		Rect.HSplitTop(Spacing, 0, &Rect);
		Rect.HSplitTop(25.0f, &Box, &Rect);
	}

	if((Client()->State() == IClient::STATE_OFFLINE && m_MenuPage == PAGE_SETTINGS) || (Client()->State() == IClient::STATE_ONLINE && m_GamePage == PAGE_SETTINGS))
	{
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/6.0f)-(Spacing*5.0)/6.0f;
		float NotActiveAlpha = Client()->State() == IClient::STATE_ONLINE ? 0.5f : 1.0f;
		int Corners = Client()->State() == IClient::STATE_ONLINE ? CUI::CORNER_T : CUI::CORNER_ALL;

		// render header background
		if(Client()->State() == IClient::STATE_OFFLINE)
			RenderBackgroundShadow(&Box, false);

		Box.HSplitBottom(25.0f, 0, &Box);

		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_GeneralButton;
		if(DoButton_MenuTabTop(&s_GeneralButton, Localize("General"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage==SETTINGS_GENERAL, &Button,
			Config()->m_UiSettingsPage == SETTINGS_GENERAL ? 1.0f : NotActiveAlpha, 1.0f, Corners))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_GENERAL);
			Config()->m_UiSettingsPage = SETTINGS_GENERAL;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		{
			static CButtonContainer s_PlayerButton;
			if(DoButton_MenuTabTop(&s_PlayerButton, Localize("Player"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage == SETTINGS_PLAYER, &Button,
				Config()->m_UiSettingsPage == SETTINGS_PLAYER ? 1.0f : NotActiveAlpha, 1.0f, Corners))
			{
				m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_PLAYER);
				Config()->m_UiSettingsPage = SETTINGS_PLAYER;
			}
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		{
			static CButtonContainer s_TeeButton;
			if(DoButton_MenuTabTop(&s_TeeButton, Localize("Tee"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage == SETTINGS_TEE, &Button,
				Config()->m_UiSettingsPage == SETTINGS_TEE ? 1.0f : NotActiveAlpha, 1.0f, Corners))
			{
				m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_TEE);
				Config()->m_UiSettingsPage = SETTINGS_TEE;
			}
		}


		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_ControlsButton;
		if(DoButton_MenuTabTop(&s_ControlsButton, Localize("Controls"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage==SETTINGS_CONTROLS, &Button,
			Config()->m_UiSettingsPage == SETTINGS_CONTROLS ? 1.0f : NotActiveAlpha, 1.0f, Corners))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_CONTROLS);
			Config()->m_UiSettingsPage = SETTINGS_CONTROLS;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_GraphicsButton;
		if(DoButton_MenuTabTop(&s_GraphicsButton, Localize("Graphics"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage==SETTINGS_GRAPHICS, &Button,
			Config()->m_UiSettingsPage == SETTINGS_GRAPHICS ? 1.0f : NotActiveAlpha, 1.0f, Corners))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_GRAPHICS);
			Config()->m_UiSettingsPage = SETTINGS_GRAPHICS;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_SoundButton;
		if(DoButton_MenuTabTop(&s_SoundButton, Localize("Sound"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage==SETTINGS_SOUND, &Button,
			Config()->m_UiSettingsPage == SETTINGS_SOUND ? 1.0f : NotActiveAlpha, 1.0f, Corners))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_SOUND);
			Config()->m_UiSettingsPage = SETTINGS_SOUND;
		}
	}
	else if((Client()->State() == IClient::STATE_OFFLINE && m_MenuPage >= PAGE_INTERNET && m_MenuPage <= PAGE_LAN) || (Client()->State() == IClient::STATE_ONLINE && m_GamePage >= PAGE_INTERNET && m_GamePage <= PAGE_LAN))
	{
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/6.0f)-(Spacing*5.0)/6.0f;
		int Corners = Client()->State() == IClient::STATE_ONLINE ? CUI::CORNER_T : CUI::CORNER_ALL;
		float NotActiveAlpha = Client()->State() == IClient::STATE_ONLINE ? 0.5f : 1.0f;

		CUIRect Left;
		Box.VSplitLeft(ButtonWidth*2.0f+Spacing, &Left, 0);

		// render header backgrounds
		if(Client()->State() == IClient::STATE_OFFLINE)
			RenderBackgroundShadow(&Left, false);

		Left.HSplitBottom(25.0f, 0, &Left);

		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_InternetButton;
		if(DoButton_MenuTabTop(&s_InternetButton, Localize("Global"), m_ActivePage==PAGE_INTERNET && Client()->State() == IClient::STATE_OFFLINE, &Button,
			m_ActivePage==PAGE_INTERNET ? 1.0f : NotActiveAlpha, 1.0f, Corners) || CheckHotKey(KEY_G))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_INTERNET);
			ServerBrowser()->SetType(IServerBrowser::TYPE_INTERNET);
			NewPage = PAGE_INTERNET;
			Config()->m_UiBrowserPage = PAGE_INTERNET;
		}

		Left.VSplitLeft(Spacing, 0, &Left); // little space
		Left.VSplitLeft(ButtonWidth, &Button, &Left);
		static CButtonContainer s_LanButton;
		if(DoButton_MenuTabTop(&s_LanButton, Localize("Local"), m_ActivePage==PAGE_LAN && Client()->State() == IClient::STATE_OFFLINE, &Button,
			m_ActivePage==PAGE_LAN ? 1.0f : NotActiveAlpha, 1.0f, Corners) || CheckHotKey(KEY_L))
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_LAN);
			ServerBrowser()->SetType(IServerBrowser::TYPE_LAN);
			NewPage = PAGE_LAN;
			Config()->m_UiBrowserPage = PAGE_LAN;
		}

		if(Client()->State() == IClient::STATE_OFFLINE)
		{
			CUIRect Right, Button;
			Box.VSplitRight(20.0f, &Box, 0);
			Box.VSplitRight(60.0f, 0, &Right);
			Right.HSplitBottom(20.0f, 0, &Button);
		}
	}
	else if(Client()->State() == IClient::STATE_OFFLINE)
	{
		if(m_MenuPage == PAGE_DEMOS)
		{
			// render header background
			RenderBackgroundShadow(&Box, false);

			Box.HSplitBottom(25.0f, 0, &Box);

			// make the header look like an active tab
			RenderTools()->DrawUIRect(&Box, vec4(1.0f, 1.0f, 1.0f, 0.75f), CUI::CORNER_ALL, 5.0f);
			Box.HMargin(2.0f, &Box);
			TextRender()->TextColor(CUI::ms_HighlightTextColor);
			TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
			UI()->DoLabel(&Box, Localize("Demos"), Box.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
			TextRender()->TextColor(CUI::ms_DefaultTextColor);
			TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
		}
	}



	if(NewPage != -1)
	{
		if(Client()->State() == IClient::STATE_OFFLINE)
			SetMenuPage(NewPage);
		else
		{
			m_GamePage = NewPage;
			if(NewPage == PAGE_INTERNET || NewPage == PAGE_LAN)
				SetMenuPage(NewPage);
		}
	}
}

void CMenus::InitLoading(int TotalWorkAmount)
{
	m_LoadCurrent = 0;
	m_LoadTotal = TotalWorkAmount;
}

void CMenus::RenderLoading(int WorkedAmount)
{
	static int64 s_LastLoadRender = 0;
	m_LoadCurrent += WorkedAmount;
	const int64 Now = time_get();
	const float Freq = time_freq();

	if(Config()->m_Debug)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "progress: %03d/%03d (+%02d) %dms", m_LoadCurrent, m_LoadTotal, WorkedAmount, s_LastLoadRender == 0 ? 0 : int((Now-s_LastLoadRender)*1000.0f/Freq));
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "loading", aBuf);
	}

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	if(s_LastLoadRender > 0 && Now-s_LastLoadRender < Freq/60)
		return;
	s_LastLoadRender = Now;
	static int64 s_LoadingStart = Now;

	m_pClient->StartRendering();
	RenderBackground((Now-s_LoadingStart)/Freq);

	CUIRect Screen = *UI()->Screen();
	const float w = 700;
	const float h = 200;
	const float x = Screen.w/2-w/2;
	const float y = Screen.h/2-h/2;
	CUIRect Rect = { x, y, w, h };

	Graphics()->BlendNormal();
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.5f), 40.0f);

	Rect.y += 20;
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
	UI()->DoLabel(&Rect, "Teeworlds", 48.0f, CUI::ALIGN_CENTER);

	const float Percent = m_LoadCurrent/(float)m_LoadTotal;
	const float Spacing = 40.0f;
	const float BarRounding = 5.0f;

	CUIRect FullBar = { x+Spacing, y+h-75.0f, w-2*Spacing, 25.0f };
	RenderTools()->DrawRoundRect(&FullBar, vec4(1.0f, 1.0f, 1.0f, 0.1f), BarRounding);

	CUIRect FillingBar = FullBar;
	FillingBar.w = (FullBar.w-2*BarRounding)*Percent+2*BarRounding;
	RenderTools()->DrawRoundRect(&FillingBar, vec4(1.0f, 1.0f, 1.0f, 0.75f), BarRounding);

	if(Percent > 0.5f)
	{
		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.7f);
		TextRender()->TextColor(0.2f, 0.2f, 0.2f, 1.0f);
	}
	char aBuf[8];
	str_format(aBuf, sizeof(aBuf), "%d%%", (int)(100*Percent));
	UI()->DoLabel(&FullBar, aBuf, 20.0f, CUI::ALIGN_CENTER);

	if(Percent > 0.5f)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
	}

	Graphics()->Swap();
}

void CMenus::RenderNews(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
}

void CMenus::RenderBackButton(CUIRect MainView)
{
	if(Client()->State() != IClient::STATE_OFFLINE)
		return;

	// same size like tabs in top but variables not really needed
	float Spacing = 3.0f;
	float ButtonWidth = (MainView.w/6.0f)-(Spacing*5.0)/6.0f;

	// render background
	MainView.HSplitBottom(60.0f, 0, &MainView);
	MainView.VSplitLeft(ButtonWidth, &MainView, 0);
	RenderBackgroundShadow(&MainView, true);

	// back to main menu
	CUIRect Button;
	MainView.HSplitTop(25.0f, &MainView, 0);
	Button = MainView;
	static CButtonContainer s_MenuButton;
	if(DoButton_Menu(&s_MenuButton, Localize("Back"), 0, &Button) || m_EscapePressed)
	{
		SetMenuPage(m_MenuPageOld);
		m_MenuPageOld = PAGE_START;
	}
}

int CMenus::MenuImageScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	if(IsDir || !str_endswith(pName, ".png"))
		return 0;

	char aBuf[IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "ui/menuimages/%s", pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load menu image from %s", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return 0;
	}

	CMenuImage MenuImage;
	MenuImage.m_OrgTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);

	unsigned char *d = (unsigned char *)Info.m_pData;
	//int Pitch = Info.m_Width*4;

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

	/* same grey like sinks
	int Freq[256] = {0};
	int OrgWeight = 0;
	int NewWeight = 192;

	// find most common frequence
	for(int y = 0; y < Info.m_Height; y++)
		for(int x = 0; x < Info.m_Width; x++)
		{
			if(d[y*Pitch+x*4+3] > 128)
				Freq[d[y*Pitch+x*4]]++;
		}

	for(int i = 1; i < 256; i++)
	{
		if(Freq[OrgWeight] < Freq[i])
			OrgWeight = i;
	}

	// reorder
	int InvOrgWeight = 255-OrgWeight;
	int InvNewWeight = 255-NewWeight;
	for(int y = 0; y < Info.m_Height; y++)
		for(int x = 0; x < Info.m_Width; x++)
		{
			int v = d[y*Pitch+x*4];
			if(v <= OrgWeight)
				v = (int)(((v/(float)OrgWeight) * NewWeight));
			else
				v = (int)(((v-OrgWeight)/(float)InvOrgWeight)*InvNewWeight + NewWeight);
			d[y*Pitch+x*4] = v;
			d[y*Pitch+x*4+1] = v;
			d[y*Pitch+x*4+2] = v;
		}
	*/

	MenuImage.m_GreyTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	mem_free(Info.m_pData);

	// set menu image data
	str_truncate(MenuImage.m_aName, sizeof(MenuImage.m_aName), pName, str_length(pName) - 4);
	str_format(aBuf, sizeof(aBuf), "load menu image %s", MenuImage.m_aName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	pSelf->m_lMenuImages.add(MenuImage);
	pSelf->RenderLoading(5);

	return 0;
}

const CMenus::CMenuImage *CMenus::FindMenuImage(const char *pName)
{
	for(int i = 0; i < m_lMenuImages.size(); i++)
	{
		if(str_comp(m_lMenuImages[i].m_aName, pName) == 0)
			return &m_lMenuImages[i];
	}
	return 0;
}

void CMenus::UpdatedFilteredVideoModes()
{
	// same format as desktop goes to recommended list
	m_lRecommendedVideoModes.clear();
	m_lOtherVideoModes.clear();

	const int DesktopG = gcd(Graphics()->DesktopWidth(), Graphics()->DesktopHeight());
	const int DesktopWidthG = Graphics()->DesktopWidth() / DesktopG;
	const int DesktopHeightG = Graphics()->DesktopHeight() / DesktopG;

	for(int i = 0; i < m_NumModes; i++)
	{
		const int G = gcd(m_aModes[i].m_Width, m_aModes[i].m_Height);
		if(m_aModes[i].m_Width/G == DesktopWidthG &&
		   m_aModes[i].m_Height/G == DesktopHeightG)
		{
			m_lRecommendedVideoModes.add(m_aModes[i]);
		}
		else
		{
			m_lOtherVideoModes.add(m_aModes[i]);
		}
	}
}

void CMenus::UpdateVideoModeSettings()
{
	m_NumModes = Graphics()->GetVideoModes(m_aModes, MAX_RESOLUTIONS, Config()->m_GfxScreen);
	UpdatedFilteredVideoModes();
}

int CMenus::GetInitAmount() const
{
	const int NumMenuImages = 5;
	return 6 + 5 * NumMenuImages;
}

void CMenus::OnInit()
{
	UpdateVideoModeSettings();
	RenderLoading(3);

	m_MousePos.x = Graphics()->ScreenWidth()/2;
	m_MousePos.y = Graphics()->ScreenHeight()/2;

	// load menu images
	m_lMenuImages.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/menuimages", MenuImageScan, this);

	// load filters
	LoadFilters();
	// add standard filters in case they are missing
	InitDefaultFilters();
	RenderLoading(1);

	// load game type icons
	Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/gametypes", GameIconScan, this);
	RenderLoading(1);

	// initial launch preparations
	if(Config()->m_ClShowWelcome)
		m_Popup = POPUP_LANGUAGE;
	Config()->m_ClShowWelcome = 0;
	if(Config()->m_ClSkipStartMenu)
		SetMenuPage(Config()->m_UiBrowserPage);

	Console()->Chain("br_filter_string", ConchainServerbrowserUpdate, this);
	Console()->Chain("add_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("remove_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("br_sort", ConchainServerbrowserSortingUpdate, this);
	Console()->Chain("br_sort_order", ConchainServerbrowserSortingUpdate, this);
	Console()->Chain("connect", ConchainConnect, this);
	Console()->Chain("add_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("snd_enable", ConchainUpdateMusicState, this);
	Console()->Chain("snd_enable_music", ConchainUpdateMusicState, this);

	RenderLoading(1);

	ServerBrowser()->SetType(Config()->m_UiBrowserPage == PAGE_LAN ? IServerBrowser::TYPE_LAN : IServerBrowser::TYPE_INTERNET);
}

void CMenus::PopupMessage(const char *pTitle, const char *pMessage, const char *pButtonLabel, int NextPopup, FPopupButtonCallback pfnButtonCallback)
{
	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aPopupTitle, pTitle, sizeof(m_aPopupTitle));
	str_copy(m_aPopupMessage, pMessage, sizeof(m_aPopupMessage));
	str_copy(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, pButtonLabel, sizeof(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel));
	m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup = NextPopup;
	m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback = pfnButtonCallback;
	m_Popup = POPUP_MESSAGE;
}

void CMenus::PopupConfirm(const char *pTitle, const char *pMessage, const char *pConfirmButtonLabel, const char *pCancelButtonLabel,
	FPopupButtonCallback pfnConfirmButtonCallback, int ConfirmNextPopup, FPopupButtonCallback pfnCancelButtonCallback, int CancelNextPopup)
{
	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aPopupTitle, pTitle, sizeof(m_aPopupTitle));
	str_copy(m_aPopupMessage, pMessage, sizeof(m_aPopupMessage));
	str_copy(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, pConfirmButtonLabel, sizeof(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel));
	m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup = ConfirmNextPopup;
	m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback = pfnConfirmButtonCallback;
	str_copy(m_aPopupButtons[BUTTON_CANCEL].m_aLabel, pCancelButtonLabel, sizeof(m_aPopupButtons[BUTTON_CONFIRM].m_aLabel));
	m_aPopupButtons[BUTTON_CANCEL].m_NextPopup = CancelNextPopup;
	m_aPopupButtons[BUTTON_CANCEL].m_pfnCallback = pfnCancelButtonCallback;
	m_Popup = POPUP_CONFIRM;
}


int CMenus::Render()
{
	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	static int s_InitTick = 5;
	if(s_InitTick > 0)
	{
		s_InitTick--;
		if(s_InitTick == 4)
		{
			m_pClient->m_pCamera->ChangePosition(CCamera::POS_START);
			UpdateMusicState();
		}
		else if(s_InitTick == 0)
		{
			// Wait a few frames before refreshing server browser,
			// else the increased rendering time at startup prevents
			// the network from being pumped effectively.
			ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_INTERNET|IServerBrowser::REFRESHFLAG_LAN);
		}
	}

	// render background only if needed
	if(IsBackgroundNeeded())
		RenderBackground(Client()->LocalTime());

	CUIRect TabBar, MainView;

	// some margin around the screen
	//Screen.Margin(10.0f, &Screen);

	static bool s_SoundCheck = false;
	if(!s_SoundCheck && m_Popup == POPUP_NONE)
	{
		if(Client()->SoundInitFailed())
			PopupMessage(Localize("Sound error"), Localize("The audio device couldn't be initialised."), Localize("Ok"));
		s_SoundCheck = true;
	}

	if(m_Popup == POPUP_NONE)
	{
		if(m_MenuPage == PAGE_START && Client()->State() == IClient::STATE_OFFLINE)
			RenderStartMenu(Screen);
		else
		{
			// do tab bar
			float BarHeight = 60.0f;
			if(Client()->State() == IClient::STATE_ONLINE && m_GamePage == PAGE_SETTINGS)
				BarHeight += 3.0f + 25.0f;
			else if(Client()->State() == IClient::STATE_ONLINE && m_GamePage >= PAGE_INTERNET && m_GamePage <= PAGE_LAN)
				BarHeight += 3.0f + 25.0f;
			float VMargin = Screen.w/2-365.0f;
			if(Config()->m_UiWideview)
				VMargin = min(VMargin, 60.0f);

			Screen.VMargin(VMargin, &MainView);
			MainView.HSplitTop(BarHeight, &TabBar, &MainView);
			RenderMenubar(TabBar);

			// news is not implemented yet
			/*if(m_MenuPage <= PAGE_NEWS || m_MenuPage > PAGE_SETTINGS || (Client()->State() == IClient::STATE_OFFLINE && m_MenuPage >= PAGE_GAME && m_MenuPage <= PAGE_CALLVOTE))
			{
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
				m_MenuPage = PAGE_INTERNET;
			}*/

			{
				// quit button
				CUIRect Button, Row;
				float TopOffset = 27.0f;
				Screen.HSplitTop(TopOffset, &Row, 0);
				Row.VSplitRight(TopOffset/* - 3.0f*/, &Row, &Button);
				static CButtonContainer s_QuitButton;

				// draw red-blending button
				vec4 Color = mix(vec4(0.f, 0.f, 0.f, 0.25f), vec4(1.f/0xff*0xf9, 1.f/0xff*0x2b, 1.f/0xff*0x2b, 0.75f), s_QuitButton.GetFade());
				RenderTools()->DrawUIRect(&Button, Color, CUI::CORNER_BL, 5.0f);

				// draw non-blending X
				CUIRect XText = Button;

				UI()->DoLabel(&XText, "\xE2\x9C\x95", XText.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
				if(UI()->DoButtonLogic(s_QuitButton.GetID(), &Button))
					m_Popup = POPUP_QUIT;

				// settings button
				if(Client()->State() == IClient::STATE_OFFLINE && (m_MenuPage == PAGE_INTERNET || m_MenuPage == PAGE_LAN || m_MenuPage == PAGE_DEMOS))
				{
					Row.VSplitRight(5.0f, &Row, 0);
					Row.VSplitRight(TopOffset, &Row, &Button);
					static CButtonContainer s_SettingsButton;
					if(DoButton_MenuTabTop(&s_SettingsButton, "\xE2\x9A\x99", false, &Button, 1.0f, 1.0f, CUI::CORNER_B))
					{
						m_MenuPageOld = m_MenuPage;
						m_MenuPage = PAGE_SETTINGS;
					}
				}
				Row.VSplitRight(5.0f, &Row, 0);
				Row.VSplitRight(TopOffset, &Row, &Button);
				static CButtonContainer s_WideButton;
				if((m_MenuPage == PAGE_INTERNET || m_MenuPage == PAGE_LAN || m_MenuPage == PAGE_DEMOS) && DoButton_MenuTabTop(&s_WideButton, Config()->m_UiWideview ? "\xe2\x96\xaa" : "\xe2\x96\xac", false, &Button, 1.0f, 1.0f, CUI::CORNER_B))
					Config()->m_UiWideview ^= 1;
			}

			// render current page
			if(Client()->State() != IClient::STATE_OFFLINE)
			{
				if(m_GamePage == PAGE_GAME)
					RenderGame(MainView);
				else if(m_GamePage == PAGE_PLAYERS)
					RenderPlayers(MainView);
				else if(m_GamePage == PAGE_SERVER_INFO)
					RenderServerInfo(MainView);
				else if(m_GamePage == PAGE_CALLVOTE)
					RenderServerControl(MainView);
				else if(m_GamePage == PAGE_SETTINGS)
					RenderSettings(MainView);
				else if(m_GamePage == PAGE_INTERNET)
					RenderServerbrowser(MainView);
				else if(m_GamePage == PAGE_LAN)
					RenderServerbrowser(MainView);
			}
			else
			{
				if(m_MenuPage == PAGE_NEWS)
					RenderNews(MainView);
				else if(m_MenuPage == PAGE_INTERNET)
					RenderServerbrowser(MainView);
				else if(m_MenuPage == PAGE_LAN)
					RenderServerbrowser(MainView);
				else if(m_MenuPage == PAGE_DEMOS)
					RenderDemoList(MainView);
				else if(m_MenuPage == PAGE_SETTINGS)
					RenderSettings(MainView);
			}
		}

		// do overlay popups
		DoPopupMenu();
	}
	else
	{
		// render full screen popup
		char aTitleBuf[128];
		const char *pTitle = "";
		const char *pExtraText = "";
		int NumOptions = 4;

		if(m_Popup == POPUP_MESSAGE || m_Popup == POPUP_CONFIRM)
		{
			pTitle = m_aPopupTitle;
			pExtraText = m_aPopupMessage;
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			pTitle = Localize("Connecting to");
			if(Client()->MapDownloadTotalsize() > 0)
			{
				str_format(aTitleBuf, sizeof(aTitleBuf), "%s: %s", Localize("Downloading map"), Client()->MapDownloadName());
				pTitle = aTitleBuf;
				pExtraText = "";
				NumOptions = 5;
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			pTitle = Localize("Language");
			NumOptions = 7;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			pTitle = Localize("Country");
			NumOptions = 8;
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			pTitle = Localize("Rename demo");
			pExtraText = Localize("Are you sure you want to rename the demo?");
			NumOptions = 6;
		}
		else if(m_Popup == POPUP_SAVE_SKIN)
		{
			pTitle = Localize("Save skin");
			pExtraText = Localize("Are you sure you want to save your skin? If a skin with this name already exists, it will be replaced.");
			NumOptions = 6;
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			pTitle = Localize("Password incorrect");
			pExtraText = "Please enter the password.";
		}
		else if(m_Popup == POPUP_QUIT)
		{
			pTitle = Localize("Quit");
			pExtraText = Localize("Are you sure that you want to quit?");
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			pTitle = Localize("Welcome to Teeworlds");
			pExtraText = Localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server.");
			NumOptions = 6;
		}

		const float ButtonHeight = 20.0f;
		const float FontSize = ButtonHeight*ms_FontmodHeight*0.8f;
		const float SpacingH = 2.0f;
		const float SpacingW = 3.0f;
		CUIRect Box = Screen;
		Box.VMargin(Box.w/2.0f-(365.0f), &Box);
		const float ButtonWidth = (Box.w/6.0f)-(SpacingW*5.0)/6.0f;
		Box.VMargin(ButtonWidth+SpacingW, &Box);
		Box.HMargin(Box.h/2.0f-((int)(NumOptions+1)*ButtonHeight+(int)(NumOptions)*SpacingH)/2.0f-10.0f, &Box);

		// render the box
		RenderTools()->DrawUIRect(&Box, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		// headline and title
		CUIRect Part;
		Box.HSplitTop(ButtonHeight+5.0f, &Part, &Box);
		Part.y += 3.0f;
		UI()->DoLabel(&Part, pTitle, Part.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		// inner box
		CUIRect BottomBar;
		Box.HSplitTop(SpacingH, 0, &Box);
		Box.HSplitBottom(ButtonHeight+5.0f+SpacingH, &Box, &BottomBar);
		BottomBar.HSplitTop(SpacingH, 0, &BottomBar);
		RenderTools()->DrawUIRect(&Box, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		if(m_Popup == POPUP_QUIT)
		{
			// additional info
			if(m_pClient->Editor()->HasUnsavedData())
			{
				Box.HSplitTop(12.0f, 0, &Part);
				UI()->DoLabel(&Part, pExtraText, FontSize, CUI::ALIGN_CENTER);
				Part.HSplitTop(20.0f, 0, &Part);
				Part.VMargin(5.0f, &Part);
				UI()->DoLabel(&Part, Localize("There's an unsaved map in the editor, you might want to save it before you quit the game."), FontSize, CUI::ALIGN_LEFT, Part.w);
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Box);
				UI()->DoLabel(&Box, pExtraText, FontSize, CUI::ALIGN_CENTER);
			}

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
				Client()->Quit();
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			Box.HMargin(4.0f, &Box);

			CUIRect Label;
			Box.HSplitTop(20.0f, &Label, &Box);
			UI()->DoLabel(&Label, pExtraText, FontSize, CUI::ALIGN_CENTER);

			CUIRect EditBox;
			Box.HSplitTop(20.0f, &EditBox, &Box);
			static float s_OffsetPassword = 0.0f;
			DoEditBoxOption(Config()->m_Password, Config()->m_Password, sizeof(Config()->m_Password), &EditBox, Localize("Password"), ButtonWidth, &s_OffsetPassword, true);

			Box.HSplitTop(2.0f, 0, &Box);

			CUIRect Save;
			Box.HSplitTop(20.0f, &Save, &Box);
			CServerInfo ServerInfo = {0};
			str_copy(ServerInfo.m_aHostname, m_aPasswordPopupServerAddress, sizeof(ServerInfo.m_aHostname));
			ServerBrowser()->UpdateFavoriteState(&ServerInfo);
			const bool Favorite = ServerInfo.m_Favorite;
			const int OnValue = Favorite ? 1 : 2;
			const char *pSaveText = Favorite ? Localize("Save password") : Localize("Save password and server as favorite");
			if(DoButton_CheckBox(&Config()->m_ClSaveServerPasswords, pSaveText, Config()->m_ClSaveServerPasswords == OnValue, &Save))
				Config()->m_ClSaveServerPasswords = Config()->m_ClSaveServerPasswords == OnValue ? 0 : OnValue;

			// buttons
			CUIRect TryAgain, Abort;
			BottomBar.VSplitMid(&Abort, &TryAgain, SpacingW);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
			{
				m_Popup = POPUP_NONE;
				m_aPasswordPopupServerAddress[0] = '\0';
			}

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || m_EnterPressed)
			{
				Client()->Connect(ServerInfo.m_aHostname);
				m_aPasswordPopupServerAddress[0] = '\0';
			}
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			static CButtonContainer s_ButtonConnect;
			if(DoButton_Menu(&s_ButtonConnect, Localize("Abort"), 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
			{
				Client()->Disconnect();
				m_Popup = POPUP_NONE;
			}

			if(Client()->MapDownloadTotalsize() > 0)
			{
				char aBuf[128];
				int64 Now = time_get();
				if(Now-m_DownloadLastCheckTime >= time_freq())
				{
					if(m_DownloadLastCheckSize > Client()->MapDownloadAmount())
					{
						// map downloaded restarted
						m_DownloadLastCheckSize = 0;
					}

					// update download speed
					float Diff = (Client()->MapDownloadAmount()-m_DownloadLastCheckSize)/((int)((Now-m_DownloadLastCheckTime)/time_freq()));
					float StartDiff = m_DownloadLastCheckSize-0.0f;
					if(StartDiff+Diff > 0.0f)
						m_DownloadSpeed = (Diff/(StartDiff+Diff))*(Diff/1.0f) + (StartDiff/(Diff+StartDiff))*m_DownloadSpeed;
					else
						m_DownloadSpeed = 0.0f;
					m_DownloadLastCheckTime = Now;
					m_DownloadLastCheckSize = Client()->MapDownloadAmount();
				}

				Box.HSplitTop(15.f, 0, &Box);
				Box.HSplitTop(ButtonHeight, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), "%d/%d KiB (%.1f KiB/s)", Client()->MapDownloadAmount()/1024, Client()->MapDownloadTotalsize()/1024,	m_DownloadSpeed/1024.0f);
				UI()->DoLabel(&Part, aBuf, FontSize, CUI::ALIGN_CENTER);

				// time left
				int SecondsLeft = max(1, m_DownloadSpeed > 0.0f ? static_cast<int>((Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/m_DownloadSpeed) : 1);
				if(SecondsLeft >= 60)
				{
					int MinutesLeft = SecondsLeft / 60;
					str_format(aBuf, sizeof(aBuf), MinutesLeft == 1 ? Localize("%i minute left") : Localize("%i minutes left"), MinutesLeft);
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), SecondsLeft == 1 ? Localize("%i second left") : Localize("%i seconds left"), SecondsLeft);
				}
				Box.HSplitTop(SpacingH, 0, &Box);
				Box.HSplitTop(ButtonHeight, &Part, &Box);
				UI()->DoLabel(&Part, aBuf, FontSize, CUI::ALIGN_CENTER);

				// progress bar
				Box.HSplitTop(SpacingH, 0, &Box);
				Box.HSplitTop(ButtonHeight, &Part, &Box);
				Part.VMargin(40.0f, &Part);
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
				Part.w = max(10.0f, (Part.w*Client()->MapDownloadAmount())/Client()->MapDownloadTotalsize());
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f);
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Box);
				UI()->DoLabel(&Box, Client()->ServerAddress(), FontSize, CUI::ALIGN_CENTER);
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			RenderLanguageSelection(Box, false);

			static CButtonContainer s_ButtonLanguage;
			if(DoButton_Menu(&s_ButtonLanguage, Localize("Ok"), 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_FIRST_LAUNCH;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			// selected filter
			CBrowserFilter *pFilter = GetSelectedBrowserFilter();
			CServerFilterInfo FilterInfo;
			pFilter->GetFilter(&FilterInfo);

			static int s_ActSelection = -2;
			if(s_ActSelection == -2)
				s_ActSelection = FilterInfo.m_Country;
			static CListBox s_ListBox;
			int OldSelected = -1;
			s_ListBox.DoStart(40.0f, m_pClient->m_pCountryFlags->Num(), 12, 1, OldSelected, &Box, false);

			for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
			{
				const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
				if(pEntry->m_Blocked)
					continue;
				if(pEntry->m_CountryCode == s_ActSelection)
					OldSelected = i;

				CListboxItem Item = s_ListBox.DoNextItem(pEntry, OldSelected == i);
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
						TextRender()->TextColor(CUI::ms_HighlightTextColor);
						TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
					}
					UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, CUI::ALIGN_CENTER);
					if(i == OldSelected)
					{
						TextRender()->TextColor(CUI::ms_DefaultTextColor);
						TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
					}
				}
			}

			const int NewSelected = s_ListBox.DoEnd();
			if(OldSelected != NewSelected)
				s_ActSelection = m_pClient->m_pCountryFlags->GetByIndex(NewSelected, true)->m_CountryCode;

			Part.VMargin(120.0f, &Part);

			static CButtonContainer s_ButtonCountry;
			if(DoButton_Menu(&s_ButtonCountry, Localize("Ok"), 0, &BottomBar) || m_EnterPressed)
			{
				FilterInfo.m_Country = s_ActSelection;
				pFilter->SetFilter(&FilterInfo);
				m_Popup = POPUP_NONE;
			}

			if(m_EscapePressed)
			{
				s_ActSelection = FilterInfo.m_Country;
				m_Popup = POPUP_NONE;
			}
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			Box.HSplitTop(27.0f, 0, &Box);
			Box.VMargin(10.0f, &Box);
			UI()->DoLabel(&Box, pExtraText, FontSize, CUI::ALIGN_LEFT);

			CUIRect EditBox;
			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			static float s_OffsetRenameDemo = 0.0f;
			DoEditBoxOption(m_aCurrentDemoFile, m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile), &EditBox, Localize("Name"), ButtonWidth, &s_OffsetRenameDemo);

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), !m_aCurrentDemoFile[0], &Yes) || m_EnterPressed)
			{
				if(m_aCurrentDemoFile[0])
				{
					m_Popup = POPUP_NONE;
					// rename demo
					if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
					{
						char aBufOld[IO_MAX_PATH_LENGTH];
						str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
						int Length = str_length(m_aCurrentDemoFile);
						char aBufNew[IO_MAX_PATH_LENGTH];
						if(Length <= 4 || m_aCurrentDemoFile[Length-5] != '.' || str_comp_nocase(m_aCurrentDemoFile+Length-4, "demo"))
							str_format(aBufNew, sizeof(aBufNew), "%s/%s.demo", m_aCurrentDemoFolder, m_aCurrentDemoFile);
						else
							str_format(aBufNew, sizeof(aBufNew), "%s/%s", m_aCurrentDemoFolder, m_aCurrentDemoFile);
						if(Storage()->RenameFile(aBufOld, aBufNew, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
						{
							DemolistPopulate();
							DemolistOnUpdate(false);
						}
						else
							PopupMessage(Localize("Error"), Localize("Unable to rename the demo"), Localize("Ok"), POPUP_RENAME_DEMO);
					}
				}
			}
		}
		else if(m_Popup == POPUP_SAVE_SKIN)
		{
			Box.HSplitTop(24.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, FontSize, CUI::ALIGN_LEFT, Box.w-20.0f);

			CUIRect EditBox;
			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			static float s_OffsetSaveSkin = 0.0f;
			DoEditBoxOption(m_aSaveSkinName, m_aSaveSkinName, sizeof(m_aSaveSkinName), &EditBox, Localize("Name"), ButtonWidth, &s_OffsetSaveSkin);

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), !m_aSaveSkinName[0], &Yes) || m_EnterPressed)
			{
				if(m_aSaveSkinName[0] && m_aSaveSkinName[0] != 'x' && m_aSaveSkinName[1] != '_')
				{
					m_Popup = POPUP_NONE;
					m_pClient->m_pSkins->SaveSkinfile(m_aSaveSkinName);
					m_aSaveSkinName[0] = 0;
					m_RefreshSkinSelector = true;
				}
			}
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			Box.HSplitTop(20.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, FontSize, CUI::ALIGN_LEFT, Box.w-20.0f);

			CUIRect EditBox;
			Box.HSplitBottom(ButtonHeight*2.0f, 0, &Box);
			Box.HSplitTop(ButtonHeight, &EditBox, &Box);

			static float s_OffsetName = 0.0f;
			DoEditBoxOption(Config()->m_PlayerName, Config()->m_PlayerName, sizeof(Config()->m_PlayerName), &EditBox, Localize("Nickname"), ButtonWidth, &s_OffsetName);

			// button
			static CButtonContainer s_EnterButton;
			if(DoButton_Menu(&s_EnterButton, Localize("Enter"), 0, &BottomBar) || m_EnterPressed)
			{
				if(Config()->m_PlayerName[0])
					m_Popup = POPUP_NONE;
				else
					PopupMessage(Localize("Error"), Localize("Nickname is empty."), Localize("Ok"), POPUP_FIRST_LAUNCH);
			}
		}
		else if(m_Popup == POPUP_MESSAGE || m_Popup == POPUP_CONFIRM)
		{
			// message
			Box.HSplitTop(27.0f, 0, &Box);
			Box.VMargin(5.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, FontSize, CUI::ALIGN_LEFT, Part.w);

			if(m_Popup == POPUP_MESSAGE)
			{
				static CButtonContainer s_ButtonConfirm;
				if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
				{
					m_Popup = m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup;
					(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
				}
			}
			else if(m_Popup == POPUP_CONFIRM)
			{
				CUIRect CancelButton, ConfirmButton;
				BottomBar.VSplitMid(&CancelButton, &ConfirmButton, SpacingW);

				static CButtonContainer s_ButtonCancel;
				if(DoButton_Menu(&s_ButtonCancel, m_aPopupButtons[BUTTON_CANCEL].m_aLabel, 0, &CancelButton) || m_EscapePressed)
				{
					m_Popup = m_aPopupButtons[BUTTON_CANCEL].m_NextPopup;
					(this->*m_aPopupButtons[BUTTON_CANCEL].m_pfnCallback)();
				}

				static CButtonContainer s_ButtonConfirm;
				if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &ConfirmButton) || m_EnterPressed)
				{
					m_Popup = m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup;
					(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
				}
			}
		}

		if(m_Popup == POPUP_NONE)
			UI()->SetActiveItem(0);
	}

	return 0;
}


void CMenus::SetActive(bool Active)
{
	m_MenuActive = Active;
	if(!m_MenuActive)
	{
		if(Client()->State() == IClient::STATE_ONLINE)
		{
			m_pClient->OnRelease();
			if(Client()->State() == IClient::STATE_ONLINE && m_SkinModified)
			{
				m_SkinModified = false;
				m_pClient->SendSkinChange();
			}
		}
	}
	else
	{
		m_SkinModified = false;
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			m_pClient->OnRelease();
		}
	}
}

void CMenus::OnReset()
{
}

bool CMenus::OnCursorMove(float x, float y, int CursorType)
{
	m_LastInput = time_get();

	if(!m_MenuActive)
		return false;

	// prev mouse position
	m_PrevMousePos = m_MousePos;

	UI()->ConvertCursorMove(&x, &y, CursorType);
	m_MousePos.x += x;
	m_MousePos.y += y;
	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();

	return true;
}

bool CMenus::OnInput(IInput::CEvent e)
{
	m_LastInput = time_get();

	// special handle esc and enter for popup purposes
	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_ESCAPE)
		{
			m_EscapePressed = true;
			SetActive(!IsActive());
			return true;
		}
	}

	if(IsActive())
	{
		if(e.m_Flags&IInput::FLAG_PRESS)
		{
			// special for popups
			if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
				m_EnterPressed = true;
			else if(e.m_Key == KEY_TAB && !Input()->KeyIsPressed(KEY_LALT) && !Input()->KeyIsPressed(KEY_RALT))
				m_TabPressed = true;
			else if(e.m_Key == KEY_DELETE)
				m_DeletePressed = true;
			else if(e.m_Key == KEY_UP)
				m_UpArrowPressed = true;
			else if(e.m_Key == KEY_DOWN)
				m_DownArrowPressed = true;
		}
		return true;
	}
	return false;
}

void CMenus::OnConsoleInit()
{
	CUIElementBase::Init(this);
}

void CMenus::OnShutdown()
{
	// save filters
	SaveFilters();
}

void CMenus::OnStateChange(int NewState, int OldState)
{
	// reset active item
	UI()->SetActiveItem(0);

	if(NewState == IClient::STATE_OFFLINE)
	{
		if(OldState >= IClient::STATE_ONLINE && NewState < IClient::STATE_QUITING)
			UpdateMusicState();

		m_Popup = POPUP_NONE;

		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				str_copy(m_aPasswordPopupServerAddress, Client()->ServerAddress(), sizeof(m_aPasswordPopupServerAddress));
				UI()->SetHotItem(&Config()->m_Password);
				UI()->SetActiveItem(&Config()->m_Password);
			}
			else
				PopupMessage(Localize("Disconnected"), Client()->ErrorString(), Localize("Ok"));
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_Popup = POPUP_CONNECTING;
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 0.0f;
	}
	else if(NewState == IClient::STATE_CONNECTING)
		m_Popup = POPUP_CONNECTING;
	else if (NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		m_Popup = POPUP_NONE;
		SetActive(false);
	}
}

void CMenus::OnRender()
{
	UI()->StartCheck();

	// reset cursor
	m_PrevCursorActive = m_CursorActive;
	m_CursorActive = false;

	if(m_KeyReaderIsActive)
	{
		m_KeyReaderIsActive = false;
		m_KeyReaderWasActive = true;
	}
	else if(m_KeyReaderWasActive)
		m_KeyReaderWasActive = false;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		SetActive(true);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
		RenderDemoPlayer(Screen);
	}

	if(Client()->State() == IClient::STATE_ONLINE && m_pClient->m_ServerMode == m_pClient->SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		PopupMessage(Localize("Disconnected"), Localize("The server is running a non-standard tuning on a pure game type."), Localize("Ok"));
	}

	if(!IsActive())
	{
		m_EscapePressed = false;
		m_EnterPressed = false;
		m_TabPressed = false;
		m_DeletePressed = false;
		m_UpArrowPressed = false;
		m_DownArrowPressed = false;
		return;
	}

	// update the ui
	const CUIRect *pScreen = UI()->Screen();
	float MouseX = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float MouseY = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;
	UI()->Update(MouseX, MouseY, MouseX*3.0f, MouseY*3.0f);

	// render
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Render();

	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->WrapClamp();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(MouseX, MouseY, 24, 24);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	Graphics()->WrapNormal();

	// render debug information
	if(Config()->m_Debug)
	{
		Graphics()->MapScreen(pScreen->x, pScreen->y, pScreen->w, pScreen->h);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%p %p %p", UI()->HotItem(), UI()->GetActiveItem(), UI()->LastActiveItem());
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 10, 10, 10, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	UI()->FinishCheck();
	m_EscapePressed = false;
	m_EnterPressed = false;
	m_TabPressed = false;
	m_DeletePressed = false;
	m_UpArrowPressed = false;
	m_DownArrowPressed = false;
}

bool CMenus::CheckHotKey(int Key) const
{
	return !m_KeyReaderIsActive && !m_KeyReaderWasActive && !m_PrevCursorActive && !m_PopupActive
		&& !Input()->KeyIsPressed(KEY_LSHIFT) && !Input()->KeyIsPressed(KEY_RSHIFT)
		&& !Input()->KeyIsPressed(KEY_LCTRL) && !Input()->KeyIsPressed(KEY_RCTRL)
		&& !Input()->KeyIsPressed(KEY_LALT)
		&& UI()->KeyIsPressed(Key);
}

bool CMenus::IsBackgroundNeeded() const
{ 
	return !m_pClient->m_InitComplete || (Client()->State() != IClient::STATE_ONLINE && !m_pClient->m_pMapLayersBackGround->MenuMapLoaded());
}

void CMenus::RenderBackground(float Time)
{
	float ScreenHeight = 300;
	float ScreenWidth = ScreenHeight*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, ScreenWidth, ScreenHeight);

	// render the tiles
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
		const float Size = 15.0f;
		const float OffsetTime = fmod(Time*0.15f, 2.0f);
		for(int y = -2; y < (int)(ScreenWidth/Size); y++)
			for(int x = -2; x < (int)(ScreenHeight/Size); x++)
			{
				Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.045f);
				IGraphics::CQuadItem QuadItem((x-OffsetTime)*Size*2+(y&1)*Size, (y+OffsetTime)*Size, Size, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
	Graphics()->QuadsEnd();

	// render border fade
	static IGraphics::CTextureHandle s_TextureBlob = Graphics()->LoadTexture("ui/blob.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	Graphics()->TextureSet(s_TextureBlob);
	Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(-100, -100, ScreenWidth+200, ScreenHeight+200);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// restore screen
	{CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);}
}

void CMenus::RenderBackgroundShadow(const CUIRect *pRect, bool TopToBottom, float Rounding)
{
	const vec4 Transparent(0.0f, 0.0f, 0.0f, 0.0f);
	const vec4 Background(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f);
	if(TopToBottom)
		RenderTools()->DrawUIRect4(pRect, Background, Background, Transparent, Transparent, CUI::CORNER_T, Rounding);
	else
		RenderTools()->DrawUIRect4(pRect, Transparent, Transparent, Background, Background, CUI::CORNER_B, Rounding);
}

void CMenus::ConchainUpdateMusicState(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CMenus *pSelf = (CMenus *)pUserData;
	if(pResult->NumArguments())
	{
		pSelf->UpdateMusicState();
	}
}

void CMenus::UpdateMusicState()
{
	bool ShouldPlay = Client()->State() == IClient::STATE_OFFLINE && Config()->m_SndEnable && Config()->m_SndMusic;
	if(ShouldPlay && !m_pClient->m_pSounds->IsPlaying(SOUND_MENU))
		m_pClient->m_pSounds->Enqueue(CSounds::CHN_MUSIC, SOUND_MENU);
	else if(!ShouldPlay && m_pClient->m_pSounds->IsPlaying(SOUND_MENU))
		m_pClient->m_pSounds->Stop(SOUND_MENU);
}

void CMenus::SetMenuPage(int NewPage)
{
	if(NewPage == m_MenuPage)
		return;

	m_MenuPage = NewPage;

	// update camera position
	{
		int CameraPos = -1;

		switch(m_MenuPage)
		{
		case PAGE_START: CameraPos = CCamera::POS_START; break;
		case PAGE_DEMOS: CameraPos = CCamera::POS_DEMOS; break;
		case PAGE_SETTINGS: CameraPos = CCamera::POS_SETTINGS_GENERAL+Config()->m_UiSettingsPage; break;
		case PAGE_INTERNET: CameraPos = CCamera::POS_INTERNET; break;
		case PAGE_LAN: CameraPos = CCamera::POS_LAN;
		}

		if(CameraPos != -1 && m_pClient && m_pClient->m_pCamera)
			m_pClient->m_pCamera->ChangePosition(CameraPos);
	}
}
