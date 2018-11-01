/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/version.h>
#include <generated/protocol.h>

#include <generated/client_data.h>
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
float CMenus::ms_ListheaderHeight = 20.0f;
float CMenus::ms_FontmodHeight = 0.8f;


CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_NextPopup = POPUP_NONE;
	m_ActivePage = PAGE_INTERNET;
	m_GamePage = PAGE_GAME;

	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_TeePartSelected = SKINPART_BODY;
	m_aSaveSkinName[0] = 0;
	m_RefreshSkinSelector = true;
	m_pSelectedSkin = 0;
	m_MenuActive = true;
	m_SeekBarActivatedTime = 0;
	m_SeekBarActive = true;
	m_UseMouseButtons = true;

	SetMenuPage(PAGE_START);

	m_InfoMode = false;
	m_PopupActive = false;

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;

	m_LastInput = time_get();

	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCallvoteReason[0] = 0;

	m_FriendlistSelectedIndex = -1;

	m_SelectedFilter = 0;

	m_SelectedServer.m_Filter = -1;
	m_SelectedServer.m_Index = -1;
}

float CMenus::ButtonFade(CButtonContainer *pBC, float Seconds, int Checked)
{
	if (UI()->HotItem() == pBC->GetID() || Checked)
	{
		pBC->m_FadeStartTime = Client()->LocalTime();
		return Seconds;
	}
	else if (pBC->m_FadeStartTime + Seconds > Client()->LocalTime())
	{
		return pBC->m_FadeStartTime + Seconds - Client()->LocalTime();
	}
	return 0.0f;
}

int CMenus::DoIcon(int ImageId, int SpriteId, const CUIRect *pRect)
{
	Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);

	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return 0;
}

int CMenus::DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(Checked?SPRITE_GUIBUTTON_ON:SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(UI()->HotItem() == pID && Active)
	{
		RenderTools()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active ? UI()->DoButtonLogic(pID, "", Checked, pRect) : 0;
}

int CMenus::DoButton_Menu(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, int Corners, float r, float FontFactor, vec4 ColorHot, bool TextFade)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds, Checked);
	float FadeVal = Fade/Seconds;
	CUIRect Text = *pRect;

	vec4 Color = mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), ColorHot, FadeVal);
	RenderTools()->DrawUIRect(pRect, Color, Corners, r);

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

			if(Fade > 0.0f)
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

	Text.HMargin(pRect->h>=20.0f?2.0f:1.0f, &Text);
	Text.HMargin((Text.h*FontFactor)/2.0f, &Text);
	if(TextFade)
	{
		TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
		TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	}
	UI()->DoLabel(&Text, pText, Text.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	if(TextFade)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}
	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
}

void CMenus::DoButton_KeySelect(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds, Checked);
	float FadeVal = Fade/Seconds;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, 5.0f);
	CUIRect Temp;
	pRect->HMargin(1.0f, &Temp);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
}

int CMenus::DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, vec4(1.0f, 1.0f, 1.0f, 0.25f), Corners, 10.0f);
	else
	{
		RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, 10.0f);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.7f);
	}

	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, CUI::ALIGN_CENTER);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, int Corners, float r, float FontFactor)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds, Checked);
	float FadeVal = Fade/Seconds;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), Corners, r);
	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	Temp.HMargin((Temp.h*FontFactor)/2.0f, &Temp);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
}

int CMenus::DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
//void CMenus::ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(Checked)
	{
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
		TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
	}

	CUIRect Label;
	pRect->VSplitLeft(5.0f, 0, &Label);
	Label.y+=2.0f;
	UI()->DoLabel(&Label, pText, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	if(Checked)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}

	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_GridHeaderIcon(CButtonContainer *pBC, int ImageID, int SpriteID, const CUIRect *pRect, int Corners)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds);

	RenderTools()->DrawUIRect(pRect, vec4(1.0f, 1.0f, 1.0f, 0.5f+(Fade/Seconds)*0.25f), Corners, 5.0f);

	CUIRect Image;
	pRect->HMargin(2.0f, &Image);
	pRect->VMargin((Image.w-Image.h)/2.0f, &Image);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pBC->GetID(), "", false, pRect);
}

int CMenus::DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect, bool Checked)
//void CMenus::ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);

	c.Margin(2.0f, &c);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, UI()->HotItem() == pID ? 1.0f : 0.6f);
	if(Checked)
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_ACTIVE);
	else
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_INACTIVE);
	IGraphics::CQuadItem QuadItem(c.x, c.y, c.w, c.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	t.y += 2.0f; // lame fix
	UI()->DoLabel(&c, pBoxText, pRect->h*ms_FontmodHeight*0.6f, CUI::ALIGN_CENTER);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int CMenus::DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButton_CheckBox_Common(pID, pText, "", pRect, Checked);
}

int CMenus::DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButton_CheckBox_Common(pID, pText, aBuf, pRect);
}

int CMenus::DoButton_SpriteID(CButtonContainer *pBC, int ImageID, int SpriteID, const CUIRect *pRect, int Corners, float r, bool Fade)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float FadeVal = ButtonFade(pBC, Seconds)/Seconds;

	CUIRect Icon = *pRect;
	Icon.Margin(2.0f, &Icon);

	if(Fade)
		RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), Corners, r);
	else
		RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, r);
	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pBC->GetID(), "", false, pRect);
}

int CMenus::DoButton_SpriteClean(int ImageID, int SpriteID, const CUIRect *pRect)
{
	int Inside = UI()->MouseInside(pRect);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Inside ? 1.0f : 0.6f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	int ReturnValue = 0;
	if(Inside && Input()->KeyPress(KEY_MOUSE_1))
		ReturnValue = 1;

	return ReturnValue;
}

int CMenus::DoButton_SpriteCleanID(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, bool Blend)
{
	int Inside = UI()->MouseInside(pRect);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, !Blend || Inside ? 1.0f : 0.6f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pID, 0, 0, pRect);
}

int CMenus::DoButton_MouseOver(int ImageID, int SpriteID, const CUIRect *pRect)
{
	int Inside = UI()->MouseInside(pRect);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Inside ? 1.0f : 0.6f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return Inside;
}

