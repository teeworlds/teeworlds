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
	m_aSaveSkinName[0] = '\0';
	m_SkinNameInput.SetBuffer(m_aSaveSkinName, sizeof(m_aSaveSkinName), MAX_SKIN_LENGTH);
	m_RefreshSkinSelector = true;
	m_pSelectedSkin = 0;
	m_MenuActive = true;
	m_aDemolistPreviousSelection[0] = '\0';
	m_SeekBarActivatedTime = 0;
	m_SeekBarActive = true;
	m_SkinModified = false;
	m_KeyReaderWasActive = false;
	m_KeyReaderIsActive = false;

	SetMenuPage(PAGE_START);
	m_MenuPageOld = PAGE_START;

	m_aDemoLoadingFile[0] = 0;
	m_DemoLoadingPopupRendered = false;

	m_LastInput = time_get();

	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCurrentDemoFile[0] = '\0';
	m_DemoNameInput.SetBuffer(m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile));
	m_aCallvoteReason[0] = 0;
	m_aFilterString[0] = 0;

	m_ActiveListBox = ACTLB_NONE;

	m_PopupCountrySelection = -2;
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
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active && UI()->DoButtonLogic(pID, pRect);
}

bool CMenus::DoButton_Menu(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, vec4 ColorHot, bool TextFade)
{
	const float FadeVal = pBC->GetFade(Checked);

	CUIRect Text = *pRect;
	pRect->Draw(mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), ColorHot, FadeVal), Rounding, Corners);

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
		TextRender()->TextSecondaryColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	}
	UI()->DoLabel(&Text, pText, Text.h*CUI::ms_FontmodHeight, TEXTALIGN_MC);
	if(TextFade)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	}
	return UI()->DoButtonLogic(pBC, pRect);
}

void CMenus::DoButton_KeySelect(CButtonContainer *pBC, const char *pText, const CUIRect *pRect)
{
	const float FadeVal = pBC->GetFade();

	pRect->Draw(vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f));

	CUIRect Label;
	pRect->HMargin(1.0f, &Label);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextSecondaryColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Label, pText, Label.h*CUI::ms_FontmodHeight, TEXTALIGN_MC);
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
}

bool CMenus::DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, float Alpha, float FontAlpha, int Corners, float Rounding, float FontFactor)
{
	const float ActualAlpha = UI()->MouseHovered(pRect) ? 1.0f : Alpha;
	const float FadeVal = pBC->GetFade(Checked)*FontAlpha;

	pRect->Draw(vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, Config()->m_ClMenuAlpha/100.0f*ActualAlpha+FadeVal*0.5f), Rounding, Corners);

	CUIRect Label;
	pRect->HMargin(pRect->h >= 20.0f ? 2.0f : 1.0f, &Label);
	Label.HMargin((Label.h*FontFactor)/2.0f, &Label);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, FontAlpha);
	TextRender()->TextSecondaryColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f*FontAlpha);
	UI()->DoLabel(&Label, pText, Label.h*CUI::ms_FontmodHeight, TEXTALIGN_MC);
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	return UI()->DoButtonLogic(pBC, pRect);
}

bool CMenus::DoButton_GridHeader(const void *pID, const char *pText, bool Checked, int Align, const CUIRect *pRect, int Corners)
{
	if(Checked)
	{
		pRect->Draw(vec4(0.9f, 0.9f, 0.9f, 0.5f), 5.0f, Corners);
		TextRender()->TextColor(CUI::ms_HighlightTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_HighlightTextOutlineColor);
	}
	else if(UI()->HotItem() == pID)
	{
		pRect->Draw(vec4(1.0f, 1.0f, 1.0f, 0.5f), 5.0f, Corners);
	}

	CUIRect Label;
	pRect->VMargin(2.0f, &Label);
	UI()->DoLabel(&Label, pText, Label.h*CUI::ms_FontmodHeight*0.8f, Align|TEXTALIGN_MIDDLE);

	if(Checked)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	}

	return UI()->DoButtonLogic(pID, pRect);
}

