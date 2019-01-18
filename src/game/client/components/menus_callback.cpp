/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include "binds.h"
#include "menus.h"

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
	int m_Modifier;
	CMenus::CButtonContainer m_BC;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
{
	{ "Move left", "+left", 0, 0},		// Localize - these strings are localized within CLocConstString
	{ "Move right", "+right", 0, 0},
	{ "Jump", "+jump", 0, 0},
	{ "Fire", "+fire", 0, 0},
	{ "Hook", "+hook", 0, 0},
	{ "Hammer", "+weapon1", 0, 0},
	{ "Pistol", "+weapon2", 0, 0},
	{ "Shotgun", "+weapon3", 0, 0},
	{ "Grenade", "+weapon4", 0, 0},
	{ "Laser", "+weapon5", 0, 0},
	{ "Next weapon", "+nextweapon", 0, 0},
	{ "Prev. weapon", "+prevweapon", 0, 0},
	{ "Vote yes", "vote yes", 0, 0},
	{ "Vote no", "vote no", 0, 0},
	{ "Chat", "chat all", 0, 0},
	{ "Team chat", "chat team", 0, 0},
	{ "Whisper", "chat whisper", 0, 0},
	{ "Show chat", "+show_chat", 0, 0},
	{ "Emoticon", "+emote", 0, 0},
	{ "Spectator mode", "+spectate", 0, 0},
	{ "Spectate next", "spectate_next", 0, 0},
	{ "Spectate previous", "spectate_previous", 0, 0},
	{ "Console", "toggle_local_console", 0, 0},
	{ "Remote console", "toggle_remote_console", 0, 0},
	{ "Screenshot", "screenshot", 0, 0},
	{ "Scoreboard", "+scoreboard", 0, 0},
	{ "Respawn", "kill", 0, 0},
	{ "Ready", "ready_change", 0, 0},
	{ "Add demo marker", "add_demomarker", 0, 0},
};

/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Laser");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Whisper");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");
	Localize("Screenshot");Localize("Scoreboard");Localize("Respawn");Localize("Ready");Localize("Add demo marker");
*/

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View, float ButtonHeight, float Spaceing)
{
	for (int i = Start; i < Stop; i++)
	{
		View.HSplitTop(Spaceing, 0, &View);

		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(ButtonHeight, &Button, &View);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Button.VSplitMid(&Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		Label.y += 2.0f;
		UI()->DoLabelScaled(&Label, aBuf, 13.0f, CUI::ALIGN_CENTER);
		int OldId = Key.m_KeyId, OldModifier = Key.m_Modifier, NewModifier;
		int NewId = DoKeyReader(&gs_aKeys[i].m_BC, &Button, OldId, OldModifier, &NewModifier);
		if(NewId != OldId || NewModifier != OldModifier)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_pBinds->Bind(OldId, OldModifier, "");
			if(NewId != 0)
				m_pClient->m_pBinds->Bind(NewId, NewModifier, gs_aKeys[i].m_pCommand);
		}
	}
}

float CMenus::RenderSettingsControlsMovement(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
	{
		gs_aKeys[i].m_KeyId = 0;
		gs_aKeys[i].m_Modifier = 0;
	}

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		for(int m = 0; m < CBinds::MODIFIER_COUNT; m++)
		{
			const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId, m);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					gs_aKeys[i].m_Modifier = m;
					break;
				}
		}
	}

	int NumOptions = 8;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	CUIRect Button;
	View.HSplitTop(Spaceing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	static int s_ButtonInpGrab = 0;
	if(pSelf->DoButton_CheckBox(&s_ButtonInpGrab, Localize("Use OS mouse acceleration"), !g_Config.m_InpGrab, &Button))
	{
		g_Config.m_InpGrab ^= 1;
	}
	View.HSplitTop(Spaceing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	pSelf->DoScrollbarOption(&g_Config.m_InpMousesens, &g_Config.m_InpMousesens, &Button, Localize("Ingame mouse sens."), 5, 500);
	View.HSplitTop(Spaceing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	pSelf->DoScrollbarOption(&g_Config.m_UiMousesens, &g_Config.m_UiMousesens, &Button, Localize("Menu mouse sens."), 5, 500);

	pSelf->UiDoGetButtons(0, 5, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsWeapon(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
	{
		gs_aKeys[i].m_KeyId = 0;
		gs_aKeys[i].m_Modifier = 0;
	}

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		for(int m = 0; m < CBinds::MODIFIER_COUNT; m++)
		{
			const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId, m);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					gs_aKeys[i].m_Modifier = m;
					break;
				}
		}
	}

	int NumOptions = 7;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(5, 12, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsVoting(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
	{
		gs_aKeys[i].m_KeyId = 0;
		gs_aKeys[i].m_Modifier = 0;
	}

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		for(int m = 0; m < CBinds::MODIFIER_COUNT; m++)
		{
			const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId, m);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					gs_aKeys[i].m_Modifier = m;
					break;
				}
		}
	}

	int NumOptions = 2;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(12, 14, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsChat(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
	{
		gs_aKeys[i].m_KeyId = 0;
		gs_aKeys[i].m_Modifier = 0;
	}

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		for(int m = 0; m < CBinds::MODIFIER_COUNT; m++)
		{
			const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId, m);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					gs_aKeys[i].m_Modifier = m;
					break;
				}
		}
	}

	int NumOptions = 4;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(14, 18, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsMisc(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
	{
		gs_aKeys[i].m_KeyId = 0;
		gs_aKeys[i].m_Modifier = 0;
	}

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		for(int m = 0; m < CBinds::MODIFIER_COUNT; m++)
		{
			const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId, m);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					gs_aKeys[i].m_Modifier = m;
					break;
				}
		}
	}

	int NumOptions = 11;
	int StartOption = 18;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(StartOption, StartOption+NumOptions, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}