int CMenus::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden, int Corners)
{
	int Inside = UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;

	FontSize *= UI()->Scale();

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
			float Offset = pRect->w/2.0f-TextRender()->TextWidth(0, FontSize, pStr, -1)/2.0f;

			for(int i = 1; i <= Len; i++)
			{
				if(Offset + TextRender()->TextWidth(0, FontSize, pStr, i) - *pOffset > MxRel)
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

		if(!m_pClient->m_pGameConsole->IsConsoleActive())
		{
			for(int i = 0; i < Input()->NumEvents(); i++)
			{
				Len = str_length(pStr);
				int NumChars = Len;
				ReturnValue |= CLineInput::Manipulate(Input()->GetEvent(i), pStr, StrSize, StrSize, &Len, &s_AtIndex, &NumChars);
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
	RenderTools()->DrawUIRect(&Textbox, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, 5.0f);
	Textbox.VMargin(2.0f, &Textbox);
	Textbox.HMargin(2.0f, &Textbox);

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
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		if(w-*pOffset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1);
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

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, CUI::ALIGN_CENTER);

	// render the cursor
	if(UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1);
		Textbox = *pRect;
		Textbox.x += Textbox.w/2.0f-w/2.0f;
		w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox.x += (w-*pOffset-TextRender()->TextWidth(0, FontSize, "|", -1)/2);

		if((2*time_get()/time_freq()) % 2)	// make it blink
			UI()->DoLabel(&Textbox, "|", FontSize, CUI::ALIGN_LEFT);
	}
	UI()->ClipDisable();

	return ReturnValue;
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

void CMenus::DoScrollbarOption(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, float VSplitVal, int Min, int Max, bool infinite)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, ScrollBar;
	pRect->VSplitLeft(VSplitVal, &Label, &ScrollBar);

	char aBuf[32];
	if(*pOption || !infinite)
		str_format(aBuf, sizeof(aBuf), "%s: %i", pStr, *pOption);
	else
		str_format(aBuf, sizeof(aBuf), "%s: \xe2\x88\x9e", pStr);
	Label.VSplitLeft(Label.h+5.0f, 0, &Label);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, aBuf, pRect->h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);

	ScrollBar.VMargin(4.0f, &ScrollBar);
	*pOption = round_to_int(DoScrollbarH(pOption, &ScrollBar, (float)(*pOption-Min)/(float)(Max-Min))*(float)(Max-Min)+(float)Min+0.1f);
}

float CMenus::DoDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback)
{
	CUIRect View = *pRect;
	CUIRect Header, Label;

	bool Active = pID == m_pActiveDropdown;
	int Corners = Active ? CUI::CORNER_T : CUI::CORNER_ALL;

	View.HSplitTop(HeaderHeight, &Header, &View);

	// background
	RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, 5.0f);

	// render icon
	CUIRect Button;
	Header.VSplitLeft(Header.h, &Button, 0);
	Button.Margin(2.0f, &Button);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, UI()->HotItem() == pID ? 1.0f : 0.6f);
	if(Active)
		RenderTools()->SelectSprite(SPRITE_MENU_EXPANDED);
	else
		RenderTools()->SelectSprite(SPRITE_MENU_COLLAPSED);
	IGraphics::CQuadItem QuadItem(Button.x, Button.y, Button.w, Button.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->QuadsEnd();

	// label
	Label = Header;
	Label.y += 2.0f;
	UI()->DoLabel(&Label, pStr, Header.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	if(UI()->DoButtonLogic(pID, 0, 0, &Header))
	{
		if(Active)
			m_pActiveDropdown = 0;
		else
			m_pActiveDropdown = (int*)pID;
	}

	// render content of expanded menu
	if(Active)
		return HeaderHeight + pfnCallback(View, this);

	return HeaderHeight;
}

void CMenus::DoInfoBox(const CUIRect *pRect, const char *pLabel, const char *pValue)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect Label, Value;
	pRect->VSplitMid(&Label, &Value);

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
	CUIRect Handle;
	static float OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = UI()->MouseY()-OffsetY;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetY = UI()->MouseY()-Handle.y;
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, Rail.w/2.0f);

	CUIRect Slider = Handle;
	Slider.VMargin(5.0f, &Slider);

	RenderTools()->DrawUIRect(&Slider, vec4(1.0f , 1.0f, 1.0f, 1.0f), CUI::CORNER_ALL, Slider.w/2.0f);

	return ReturnValue;
}

float CMenus::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetX;
	pRect->VSplitLeft(33, &Handle, 0);

	Handle.x += (pRect->w-Handle.w)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->CheckActiveItem(pID))
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		float Min = pRect->x;
		float Max = pRect->w-Handle.w;
		float Cur = UI()->MouseX()-OffsetX;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetX = UI()->MouseX()-Handle.x;
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, Rail.h/2.0f);

	CUIRect Slider = Handle;
	Slider.HMargin(5.0f, &Slider);

	RenderTools()->DrawUIRect(&Slider, vec4(1.0f , 1.0f, 1.0f, 1.0f), CUI::CORNER_ALL, Slider.h/2.0f);

	return ReturnValue;
}

void CMenus::UiDoListboxHeader(CListBoxState* pState, const CUIRect *pRect, const char *pTitle,
							   float HeaderHeight, float Spacing)
{
	CUIRect Header;
	CUIRect View = *pRect;

	// background
	View.HSplitTop(ms_ListheaderHeight+Spacing, &Header, 0);
	RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_T, 5.0f);

	// draw header
	View.HSplitTop(ms_ListheaderHeight, &Header, &View);
	Header.y += 2.0f;
	UI()->DoLabel(&Header, pTitle, Header.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

	View.HSplitTop(Spacing, &Header, &View);

	// setup the variables
	pState->m_ListBoxOriginalView = View;
}

void CMenus::UiDoListboxStart(CListBoxState* pState, const void *pID, float RowHeight,
							  const char *pBottomText, int NumItems, int ItemsPerRow, int SelectedIndex,
							  const CUIRect *pRect, bool Background)
{
	CUIRect View, Scroll, Row;
	if(pRect)
		View = *pRect;
	else
		View = pState->m_ListBoxOriginalView;
	CUIRect Footer;

	// background
	if(Background)
		RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	// draw footers
	if(pBottomText)
	{
		View.HSplitBottom(ms_ListheaderHeight, &View, &Footer);
		Footer.VSplitLeft(10.0f, 0, &Footer);
		Footer.y += 2.0f;
		UI()->DoLabel(&Footer, pBottomText, Footer.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	}

	// prepare the scroll
	View.VSplitRight(20.0f, &View, &Scroll);

	// scroll background
	RenderTools()->DrawUIRect(&Scroll, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// list background
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	// setup the variables
	pState->m_ListBoxOriginalView = View;
	pState->m_ListBoxSelectedIndex = SelectedIndex;
	pState->m_ListBoxNewSelected = SelectedIndex;
	pState->m_ListBoxItemIndex = 0;
	pState->m_ListBoxRowHeight = RowHeight;
	pState->m_ListBoxNumItems = NumItems;
	pState->m_ListBoxItemsPerRow = ItemsPerRow;
	pState->m_ListBoxDoneEvents = 0;
	pState->m_ListBoxItemActivated = false;

	// do the scrollbar
	View.HSplitTop(pState->m_ListBoxRowHeight, &Row, 0);

	int NumViewable = (int)(pState->m_ListBoxOriginalView.h/Row.h) + 1;
	int Num = (NumItems+pState->m_ListBoxItemsPerRow-1)/pState->m_ListBoxItemsPerRow-NumViewable+1;
	if(Num < 0)
		Num = 0;
	if(Num > 0)
	{
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&View))
			pState->m_ListBoxScrollValue -= 3.0f/Num;
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&View))
			pState->m_ListBoxScrollValue += 3.0f/Num;

		if(pState->m_ListBoxScrollValue < 0.0f) pState->m_ListBoxScrollValue = 0.0f;
		if(pState->m_ListBoxScrollValue > 1.0f) pState->m_ListBoxScrollValue = 1.0f;
	}

	Scroll.HMargin(5.0f, &Scroll);
	pState->m_ListBoxScrollValue = DoScrollbarV(pID, &Scroll, pState->m_ListBoxScrollValue);

	// the list
	pState->m_ListBoxView = pState->m_ListBoxOriginalView;
	UI()->ClipEnable(&pState->m_ListBoxView);
	pState->m_ListBoxView.y -= pState->m_ListBoxScrollValue*Num*Row.h;
}