bool CMenus::DoButton_CheckBox(const void *pID, const char *pText, bool Checked, const CUIRect *pRect, bool Locked)
{
	if(Locked)
	{
		TextRender()->TextColor(0.5f, 0.5f, 0.5f, 1.0f);
	}

	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Checkbox, Label;
	pRect->VSplitLeft(pRect->h, &Checkbox, &Label);
	Label.VSplitLeft(5.0f, 0, &Label);

	Checkbox.Margin(2.0f, &Checkbox);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	if(Locked)
		Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	else
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	IGraphics::CQuadItem QuadItem(Checkbox.x, Checkbox.y, Checkbox.w, Checkbox.h);
	if(UI()->HotItem() == pID)
	{
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_HOVER);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	RenderTools()->SelectSprite(Checked ? SPRITE_MENU_CHECKBOX_ACTIVE : SPRITE_MENU_CHECKBOX_INACTIVE);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	UI()->DoLabel(&Label, pText, Label.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_ML);

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
		pRect->Draw(vec4(0.0f + FadeVal, 0.0f + FadeVal, 0.0f + FadeVal, 0.25f + FadeVal * 0.5f), Rounding, Corners);
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

	return UI()->DoButtonLogic(pBC, pRect);
}

float CMenus::DoIndependentDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, bool *pActive)
{
	CUIRect View = *pRect;
	CUIRect Header;

	View.HSplitTop(HeaderHeight, &Header, &View);

	// background
	Header.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f, *pActive ? CUIRect::CORNER_T : CUIRect::CORNER_ALL);

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
	UI()->DoLabel(&Header, pStr, Header.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_MC);

	if(UI()->DoButtonLogic(pID, &Header))
		*pActive ^= 1;

	// render content of expanded menu
	if(*pActive)
		return HeaderHeight + (this->*pfnCallback)(View);

	return HeaderHeight;
}

void CMenus::DoInfoBox(const CUIRect *pRect, const char *pLabel, const char *pValue)
{
	pRect->Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	CUIRect Label, Value;
	pRect->VSplitMid(&Label, &Value, 2.0f);

	Value.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

	const float FontSize = pRect->h*CUI::ms_FontmodHeight*0.8f;
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s:", pLabel);
	UI()->DoLabel(&Label, aBuf, FontSize, TEXTALIGN_MC);
	UI()->DoLabel(&Value, pValue, FontSize, TEXTALIGN_MC);
}

void CMenus::DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active)
{
	CUIRect Handle;
	pRect->VSplitLeft(7, &Handle, 0); // Slider size

	Handle.x += (pRect->w-Handle.w)*Current;

	// render
	CUIRect Rail;
	pRect->HMargin(4.0f, &Rail);
	Rail.Draw(vec4(1.0f, 1.0f, 1.0f, Active ? 0.25f : 0.125f), Rail.h/2.0f);

	CUIRect ToleranceArea = Rail;
	ToleranceArea.w *= Tolerance;
	ToleranceArea.x += (Rail.w-ToleranceArea.w)/2.0f;
	vec4 ToleranceColor = Active ? vec4(0.8f, 0.35f, 0.35f, 1.0f) : vec4(0.7f, 0.5f, 0.5f, 1.0f);
	ToleranceArea.Draw(ToleranceColor, ToleranceArea.h/2.0f);

	CUIRect Slider = Handle;
	Slider.HMargin(4.0f, &Slider);
	vec4 SliderColor = Active ? vec4(0.95f, 0.95f, 0.95f, 1.0f) : vec4(0.8f, 0.8f, 0.8f, 1.0f);
	Slider.Draw(SliderColor, Slider.h/2.0f);
}

