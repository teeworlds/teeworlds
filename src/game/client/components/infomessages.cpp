/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "infomessages.h"

#include "chat.h"
#include "skins.h"

void CInfoMessages::OnReset()
{
	m_InfoMsgCurrent = 0;
	for(int i = 0; i < MAX_INFOMSGS; i++)
		m_aInfoMsgs[i].m_Tick = -100000;
}

void CInfoMessages::AddInfoMsg(int Type, CInfoMsg NewMsg)
{
	NewMsg.m_Type = Type;
	NewMsg.m_Tick = Client()->GameTick();

	m_InfoMsgCurrent = (m_InfoMsgCurrent+1)%MAX_INFOMSGS;
	m_aInfoMsgs[m_InfoMsgCurrent] = NewMsg;
}

void CInfoMessages::OnMessage(int MsgType, void *pRawMsg)
{
	bool Race = m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE;

	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		if(Race && m_pClient->m_Snap.m_pGameDataRace && m_pClient->m_Snap.m_pGameDataRace->m_RaceFlags&RACEFLAG_HIDE_KILLMSG)
			return;

		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;

		// unpack messages
		CInfoMsg Kill;
		Kill.m_Player1ID = pMsg->m_Victim;
		str_format(Kill.m_aPlayer1Name, sizeof(Kill.m_aPlayer1Name), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[Kill.m_Player1ID].m_aName : "");
		Kill.m_Player1RenderInfo = m_pClient->m_aClients[Kill.m_Player1ID].m_RenderInfo;

		Kill.m_Player2ID = pMsg->m_Killer;
		if (Kill.m_Player2ID >= 0)
		{
			str_format(Kill.m_aPlayer2Name, sizeof(Kill.m_aPlayer2Name), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[Kill.m_Player2ID].m_aName : "");
			Kill.m_Player2RenderInfo = m_pClient->m_aClients[Kill.m_Player2ID].m_RenderInfo;
		}
		else
		{
			bool IsTeamplay = (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) != 0;
			int KillerTeam = - 1 - Kill.m_Player2ID;
			Kill.m_aPlayer2Name[0] = 0;
			int Skin = m_pClient->m_pSkins->Find("dummy", false);
			if (Skin != -1)
			{
				const CSkins::CSkin *pDummy = m_pClient->m_pSkins->Get(Skin);
				for(int p = 0; p < NUM_SKINPARTS; p++)
				{
					Kill.m_Player2RenderInfo.m_aTextures[p] = pDummy->m_apParts[p]->m_OrgTexture;
					if(IsTeamplay)
					{
						int ColorVal = m_pClient->m_pSkins->GetTeamColor(0, 0x000000, KillerTeam, p);
						Kill.m_Player2RenderInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(ColorVal, p==SKINPART_MARKING);
					}
					else
						Kill.m_Player2RenderInfo.m_aColors[p] = m_pClient->m_pSkins->GetColorV4(0x000000, p==SKINPART_MARKING);
					Kill.m_Player2RenderInfo.m_aColors[p].a *= .5f;
				}
				Kill.m_Player2RenderInfo.m_Size = 64.0f;
			}
		}

		Kill.m_Weapon = pMsg->m_Weapon;
		Kill.m_ModeSpecial = pMsg->m_ModeSpecial;
		Kill.m_FlagCarrierBlue = m_pClient->m_Snap.m_pGameDataFlag ? m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue : -1;

		AddInfoMsg(INFOMSG_KILL, Kill);
	}
	else if(MsgType == NETMSGTYPE_SV_RACEFINISH && Race)
	{
		CNetMsg_Sv_RaceFinish *pMsg = (CNetMsg_Sv_RaceFinish *)pRawMsg;

		char aBuf[256];
		char aTime[32];
		char aLabel[64];

		FormatTime(aTime, sizeof(aTime), pMsg->m_Time, m_pClient->RacePrecision());
		m_pClient->GetPlayerLabel(aLabel, sizeof(aLabel), pMsg->m_ClientID, m_pClient->m_aClients[pMsg->m_ClientID].m_aName);

		str_format(aBuf, sizeof(aBuf), "%2d: %s: finished in %s", pMsg->m_ClientID, m_pClient->m_aClients[pMsg->m_ClientID].m_aName, aTime);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "race", aBuf);

		if(pMsg->m_RecordType != RECORDTYPE_NONE)
		{
			if(pMsg->m_RecordType == RECORDTYPE_MAP)
				str_format(aBuf, sizeof(aBuf), Localize("'%s' has set a new map record: %s"), aLabel, aTime);
			else // RECORDTYPE_PLAYER
				str_format(aBuf, sizeof(aBuf), Localize("'%s' has set a new personal record: %s"), aLabel, aTime);
			
			if(pMsg->m_Diff < 0)
			{
				char aImprovement[64];
				FormatTimeDiff(aTime, sizeof(aTime), absolute(pMsg->m_Diff), m_pClient->RacePrecision(), false);
				str_format(aImprovement, sizeof(aImprovement), Localize(" (%s seconds faster)"), aTime);
				str_append(aBuf, aImprovement, sizeof(aBuf));
			}

			m_pClient->m_pChat->AddLine(aBuf);
		}

		if(m_pClient->m_Snap.m_pGameDataRace && m_pClient->m_Snap.m_pGameDataRace->m_RaceFlags&RACEFLAG_FINISHMSG_AS_CHAT)
		{
			if(pMsg->m_RecordType == RECORDTYPE_NONE) // don't print the time twice
			{
				str_format(aBuf, sizeof(aBuf), Localize("'%s' finished in: %s"), aLabel, aTime);
				m_pClient->m_pChat->AddLine(aBuf);
			}
		}
		else
		{
			CInfoMsg Finish;
			Finish.m_Player1ID = pMsg->m_ClientID;
			str_format(Finish.m_aPlayer1Name, sizeof(Finish.m_aPlayer1Name), "%s", g_Config.m_ClShowsocial ? m_pClient->m_aClients[pMsg->m_ClientID].m_aName : "");
			Finish.m_Player1RenderInfo = m_pClient->m_aClients[Finish.m_Player1ID].m_RenderInfo;

			Finish.m_Time = pMsg->m_Time;
			Finish.m_Diff = pMsg->m_Diff;
			Finish.m_RecordType = pMsg->m_RecordType;

			AddInfoMsg(INFOMSG_FINISH, Finish);
		}
	}
}

