/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/textrender.h>

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
	{ "Scoreboard", "+scoreboard", 0, 0},
	{ "Statboard", "+stats", 0, 0},
	{ "Emoticon", "+emote", 0, 0},

	{ "Spectator mode", "+spectate", 0, 0},
	{ "Spectate next", "spectate_next", 0, 0},
	{ "Spectate previous", "spectate_previous", 0, 0},
	{ "Console", "toggle_local_console", 0, 0},
	{ "Remote console", "toggle_remote_console", 0, 0},
	{ "Screenshot", "screenshot", 0, 0},
	{ "Respawn", "kill", 0, 0},
	{ "Ready", "ready_change", 0, 0},
	{ "Add demo marker", "add_demomarker", 0, 0},
	{ "Toggle sounds", "snd_toggle", 0, 0},
	{ "Toggle dynamic camera", "toggle cl_dynamic_camera 1 0", 0, 0},
};

/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Laser");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Whisper");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");
	Localize("Screenshot");Localize("Scoreboard");Localize("Statboard");Localize("Respawn");Localize("Ready");Localize("Add demo marker");Localize("Toggle sounds");Localize("Toggle dynamic camera")
*/

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View, float ButtonHeight, float Spacing)
{
	for (int i = Start; i < Stop; i++)
	{
		View.HSplitTop(Spacing, 0, &View);

		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(ButtonHeight, &Button, &View);
		RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

		Button.VSplitMid(&Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		Label.y += 2.0f;
		UI()->DoLabel(&Label, aBuf, 13.0f, CUI::ALIGN_CENTER);
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

static void UpdateBindKeys(CBinds* pBinds)
{
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
			const char *pBind = pBinds->Get(KeyId, m);
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
}

float CMenus::RenderSettingsControlsMouse(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 3;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	CUIRect Button;
	View.HSplitTop(Spacing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	static int s_ButtonInpGrab = 0;
	if(DoButton_CheckBox(&s_ButtonInpGrab, Localize("Use OS mouse acceleration"), !Config()->Values()->m_InpGrab, &Button))
	{
		Config()->Values()->m_InpGrab ^= 1;
	}
	View.HSplitTop(Spacing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	DoScrollbarOption(&Config()->Values()->m_InpMousesens, &Config()->Values()->m_InpMousesens, &Button, Localize("Ingame mouse sens."), 1, 500, &LogarithmicScrollbarScale);
	View.HSplitTop(Spacing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	DoScrollbarOption(&Config()->Values()->m_UiMousesens, &Config()->Values()->m_UiMousesens, &Button, Localize("Menu mouse sens."), 1, 500, &LogarithmicScrollbarScale);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsJoystick(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	bool JoystickEnabled = Config()->Values()->m_JoystickEnable;
	int NumJoysticks = m_pClient->Input()->NumJoysticks();
	int NumOptions = 2; // expandable header & message
	if(JoystickEnabled && NumJoysticks > 0)
	{
		if(NumJoysticks > 1)
		{
			NumOptions++; // joystick selection
		}
		NumOptions += 2; // sensitivity & tolerance
		NumOptions += m_pClient->Input()->GetJoystickNumAxes(); // axis selection
	}
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing+Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	CUIRect Button;
	View.HSplitTop(Spacing, 0, &View);
	View.HSplitTop(ButtonHeight, &Button, &View);
	static int s_ButtonJoystickEnable = 0;
	if(DoButton_CheckBox(&s_ButtonJoystickEnable, Localize("Enable joystick"), Config()->Values()->m_JoystickEnable, &Button))
	{
		Config()->Values()->m_JoystickEnable ^= 1;
	}
	if(JoystickEnabled)
	{
		if(NumJoysticks > 0)
		{
			// show joystick device selection if more than one available
			if(NumJoysticks > 1)
			{
				View.HSplitTop(Spacing, 0, &View);
				View.HSplitTop(ButtonHeight, &Button, &View);
				static CButtonContainer s_ButtonJoystickId;
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "Joystick %d: %s", m_pClient->Input()->GetJoystickIndex(), m_pClient->Input()->GetJoystickName());
				if(DoButton_Menu(&s_ButtonJoystickId, aBuf, 0, &Button))
				{
					m_pClient->Input()->SelectNextJoystick();
				}
			}

			View.HSplitTop(Spacing, 0, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			DoScrollbarOption(&Config()->Values()->m_JoystickSens, &Config()->Values()->m_JoystickSens, &Button, Localize("Joystick sens."), 1, 500, &LogarithmicScrollbarScale);

			View.HSplitTop(Spacing, 0, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			DoScrollbarOption(&Config()->Values()->m_JoystickTolerance, &Config()->Values()->m_JoystickTolerance, &Button, Localize("Joystick jitter tolerance"), 0, 50);

			// shrink view and draw background
			View.HSplitTop(Spacing, 0, &View);
			View.VSplitLeft(View.w/6, 0, &View);
			View.VSplitRight(View.w/5, &View, 0);
			RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.125f), CUI::CORNER_ALL, 5.0f);

			DoJoystickAxisPicker(View);
		}
		else
		{
			View.HSplitTop((View.h-ButtonHeight)/2.0f, 0, &View);
			View.HSplitTop(ButtonHeight, &Button, &View);
			m_pClient->UI()->DoLabel(&Button, Localize("No joysticks found. Plug in a joystick and restart the game."), 13.0f, CUI::ALIGN_CENTER);
		}
	}

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsMovement(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 5;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(0, 5, View, ButtonHeight, Spacing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsWeapon(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 7;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(5, 12, View, ButtonHeight, Spacing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsVoting(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 2;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(12, 14, View, ButtonHeight, Spacing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsChat(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 4;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(14, 18, View, ButtonHeight, Spacing);

	return BackgroundHeight;
}

float CMenus::RenderSettingsControlsScoreboard(CUIRect View)
{
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
			const char *pBind = m_pClient->m_pBinds->Get(KeyId, m);
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
	int StartOption = 18;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(StartOption, StartOption+NumOptions, View, ButtonHeight, Spacing);

	View.HSplitTop(ButtonHeight*2+Spacing*3, 0, &View);
	View.VSplitLeft(View.w/3, 0, &View);
	View.VSplitRight(View.w/2, &View, 0);
	static int s_StatboardConfigDropdown = 0;
	static bool s_StatboardConfigActive = false;
	float Split = DoIndependentDropdownMenu(&s_StatboardConfigDropdown, &View, Localize("Configure statboard"), 20.0f, &CMenus::RenderSettingsControlsStats, &s_StatboardConfigActive);

	return BackgroundHeight+Split;
}

float CMenus::RenderSettingsControlsMisc(CUIRect View)
{
	UpdateBindKeys(m_pClient->m_pBinds);

	int NumOptions = 12;
	int StartOption = 20;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)NumOptions*ButtonHeight+(float)NumOptions*Spacing;

	View.HSplitTop(BackgroundHeight, &View, 0);
	RenderTools()->DrawUIRect(&View, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

	UiDoGetButtons(StartOption, StartOption+NumOptions, View, ButtonHeight, Spacing);

	return BackgroundHeight;
}

void CMenus::DoJoystickAxisPicker(CUIRect View)
{
	CUIRect Row, Button;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	
	float DeviceLabelWidth = View.w*0.30f;
	float StatusWidth = View.w*0.30f;
	float BindWidth = View.w*0.1f;
	float StatusMargin = View.w*0.05f;
	View.HSplitTop(Spacing, 0, &View);
	View.HSplitTop(ButtonHeight, &Row, &View);
	Row.VSplitLeft(StatusWidth, &Button, &Row);
	m_pClient->UI()->DoLabel(&Button, Localize("Device"), 13.0f, CUI::ALIGN_CENTER);
	Row.VSplitLeft(StatusMargin, 0, &Row);
	Row.VSplitLeft(StatusWidth, &Button, &Row);
	m_pClient->UI()->DoLabel(&Button, Localize("Status"), 13.0f, CUI::ALIGN_CENTER);
	Row.VSplitLeft(2*StatusMargin, 0, &Row);
	Row.VSplitLeft(2*BindWidth, &Button, &Row);
	m_pClient->UI()->DoLabel(&Button, Localize("Aim bind"), 13.0f, CUI::ALIGN_CENTER);

	static int s_aActive[g_MaxJoystickAxes][2];
	for(int i = 0; i < min(m_pClient->Input()->GetJoystickNumAxes(), g_MaxJoystickAxes); i++)
	{
		bool Active = Config()->Values()->m_JoystickX == i || Config()->Values()->m_JoystickY == i;

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Row, &View);
		RenderTools()->DrawUIRect(&Row, vec4(0.0f, 0.0f, 0.0f, 0.125f), CUI::CORNER_ALL, 5.0f);

		// Device label
		Row.VSplitLeft(DeviceLabelWidth, &Button, &Row);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Joystick Axis #%d"), i+1);
		if(!Active)
			m_pClient->TextRender()->TextColor(0.7f, 0.7f, 0.7f, 1.0f);
		else
			m_pClient->TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_pClient->UI()->DoLabel(&Button, aBuf, 13.0f, CUI::ALIGN_CENTER);

		// Device status
		Row.VSplitLeft(StatusMargin, 0, &Row);
		Row.VSplitLeft(StatusWidth, &Button, &Row);
		Button.HMargin((ButtonHeight-14.0f)/2.0f, &Button);
		DoJoystickBar(&Button, (m_pClient->Input()->GetJoystickAxisValue(i)+1.0f)/2.0f, Config()->Values()->m_JoystickTolerance/50.0f, Active);

		// Bind to X,Y
		Row.VSplitLeft(2*StatusMargin, 0, &Row);
		Row.VSplitLeft(BindWidth, &Button, &Row);
		if(DoButton_CheckBox(&s_aActive[i][0], "X", Config()->Values()->m_JoystickX == i, &Button, Config()->Values()->m_JoystickY == i))
			Config()->Values()->m_JoystickX = i;
		Row.VSplitLeft(BindWidth, &Button, &Row);
		if(DoButton_CheckBox(&s_aActive[i][1], "Y", Config()->Values()->m_JoystickY == i, &Button, Config()->Values()->m_JoystickX == i))
			Config()->Values()->m_JoystickY = i;
		Row.VSplitLeft(StatusMargin, 0, &Row);
	}
}