CMenus::CListboxItem CMenus::UiDoListboxNextRow(CListBoxState* pState)
{
	static CUIRect s_RowView;
	CListboxItem Item = {0};
	if(pState->m_ListBoxItemIndex%pState->m_ListBoxItemsPerRow == 0)
		pState->m_ListBoxView.HSplitTop(pState->m_ListBoxRowHeight /*-2.0f*/, &s_RowView, &pState->m_ListBoxView);

	s_RowView.VSplitLeft(s_RowView.w/(pState->m_ListBoxItemsPerRow-pState->m_ListBoxItemIndex%pState->m_ListBoxItemsPerRow)/(UI()->Scale()), &Item.m_Rect, &s_RowView);

	Item.m_Visible = 1;
	//item.rect = row;

	Item.m_HitRect = Item.m_Rect;

	//CUIRect select_hit_box = item.rect;

	if(pState->m_ListBoxSelectedIndex == pState->m_ListBoxItemIndex)
		Item.m_Selected = 1;

	// make sure that only those in view can be selected
	if(Item.m_Rect.y+Item.m_Rect.h > pState->m_ListBoxOriginalView.y)
	{

		if(Item.m_HitRect.y < pState->m_ListBoxOriginalView.y) // clip the selection
		{
			Item.m_HitRect.h -= pState->m_ListBoxOriginalView.y-Item.m_HitRect.y;
			Item.m_HitRect.y = pState->m_ListBoxOriginalView.y;
		}

	}
	else
		Item.m_Visible = 0;

	// check if we need to do more
	if(Item.m_Rect.y > pState->m_ListBoxOriginalView.y+pState->m_ListBoxOriginalView.h)
		Item.m_Visible = 0;

	pState->m_ListBoxItemIndex++;
	return Item;
}

CMenus::CListboxItem CMenus::UiDoListboxNextItem(CListBoxState* pState, const void *pId, bool Selected)
{
	int ThisItemIndex = pState->m_ListBoxItemIndex;
	if(Selected)
	{
		if(pState->m_ListBoxSelectedIndex == pState->m_ListBoxNewSelected)
			pState->m_ListBoxNewSelected = ThisItemIndex;
		pState->m_ListBoxSelectedIndex = ThisItemIndex;
	}

	CListboxItem Item = UiDoListboxNextRow(pState);

	if(Item.m_Visible && UI()->DoButtonLogic(pId, "", pState->m_ListBoxSelectedIndex == pState->m_ListBoxItemIndex, &Item.m_HitRect))
		pState->m_ListBoxNewSelected = ThisItemIndex;

	// process input, regard selected index
	if(pState->m_ListBoxSelectedIndex == ThisItemIndex)
	{
		if(!pState->m_ListBoxDoneEvents)
		{
			pState->m_ListBoxDoneEvents = 1;

			if(m_EnterPressed || (UI()->CheckActiveItem(pId) && Input()->MouseDoubleClick()))
			{
				pState->m_ListBoxItemActivated = true;
				UI()->SetActiveItem(0);
			}
			else
			{
				int NewIndex = -1;
				if(Input()->KeyPress(KEY_DOWN)) NewIndex = pState->m_ListBoxNewSelected + 1;
				if(Input()->KeyPress(KEY_UP)) NewIndex = pState->m_ListBoxNewSelected - 1;
				if(NewIndex > -1 && NewIndex < pState->m_ListBoxNumItems)
				{
					// scroll
					float Offset = (NewIndex/pState->m_ListBoxItemsPerRow-pState->m_ListBoxNewSelected/pState->m_ListBoxItemsPerRow)*pState->m_ListBoxRowHeight;
					int Scroll = pState->m_ListBoxOriginalView.y > Item.m_Rect.y+Offset ? -1 :
									pState->m_ListBoxOriginalView.y+pState->m_ListBoxOriginalView.h < Item.m_Rect.y+Item.m_Rect.h+Offset ? 1 : 0;
					if(Scroll)
					{
						int NumViewable = (int)(pState->m_ListBoxOriginalView.h/pState->m_ListBoxRowHeight) + 1;
						int ScrollNum = (pState->m_ListBoxNumItems+pState->m_ListBoxItemsPerRow-1)/pState->m_ListBoxItemsPerRow-NumViewable+1;
						if(Scroll < 0)
						{
							int Num = (pState->m_ListBoxOriginalView.y-Item.m_Rect.y-Offset+pState->m_ListBoxRowHeight-1.0f)/pState->m_ListBoxRowHeight;
							pState->m_ListBoxScrollValue -= (1.0f/ScrollNum)*Num;
						}
						else
						{
							int Num = (Item.m_Rect.y+Item.m_Rect.h+Offset-(pState->m_ListBoxOriginalView.y+pState->m_ListBoxOriginalView.h)+pState->m_ListBoxRowHeight-1.0f)/
								pState->m_ListBoxRowHeight;
							pState->m_ListBoxScrollValue += (1.0f/ScrollNum)*Num;
						}
						if(pState->m_ListBoxScrollValue < 0.0f) pState->m_ListBoxScrollValue = 0.0f;
						if(pState->m_ListBoxScrollValue > 1.0f) pState->m_ListBoxScrollValue = 1.0f;
					}

					pState->m_ListBoxNewSelected = NewIndex;
				}
			}
		}

		//selected_index = i;
		CUIRect r = Item.m_Rect;
		RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
	}

	return Item;
}

int CMenus::UiDoListboxEnd(CListBoxState* pState, bool *pItemActivated)
{
	UI()->ClipDisable();
	if(pItemActivated)
		*pItemActivated = pState->m_ListBoxItemActivated;
	return pState->m_ListBoxNewSelected;
}

int CMenus::DoKeyReader(CButtonContainer *pBC, const CUIRect *pRect, int Key)
{
	// process
	static const void *pGrabbedID = 0;
	static bool MouseReleased = true;
	static int ButtonUsed = 0;
	int Inside = UI()->MouseInside(pRect);
	int NewKey = Key;

	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && pGrabbedID == pBC->GetID())
		MouseReleased = true;

	if(UI()->CheckActiveItem(pBC->GetID()))
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
				NewKey = m_Binder.m_Key.m_Key;
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pBC->GetID();
		}

		if(ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Inside)
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

	if(Inside)
		UI()->SetHotItem(pBC->GetID());

	// draw
	if (UI()->CheckActiveItem(pBC->GetID()) && ButtonUsed == 0)
		DoButton_KeySelect(pBC, "???", 0, pRect);
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pBC, "", 0, pRect);
		else
			DoButton_KeySelect(pBC, Input()->KeyName(Key), 0, pRect);
	}
	return NewKey;
}