void CInfoMessages::OnRender()
{
	if(!g_Config.m_ClShowhud)
		return;

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;

	for(int i = 1; i <= MAX_INFOMSGS; i++)
	{
		const CInfoMsg *pInfoMsg = &m_aInfoMsgs[(m_InfoMsgCurrent+i)%MAX_INFOMSGS];
		if(Client()->GameTick() > pInfoMsg->m_Tick+50*10)
			continue;

		if(pInfoMsg->m_Type == INFOMSG_KILL)
			RenderKillMsg(pInfoMsg, StartX, y);
		else if(pInfoMsg->m_Type == INFOMSG_FINISH)
			RenderFinishMsg(pInfoMsg, StartX, y);

		y += 46.0f;
	}
}

void CInfoMessages::RenderKillMsg(const CInfoMsg *pInfoMsg, float x, float y) const
{
	float FontSize = 36.0f;
	float KillerNameW = TextRender()->TextWidth(0, FontSize, pInfoMsg->m_aPlayer2Name, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);
	float VictimNameW = TextRender()->TextWidth(0, FontSize, pInfoMsg->m_aPlayer1Name, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);

	// render victim name
	x -= VictimNameW;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

	RenderTools()->DrawClientID(TextRender(), &Cursor, pInfoMsg->m_Player1ID);
	TextRender()->TextEx(&Cursor, pInfoMsg->m_aPlayer1Name, -1);

	// render victim tee
	x -= 24.0f;

	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
	{
		if(pInfoMsg->m_ModeSpecial&1)
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			if(pInfoMsg->m_Player1ID == pInfoMsg->m_FlagCarrierBlue)
				RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);
			else
				RenderTools()->SelectSprite(SPRITE_FLAG_RED);

			float Size = 56.0f;
			IGraphics::CQuadItem QuadItem(x, y-16, Size/2, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
	}

	RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfoMsg->m_Player1RenderInfo, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28));
	x -= 32.0f;

	// render weapon
	x -= 44.0f;
	if (pInfoMsg->m_Weapon >= 0)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[pInfoMsg->m_Weapon].m_pSpriteBody);
		RenderTools()->DrawSprite(x, y+28, 96);
		Graphics()->QuadsEnd();
	}
	x -= 52.0f;

	if(pInfoMsg->m_Player1ID != pInfoMsg->m_Player2ID)
	{
		if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS)
		{
			if(pInfoMsg->m_ModeSpecial&2)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				if(pInfoMsg->m_Player2ID == pInfoMsg->m_FlagCarrierBlue)
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
		RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfoMsg->m_Player2RenderInfo, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28));
		x -= 32.0f;

		if(pInfoMsg->m_Player2ID >= 0)
		{
			// render killer name
			x -= KillerNameW;
			TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

			RenderTools()->DrawClientID(TextRender(), &Cursor, pInfoMsg->m_Player2ID);

			TextRender()->TextEx(&Cursor, pInfoMsg->m_aPlayer2Name, -1);
		}
	}
}

