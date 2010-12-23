/* (c) Rajh. */

#include <cstdio>

#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/effects.h>
#include <game/client/components/skins.h>

#include "ghost.h"

/*
Note:
Freezing fucks up the ghost
the ghost isnt really sync
don't really get the client tick system for prediction
can used PrevChar and PlayerChar and it would be fluent and accurate but won't be predicted
so it will be affected by lags
*/

CGhost::CGhost()
{
	//TODO: Load a bestPath from a file ?
	m_CurPath.clear();
	m_BestPath.clear();
	m_CurPos = 0;
	m_Recording = false;
	m_Rendering = false;
	m_RaceState = RACE_NONE;
	m_NewRecord = false;
	m_PrevTime = -1;
	m_StartRenderTick = -1;
	m_StartRecordTick = -1;
}

void CGhost::SetGeneralInfos(CNetObj_Character Player, CTeeRenderInfo RenderInfo, CAnimState AnimState)
{
	if(!m_Recording)
		return;

	m_CurrentInfos.Player = Player;
	m_CurrentInfos.RenderInfo = RenderInfo;
	m_CurrentInfos.AnimState = AnimState;
	AddInfos();
}

void CGhost::AddInfos()
{
	if(!m_Recording)
		return;

	// Max 10 minutes
	// Just to be sure it doesnt eat too much memory, the first test should be enough anyway
	if((Client()->GameTick()-m_StartRecordTick) > Client()->GameTickSpeed()*60*10 || m_CurPath.size() > 50*15*60)
	{
		dbg_msg("ghost","10 minutes elapsed. Stopping ghost record");
		StopRecord();
		m_CurPath.clear();
		return;
	}
	
	//TODO: I don't know what the fuck is happening atm
	if(m_CurPath.size() == 0)
	{
		m_CurPath.add(m_CurrentInfos);
		m_CurPath.add(m_CurrentInfos);
		m_CurPath.add(m_CurrentInfos);
	}
	m_CurPath.add(m_CurrentInfos);
}

void CGhost::OnRender()
{
	// only for race
	if(!m_pClient->m_IsRace || !g_Config.m_ClGhost)
		return;

	// Check if the race line is crossed then start the render of the ghost if one
	//TODO: Is it working for high speed ?
	if(m_RaceState != RACE_STARTED && (m_pClient->Collision()->GetCollisionRace(m_pClient->Collision()->GetIndex(m_pClient->m_LocalCharacterPos, m_pClient->m_LocalCharacterPos)) == TILE_BEGIN))
	{
		//dbg_msg("ghost","race started");
		m_RaceState = RACE_STARTED;
		StartRender();
		StartRecord();
	}

	if(m_RaceState == RACE_FINISHED)
	{
		if(m_NewRecord || m_BestPath.size() == 0)
		{
			//dbg_msg("ghost","new path saved");
			m_NewRecord = false;
			m_BestPath = m_CurPath;
		}
		StopRecord();
		StopRender();
		m_RaceState = RACE_NONE;
	}

	// Play the ghost
	if(!m_Rendering)
		return;
	
	m_CurPos = Client()->GameTick()-m_StartRenderTick;

	if(m_BestPath.size() == 0 || m_CurPos < 0 || m_CurPos >= m_BestPath.size())
	{
		//dbg_msg("ghost","Ghost path done");
		m_Rendering = false;
		return;
	}

	RenderGhostHook();
	RenderGhostWeapon();
	RenderGhost();
}

void CGhost::RenderGhost()
{
	CNetObj_Character Player = m_BestPath[m_CurPos].Player;
	CNetObj_Character Prev = m_BestPath[m_CurPos].Player;
	//TODO: Dunno sometimes it need m_CurPos-2 don't really get it
	if(m_CurPos > 0)
		Prev = m_BestPath[m_CurPos-1].Player;

	CAnimState State = m_BestPath[m_CurPos].AnimState;
	CTeeRenderInfo RenderInfo = m_BestPath[m_CurPos].RenderInfo;

	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, Client()->IntraGameTick())/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->IntraGameTick()*2);

	RenderInfo.m_ColorBody.a = 0.5f;
	RenderInfo.m_ColorFeet.a = 0.5f;
	// Render ghost
	RenderTools()->RenderTee(&State, &RenderInfo, 0, Direction, Position);
}

void CGhost::RenderGhostWeapon()
{
	CNetObj_Character Player = m_BestPath[m_CurPos].Player;
	CNetObj_Character Prev = m_BestPath[m_CurPos].Player;
	if(m_CurPos > 0)
		Prev = m_BestPath[m_CurPos-1].Player;
	CAnimState State = m_BestPath[m_CurPos].AnimState;
	CTeeRenderInfo RenderInfo = m_BestPath[m_CurPos].RenderInfo;

	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, Client()->IntraGameTick())/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->IntraGameTick()*2);

	if (Player.m_Weapon != WEAPON_GRENADE)
		return;

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetRotation(State.GetAttach()->m_Angle*pi*2+Angle);

	// normal weapons
	int iw = clamp(Player.m_Weapon, 0, NUM_WEAPONS-1);
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_pSpriteBody, Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

	float Recoil = 0.0f;
	vec2 p;
	Recoil = 0;
	//TODO: Handle recoil
	/*float a = (m_StartTick+(Client()->GameTick()-m_RecordStartTick)-Player.m_AttackTick+Client()->IntraGameTick())/5.0f;
	if(a < 1)
		Recoil = sinf(a*pi);*/
	p = Position + Direction * g_pData->m_Weapons.m_aId[iw].m_Offsetx - Direction*Recoil*10.0f;
	p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
	RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
	Graphics()->QuadsEnd();
}

