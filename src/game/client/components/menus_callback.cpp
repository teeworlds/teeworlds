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
		UI()->DoLabelScaled(&Label, aBuf, 13.0f, 0);
		int OldId = Key.m_KeyId;
		int NewId = DoKeyReader((void *)&gs_aKeys[i].m_Name, &Button, OldId);
		if(NewId != OldId)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_pBinds->Bind(OldId, "");
			if(NewId != 0)
				m_pClient->m_pBinds->Bind(NewId, gs_aKeys[i].m_pCommand);
		}
	}
}

float CMenus::RenderSettingsControlsMovement(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	int NumOptions = 6;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	CUIRect Button;
	View.HSplitTop(Spaceing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	pSelf->DoScrollbarOption(&g_Config.m_InpMousesens, &g_Config.m_InpMousesens, &Button, Localize("Mouse sens."), 150.0f, 5, 500);

	pSelf->UiDoGetButtons(0, 5, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsWeapon(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
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
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
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
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	int NumOptions = 3;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(14, 17, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsMisc(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = pSelf->m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	int NumOptions = 9;
	float ButtonHeight = 20.0f;
	float Spaceing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spaceing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	pSelf->RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	pSelf->UiDoGetButtons(17, 26, View, ButtonHeight, Spaceing);

	return BackgroundHeight;
}