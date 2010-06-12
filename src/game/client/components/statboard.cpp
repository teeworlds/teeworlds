#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/motd.h>
#include "skins.h"
#include "statboard.h"
#include "coopboard.h"
#include "camera.h"
#include "controls.h"


CStatboard::CStatboard()
{
	OnReset();
}

void CStatboard::ConKeyStatboard(IConsole::IResult *pResult, void *pUserData)
{
	((CStatboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CStatboard::OnReset()
{
	m_Active = false;
	m_StatJustActivated = false;
	m_LastRequest = -1;
	m_Marked = 0;
	m_CurrentLine = 0;
	m_CurrentRow = 0;
	m_StartX = 0;
	m_StartY = 0;
}

void CStatboard::OnConsoleInit()
{
	Console()->Register("+statboard", "", CFGFLAG_CLIENT, ConKeyStatboard, this, "Show statboard");
}

void CStatboard::RenderStatboard()
{
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	
	float w = 320.0f;
	float h = 400.0f;
	float Info = 0;
	if(m_pClient->m_Points > 0)
	{
		Info = 15;
		h += 250.0f;
	}
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
	{
		Info = 15;
		h += 170.0f;
	}

	//float FontHeight = 50.0f;
	//float TeeSizeMod = 1.0f;
	float x = (Width/2-w)+25;
	
	float Offset = 0;
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
		Offset = 170.0f;
	
	CUIRect MainView;
	MainView.x = (Width/2-w)+15;
	MainView.y = 220;
	MainView.w = w*2-32.5;
	MainView.h = 20+h;
	
	CUIRect LeftView, RightView, Label;
	
	Graphics()->MapScreen(0, 0, Width, Height);
	
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(Width/2-w, 200.0f, w*2, 30+h+Info, 17.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.25);
	RenderTools()->DrawRoundRect((Width/2-w)+35, 220.0f, w*2-52.5, 50, 17.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.15);
	RenderTools()->DrawRoundRect((Width/2-w)+15, 290.0f, w*2-32.5, 185, 17.0f);
	Graphics()->SetColor(0.7, 0.7, 0.7, 0.15);
	RenderTools()->DrawRoundRect((Width/2-w)+15, 495.0f, w*2-32.5, 115, 17.0f);
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
	{
		Graphics()->SetColor(0.7, 0.7, 0.7, 0.25);
		RenderTools()->DrawRoundRect((Width/2-w)+15, 630.0f, w*2-32.5, 50, 17.0f);
		Graphics()->SetColor(0.7, 0.7, 0.7, 0.15);
		RenderTools()->DrawRoundRect((Width/2-w)+15, 700.0f, w*2-32.5, 80, 17.0f);
		for(int i = 0; i < 5; i++)
		{
			Graphics()->SetColor(0,0,0,0.15f);
			RenderTools()->DrawRoundRect(x, 710.0f, MainView.w/5-20, 60, 17.0f);
			x += MainView.w/5;
		}
	}
	if(m_pClient->m_Points > 0)
	{
		Graphics()->SetColor(0.7, 0.7, 0.7, 0.25);
		RenderTools()->DrawRoundRect((Width/2-w)+15, 630.0f+Offset, w*2-32.5, 50, 17.0f);
		Graphics()->SetColor(0.7, 0.7, 0.7, 0.15);
		RenderTools()->DrawRoundRect((Width/2-w)+15, 700.0f+Offset, w*2-32.5, 160, 17.0f);
		x = (Width/2-w)+25;
		for(int i = 0; i < 2; i++)
		{
			Graphics()->SetColor(0,0,0,0.15f);
			RenderTools()->DrawRoundRect(x, 710.0f+Offset, MainView.w/2-20, 60, 17.0f);
			x += MainView.w/2;
		}
		x = (Width/2-w)+25;
		for(int i = 0; i < 2; i++)
		{
			Graphics()->SetColor(0,0,0,0.15f);
			RenderTools()->DrawRoundRect(x, 790.0f+Offset, MainView.w/2-20, 60, 17.0f);
			x += MainView.w/2;
		}
	}
	Graphics()->QuadsEnd();

	MainView.VSplitLeft(MainView.w/2, &LeftView, &RightView);
	
	// render tee
	CTeeRenderInfo SkinInfo = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalCid].m_RenderInfo;
	if(g_Config.m_PlayerUseCustomColor)
	{
		SkinInfo.m_Texture = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(g_Config.m_PlayerSkin))->m_ColorTexture;
		SkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorBody);
		SkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColor(g_Config.m_PlayerColorFeet);
	}
	SkinInfo.m_Size = UI()->Scale()*100.0f;
	
	RenderTools()->RenderTee(CAnimState::GetIdle(), &SkinInfo, 0, vec2(1, 0), vec2(LeftView.x+35, LeftView.y+30));
	
	// render specialized weapon
	if(m_pClient->m_Weapon > -1)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Weapon].m_pSpriteBody);
		RenderTools()->DrawSprite(MainView.x+MainView.w-70, MainView.y+25, g_pData->m_Weapons.m_aId[m_pClient->m_Weapon].m_VisualSize);
					
		Graphics()->QuadsEnd();
	}
	
	// render headline
	MainView.VSplitLeft(MainView.w/2-TextRender()->TextWidth(0, 40.0f, "Personal Stats", -1)/2, 0, &Label);
	UI()->DoLabel(&Label, "Personal Stats", 40.0f, -1);
	
	char aBuf[16];
	
	// left site
	LeftView.HSplitTop(90.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Level: %d", m_pClient->m_Level);
	LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	LeftView.HSplitTop(60.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Str: %d", m_pClient->m_Str);
	LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	LeftView.HSplitTop(40.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Sta: %d", m_pClient->m_Sta);
	LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	LeftView.HSplitTop(105.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Attack: %d%%", m_pClient->m_Str+99);
	LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	LeftView.HSplitTop(40.0f, &Label, &LeftView);
	str_format(aBuf, sizeof(aBuf), "Defence: %d%%", m_pClient->m_Sta+99);
	LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	// right site
	RightView.HSplitTop(90.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Exp: %.2f%%", m_pClient->m_Exp);
	RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	RightView.HSplitTop(60.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Dex: %d", m_pClient->m_Dex);
	RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	RightView.HSplitTop(40.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Int: %d", m_pClient->m_Int);
	RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	RightView.HSplitTop(105.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Reload: %d%%", m_pClient->m_Dex+99);
	RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	RightView.HSplitTop(40.0f, &Label, &RightView);
	str_format(aBuf, sizeof(aBuf), "Ammo: %d%%", m_pClient->m_Int+99);
	RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
	UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	
	// dont do the rest if not needed
	if((m_pClient->m_Level < 20 || m_pClient->m_Weapon > -1) && m_pClient->m_Points < 1)
		return;	
	
	w = MainView.w/5-20;
	
	MainView.HSplitTop(410, 0, &MainView);
	
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
	{
		// render headline for weapon selection
		MainView.VSplitLeft(MainView.w/2-TextRender()->TextWidth(0, 40, "Weapon choice", -1)/2, 0, &Label);
		UI()->DoLabel(&Label, "Weapon choice", 40.0f, -1);
	}
	
	if(m_pClient->m_Points > 0)
	{
		// render headline for stat selection
		MainView.HSplitTop(Offset, 0, &MainView);
		char aStats[32];
		str_format(aStats, sizeof(aStats), "Stat choice (Points: %d)", m_pClient->m_Points);
		MainView.VSplitLeft(MainView.w/2-TextRender()->TextWidth(0, 40, aStats, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aStats, 40.0f, -1);
	}

	// render key info
	TextRender()->Text(0, MainView.x+MainView.w-TextRender()->TextWidth(0, 20.0f, "Use left and right mouse button to choose.", -1), 213+h, 20.0f, "Use left and right mouse button to choose.", -1);
	
	// cursor stuff
	int LineCount = 0;
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
		LineCount = 1;
	if(m_pClient->m_Points > 0)
		LineCount += 2;
		
	int NumEntries = 0;
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
		NumEntries = 5;
	if(m_pClient->m_Points > 0)
		NumEntries += 4;
		
	// get the active line
	if(m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
	{
		int CursorPosY = (int)m_pClient->m_pControls->m_TargetPos.y;
		if(CursorPosY >= m_StartY)
		{
			m_CurrentLine = ((CursorPosY - m_StartY) / 32);
			
			if(m_CurrentLine >= LineCount)
			{
				m_StartY = CursorPosY - (LineCount*32);
				m_CurrentLine = LineCount-1;
			}
		}
		else
			m_StartY = CursorPosY;
	}
	else
	{
		int CameraPosY = (int)m_pClient->m_pCamera->m_Center.y;
		if(CameraPosY >= m_StartY)
		{
			m_CurrentLine = ((CameraPosY - m_StartY) / 32);
				
			if(m_CurrentLine >= LineCount)
			{
				m_StartY = CameraPosY - (LineCount*32);
				m_CurrentLine = LineCount-1;
			}
		}
		else
			m_StartY = CameraPosY;
	}
	
	int RowCount = 0;
	if(!m_CurrentLine && m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
		RowCount = 5;
	else
		RowCount = 2;
		
	// set the current row
	if(m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
	{
		int CursorPosX = (int)m_pClient->m_pControls->m_TargetPos.x;
		if(CursorPosX >= m_StartX)
		{
			m_CurrentRow = ((CursorPosX - m_StartX) / 32);
			
			if(m_CurrentRow >= RowCount)
			{
				m_StartX = CursorPosX - (RowCount*32);
				m_CurrentRow = RowCount-1;
			}
		}
		else
			m_StartX = CursorPosX;
	}
	else
	{
		int CameraPosX = (int)m_pClient->m_pCamera->m_Center.x;
		if(CameraPosX >= m_StartX)
		{
			m_CurrentRow = ((CameraPosX - m_StartX) / 32);
			
			if(m_CurrentRow >= RowCount)
			{
				m_StartX = CameraPosX - (RowCount*32);
				m_CurrentRow = RowCount-1;
			}
		}
		else
			m_StartX = CameraPosX;
	}
	
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0 && m_CurrentLine == 0)
	{
		x = MainView.x+10;
		for(int i = 0; i < 5; i++)
		{		
			if(i == m_CurrentRow)
			{
				// background so it's easy to find the local player
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1,1,1,0.25f);
				RenderTools()->DrawRoundRect(x, 710, w, 60, 17.0f);
				Graphics()->QuadsEnd();
			}
			
			x += w+20;
		}
	}
	else if(m_pClient->m_Points > 0)
	{
		x = MainView.x+10;
		w = MainView.w/2-20;
		int y = 800+m_CurrentLine*80;
		if(m_pClient->m_Level < 20 || m_pClient->m_Weapon > -1) // ugly
			y = 710+m_CurrentLine*80;
		
		for(int i = 0; i < 2; i++)
		{		
			if(i == m_CurrentRow)
			{
				// background so it's easy to find the local player
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1,1,1,0.25f);
				RenderTools()->DrawRoundRect(x, y, w, 60, 17.0f);
				Graphics()->QuadsEnd();
			}
			
			x += w+20;
		}
	}
	
	// render the stuff in buton here so its over the cursor
	// render the weapons
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
	{
		if(m_pClient->m_Points < 1) // ugly
			MainView.HSplitTop(Offset, 0, &MainView);
		vec2 WeaponPos = vec2(MainView.x+62.5, (MainView.y-60));
		for(int i = 0; i < NUM_WEAPONS-1; i++)
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[i].m_pSpriteBody);
			RenderTools()->DrawSprite(WeaponPos.x, WeaponPos.y, g_pData->m_Weapons.m_aId[i].m_VisualSize);
						
			Graphics()->QuadsEnd();
			
			WeaponPos.x += MainView.w/5;
		}
	}
	
	if(m_pClient->m_Points > 0)
	{
		// stats button text
		LeftView.HSplitTop(60+Offset, 0, &LeftView);
		LeftView.HSplitTop(105.0f, &Label, &LeftView);
		str_format(aBuf, sizeof(aBuf), "Add Str", m_pClient->m_Str+99);
		LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 30.0f, -1);
		
		LeftView.HSplitTop(80.0f, &Label, &LeftView);
		str_format(aBuf, sizeof(aBuf), "Add Sta", m_pClient->m_Sta+99);
		LeftView.VSplitLeft(LeftView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 30.0f, -1);
		
		RightView.HSplitTop(60+Offset, 0, &RightView);
		RightView.HSplitTop(105.0f, &Label, &RightView);
		str_format(aBuf, sizeof(aBuf), "Add Dex", m_pClient->m_Str+99);
		RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 30.0f, -1);
		
		RightView.HSplitTop(80.0f, &Label, &RightView);
		str_format(aBuf, sizeof(aBuf), "Add Int", m_pClient->m_Sta+99);
		RightView.VSplitLeft(RightView.w/2-TextRender()->TextWidth(0, 30.0f, aBuf, -1)/2, 0, &Label);
		UI()->DoLabel(&Label, aBuf, 30.0f, -1);
	}
	
	// get the marked item
	if(m_pClient->m_Level >= 20 && m_pClient->m_Weapon < 0)
	{
		if(!m_CurrentLine)
			m_Marked = m_CurrentRow;
		else if(m_CurrentLine == 1)
			m_Marked = 5 + m_CurrentRow;
		else
			m_Marked = 7 + m_CurrentRow;
	}
	else if(m_pClient->m_Points > 0)
		m_Marked = 5 + (m_CurrentLine*RowCount+m_CurrentRow);
		
	// finally send the message
	SendStatMsg(m_Marked);
}

void CStatboard::SendStatMsg(int Num)
{
	if(Input()->KeyPressed(KEY_MOUSE_1) && Input()->KeyPressed(KEY_MOUSE_2))
	{
		if(Client()->GameTick() > m_LastRequest+Client()->GameTickSpeed()/12)
		{
			// ugly but we need to change those values
			if(Num == 6)
				Num = 7;
			else if(Num == 7)
				Num = 6;
				
			// send msg
			CNetMsg_Cl_Stats Msg;
			Msg.m_Stat = Num;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}
		m_LastRequest = Client()->GameTick();
	}
}
void CStatboard::OnRender()
{
	if(!m_pClient->m_IsLvlx || !m_pClient->m_LoggedIn)
		return;

	// if we activly wanna look on the scoreboard	
	if(m_Active && !m_pClient->m_pCoopboard->m_Active && m_pClient->m_Snap.m_pGameobj && !m_pClient->m_Snap.m_pGameobj->m_GameOver)
	{
		if(!m_StatJustActivated)
		{
			if(m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Active)
			{
				m_StartX = (int)m_pClient->m_pControls->m_TargetPos.x;
				m_StartY = (int)m_pClient->m_pControls->m_TargetPos.y;
			}
			else
			{
				m_StartX = (int)m_pClient->m_pCamera->m_Center.x;
				m_StartY = (int)m_pClient->m_pCamera->m_Center.y;
			}
		}
		
		m_StatJustActivated = true;
		
		// clear motd
		m_pClient->m_pMotd->Clear();
		
		RenderStatboard();
	}
	else if(m_StatJustActivated)
		m_StatJustActivated = false;
}