int CMenus::DoKeyReader(CButtonContainer *pBC, const CUIRect *pRect, int Key, int Modifier, int* pNewModifier)
{
	// process
	static const void *s_pGrabbedID = 0;
	static bool s_MouseReleased = true;
	static int s_ButtonUsed = 0;

	const bool Hovered = UI()->MouseHovered(pRect);
	int NewKey = Key;
	*pNewModifier = Modifier;

	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && s_pGrabbedID == pBC)
		s_MouseReleased = true;

	if(UI()->CheckActiveItem(pBC))
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
			{
				NewKey = m_Binder.m_Key.m_Key;
				*pNewModifier = m_Binder.m_Modifier;
			}
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			s_MouseReleased = false;
			s_pGrabbedID = pBC;
		}

		if(s_ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Hovered)
				NewKey = 0;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pBC)
	{
		if(s_MouseReleased)
		{
			if(UI()->MouseButton(0))
			{
				m_Binder.m_TakeKey = true;
				m_Binder.m_GotKey = false;
				UI()->SetActiveItem(pBC);
				s_ButtonUsed = 0;
			}

			if(UI()->MouseButton(1))
			{
				UI()->SetActiveItem(pBC);
				s_ButtonUsed = 1;
			}
		}
	}

	if(Hovered)
		UI()->SetHotItem(pBC);

	// draw
	if(UI()->CheckActiveItem(pBC) && s_ButtonUsed == 0)
	{
		DoButton_KeySelect(pBC, "???", pRect);
		m_KeyReaderIsActive = true;
	}
	else if(NewKey == 0)
	{
		DoButton_KeySelect(pBC, "", pRect);
	}
	else
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s%s", CBinds::GetModifierName(*pNewModifier), Input()->KeyName(NewKey));
		DoButton_KeySelect(pBC, aBuf, pRect);
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
		int NumButtons = 6;
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/NumButtons)-(Spacing*(NumButtons-1))/NumButtons;
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
		int NumButtons = 5;
		float Spacing = 3.0f;
		float ButtonWidth = (Box.w/NumButtons)-(Spacing*(NumButtons-1))/NumButtons;
		float NotActiveAlpha = Client()->State() == IClient::STATE_ONLINE ? 0.5f : 1.0f;
		int Corners = Client()->State() == IClient::STATE_ONLINE ? CUIRect::CORNER_T : CUIRect::CORNER_ALL;

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


		// TODO: replace tee page to something else
		// Box.VSplitLeft(Spacing, 0, &Box); // little space
		// Box.VSplitLeft(ButtonWidth, &Button, &Box);
		// {
		// 	static CButtonContainer s_TeeButton;
		// 	if(DoButton_MenuTabTop(&s_TeeButton, Localize("TBD"), Client()->State() == IClient::STATE_OFFLINE && Config()->m_UiSettingsPage == SETTINGS_TBD, &Button,
		// 		Config()->m_UiSettingsPage == SETTINGS_TBD ? 1.0f : NotActiveAlpha, 1.0f, Corners))
		// 	{
		// 		m_pClient->m_pCamera->ChangePosition(CCamera::POS_SETTINGS_TBD);
		// 		Config()->m_UiSettingsPage = SETTINGS_TBD;
		// 	}
		// }

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
		int Corners = Client()->State() == IClient::STATE_ONLINE ? CUIRect::CORNER_T : CUIRect::CORNER_ALL;
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
			Box.Draw(vec4(1.0f, 1.0f, 1.0f, 0.75f));
			UI()->DoLabelSelected(&Box, Localize("Demos"), true, Box.h*CUI::ms_FontmodHeight, TEXTALIGN_MC);
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
	Rect.Draw(vec4(0.0f, 0.0f, 0.0f, 0.5f), 40.0f);

	Rect.y += 20;
	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	UI()->DoLabel(&Rect, "Teeworlds", 48.0f, TEXTALIGN_CENTER);

	const float Percent = m_LoadCurrent/(float)m_LoadTotal;
	const float Spacing = 40.0f;
	const float BarRounding = 5.0f;

	CUIRect FullBar = { x+Spacing, y+h-75.0f, w-2*Spacing, 25.0f };
	FullBar.Draw(vec4(1.0f, 1.0f, 1.0f, 0.1f), BarRounding);

	CUIRect FillingBar = FullBar;
	FillingBar.w = (FullBar.w-2*BarRounding)*Percent+2*BarRounding;
	FillingBar.Draw(vec4(1.0f, 1.0f, 1.0f, 0.75f), BarRounding);

	if(Percent > 0.5f)
	{
		TextRender()->TextSecondaryColor(1.0f, 1.0f, 1.0f, 0.7f);
		TextRender()->TextColor(0.2f, 0.2f, 0.2f, 1.0f);
	}
	char aBuf[8];
	str_format(aBuf, sizeof(aBuf), "%d%%", (int)(100*Percent));
	UI()->DoLabel(&FullBar, aBuf, 20.0f, TEXTALIGN_MC);

	if(Percent > 0.5f)
	{
		TextRender()->TextColor(CUI::ms_DefaultTextColor);
		TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
	}

	Graphics()->Swap();
}