void CMenus::RenderMenubar(CUIRect r)
{
	CUIRect Box = r;
	CUIRect Button;

	m_ActivePage = m_MenuPage;
	int NewPage = -1;

	if(Client()->State() != IClient::STATE_OFFLINE)
		m_ActivePage = m_GamePage;

	if((Client()->State() == IClient::STATE_OFFLINE && m_MenuPage == PAGE_SETTINGS) || (Client()->State() == IClient::STATE_ONLINE && m_GamePage == PAGE_SETTINGS))
	{
		if(Client()->State() == IClient::STATE_ONLINE && (g_Config.m_UiSettingsPage == SETTINGS_PLAYER || g_Config.m_UiSettingsPage == SETTINGS_TEE))
		{
			g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
		}
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/6.0f)-(Spacing*5.0)/6.0f;

		// render header background
		RenderTools()->DrawUIRect4(&Box, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

		Box.HSplitBottom(25.0f, 0, &Box);

		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_GeneralButton;
		if(DoButton_MenuTabTop(&s_GeneralButton, Localize("General"), g_Config.m_UiSettingsPage==SETTINGS_GENERAL, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_GENERAL;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_PlayerButton;
		if(Client()->State() != IClient::STATE_ONLINE && DoButton_MenuTabTop(&s_PlayerButton, Localize("Player"), g_Config.m_UiSettingsPage == SETTINGS_PLAYER, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_PLAYER;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_TeeButton;
		if(Client()->State() != IClient::STATE_ONLINE && DoButton_MenuTabTop(&s_TeeButton, Localize("Tee"), g_Config.m_UiSettingsPage == SETTINGS_TEE, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_TEE;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_ControlsButton;
		if(DoButton_MenuTabTop(&s_ControlsButton, Localize("Controls"), g_Config.m_UiSettingsPage==SETTINGS_CONTROLS, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_CONTROLS;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_GraphicsButton;
		if(DoButton_MenuTabTop(&s_GraphicsButton, Localize("Graphics"), g_Config.m_UiSettingsPage==SETTINGS_GRAPHICS, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_GRAPHICS;
		}

		Box.VSplitLeft(Spacing, 0, &Box); // little space
		Box.VSplitLeft(ButtonWidth, &Button, &Box);
		static CButtonContainer s_SoundButton;
		if(DoButton_MenuTabTop(&s_SoundButton, Localize("Sound"), g_Config.m_UiSettingsPage==SETTINGS_SOUND, &Button))
		{
			g_Config.m_UiSettingsPage = SETTINGS_SOUND;
		}
	}
	else if(Client()->State() == IClient::STATE_OFFLINE)
	{
		// render menu tabs
		if(m_MenuPage >= PAGE_INTERNET && m_MenuPage <= PAGE_FRIENDS)
		{
			float Spacing = 3.0f;
			float ButtonWidth = (Box.w/6.0f)-(Spacing*5.0)/6.0f;

			CUIRect Left, Right;
			Box.VSplitLeft(ButtonWidth*2.0f+Spacing, &Left, 0);
			Box.VSplitRight(ButtonWidth, 0, &Right);

			// render header backgrounds
			RenderTools()->DrawUIRect4(&Left, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);
			RenderTools()->DrawUIRect4(&Right, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

			Left.HSplitBottom(25.0f, 0, &Left);
			Right.HSplitBottom(25.0f, 0, &Right);

			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_InternetButton;
			if(DoButton_MenuTabTop(&s_InternetButton, Localize("Global"), m_ActivePage==PAGE_INTERNET, &Button))
			{
				m_pClient->m_pCamera->ChangePosition(CCamera::POS_INTERNET);
				ServerBrowser()->SetType(IServerBrowser::TYPE_INTERNET);
				NewPage = PAGE_INTERNET;
				g_Config.m_UiBrowserPage = PAGE_INTERNET;
			}

			Left.VSplitLeft(Spacing, 0, &Left); // little space
			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_LanButton;
			if(DoButton_MenuTabTop(&s_LanButton, Localize("Local"), m_ActivePage==PAGE_LAN, &Button))
			{
				m_pClient->m_pCamera->ChangePosition(CCamera::POS_LAN);
				ServerBrowser()->SetType(IServerBrowser::TYPE_LAN);
				NewPage = PAGE_LAN;
				g_Config.m_UiBrowserPage = PAGE_LAN;
			}

			char aBuf[32];
			if(m_BorwserPage == PAGE_BROWSER_BROWSER)
				str_copy(aBuf, Localize("Friends"), sizeof(aBuf));
			else if(m_BorwserPage == PAGE_BROWSER_FRIENDS)
				str_copy(aBuf, Localize("Browser"), sizeof(aBuf));
			static CButtonContainer s_FilterButton;
			if(DoButton_Menu(&s_FilterButton, aBuf, 0, &Right))
			{
				m_BorwserPage++;
				if(m_BorwserPage >= NUM_PAGE_BROWSER)
					m_BorwserPage = 0;
			}
		}
		else if(m_MenuPage == PAGE_DEMOS)
		{
			// render header background
			RenderTools()->DrawUIRect4(&Box, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

			Box.HSplitBottom(25.0f, 0, &Box);

			// make the header look like an active tab
			RenderTools()->DrawUIRect(&Box, vec4(1.0f, 1.0f, 1.0f, 0.75f), CUI::CORNER_ALL, 5.0f);
			Box.HMargin(2.0f, &Box);
			TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
			TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
			UI()->DoLabel(&Box, Localize("Demo"), Box.h*ms_FontmodHeight, CUI::ALIGN_CENTER);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
		}
	}
	else
	{
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/6.0f)-(Spacing*5.0)/6.0f;

		// render header backgrounds
		CUIRect Left, Right;
		Box.VSplitLeft(ButtonWidth*4.0f+Spacing*3.0f, &Left, 0);
		Box.VSplitRight(ButtonWidth, 0, &Right);
		RenderTools()->DrawUIRect4(&Left, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);
		RenderTools()->DrawUIRect4(&Right, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

		Left.HSplitBottom(25.0f, 0, &Left);
		Right.HSplitBottom(25.0f, 0, &Right);

		// online menus
		if(m_GamePage != PAGE_SETTINGS) // Game stuff
		{
			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_GameButton;
			if(DoButton_MenuTabTop(&s_GameButton, Localize("Game"), m_ActivePage==PAGE_GAME, &Button))
				NewPage = PAGE_GAME;

			Left.VSplitLeft(Spacing, 0, &Left); // little space
			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_PlayersButton;
			if(DoButton_MenuTabTop(&s_PlayersButton, Localize("Players"), m_ActivePage==PAGE_PLAYERS, &Button))
				NewPage = PAGE_PLAYERS;

			Left.VSplitLeft(Spacing, 0, &Left); // little space
			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_ServerInfoButton;
			if(DoButton_MenuTabTop(&s_ServerInfoButton, Localize("Server info"), m_ActivePage==PAGE_SERVER_INFO, &Button))
				NewPage = PAGE_SERVER_INFO;

			Left.VSplitLeft(Spacing, 0, &Left); // little space
			Left.VSplitLeft(ButtonWidth, &Button, &Left);
			static CButtonContainer s_CallVoteButton;
			if(DoButton_MenuTabTop(&s_CallVoteButton, Localize("Call vote"), m_ActivePage==PAGE_CALLVOTE, &Button))
				NewPage = PAGE_CALLVOTE;

			static CButtonContainer s_SettingsButton;
			if(DoButton_MenuTabTop(&s_SettingsButton, Localize("Settings"), 0, &Right))
				NewPage = PAGE_SETTINGS;
		}
	}

	if(NewPage != -1)
	{
		if(Client()->State() == IClient::STATE_OFFLINE)
			SetMenuPage(NewPage);
		else
			m_GamePage = NewPage;
	}
}

void CMenus::RenderLoading()
{
	// TODO: not supported right now due to separate render thread

	static int64 LastLoadRender = 0;
	float Percent = m_LoadCurrent++/(float)m_LoadTotal;

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	if(time_get()-LastLoadRender < time_freq()/60)
		return;

	LastLoadRender = time_get();

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	RenderBackground();

	float w = 700;
	float h = 200;
	float x = Screen.w/2-w/2;
	float y = Screen.h/2-h/2;
	CUIRect Rect = {x, y, w, h};

	Graphics()->BlendNormal();
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.5f), 40.0f);

	const char *pCaption = Localize("Loading");

	Rect.x = x;
	Rect.y = y+20;
	Rect.w = w;
	Rect.h = h;
	UI()->DoLabel(&Rect, pCaption, 48.0f, CUI::ALIGN_CENTER);

	Rect.x = x+40.0f;
	Rect.y = y+h-75.0f;
	Rect.w = (w-80.0f)*Percent;
	Rect.h = 25.0f;
	RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.75f), 5.0f);

	Graphics()->Swap();
}

void CMenus::RenderNews(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
}

void CMenus::RenderBackButton(CUIRect MainView)
{
	// same size like tabs in top but variables not really needed
	float Spacing = 3.0f;
	float ButtonWidth = (MainView.w/6.0f)-(Spacing*5.0)/6.0f;

	// render background
	MainView.HSplitBottom(60.0f, 0, &MainView);
	MainView.VSplitLeft(ButtonWidth, &MainView, 0);
	RenderTools()->DrawUIRect4(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	// back to main menu
	CUIRect Button;
	MainView.HSplitTop(25.0f, &MainView, 0);
	Button = MainView;
	static CButtonContainer s_MenuButton;
	if(DoButton_Menu(&s_MenuButton, Localize("Back"), 0, &Button) || (m_EscapePressed && Client()->State() != IClient::STATE_ONLINE))
	{
		if(Client()->State() != IClient::STATE_OFFLINE)
			m_GamePage = PAGE_GAME;
		else
			SetMenuPage(PAGE_START);
	}
}

int CMenus::MenuImageScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	char aBuf[512];
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
	str_copy(MenuImage.m_aName, pName, min((int)sizeof(MenuImage.m_aName),l-3));
	str_format(aBuf, sizeof(aBuf), "load menu image %s", MenuImage.m_aName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	pSelf->m_lMenuImages.add(MenuImage);

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

void CMenus::UpdateVideoFormats()
{
	m_NumVideoFormats = 0;
	for(int i = 0; i < m_NumModes; i++)
	{
		int G = gcd(m_aModes[i].m_Width, m_aModes[i].m_Height);
		int Width = m_aModes[i].m_Width/G;
		int Height = m_aModes[i].m_Height/G;

		// check if we already have the format
		bool Found = false;
		for(int j = 0; j < m_NumVideoFormats; j++)
		{
			if(Width == m_aVideoFormats[j].m_WidthValue && Height == m_aVideoFormats[j].m_HeightValue)
			{
				Found = true;
				break;
			}
		}

		if(!Found)
		{
			m_aVideoFormats[m_NumVideoFormats].m_WidthValue = Width;
			m_aVideoFormats[m_NumVideoFormats].m_HeightValue = Height;
			m_NumVideoFormats++;

			// sort the array
			for(int k = 0; k < m_NumVideoFormats-1; k++) // ffs, bubblesort
			{
				for(int j = 0; j < m_NumVideoFormats-k-1; j++)
				{
					if((float)m_aVideoFormats[j].m_WidthValue/(float)m_aVideoFormats[j].m_HeightValue > (float)m_aVideoFormats[j+1].m_WidthValue/(float)m_aVideoFormats[j+1].m_HeightValue)
					{
						CVideoFormat Tmp = m_aVideoFormats[j];
						m_aVideoFormats[j] = m_aVideoFormats[j+1];
						m_aVideoFormats[j+1] = Tmp;
					}
				}
			}
		}
	}
}

void CMenus::UpdatedFilteredVideoModes()
{
	// same format as desktop goes to recommended list
	m_lRecommendedVideoModes.clear();
	m_lOtherVideoModes.clear();

	const int DeskTopG = gcd(Graphics()->DesktopWidth(), Graphics()->DesktopHeight());
	const int DeskTopWidthG = Graphics()->DesktopWidth() / DeskTopG;
	const int DeskTopHeightG = Graphics()->DesktopHeight() / DeskTopG;

	for(int i = 0; i < m_NumModes; i++)
	{
		const int G = gcd(m_aModes[i].m_Width, m_aModes[i].m_Height);
		if(m_aModes[i].m_Width/G == DeskTopWidthG &&
		   m_aModes[i].m_Height/G == DeskTopHeightG)
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
	m_NumModes = Graphics()->GetVideoModes(m_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
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

void CMenus::OnInit()
{
	UpdateVideoModeSettings();

	m_MousePos.x = Graphics()->ScreenWidth()/2;
	m_MousePos.y = Graphics()->ScreenHeight()/2;

	// load menu images
	m_lMenuImages.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "ui/menuimages", MenuImageScan, this);

	// clear filter lists
	//m_lFilters.clear();

	if(g_Config.m_ClShowWelcome)
		m_Popup = POPUP_LANGUAGE;
	g_Config.m_ClShowWelcome = 0;
	if(g_Config.m_ClSkipStartMenu)
		SetMenuPage(g_Config.m_UiBrowserPage);

	Console()->Chain("add_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("remove_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("add_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("snd_enable_music", ConchainToggleMusic, this);

	m_TextureBlob = Graphics()->LoadTexture("ui/blob.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

	// setup load amount
	m_LoadCurrent = 0;
	m_LoadTotal = g_pData->m_NumImages;
	if(!g_Config.m_ClThreadsoundloading)
		m_LoadTotal += g_pData->m_NumSounds;
}

void CMenus::PopupMessage(const char *pTopic, const char *pBody, const char *pButton, int Next)
{
	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aMessageTopic, pTopic, sizeof(m_aMessageTopic));
	str_copy(m_aMessageBody, pBody, sizeof(m_aMessageBody));
	str_copy(m_aMessageButton, pButton, sizeof(m_aMessageButton));
	m_Popup = POPUP_MESSAGE;
	m_NextPopup = Next;
}


int CMenus::Render()
{
	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	static bool s_First = true;
	if(s_First)
	{
		// refresh server browser before we are in browser menu to save time
		m_pClient->m_pCamera->ChangePosition(CCamera::POS_START);
		ServerBrowser()->Refresh(IServerBrowser::REFRESHFLAG_INTERNET|IServerBrowser::REFRESHFLAG_LAN);
		ServerBrowser()->SetType(g_Config.m_UiBrowserPage == PAGE_LAN ? IServerBrowser::TYPE_LAN : IServerBrowser::TYPE_INTERNET);

		m_pClient->m_pSounds->Enqueue(CSounds::CHN_MUSIC, SOUND_MENU);
		s_First = false;
	}

	// render background only if needed
	if(Client()->State() != IClient::STATE_ONLINE && !m_pClient->m_pMapLayersBackGround->MenuMapLoaded())
		RenderBackground();

	CUIRect TabBar, MainView;

	// some margin around the screen
	//Screen.Margin(10.0f, &Screen);

	static bool s_SoundCheck = false;
	if(!s_SoundCheck && m_Popup == POPUP_NONE)
	{
		if(Client()->SoundInitFailed())
			m_Popup = POPUP_SOUNDERROR;
		s_SoundCheck = true;
	}

	if(m_Popup == POPUP_NONE)
	{
		if(m_MenuPage == PAGE_START && Client()->State() == IClient::STATE_OFFLINE)
			RenderStartMenu(Screen);
		else
		{
			// do tab bar
			Screen.VMargin(Screen.w/2-365.0f, &MainView);
			MainView.HSplitTop(60.0f, &TabBar, &MainView);
			RenderMenubar(TabBar);

			// news is not implemented yet
			/*if(m_MenuPage <= PAGE_NEWS || m_MenuPage > PAGE_SETTINGS || (Client()->State() == IClient::STATE_OFFLINE && m_MenuPage >= PAGE_GAME && m_MenuPage <= PAGE_CALLVOTE))
			{
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
				m_MenuPage = PAGE_INTERNET;
			}*/
			// quit button
			CUIRect Button;
			float TopOffset = 35.0f;
			Screen.HSplitTop(TopOffset, &Button, 0);
			Button.VSplitRight(TopOffset - 3.0f, 0, &Button);
			static CButtonContainer s_QuitButton;
			if(DoButton_Menu(&s_QuitButton, "X", 0, &Button, 0, CUI::CORNER_BL))
				m_Popup = POPUP_QUIT;

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
				else if(m_MenuPage == PAGE_FRIENDS)
					RenderServerbrowser(MainView);
				else if(m_MenuPage == PAGE_SETTINGS)
					RenderSettings(MainView);
			}
		}

		// do overlay popups
		DoPopupMenu();
	}
	else
	{
		// make sure that other windows doesn't do anything funnay!
		//UI()->SetHotItem(0);
		//UI()->SetActiveItem(0);
		char aBuf[128];
		const char *pTitle = "";
		const char *pExtraText = "";
		const char *pButtonText = "";
		CUI::EAlignment ExtraAlign = CUI::ALIGN_CENTER;
		int NumOptions = 4;

		if(m_Popup == POPUP_MESSAGE)
		{
			pTitle = m_aMessageTopic;
			pExtraText = m_aMessageBody;
			pButtonText = m_aMessageButton;
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			pTitle = Localize("Connecting to");
			pButtonText = Localize("Abort");
			if(Client()->MapDownloadTotalsize() > 0)
			{
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Downloading map"), Client()->MapDownloadName());
				pTitle = aBuf;
				pExtraText = "";
				NumOptions = 5;
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			pTitle = Localize("Language");
			pButtonText = Localize("Ok");
			NumOptions = 7;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			pTitle = Localize("Country");
			pButtonText = Localize("Ok");
			NumOptions = 8;
		}
		else if(m_Popup == POPUP_DISCONNECTED)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Client()->ErrorString();
			pButtonText = Localize("Ok");
		}
		else if(m_Popup == POPUP_PURE)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Localize("The server is running a non-standard tuning on a pure game type.");
			pButtonText = Localize("Ok");
			ExtraAlign = CUI::ALIGN_LEFT;
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			pTitle = Localize("Delete demo");
			pExtraText = Localize("Are you sure that you want to delete the demo?");
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			pTitle = Localize("Rename demo");
			pExtraText = Localize("Are you sure you want to rename the demo?");
			NumOptions = 6;
		}
		else if(m_Popup == POPUP_REMOVE_FRIEND)
		{
			pTitle = Localize("Remove friend");
			pExtraText = Localize("Are you sure that you want to remove the player from your friends list?");
		}
		else if(m_Popup == POPUP_SAVE_SKIN)
		{
			pTitle = Localize("Save skin");
			pExtraText = Localize("Are you sure you want to save your skin? If a skin with this name already exists, it will be replaced.");
			NumOptions = 6;
			ExtraAlign = CUI::ALIGN_LEFT;
		}
		else if(m_Popup == POPUP_DELETE_SKIN)
		{
			pTitle = Localize("Delete skin");
			pExtraText = Localize("Are you sure that you want to delete the skin?");
		}
		else if(m_Popup == POPUP_SOUNDERROR)
		{
			pTitle = Localize("Sound error");
			pExtraText = Localize("The audio device couldn't be initialised.");
			pButtonText = Localize("Ok");
			ExtraAlign = CUI::ALIGN_LEFT;
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
			pButtonText = Localize("Enter");
			NumOptions = 6;
			ExtraAlign = CUI::ALIGN_LEFT;
		}

		CUIRect Box, Part, BottomBar;
		float ButtonHeight = 20.0f;
		float ButtonHeightBig = ButtonHeight+5.0f;
		float SpacingH = 2.0f;
		float SpacingW = 3.0f;
		Box = Screen;
		Box.VMargin(Box.w/2.0f-(365.0f), &Box);
		float ButtonWidth = (Box.w/6.0f)-(SpacingW*5.0)/6.0f;
		Box.VMargin(ButtonWidth+SpacingW, &Box);
		Box.HMargin(Box.h/2.0f-((int)(NumOptions+1)*ButtonHeight+(int)(NumOptions)*SpacingH)/2.0f-10.0f, &Box);

		// render the box
		RenderTools()->DrawUIRect(&Box, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		// headline
		Box.HSplitTop(ButtonHeightBig, &Part, &Box);
		Part.y += 3.0f;
		UI()->DoLabel(&Part, pTitle, Part.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

		// inner box
		Box.HSplitTop(SpacingH, 0, &Box);
		Box.HSplitBottom(ButtonHeight+5.0f+SpacingH, &Box, &BottomBar);
		BottomBar.HSplitTop(SpacingH, 0, &BottomBar);
		RenderTools()->DrawUIRect(&Box, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		if(m_Popup == POPUP_QUIT)
		{
			CUIRect Yes, No;

			// additional info
			if(m_pClient->Editor()->HasUnsavedData())
			{
				Box.HSplitTop(12.0f, 0, &Part);
				UI()->DoLabel(&Part, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);
				Part.HSplitTop(20.0f, 0, &Part);
				Part.VMargin(5.0f, &Part);
				UI()->DoLabel(&Part, Localize("There's an unsaved map in the editor, you might want to save it before you quit the game."), ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT, Part.w);
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Box);
				UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);
			}

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
				Client()->Quit();
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			CUIRect EditBox, TryAgain, Abort;

			Box.HSplitTop(12.0f, 0, &Box);
			UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);

			Box.HSplitBottom(ButtonHeight*1.7f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			static float s_OffsetPassword = 0.0f;
			DoEditBoxOption(g_Config.m_Password, g_Config.m_Password, sizeof(g_Config.m_Password), &EditBox, Localize("Password"), ButtonWidth, &s_OffsetPassword, true);

			// buttons
			BottomBar.VSplitMid(&Abort, &TryAgain);
			Abort.VSplitRight(SpacingW/2.0f, &Abort, 0);
			TryAgain.VSplitLeft(SpacingW/2.0f, 0, &TryAgain);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || m_EnterPressed)
			{
				Client()->Connect(g_Config.m_UiServerAddress);
			}
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			static CButtonContainer s_ButtonConnect;
			if(DoButton_Menu(&s_ButtonConnect, pButtonText, 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
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
				UI()->DoLabel(&Part, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

				// time left
				const char *pTimeLeftString;
				int TimeLeft = max(1, m_DownloadSpeed > 0.0f ? static_cast<int>((Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/m_DownloadSpeed) : 1);
				if(TimeLeft >= 60)
				{
					TimeLeft /= 60;
					pTimeLeftString = TimeLeft == 1 ? Localize("%i minute left") : Localize("%i minutes left");
				}
				else
					pTimeLeftString = TimeLeft == 1 ? Localize("%i second left") : Localize("%i seconds left");
				Box.HSplitTop(SpacingH, 0, &Box);
				Box.HSplitTop(ButtonHeight, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), pTimeLeftString, TimeLeft);
				UI()->DoLabel(&Part, aBuf, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);

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
				UI()->DoLabel(&Box, g_Config.m_UiServerAddress, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			RenderLanguageSelection(Box, false);

			static CButtonContainer s_ButtonLanguage;
			if(DoButton_Menu(&s_ButtonLanguage, pButtonText, 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_FIRST_LAUNCH;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			// selected filter
			CBrowserFilter *pFilter = &m_lFilters[m_SelectedFilter];
			CServerFilterInfo FilterInfo;
			pFilter->GetFilter(&FilterInfo);

			static int ActSelection = -2;
			if(ActSelection == -2)
				ActSelection = FilterInfo.m_Country;
			static CListBoxState s_ListBoxState;
			int OldSelected = -1;
			UiDoListboxStart(&s_ListBoxState, &s_ListBoxState, 40.0f, 0, m_pClient->m_pCountryFlags->Num(), 12, OldSelected, &Box, false);

			for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
			{
				const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
				if(pEntry->m_CountryCode == ActSelection)
					OldSelected = i;

				CListboxItem Item = UiDoListboxNextItem(&s_ListBoxState, &pEntry->m_CountryCode, OldSelected == i);
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
						UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, CUI::ALIGN_CENTER);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
						TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
					}
					else
						UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, CUI::ALIGN_CENTER);
				}
			}

			const int NewSelected = UiDoListboxEnd(&s_ListBoxState, 0);
			if(OldSelected != NewSelected)
				ActSelection = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;

			Part.VMargin(120.0f, &Part);

			static CButtonContainer s_ButtonCountry;
			if(DoButton_Menu(&s_ButtonCountry, pButtonText, 0, &BottomBar) || m_EnterPressed)
			{
				FilterInfo.m_Country = ActSelection;
				pFilter->SetFilter(&FilterInfo);
				m_Popup = POPUP_NONE;
			}

			if(m_EscapePressed)
			{
				ActSelection = FilterInfo.m_Country;
				m_Popup = POPUP_NONE;
			}
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			CUIRect Yes, No;
			Box.HSplitTop(27.0f, 0, &Box);
			UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// delete demo
				if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
				{
					char aBuf[512];
					str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					if(Storage()->RemoveFile(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
					{
						DemolistPopulate();
						DemolistOnUpdate(false);
					}
					else
						PopupMessage(Localize("Error"), Localize("Unable to delete the demo"), Localize("Ok"));
				}
			}
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			CUIRect Yes, No, EditBox;

			Box.HSplitTop(27.0f, 0, &Box);
			UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);

			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			static float s_OffsetRenameDemo = 0.0f;
			DoEditBoxOption(m_aCurrentDemoFile, m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile), &EditBox, Localize("Name"), ButtonWidth, &s_OffsetRenameDemo);

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

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
						char aBufOld[512];
						str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
						int Length = str_length(m_aCurrentDemoFile);
						char aBufNew[512];
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
		else if(m_Popup == POPUP_REMOVE_FRIEND)
		{
			CUIRect Yes, No;
			Box.HSplitTop(27.0f, 0, &Box);
			UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// remove friend
				if(m_FriendlistSelectedIndex >= 0)
				{
					m_pClient->Friends()->RemoveFriend(m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName,
						m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aClan);
					FriendlistOnUpdate();
					Client()->ServerBrowserUpdate();
				}
			}
		}
		else if(m_Popup == POPUP_SAVE_SKIN)
		{
			CUIRect Yes, No, EditBox;

			Box.HSplitTop(24.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign, Box.w-20.0f);

			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			static float s_OffsetSaveSkin = 0.0f;
			DoEditBoxOption(m_aSaveSkinName, m_aSaveSkinName, sizeof(m_aSaveSkinName), &EditBox, Localize("Name"), ButtonWidth, &s_OffsetSaveSkin);

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), !m_aSaveSkinName[0], &Yes) || m_EnterPressed)
			{
				if(m_aSaveSkinName[0] && m_aSaveSkinName[0] != 'x' && m_aSaveSkinName[1] != '_')
				{
					m_Popup = POPUP_NONE;
					SaveSkinfile();
					m_aSaveSkinName[0] = 0;
					m_RefreshSkinSelector = true;
				}
			}
		}
		else if(m_Popup == POPUP_DELETE_SKIN)
		{
			CUIRect Yes, No;
			Box.HSplitTop(27.0f, 0, &Box);
			UI()->DoLabel(&Box, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign);

			// buttons
			BottomBar.VSplitMid(&No, &Yes);
			No.VSplitRight(SpacingW/2.0f, &No, 0);
			Yes.VSplitLeft(SpacingW/2.0f, 0, &Yes);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// delete demo
				if(m_pSelectedSkin)
				{
					char aBuf[512];
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
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			CUIRect EditBox;

			Box.HSplitTop(20.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign, Box.w-20.0f);

			Box.HSplitBottom(ButtonHeight*2.0f, 0, &Box);
			Box.HSplitTop(ButtonHeight, &EditBox, &Box);

			static float s_OffsetName = 0.0f;
			DoEditBoxOption(g_Config.m_PlayerName, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), &EditBox, Localize("Nickname"), ButtonWidth, &s_OffsetName);

			// button
			static CButtonContainer s_EnterButton;
			if(DoButton_Menu(&s_EnterButton, pButtonText, 0, &BottomBar) || m_EnterPressed)
			{
				if(g_Config.m_PlayerName[0])
					m_Popup = POPUP_NONE;
				else
					PopupMessage(Localize("Error"), Localize("Nickname is empty."), Localize("Ok"), POPUP_FIRST_LAUNCH);
			}
		}
		else
		{
			Box.HSplitTop(27.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, pExtraText, ButtonHeight*ms_FontmodHeight*0.8f, ExtraAlign, -1);

			// button
			static CButtonContainer s_Button;
			if(DoButton_Menu(&s_Button, pButtonText, 0, &BottomBar) || m_EscapePressed || m_EnterPressed)
				m_Popup = m_NextPopup;
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
		}
	}
	else if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->OnRelease();
	}
}

void CMenus::OnReset()
{
}

bool CMenus::OnMouseMove(float x, float y)
{
	m_LastInput = time_get();

	if(!m_MenuActive)
		return false;

	// prev mouse position
	m_PrevMousePos = m_MousePos;

	UI()->ConvertMouseMove(&x, &y);
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
			else if(e.m_Key == KEY_DELETE)
				m_DeletePressed = true;
		}
		return true;
	}
	return false;
}

void CMenus::OnConsoleInit()
{
	// load filters
	LoadFilters();

	// add standard filters in case they are missing
	bool FilterStandard = false;
	bool FilterFav = false;
	bool FilterAll = false;
	for(int i = 0; i < m_lFilters.size(); i++)
	{
		switch(m_lFilters[i].Custom())
		{
		case CBrowserFilter::FILTER_STANDARD:
			FilterStandard = true;
			break;
		case CBrowserFilter::FILTER_FAVORITES:
			FilterFav = true;
			break;
		case CBrowserFilter::FILTER_ALL:
			FilterAll = true;
		}
	}
	if(!FilterStandard)
	{
		// put it on top
		int Pos = m_lFilters.size();
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_STANDARD, "Teeworlds", ServerBrowser(), IServerBrowser::FILTER_COMPAT_VERSION | IServerBrowser::FILTER_PURE | IServerBrowser::FILTER_PURE_MAP | IServerBrowser::FILTER_PING, 999, -1, "", ""));
		for(; Pos > 0; --Pos)
			Move(true, Pos);
	}
	if(!FilterFav)
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_FAVORITES, Localize("Favorites"), ServerBrowser(), IServerBrowser::FILTER_FAVORITE | IServerBrowser::FILTER_PING, 999, -1, "", ""));
	if(!FilterAll)
		m_lFilters.add(CBrowserFilter(CBrowserFilter::FILTER_ALL, Localize("All"), ServerBrowser(), IServerBrowser::FILTER_PING, 999, -1, "", ""));
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
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		m_Popup = POPUP_NONE;
		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				UI()->SetHotItem(&g_Config.m_Password);
				UI()->SetActiveItem(&g_Config.m_Password);
			}
			else
				m_Popup = POPUP_DISCONNECTED;
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_Popup = POPUP_CONNECTING;
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 0.0f;
		//client_serverinfo_request();
	}
	else if(NewState == IClient::STATE_CONNECTING)
		m_Popup = POPUP_CONNECTING;
	else if (NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		m_Popup = POPUP_NONE;
		SetActive(false);
	}
}