void CInfoMessages::RenderFinishMsg(const CInfoMsg *pInfoMsg, float x, float y) const
{
	float FontSize = 36.0f;
	float PlayerNameW = TextRender()->TextWidth(0, FontSize, pInfoMsg->m_aPlayer1Name, -1, -1.0f) + RenderTools()->GetClientIdRectSize(FontSize);

	// render diff
	if(pInfoMsg->m_Diff != 0)
	{
		char aBuf[32];
		char aDiff[32];
		FormatTimeDiff(aDiff, sizeof(aDiff), pInfoMsg->m_Diff, m_pClient->RacePrecision());
		str_format(aBuf, sizeof(aBuf), "(%s)", aDiff);
		float DiffW = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);

		x -= DiffW;
		if(pInfoMsg->m_Diff < 0)
			TextRender()->TextColor(0.5f, 1.0f, 0.5f, 1.0f);
		else
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, 1.0f);
		TextRender()->Text(0, x, y, FontSize, aBuf, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		x -= 16.0f;
	}

	// render time
	char aTime[32];
	FormatTime(aTime, sizeof(aTime), pInfoMsg->m_Time, m_pClient->RacePrecision());
	float TimeW = TextRender()->TextWidth(0, FontSize, aTime, -1, -1.0f);

	x -= TimeW;
	if(pInfoMsg->m_RecordType == RECORDTYPE_PLAYER)
		TextRender()->TextColor(0.2f, 0.6f, 1.0f, 1.0f);
	else if(pInfoMsg->m_RecordType == RECORDTYPE_MAP)
		TextRender()->TextColor(1.0f, 0.5f, 0.0f, 1.0f);
	TextRender()->Text(0, x, y, FontSize, aTime, -1);

	x -= 52.0f + 10.0f;

	// render flag
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_RACEFLAG].m_Id);
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem QuadItem(x, y, 52, 52);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	x -= 10.0f;

	// render player name
	x -= PlayerNameW;
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);

	RenderTools()->DrawClientID(TextRender(), &Cursor, pInfoMsg->m_Player1ID);
	TextRender()->TextEx(&Cursor, pInfoMsg->m_aPlayer1Name, -1);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	x -= 28.0f;

	// render player tee
	int Emote = pInfoMsg->m_RecordType != RECORDTYPE_NONE ? EMOTE_HAPPY : EMOTE_NORMAL;
	RenderTools()->RenderTee(CAnimState::GetIdle(), &pInfoMsg->m_Player1RenderInfo, Emote, vec2(-1,0), vec2(x, y+28));
}