void CMenus::RenderNews(CUIRect MainView)
{
	MainView.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f), 10.0f);
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
	if(DoButton_Menu(&s_MenuButton, Localize("Back"), 0, &Button) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
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
	const int Step = Info.GetPixelSize();

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
			m_aModes[i].m_Height/G == DesktopHeightG &&
			m_aModes[i].m_Width <= Graphics()->DesktopWidth() &&
			m_aModes[i].m_Height <= Graphics()->DesktopHeight())
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

void CMenus::PopupCountry(int Selection, FPopupButtonCallback pfnOkButtonCallback)
{
	m_PopupCountrySelection = Selection;
	m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup = POPUP_NONE;
	m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback = pfnOkButtonCallback;
	m_Popup = POPUP_COUNTRY;
}


void CMenus::RenderMenu(CUIRect Screen)
{
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
				VMargin = minimum(VMargin, 60.0f);

			CUIRect TabBar, MainView;
			Screen.VMargin(VMargin, &MainView);
			MainView.HSplitTop(BarHeight, &TabBar, &MainView);
			RenderMenubar(TabBar);

			// render top buttons
			{
				// quit button
				CUIRect Button, Row;
				float TopOffset = 27.0f;
				Screen.HSplitTop(TopOffset, &Row, 0);
				Row.VSplitRight(TopOffset/* - 3.0f*/, &Row, &Button);
				static CButtonContainer s_QuitButton;

				// draw red-blending button
				vec4 Color = mix(vec4(0.f, 0.f, 0.f, 0.25f), vec4(1.f/0xff*0xf9, 1.f/0xff*0x2b, 1.f/0xff*0x2b, 0.75f), s_QuitButton.GetFade());
				Button.Draw(Color, 5.0f, CUIRect::CORNER_BL);

				// draw non-blending X
				UI()->DoLabel(&Button, "\xE2\x9C\x95", Button.h*CUI::ms_FontmodHeight, TEXTALIGN_CENTER);
				if(UI()->DoButtonLogic(&s_QuitButton, &Button))
					m_Popup = POPUP_QUIT;

				// settings button
				if(Client()->State() == IClient::STATE_OFFLINE && (m_MenuPage == PAGE_INTERNET || m_MenuPage == PAGE_LAN || m_MenuPage == PAGE_DEMOS))
				{
					Row.VSplitRight(5.0f, &Row, 0);
					Row.VSplitRight(TopOffset, &Row, &Button);
					static CButtonContainer s_SettingsButton;
					if(DoButton_MenuTabTop(&s_SettingsButton, "\xE2\x9A\x99", false, &Button, 1.0f, 1.0f, CUIRect::CORNER_B))
					{
						m_MenuPageOld = m_MenuPage;
						m_MenuPage = PAGE_SETTINGS;
					}
				}
				Row.VSplitRight(5.0f, &Row, 0);
				Row.VSplitRight(TopOffset, &Row, &Button);
				static CButtonContainer s_WideButton;
				if((m_MenuPage == PAGE_INTERNET || m_MenuPage == PAGE_LAN || m_MenuPage == PAGE_DEMOS) && DoButton_MenuTabTop(&s_WideButton, Config()->m_UiWideview ? "\xe2\x96\xaa" : "\xe2\x96\xac", false, &Button, 1.0f, 1.0f, CUIRect::CORNER_B))
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

		UI()->RenderPopupMenus();
	}
	else
	{
		// render full screen popup
		char aTitleBuf[128];
		const char *pTitle = "";
		int NumOptions = 4;

		if(m_Popup == POPUP_MESSAGE || m_Popup == POPUP_CONFIRM)
		{
			pTitle = m_aPopupTitle;
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			pTitle = Localize("Connecting to");
			if(Client()->MapDownloadTotalsize() > 0)
			{
				str_format(aTitleBuf, sizeof(aTitleBuf), "%s: %s", Localize("Downloading map"), Client()->MapDownloadName());
				pTitle = aTitleBuf;
				NumOptions = 5;
			}
		}
		else if(m_Popup == POPUP_LOADING_DEMO)
		{
			pTitle = Localize("Loading demo");
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			pTitle = Localize("Language");
			NumOptions = 7;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			pTitle = Localize("Flag");
			NumOptions = 8;
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			pTitle = Localize("Rename demo");
			NumOptions = 6;
		}
		else if(m_Popup == POPUP_SAVE_SKIN)
		{
			pTitle = Localize("Save skin");
			NumOptions = 6;
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			pTitle = Localize("Password incorrect");
		}
		else if(m_Popup == POPUP_QUIT)
		{
			pTitle = Localize("Quit");
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			pTitle = Localize("Welcome to Teeworlds");
			NumOptions = 6;
		}

		const float ButtonHeight = 20.0f;
		const float FontSize = ButtonHeight*CUI::ms_FontmodHeight*0.8f;
		const float SpacingH = 2.0f;
		const float SpacingW = 3.0f;
		CUIRect Box = Screen;
		Box.VMargin(Box.w/2.0f-(365.0f), &Box);
		const float ButtonWidth = (Box.w/6.0f)-(SpacingW*5.0)/6.0f;
		Box.VMargin(ButtonWidth+SpacingW, &Box);
		Box.HMargin(Box.h/2.0f-((int)(NumOptions+1)*ButtonHeight+(int)(NumOptions)*SpacingH)/2.0f-10.0f, &Box);

		// render the box
		Box.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		// headline and title
		CUIRect Part;
		Box.HSplitTop(ButtonHeight+5.0f, &Part, &Box);
		Part.y += 3.0f;
		UI()->DoLabel(&Part, pTitle, Part.h*CUI::ms_FontmodHeight*0.8f, TEXTALIGN_CENTER);

		// inner box
		CUIRect BottomBar;
		Box.HSplitTop(SpacingH, 0, &Box);
		Box.HSplitBottom(ButtonHeight+5.0f+SpacingH, &Box, &BottomBar);
		BottomBar.HSplitTop(SpacingH, 0, &BottomBar);
		Box.Draw(vec4(0.0f, 0.0f, 0.0f, 0.25f));

		if(m_Popup == POPUP_QUIT)
		{
			// additional info
			CUIRect Label;
			if(m_pClient->Editor()->HasUnsavedData())
			{
				Box.HSplitTop(12.0f, 0, &Part);
				Part.HSplitTop(20.0f, &Label, &Part);
				Part.VMargin(5.0f, &Part);
				UI()->DoLabel(&Part, Localize("There's an unsaved map in the editor, you might want to save it before you quit the game."), FontSize, TEXTALIGN_ML, Part.w);
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Label);
			}
			UI()->DoLabel(&Label, Localize("Are you sure that you want to quit?"), FontSize, TEXTALIGN_CENTER);

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonAbort;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
				Client()->Quit();
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			Box.HMargin(4.0f, &Box);

			CUIRect Label;
			Box.HSplitTop(20.0f, &Label, &Box);
			UI()->DoLabel(&Label, Localize("Please enter the password."), FontSize, TEXTALIGN_CENTER);

			CUIRect EditBox;
			Box.HSplitTop(20.0f, &EditBox, &Box);
			static CLineInput s_PasswordInput(Config()->m_Password, sizeof(Config()->m_Password));
			s_PasswordInput.SetHidden(true);
			UI()->DoEditBoxOption(&s_PasswordInput, &EditBox, Localize("Password"), ButtonWidth);

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
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
			{
				m_Popup = POPUP_NONE;
				m_aPasswordPopupServerAddress[0] = '\0';
			}

			static CButtonContainer s_ButtonTryAgain;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
			{
				Client()->Connect(ServerInfo.m_aHostname);
				m_aPasswordPopupServerAddress[0] = '\0';
			}
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			static CButtonContainer s_ButtonConnect;
			if(DoButton_Menu(&s_ButtonConnect, Localize("Abort"), 0, &BottomBar) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
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
				UI()->DoLabel(&Part, aBuf, FontSize, TEXTALIGN_CENTER);

				// time left
				int SecondsLeft = maximum(1, m_DownloadSpeed > 0.0f ? static_cast<int>((Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/m_DownloadSpeed) : 1);
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
				UI()->DoLabel(&Part, aBuf, FontSize, TEXTALIGN_CENTER);

				// progress bar
				Box.HSplitTop(SpacingH, 0, &Box);
				Box.HSplitTop(ButtonHeight, &Part, &Box);
				Part.VMargin(40.0f, &Part);
				Part.Draw(vec4(1.0f, 1.0f, 1.0f, 0.25f));
				Part.w = maximum(10.0f, (Part.w*Client()->MapDownloadAmount())/Client()->MapDownloadTotalsize());
				Part.Draw(vec4(1.0f, 1.0f, 1.0f, 0.5f));
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Box);
				UI()->DoLabel(&Box, Client()->ServerAddress(), FontSize, TEXTALIGN_CENTER);
			}
		}
		else if(m_Popup == POPUP_LOADING_DEMO)
		{
			if(m_DemoLoadingPopupRendered)
			{
				m_Popup = POPUP_NONE;
				m_DemoLoadingPopupRendered = false;
				const char *pError = Client()->DemoPlayer_Play(m_aDemoLoadingFile, m_DemoLoadingStorageType);
				if(pError)
					PopupMessage(Localize("Error loading demo"), pError, Localize("Ok"));
				m_aDemoLoadingFile[0] = 0;
			}
			else
			{
				Box.HSplitTop(27.0f, 0, &Box);
				UI()->DoLabel(&Box, m_aDemoLoadingFile, FontSize, TEXTALIGN_CENTER);
				// wait until next frame to load the demo
				m_DemoLoadingPopupRendered = true;
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			RenderLanguageSelection(Box, false);

			static CButtonContainer s_ButtonLanguage;
			if(DoButton_Menu(&s_ButtonLanguage, Localize("Ok"), 0, &BottomBar) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
				m_Popup = POPUP_FIRST_LAUNCH;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			// selected filter
			static CListBox s_ListBox;
			int OldSelected = -1;
			s_ListBox.DoStart(40.0f, m_pClient->m_pCountryFlags->Num(), 12, 1, OldSelected, &Box, false);

			for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
			{
				const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
				if(pEntry->m_Blocked)
					continue;
				if(pEntry->m_CountryCode == m_PopupCountrySelection)
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

					UI()->DoLabelSelected(&Label, pEntry->m_aCountryCodeString, Item.m_Selected, 10.0f, TEXTALIGN_CENTER);
				}
			}

			const int NewSelected = s_ListBox.DoEnd();
			if(OldSelected != NewSelected)
				m_PopupCountrySelection = m_pClient->m_pCountryFlags->GetByIndex(NewSelected, true)->m_CountryCode;

			CUIRect OkButton, CancelButton;
			BottomBar.VSplitMid(&CancelButton, &OkButton, SpacingW);

			static CButtonContainer s_CancelButton;
			if(DoButton_Menu(&s_CancelButton, Localize("Cancel"), 0, &CancelButton) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
			{
				m_PopupCountrySelection = -2;
				m_Popup = POPUP_NONE;
			}

			static CButtonContainer s_OkButton;
			if(DoButton_Menu(&s_OkButton, Localize("Ok"), 0, &OkButton) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER) || s_ListBox.WasItemActivated())
			{
				(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
				m_Popup = POPUP_NONE;
			}
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			Box.HSplitTop(27.0f, 0, &Box);
			Box.VMargin(10.0f, &Box);
			UI()->DoLabel(&Box, Localize("Are you sure you want to rename the demo?"), FontSize, TEXTALIGN_LEFT);

			CUIRect EditBox;
			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			UI()->DoEditBoxOption(&m_DemoNameInput, &EditBox, Localize("Name"), ButtonWidth);

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), !m_aCurrentDemoFile[0], &Yes) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
			{
				if(m_aCurrentDemoFile[0])
				{
					m_Popup = POPUP_NONE;
					// rename demo
					if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
					{
						const char *pExt = ".demo";
						const int ExtLength = str_length(pExt);
						char aBufOld[IO_MAX_PATH_LENGTH];
						str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
						int Length = str_length(m_aCurrentDemoFile);
						if(Length < ExtLength || str_comp(m_aCurrentDemoFile + Length - ExtLength, pExt))
							str_append(m_aCurrentDemoFile, pExt, sizeof(m_aCurrentDemoFile));
						char aPathNew[IO_MAX_PATH_LENGTH];
						str_format(aPathNew, sizeof(aPathNew), "%s/%s", m_aCurrentDemoFolder, m_aCurrentDemoFile);
						if(Storage()->RenameFile(aBufOld, aPathNew, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
						{
							str_copy(m_aDemolistPreviousSelection, m_aCurrentDemoFile, sizeof(m_aDemolistPreviousSelection));
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
			UI()->DoLabel(&Part, Localize("Are you sure you want to save your skin? If a skin with this name already exists, it will be replaced."), FontSize, TEXTALIGN_LEFT, Part.w);

			CUIRect EditBox;
			Box.HSplitBottom(Box.h/2.0f, 0, &Box);
			Box.HSplitTop(20.0f, &EditBox, &Box);

			UI()->DoEditBoxOption(&m_SkinNameInput, &EditBox, Localize("Name"), ButtonWidth);

			// buttons
			CUIRect Yes, No;
			BottomBar.VSplitMid(&No, &Yes, SpacingW);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), !m_aSaveSkinName[0], &Yes) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
			{
				if(m_aSaveSkinName[0])
				{
					if(m_aSaveSkinName[0] != 'x' && m_aSaveSkinName[1] != '_')
					{
						if(m_pClient->m_pSkins->SaveSkinfile(m_aSaveSkinName))
						{
							m_Popup = POPUP_NONE;
							m_RefreshSkinSelector = true;
						}
						else
							PopupMessage(Localize("Error"), Localize("Unable to save the skin"), Localize("Ok"), POPUP_SAVE_SKIN);
					}
					else
						PopupMessage(Localize("Error"), Localize("Unable to save the skin with a reserved name"), Localize("Ok"), POPUP_SAVE_SKIN);
				}
			}
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			Box.HSplitTop(20.0f, 0, &Box);
			Box.VMargin(10.0f, &Part);
			UI()->DoLabel(&Part, Localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server."), FontSize, TEXTALIGN_LEFT, Part.w);

			CUIRect EditBox;
			Box.HSplitBottom(ButtonHeight*2.0f, 0, &Box);
			Box.HSplitTop(ButtonHeight, &EditBox, &Box);

			static CLineInput s_PlayerNameInput(Config()->m_PlayerName, sizeof(Config()->m_PlayerName), MAX_NAME_LENGTH);
			UI()->DoEditBoxOption(&s_PlayerNameInput, &EditBox, Localize("Nickname"), ButtonWidth);

			// button
			static CButtonContainer s_EnterButton;
			if(DoButton_Menu(&s_EnterButton, Localize("Enter"), 0, &BottomBar) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
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
			Box.Margin(5.0f, &Part);
			UI()->DoLabel(&Part, m_aPopupMessage, FontSize, TEXTALIGN_ML, Part.w);

			if(m_Popup == POPUP_MESSAGE)
			{
				static CButtonContainer s_ButtonConfirm;
				if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &BottomBar) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
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
				if(DoButton_Menu(&s_ButtonCancel, m_aPopupButtons[BUTTON_CANCEL].m_aLabel, 0, &CancelButton) || UI()->ConsumeHotkey(CUI::HOTKEY_ESCAPE))
				{
					m_Popup = m_aPopupButtons[BUTTON_CANCEL].m_NextPopup;
					(this->*m_aPopupButtons[BUTTON_CANCEL].m_pfnCallback)();
				}

				static CButtonContainer s_ButtonConfirm;
				if(DoButton_Menu(&s_ButtonConfirm, m_aPopupButtons[BUTTON_CONFIRM].m_aLabel, 0, &ConfirmButton) || UI()->ConsumeHotkey(CUI::HOTKEY_ENTER))
				{
					m_Popup = m_aPopupButtons[BUTTON_CONFIRM].m_NextPopup;
					(this->*m_aPopupButtons[BUTTON_CONFIRM].m_pfnCallback)();
				}
			}
		}

		if(m_Popup == POPUP_NONE)
			UI()->SetActiveItem(0);
	}
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
	if(e.m_Flags&IInput::FLAG_PRESS && e.m_Key == KEY_ESCAPE)
	{
		SetActive(!IsActive());
		UI()->OnInput(e);
		return true;
	}
	if(IsActive())
	{
		UI()->OnInput(e);
		return true;
	}
	return false;
}

void CMenus::OnConsoleInit()
{
	Console()->Register("play", "r[file]", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Play, this, "Play the file specified");
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

	if(m_KeyReaderIsActive)
	{
		m_KeyReaderIsActive = false;
		m_KeyReaderWasActive = true;
	}
	else if(m_KeyReaderWasActive)
		m_KeyReaderWasActive = false;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		SetActive(true);

	if(Client()->State() == IClient::STATE_ONLINE && m_pClient->m_ServerMode == m_pClient->SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		PopupMessage(Localize("Disconnected"), Localize("The server is running a non-standard tuning on a pure game type."), Localize("Ok"));
	}

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK || IsActive())
	{
		// update the ui
		const CUIRect *pScreen = UI()->Screen();
		float MouseX = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
		float MouseY = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;
		UI()->Update(MouseX, MouseY, MouseX*3.0f, MouseY*3.0f);

		// render demo player or main menu
		UI()->MapScreen();
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
			RenderDemoPlayer(*pScreen);
		else
			RenderMenu(*pScreen);

		if(IsActive())
		{
			UI()->RenderTooltip();
			RenderTools()->RenderCursor(MouseX, MouseY, 24.0f);

			// render debug information
			if(Config()->m_Debug)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "%p %p %p", UI()->HotItem(), UI()->GetActiveItem(), UI()->LastActiveItem());
				static CTextCursor s_Cursor(10, 10, 10);
				s_Cursor.Reset();
				TextRender()->TextOutlined(&s_Cursor, aBuf, -1);
			}

			UI()->FinishCheck();
		}
	}

	UI()->ClearHotkeys();
}

bool CMenus::CheckHotKey(int Key) const
{
	return !m_KeyReaderIsActive && !m_KeyReaderWasActive && !UI()->IsInputActive() && !UI()->IsPopupActive()
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
	const float ScreenHeight = 300.0f;
	const float ScreenWidth = ScreenHeight * Graphics()->ScreenAspect();
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

	UI()->MapScreen();
}

void CMenus::RenderBackgroundShadow(const CUIRect *pRect, bool TopToBottom, float Rounding)
{
	const vec4 Transparent(0.0f, 0.0f, 0.0f, 0.0f);
	const vec4 Background(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f);
	if(TopToBottom)
		pRect->Draw4(Background, Background, Transparent, Transparent, Rounding, CUIRect::CORNER_T);
	else
		pRect->Draw4(Transparent, Transparent, Background, Background, Rounding, CUIRect::CORNER_B);
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