void CGhost::RenderGhostHook()
{
	CNetObj_Character Player = m_BestPath[m_CurPos].Player;
	CNetObj_Character Prev = m_BestPath[m_CurPos].Player;
	if(m_CurPos > 0)
		Prev = m_BestPath[m_CurPos-1].Player;

	if (Prev.m_HookState<=0 || Player.m_HookState<=0)
		return;

	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, Client()->IntraGameTick())/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Pos = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->IntraGameTick()*2);

	vec2 HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Player.m_HookX, Player.m_HookY), Client()->IntraGameTick());
	float d = distance(Pos, HookPos);
	vec2 Dir = normalize(Pos-HookPos);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetRotation(GetAngle(Dir)+pi);

	// render head
	RenderTools()->SelectSprite(SPRITE_HOOK_HEAD);
	IGraphics::CQuadItem QuadItem(HookPos.x, HookPos.y, 24,16);
	Graphics()->QuadsDraw(&QuadItem, 1);

	// render chain
	RenderTools()->SelectSprite(SPRITE_HOOK_CHAIN);
	IGraphics::CQuadItem Array[1024];
	int i = 0;
	for(float f = 24; f < d && i < 1024; f += 24, i++)
	{
		vec2 p = HookPos + Dir*f;
		Array[i] = IGraphics::CQuadItem(p.x, p.y,24,16);
	}

	Graphics()->QuadsDraw(Array, i);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();

}

void CGhost::StartRecord()
{
	m_Recording = true;
	m_CurPath.clear();
	m_StartRecordTick = Client()->GameTick();
}

void CGhost::StopRecord()
{
	m_Recording = false;
}

void CGhost::StartRender()
{
	m_CurPos = 0;
	m_Rendering = true;
	m_StartRenderTick = Client()->GameTick();
}

void CGhost::StopRender()
{
	m_Rendering = false;
}


void CGhost::ConGPlay(IConsole::IResult *pResult, void *pUserData)
{
	((CGhost *)pUserData)->StartRender();
}

void CGhost::OnConsoleInit()
{
	Console()->Register("gplay","", CFGFLAG_CLIENT, ConGPlay, this, "");
}

void CGhost::OnMessage(int MsgType, void *pRawMsg)
{
	if(!g_Config.m_ClGhost || m_pClient->m_Snap.m_Spectate)
		return;
	
	// only for race
	if(!m_pClient->m_IsRace)
		return;
		
	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalCid)
		{
			OnReset();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_Cid == -1 && m_RaceState == RACE_STARTED)
		{
			const char* pMessage = pMsg->m_pMessage;
			
			int Num = 0;
			while(str_comp_num(pMessage, " finished in: ", 14))
			{
				pMessage++;
				Num++;
				if(!pMessage[0])
					return;
			}
			
			// store the name
			char aName[64];
			str_copy(aName, pMsg->m_pMessage, Num+1);
			
			// prepare values and state for saving
			int Minutes;
			float Seconds;
			if(!str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalCid].m_aName) && sscanf(pMessage, " finished in: %d minute(s) %f", &Minutes, &Seconds) == 2)
			{
				m_RaceState = RACE_FINISHED;
				/*if(m_PrevTime != -1)
					dbg_msg("ghost","Finished, ghost time : %f", m_PrevTime);*/
				if(m_Recording)
				{
					float CurTime = Minutes*60 + Seconds;
					if(m_CurPos >= m_BestPath.size() && m_PrevTime > CurTime)
					{
						dbg_msg("ghost","ERROR : Ghost finished before with a worst score");
					}
					if(m_CurPos < m_BestPath.size() && m_PrevTime < CurTime)
					{
						dbg_msg("Ghost","ERROR : Ghost did not finish with a better score");
					}
					if(CurTime < m_PrevTime || m_PrevTime == -1)
					{
						m_NewRecord = true;
						m_PrevTime = CurTime;
					}
				}
			}
		}
	}
}

void CGhost::OnReset()
{
	StopRecord();
	StopRender();
	m_RaceState = RACE_NONE;
	m_NewRecord = false;
	m_CurPath.clear();
	m_StartRenderTick = -1;
	dbg_msg("ghost","Reset");
}

void CGhost::OnMapLoad()
{
	dbg_msg("ghost","MapLoad");
	OnReset();
	m_PrevTime = -1;
	m_BestPath.clear();
}
