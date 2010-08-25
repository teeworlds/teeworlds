
#include <base/math.h>


#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "menus.h"
#include "motd.h"
#include "voting.h"

void CMenus::RenderRCON(CUIRect MainView)
{
	CUIRect ButtonBar;
	//
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	
	//TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	//RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);
	
	MainView.Margin(10.00f, &MainView);
	
	MainView.VSplitRight(150.0f, &MainView, &ButtonBar);
	
	CUIRect ButtonKick;
	CUIRect ButtonMute;
	//CUIRect ButtonUnMute;
	CUIRect ButtonKill;
	CUIRect ButtonBan;
	
	ButtonBar.VMargin(10.0f, &ButtonBar);
	
	
	CUIRect ButtonBar1, ButtonBar2, ButtonBar3, ButtonBar4, ButtonBar5;
	ButtonBar.HSplitTop(30.0f, &ButtonKick, &ButtonBar1);
	ButtonBar1.HMargin(10.0f, &ButtonBar1);
	ButtonBar1.HSplitTop(30.0f, &ButtonMute, &ButtonBar2);
	ButtonBar2.HMargin(10.0f, &ButtonBar2);
	ButtonBar2.HSplitTop(30.0f, &ButtonKill, &ButtonBar3);
	ButtonBar3.HMargin(10.0f, &ButtonBar3);
	ButtonBar3.HSplitTop(30.0f, &ButtonBan, &ButtonBar4);
	//MainView.VSplitLeft(5.0f, &ButtonBar, &ButtonMute);
	//MainView.VSplitLeft(5.0f, &ButtonBar, &ButtonUnMute);
	//MainView.VSplitLeft(5.0f, &ButtonBar, &ButtonKill);
	//MainView.VSplitLeft(5.0f, &ButtonBar, &ButtonBan);
	
	
	
	int NumOptions = 0;
	int Selected = -1;
	static int aPlayerIDs[MAX_CLIENTS];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i])
			continue;
		if(m_RCONSelectedPlayer == i)
			Selected = NumOptions;
		aPlayerIDs[NumOptions++] = i;
	}

	static int s_VoteList = 0;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
	UiDoListboxStart(&s_VoteList, &List, 24.0f, Localize("Players"), "", NumOptions, 1, Selected, s_ScrollValue);
	
	for(int i = 0; i < NumOptions; i++)
	{
		CListboxItem Item = UiDoListboxNextItem(&aPlayerIDs[i]);
		
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Item.m_Rect.h;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0), vec2(Item.m_Rect.x+Item.m_Rect.h/2, Item.m_Rect.y+Item.m_Rect.h/2));
			Item.m_Rect.x +=Info.m_Size;
			UI()->DoLabel(&Item.m_Rect, m_pClient->m_aClients[aPlayerIDs[i]].m_aName, 16.0f, -1);
		}
	}
	
	Selected = UiDoListboxEnd(&s_ScrollValue, 0);
	m_RCONSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
	static int s_KickButton = 0;
	char buf[512];
	if(DoButton_Menu(&s_KickButton, Localize("Kick"), 0, &ButtonKick))
		{
			if(m_RCONSelectedPlayer >= 0 && m_RCONSelectedPlayer < MAX_CLIENTS) {
				
			
				str_format(buf, sizeof(buf), "rcon kick %d", m_RCONSelectedPlayer);
				m_pClient->Console()->ExecuteLine(buf, 4, -1);
			}
	}
	static int s_KillButton = 0;
	if(DoButton_Menu(&s_KillButton, Localize("Kill"), 0, &ButtonKill))
		{
			if(m_RCONSelectedPlayer >= 0 && m_RCONSelectedPlayer < MAX_CLIENTS) {
				
			
				str_format(buf, sizeof(buf), "rcon kill_pl  %d", m_RCONSelectedPlayer);
				m_pClient->Console()->ExecuteLine(buf, 4, -1);
			}
	}
	static int s_MuteButton = 0;
	if(DoButton_Menu(&s_MuteButton, Localize("Mute"), 0, &ButtonMute))
		{
			if(m_RCONSelectedPlayer >= 0 && m_RCONSelectedPlayer < MAX_CLIENTS) {
				
			
				str_format(buf, sizeof(buf), "rcon mute %d 100", m_RCONSelectedPlayer);
				m_pClient->Console()->ExecuteLine(buf, 4, -1);
			}
	}
	static int s_BanButton = 0;
	if(DoButton_Menu(&s_BanButton, Localize("Ban"), 0, &ButtonBan))
		{
			if(m_RCONSelectedPlayer >= 0 && m_RCONSelectedPlayer < MAX_CLIENTS) {
				
			
				str_format(buf, sizeof(buf), "rcon ban %d 300", m_RCONSelectedPlayer);
				m_pClient->Console()->ExecuteLine(buf, 4, -1);
			}
	}
}