extern "C" void font_debug_render();

void CMenus::OnRender()
{
	UI()->StartCheck();

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
		m_Popup = POPUP_PURE;
	}

	if(!IsActive())
	{
		m_EscapePressed = false;
		m_EnterPressed = false;
		m_DeletePressed = false;
		return;
	}

	// update the ui
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;

	int Buttons = 0;
	if(m_UseMouseButtons)
	{
		if(Input()->KeyIsPressed(KEY_MOUSE_1)) Buttons |= 1;
		if(Input()->KeyIsPressed(KEY_MOUSE_2)) Buttons |= 2;
		if(Input()->KeyIsPressed(KEY_MOUSE_3)) Buttons |= 4;
	}

	UI()->Update(mx,my,mx*3.0f,my*3.0f,Buttons);

	// render
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Render();

	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(mx, my, 24, 24);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render debug information
	if(g_Config.m_Debug)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%p %p %p", UI()->HotItem(), UI()->GetActiveItem(), UI()->LastActiveItem());
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 10, 10, 10, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	UI()->FinishCheck();
	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
}

void CMenus::RenderBackground()
{
	//Graphics()->Clear(1,1,1);
	//render_sunrays(0,0);

	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	// render background color
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
		//vec4 bottom(gui_color.r*0.3f, gui_color.g*0.3f, gui_color.b*0.3f, 1.0f);
		//vec4 bottom(0, 0, 0, 1.0f);
		vec4 Bottom(0.45f, 0.45f, 0.45f, 1.0f);
		vec4 Top(0.45f, 0.45f, 0.45f, 1.0f);
		IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(1, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(2, Bottom.r, Bottom.g, Bottom.b, Bottom.a),
			IGraphics::CColorVertex(3, Bottom.r, Bottom.g, Bottom.b, Bottom.a)};
		Graphics()->SetColorVertex(Array, 4);
		IGraphics::CQuadItem QuadItem(0, 0, sw, sh);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render the tiles
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
		float Size = 15.0f;
		float OffsetTime = fmod(Client()->LocalTime()*0.15f, 2.0f);
		for(int y = -2; y < (int)(sw/Size); y++)
			for(int x = -2; x < (int)(sh/Size); x++)
			{
				Graphics()->SetColor(0,0,0,0.045f);
				IGraphics::CQuadItem QuadItem((x-OffsetTime)*Size*2+(y&1)*Size, (y+OffsetTime)*Size, Size, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(m_TextureBlob);
	Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		QuadItem = IGraphics::CQuadItem(-100, -100, sw+200, sh+200);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// restore screen
	{CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);}
}

void CMenus::ConchainToggleMusic(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CMenus *pSelf = (CMenus *)pUserData;
	if(pResult->NumArguments())
	{
		pSelf->ToggleMusic();
	}
}

void CMenus::ToggleMusic()
{
	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		if(g_Config.m_SndMusic && !m_pClient->m_pSounds->IsPlaying(SOUND_MENU))
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		else if(!g_Config.m_SndMusic && m_pClient->m_pSounds->IsPlaying(SOUND_MENU))
			m_pClient->m_pSounds->Stop(SOUND_MENU);
	}
}

void CMenus::SetMenuPage(int NewPage) {
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
		case PAGE_SETTINGS: CameraPos = CCamera::POS_SETTINGS; break;
		case PAGE_INTERNET:
		case PAGE_LAN: CameraPos = CCamera::POS_INTERNET;
		}

		if(CameraPos != -1 && m_pClient && m_pClient->m_pCamera)
			m_pClient->m_pCamera->ChangePosition(CameraPos);
	}
}
