/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "killmessages.h"

#include "skins.h"

void CKillMessages::OnReset()
{
	m_KillmsgCurrent = 0;
	for(int i = 0; i < MAX_KILLMSGS; i++)
		m_aKillmsgs[i].m_Tick = -100000;
}

void CKillMessages::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;

		// unpack messages
		CKillMsg Kill;
		Kill.m_VictimID = pMsg->m_Victim;
		Kill.m_VictimTeam = m_pClient->m_aClients[Kill.m_VictimID].m_Team;
		str_format(Kill.m_aVictimName, sizeof(Kill.m_aVictimName), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[Kill.m_VictimID].m_aName : "");
		Kill.m_VictimRenderInfo = m_pClient->m_aClients[Kill.m_VictimID].m_RenderInfo;

		Kill.m_KillerID = pMsg->m_Killer;
		if (Kill.m_KillerID >= 0)
		{
			Kill.m_KillerTeam = m_pClient->m_aClients[Kill.m_KillerID].m_Team;
			str_format(Kill.m_aKillerName, sizeof(Kill.m_aKillerName), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[Kill.m_KillerID].m_aName : "");
			Kill.m_KillerRenderInfo = m_pClient->m_aClients[Kill.m_KillerID].m_RenderInfo;
		}
		else
		{
			bool IsTeamplay = (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) != 0;
			Kill.m_KillerTeam = - 1 - Kill.m_KillerID;
			Kill.m_aKillerName[0] = 0;
			int Skin = m_pClient->m_pSkins->Find("dummy", false);
			if (Skin != -1)
			{
				const CSkins::CSkin *pDummy = m_pClient->m_pSkins->Get(Skin);
				for(int p = 0; p < NUM_SKINPARTS; p++)
				{
					Kill.m_KillerRenderInfo.m_aTextures[p] = pDummy->m_apParts[p]->m_OrgTexture;
					if(IsTeamplay)
					{
						int ColorVal = m_pClient->m_pSkins->GetTeamColor(0, 0x000000, Kill.m_KillerTeam, p);
						Kill.m_KillerRenderInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(ColorVal, p==SKINPART_MARKING);
					}
					else
						Kill.m_KillerRenderInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(0x000000, p==SKINPART_MARKING);
					Kill.m_KillerRenderInfo.m_aColors[p].a *= .5f;
				}
				Kill.m_KillerRenderInfo.m_Size = 64.0f;
			}
		}

		Kill.m_Weapon = pMsg->m_Weapon;
		Kill.m_ModeSpecial = pMsg->m_ModeSpecial;
		Kill.m_Tick = Client()->GameTick();

		Kill.m_FlagCarrierBlue = m_pClient->m_Snap.m_pGameDataFlag ? m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue : -1;

		// add the message
		m_KillmsgCurrent = (m_KillmsgCurrent+1)%MAX_KILLMSGS;
		m_aKillmsgs[m_KillmsgCurrent] = Kill;
	}
}

void CKillMessages::OnRender()
{
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;

	for(int i = 1; i <= MAX_KILLMSGS; i++)
	{
		int r = (m_KillmsgCurrent+i)%MAX_KILLMSGS;
		if(Client()->GameTick() > m_aKillmsgs[r].m_Tick+50*10)
			continue;

		float FontSize = 36.0f;
		float KillerNameW = TextRender()->TextWidth(0, FontSize, m_aKillmsgs[r].m_aKillerName, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);
		float VictimNameW = TextRender()->TextWidth(0, FontSize, m_aKillmsgs[r].m_aVictimName, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);

		float x = StartX;

		// render victim name
		x -= VictimNameW;
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

		RenderTools()->DrawClientID(TextRender(), &Cursor, m_aKillmsgs[r].m_VictimID);
		TextRender()->TextEx(&Cursor, m_aKillmsgs[r].m_aVictimName, -1);

		// render victim tee
		x -= 24.0f;

		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			if(m_aKillmsgs[r].m_ModeSpecial&1)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				if(m_aKillmsgs[r].m_VictimID == m_aKillmsgs[r].m_FlagCarrierBlue)
					RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);
				else
					RenderTools()->SelectSprite(SPRITE_FLAG_RED);

				float Size = 56.0f;
				IGraphics::CQuadItem QuadItem(x, y-16, Size/2, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}

		RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aKillmsgs[r].m_VictimRenderInfo, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28));
		x -= 32.0f;

		// render weapon
		x -= 44.0f;
		if (m_aKillmsgs[r].m_Weapon >= 0)
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_aKillmsgs[r].m_Weapon].m_pSpriteBody);
			RenderTools()->DrawSprite(x, y+28, 96);
			Graphics()->QuadsEnd();
		}
		x -= 52.0f;

		if(m_aKillmsgs[r].m_VictimID != m_aKillmsgs[r].m_KillerID)
		{
			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
			{
				if(m_aKillmsgs[r].m_ModeSpecial&2)
				{
					Graphics()->BlendNormal();
					Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
					Graphics()->QuadsBegin();

					if(m_aKillmsgs[r].m_KillerID == m_aKillmsgs[r].m_FlagCarrierBlue)
						RenderTools()->SelectSprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
					else
						RenderTools()->SelectSprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

					float Size = 56.0f;
					IGraphics::CQuadItem QuadItem(x-56, y-16, Size/2, Size);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}

			// render killer tee
			x -= 24.0f;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aKillmsgs[r].m_KillerRenderInfo, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28));
			x -= 32.0f;

			if(m_aKillmsgs[r].m_KillerID >= 0)
			{
				// render killer name
				x -= KillerNameW;
				TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

				RenderTools()->DrawClientID(TextRender(), &Cursor, m_aKillmsgs[r].m_KillerID);

				TextRender()->TextEx(&Cursor, m_aKillmsgs[r].m_aKillerName, -1);
			}

		}

		y += 46.0f;
	}
}
