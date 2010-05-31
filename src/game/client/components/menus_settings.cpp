
#include <base/math.h>


#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "binds.h"
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
		if(Event.m_Flags&IInput::FLAG_PRESS && Event.m_Key != KEY_ESCAPE)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;
		}
		return true;
	}

	return false;
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CUIRect Button;
	CUIRect LeftView, RightView;

	MainView.VSplitLeft(MainView.w/2, &LeftView, &RightView);
    LeftView.HSplitTop(20.0f, &Button, &LeftView);

	// render settings
	{
		char aBuf[128];

		LeftView.HSplitTop(20.0f, &Button, &LeftView);
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
		UI()->DoLabel(&Button, aBuf, 14.0, -1);
		Button.VSplitLeft(80.0f, 0, &Button);
		Button.VSplitLeft(180.0f, &Button, 0);
		static float Offset = 0.0f;
		if(DoEditBox(g_Config.m_PlayerName, &Button, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 14.0f, &Offset))
			m_NeedSendinfo = true;

		// extra spacing
		LeftView.HSplitTop(10.0f, 0, &LeftView);

		static int s_DynamicCameraButton = 0;
		LeftView.HSplitTop(20.0f, &Button, &LeftView);
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

		LeftView.HSplitTop(20.0f, &Button, &LeftView);
		if(DoButton_CheckBox(&g_Config.m_ClAutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		LeftView.HSplitTop(20.0f, &Button, &LeftView);
		if(DoButton_CheckBox(&g_Config.m_ClNameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
			g_Config.m_ClNameplates ^= 1;

		//if(config.cl_nameplates)
		{
			LeftView.HSplitTop(20.0f, &Button, &LeftView);
			Button.VSplitLeft(15.0f, 0, &Button);
			if(DoButton_CheckBox(&g_Config.m_ClNameplatesAlways, Localize("Always show name plates"), g_Config.m_ClNameplatesAlways, &Button))
				g_Config.m_ClNameplatesAlways ^= 1;
		}

        {
            const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(max(0, m_pClient->m_pSkins->Find(g_Config.m_PlayerSkin)));

            CTeeRenderInfo OwnSkinInfo;
            OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
            OwnSkinInfo.m_ColorBody = vec4(1, 1, 1, 1);
            OwnSkinInfo.m_ColorFeet = vec4(1, 1, 1, 1);

            if(g_Config.m_PlayerUseCustomColor)
            {
                OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorBody);
                OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorFeet);
                OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
            }

            OwnSkinInfo.m_Size = UI()->Scale()*50.0f;

            LeftView.HSplitTop(20.0f, &Button, &LeftView);
            LeftView.HSplitTop(20.0f, &Button, &LeftView);

            str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
            UI()->DoLabel(&Button, aBuf, 14.0, -1);

            CUIRect SkinRect;
            LeftView.VSplitLeft(LeftView.w/1.2f, &SkinRect, 0);
            SkinRect.HSplitTop(50.0f, &SkinRect, 0);
            RenderTools()->DrawUIRect(&SkinRect, vec4(1, 1, 1, 0.25f), CUI::CORNER_ALL, 10.0f);

            Button.VSplitLeft(30.0f, 0, &Button);
            Button.HSplitTop(50.0f, 0, &Button);
            RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Button.x, Button.y));

            LeftView.HSplitTop(20.0f, &Button, &LeftView);
            Button.HSplitTop(15.0f, 0, &Button);
            Button.VSplitLeft(100.0f, 0, &Button);

            str_format(aBuf, sizeof(aBuf), "%s", g_Config.m_PlayerSkin);
            UI()->DoLabel(&Button, aBuf, 14.0, -1);
        }

		RightView.HSplitTop(20.0f, &Button, &RightView);
		RightView.HSplitTop(20.0f, &Button, &RightView);

		if(DoButton_CheckBox(&g_Config.m_PlayerColorBody, Localize("Custom colors"), g_Config.m_PlayerUseCustomColor, &Button))
		{
			g_Config.m_PlayerUseCustomColor = g_Config.m_PlayerUseCustomColor?0:1;
			m_NeedSendinfo = true;
		}

		if(g_Config.m_PlayerUseCustomColor)
		{
			int *paColors[2];
			paColors[0] = &g_Config.m_PlayerColorBody;
			paColors[1] = &g_Config.m_PlayerColorFeet;

			const char *paParts[] = {
				Localize("Body"),
				Localize("Feet")};
			const char *paLabels[] = {
				Localize("Hue"),
				Localize("Sat."),
				Localize("Lht.")};
			static int s_aColorSlider[2][3] = {{0}};
			//static float v[2][3] = {{0, 0.5f, 0.25f}, {0, 0.5f, 0.25f}};

			for(int i = 0; i < 2; i++)
			{
				CUIRect Text;
				RightView.HSplitTop(20.0f, &Text, &RightView);
				Text.VSplitLeft(15.0f, 0, &Text);
				UI()->DoLabel(&Text, paParts[i], 14.0f, -1);

				int PrevColor = *paColors[i];
				int Color = 0;
				for(int s = 0; s < 3; s++)
				{
					CUIRect Text;
					RightView.HSplitTop(19.0f, &Button, &RightView);
					Button.VSplitLeft(30.0f, 0, &Button);
					Button.VSplitLeft(70.0f, &Text, &Button);
					Button.VSplitRight(5.0f, &Button, 0);
					Button.HSplitTop(4.0f, 0, &Button);

					float k = ((PrevColor>>((2-s)*8))&0xff)  / 255.0f;
					k = DoScrollbarH(&s_aColorSlider[i][s], &Button, k);
					Color <<= 8;
					Color += clamp((int)(k*255), 0, 255);
					UI()->DoLabel(&Text, paLabels[s], 15.0f, -1);

				}

				if(*paColors[i] != Color)
					m_NeedSendinfo = true;

				*paColors[i] = Color;
				RightView.HSplitTop(5.0f, 0, &RightView);
			}
		}

        MainView.HSplitTop(MainView.h/2, 0, &MainView);

		// render skinselector
		static const int s_MaxSkins = 256;
		static const CSkins::CSkin *s_paSkinList[s_MaxSkins];
		static int s_NumSkins = -1;
		static float s_ScrollValue = 0;
		if(s_NumSkins == -1)
		{
			mem_zero(s_paSkinList, sizeof(s_paSkinList));
			s_NumSkins = 0;
			for(int i = 0; i < m_pClient->m_pSkins->Num() && i < s_MaxSkins; ++i)
			{
				const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
				// no special skins
				if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
					continue;
				s_paSkinList[s_NumSkins++] = s;
			}
		}

		int OldSelected = -1;
		UiDoListboxStart(&s_NumSkins , &MainView, 50.0f, Localize("Skins"), "", s_NumSkins, 4, OldSelected, s_ScrollValue);

		for(int i = 0; i < s_NumSkins; ++i)
		{
			const CSkins::CSkin *s = s_paSkinList[i];
			if(s == 0)
				continue;

			if(str_comp(s->m_aName, g_Config.m_PlayerSkin) == 0)
				OldSelected = i;

			CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
			if(Item.m_Visible)
			{
				CTeeRenderInfo Info;
				Info.m_Texture = s->m_OrgTexture;
				Info.m_ColorBody = vec4(1, 1, 1, 1);
				Info.m_ColorFeet = vec4(1, 1, 1, 1);

				if(g_Config.m_PlayerUseCustomColor)
				{
					Info.m_ColorBody = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorBody);
					Info.m_ColorFeet = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorFeet);
					Info.m_Texture = s->m_ColorTexture;
				}

				Info.m_Size = UI()->Scale()*50.0f;
				Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
				RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1, 0), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));

				if(g_Config.m_Debug)
				{
					Graphics()->TextureSet(-1);
					Graphics()->QuadsBegin();
					Graphics()->SetColor(s->m_BloodColor.r, s->m_BloodColor.g, s->m_BloodColor.b, 1.0f);
					IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, 12, 12);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}
		}

		const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
		if(OldSelected != NewSelected)
		{
			mem_copy(g_Config.m_PlayerSkin, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_PlayerSkin));
			m_NeedSendinfo = true;
		}
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
	// we need to do localize so the scripts can pickup the string
	{ "Move left", "+left", 0},		// Localize - these strings are localized within CLocConstString
	{ "Move right", "+right", 0 },
	{ "Jump", "+jump", 0 },
	{ "Fire", "+fire", 0 },
	{ "Hook", "+hook", 0 },
	{ "Hammer", "+weapon1", 0 },
	{ "Pistol", "+weapon2", 0 },
	{ "Shotgun", "+weapon3", 0 },
	{ "Grenade", "+weapon4", 0 },
	{ "Rifle", "+weapon5", 0 },
	{ "Next weapon", "+nextweapon", 0 },
	{ "Prev. weapon", "+prevweapon", 0 },
	{ "Vote yes", "vote yes", 0 },
	{ "Vote no", "vote no", 0 },
	{ "Chat", "chat all", 0 },
	{ "Team chat", "chat team", 0 },
	{ "Show chat", "+show_chat", 0 },
	{ "Emoticon", "+emote", 0 },
	{ "Console", "toggle_local_console", 0 },
	{ "Remote console", "toggle_remote_console", 0 },
	{ "Screenshot", "screenshot", 0 },
	{ "Scoreboard", "+scoreboard", 0 },
};

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View)
{
	for (int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(130.0f, &Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		UI()->DoLabel(&Label, aBuf, 14.0f, -1);
		int OldId = Key.m_KeyId;
		int NewId = DoKeyReader((void *)&gs_aKeys[i].m_Name, &Button, OldId);
		if(NewId != OldId)
		{
			m_pClient->m_pBinds->Bind(OldId, "");
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
	MainView.VSplitLeft(MainView.w/2-5.0f, &MovementSettings, &VotingSettings);

	// movement settings
	{
		MovementSettings.HSplitTop(MainView.h/2-5.0f, &MovementSettings, &WeaponSettings);
		RenderTools()->DrawUIRect(&MovementSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		MovementSettings.Margin(10.0f, &MovementSettings);

		TextRender()->Text(0, MovementSettings.x, MovementSettings.y, 14, Localize("Movement"), -1);

		MovementSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &MovementSettings);

		{
			CUIRect Button, Label;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(130.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Mouse sens."), 14.0f, -1);
			Button.HMargin(2.0f, &Button);
			g_Config.m_InpMousesens = (int)(DoScrollbarH(&g_Config.m_InpMousesens, &Button, (g_Config.m_InpMousesens-5)/500.0f)*500.0f)+5;
			//*key.key = ui_do_key_reader(key.key, &Button, *key.key);
			MovementSettings.HSplitTop(20.0f, 0, &MovementSettings);
		}

		UiDoGetButtons(0, 5, MovementSettings);

	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(10.0f, 0, &WeaponSettings);
		RenderTools()->DrawUIRect(&WeaponSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		WeaponSettings.Margin(10.0f, &WeaponSettings);

		TextRender()->Text(0, WeaponSettings.x, WeaponSettings.y, 14, Localize("Weapon"), -1);

		WeaponSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &WeaponSettings);
		UiDoGetButtons(5, 12, WeaponSettings);
	}

	// voting settings
	{
		VotingSettings.VSplitLeft(10.0f, 0, &VotingSettings);
		VotingSettings.HSplitTop(MainView.h/4-5.0f, &VotingSettings, &ChatSettings);
		RenderTools()->DrawUIRect(&VotingSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		VotingSettings.Margin(10.0f, &VotingSettings);

		TextRender()->Text(0, VotingSettings.x, VotingSettings.y, 14, Localize("Voting"), -1);

		VotingSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &VotingSettings);
		UiDoGetButtons(12, 14, VotingSettings);
	}

	// chat settings
	{
		ChatSettings.HSplitTop(10.0f, 0, &ChatSettings);
		ChatSettings.HSplitTop(MainView.h/4-10.0f, &ChatSettings, &MiscSettings);
		RenderTools()->DrawUIRect(&ChatSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		ChatSettings.Margin(10.0f, &ChatSettings);

		TextRender()->Text(0, ChatSettings.x, ChatSettings.y, 14, Localize("Chat"), -1);

		ChatSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &ChatSettings);
		UiDoGetButtons(14, 17, ChatSettings);
	}

	// misc settings
	{
		MiscSettings.HSplitTop(10.0f, 0, &MiscSettings);
		MiscSettings.HSplitTop(MainView.h/2-5.0f-45.0f, &MiscSettings, &ResetButton);
		RenderTools()->DrawUIRect(&MiscSettings, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
		MiscSettings.Margin(10.0f, &MiscSettings);

		TextRender()->Text(0, MiscSettings.x, MiscSettings.y, 14, Localize("Miscellaneous"), -1);

		MiscSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &MiscSettings);
		UiDoGetButtons(17, 21, MiscSettings);
	}

	// defaults
	ResetButton.HSplitTop(10.0f, 0, &ResetButton);
	static int s_DefaultButton = 0;
	if(DoButton_Menu((void*)&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
		m_pClient->m_pBinds->SetDefaults();
}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CUIRect Button;
	char aBuf[128];

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = -1;
	const static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	const static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	const static int s_GfxColorDepth = g_Config.m_GfxColorDepth;

	if(s_NumNodes == -1)
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);

	CUIRect ModeList;
	MainView.VSplitLeft(300.0f, &MainView, &ModeList);

	// draw allmodes switch
	ModeList.HSplitTop(20, &Button, &ModeList);
	if(DoButton_CheckBox(&g_Config.m_GfxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
	{
		g_Config.m_GfxDisplayAllModes ^= 1;
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS);
	}

	// display mode list
	static float s_ScrollValue = 0;
	int OldSelected = -1;
	str_format(aBuf, sizeof(aBuf), "%s: %dx%d %d bit", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxColorDepth);
	UiDoListboxStart(&s_NumNodes , &ModeList, 24.0f, Localize("Display Modes"), aBuf, s_NumNodes, 1, OldSelected, s_ScrollValue);

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
			str_format(aBuf, sizeof(aBuf), "  %dx%d %d bit", s_aModes[i].m_Width, s_aModes[i].m_Height, Depth);
			UI()->DoLabel(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red+s_aModes[NewSelected].m_Green+s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_Height;
		m_NeedRestart = true;
	}

	// switches
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxFullscreen, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		g_Config.m_GfxFullscreen ^= 1;
		m_NeedRestart = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxVsync, Localize("V-Sync"), g_Config.m_GfxVsync, &Button))
	{
		g_Config.m_GfxVsync ^= 1;
		m_NeedRestart = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox_Number(&g_Config.m_GfxFsaaSamples, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button))
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples+1)%17;
		m_NeedRestart = true;
	}

	MainView.HSplitTop(40.0f, &Button, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxTextureQuality, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		m_NeedRestart = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxTextureCompression, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		m_NeedRestart = true;
	}

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_GfxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button))
		g_Config.m_GfxHighDetail ^= 1;

	//

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	UI()->DoLabel(&Text, Localize("UI Color"), 14.0f, -1);

	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht."),
		Localize("Alpha")};
	int *pColorSlider[4] = {&g_Config.m_UiColorHue, &g_Config.m_UiColorSat, &g_Config.m_UiColorLht, &g_Config.m_UiColorAlpha};
	for(int s = 0; s < 4; s++)
	{
		CUIRect Text;
		MainView.HSplitTop(19.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		Button.VSplitLeft(50.0f, &Text, &Button);
		Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = (*pColorSlider[s]) / 255.0f;
		k = DoScrollbarH(pColorSlider[s], &Button, k);
		*pColorSlider[s] = (int)(k*255.0f);
		UI()->DoLabel(&Text, paLabels[s], 15.0f, -1);
	}
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CUIRect Button;
	MainView.VSplitLeft(300.0f, &MainView, 0);

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		m_NeedRestart = true;
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_SndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	// sample rate box
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_SndRate);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		UI()->DoLabel(&Button, Localize("Sample rate"), 14.0f, -1);
		Button.VSplitLeft(110.0f, 0, &Button);
		Button.VSplitLeft(180.0f, &Button, 0);
		static float Offset = 0.0f;
		DoEditBox(&g_Config.m_SndRate, &Button, aBuf, sizeof(aBuf), 14.0f, &Offset);
		int Before = g_Config.m_SndRate;
		g_Config.m_SndRate = str_toint(aBuf);

		if(g_Config.m_SndRate != Before)
			m_NeedRestart = true;

		if(g_Config.m_SndRate < 1)
			g_Config.m_SndRate = 1;
	}

	// volume slider
	{
		CUIRect Button, Label;
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(110.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabel(&Label, Localize("Sound volume"), 14.0f, -1);
		g_Config.m_SndVolume = (int)(DoScrollbarH(&g_Config.m_SndVolume, &Button, g_Config.m_SndVolume/100.0f)*100.0f);
		MainView.HSplitTop(20.0f, 0, &MainView);
	}
}

struct LANGUAGE
{
	LANGUAGE() {}
	LANGUAGE(const char *n, const char *f) : m_Name(n), m_FileName(f) {}

	string m_Name;
	string m_FileName;

	bool operator<(const LANGUAGE &Other) { return m_Name < Other.m_Name; }
};


int fs_listdir(const char *pDir, FS_LISTDIR_CALLBACK cb, void *pUser);

void GatherLanguages(const char *pName, int IsDir, void *pUser)
{
	if(IsDir || pName[0] == '.')
		return;

	sorted_array<LANGUAGE> &Languages = *((sorted_array<LANGUAGE> *)pUser);
	char aFileName[128];
	str_format(aFileName, sizeof(aFileName), "data/languages/%s", pName);

	char NiceName[128];
	str_format(NiceName, sizeof(NiceName), "%s", pName);
	NiceName[0] = str_uppercase(NiceName[0]);


	for(char *p = NiceName; *p; p++)
		if(*p == '.')
			*p = 0;

	Languages.add(LANGUAGE(NiceName, aFileName));
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	static int s_LanguageList  = 0;
	static int s_SelectedLanguage = 0;
	static sorted_array<LANGUAGE> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(LANGUAGE("English", ""));
		fs_listdir("data/languages", GatherLanguages, &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

	CUIRect List = MainView;
	UiDoListboxStart(&s_LanguageList , &List, 24.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);

	for(sorted_array<LANGUAGE>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem(&r.front());

		if(Item.m_Visible)
			UI()->DoLabel(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName);
	}
}

void CMenus::RenderSettings(CUIRect MainView)
{
	static int s_SettingsPage = 0;

	// render background
	CUIRect Temp, TabBar;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	CUIRect Button;

	const char *aTabs[] = {
		Localize("General"),
		Localize("Player"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound")};

	int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_SettingsTab(aTabs[i], aTabs[i], s_SettingsPage == i, &Button))
			s_SettingsPage = i;
	}

	MainView.Margin(10.0f, &MainView);

	if(s_SettingsPage == 0)
		RenderSettingsGeneral(MainView);
	else if(s_SettingsPage == 1)
		RenderSettingsPlayer(MainView);
	else if(s_SettingsPage == 2)
		RenderSettingsControls(MainView);
	else if(s_SettingsPage == 3)
		RenderSettingsGraphics(MainView);
	else if(s_SettingsPage == 4)
		RenderSettingsSound(MainView);

	if(m_NeedRestart)
	{
		CUIRect RestartWarning;
		MainView.HSplitBottom(40, &MainView, &RestartWarning);
		UI()->DoLabel(&RestartWarning, Localize("You must restart the game for all settings to take effect."), 15.0f, -1, 220);
	}
}
