/* (c) Rajh, Redix and Sushi. */

#include <engine/shared/config.h>
#include <engine/ghost.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/graphics.h>

#include <game/ghost.h>
#include <game/teerace.h>

#include "skins.h"
#include "menus.h"
#include "controls.h"
#include "players.h"
#include "ghost.h"

CGhost::CGhost()
	: m_StartRenderTick(-1),
	m_LastRecordTick(-1),
	m_LastDeathTick(-1),
	m_CurPos(0),
	m_Rendering(false),
	m_Recording(false)
{ }

void CGhost::AddInfos(CNetObj_Character Char)
{
	CGhostCharacter GhostChar = CGhostTools::GetGhostCharacter(Char);
	m_CurGhost.m_lPath.add(CGhostTools::GetNetObjCharacter(GhostChar));
	if(GhostRecorder()->IsRecording())
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_CHARACTER, (const char*)&GhostChar, sizeof(GhostChar));
}

int CGhost::GetSlot()
{
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		if(!m_aActiveGhosts[i].m_lPath.size())
			return i;
	return -1;
}

bool CGhost::IsStart(vec2 PrevPos, vec2 Pos)
{
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);

	int EnemyTeam = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Team ^ 1;
	int TilePos = m_pClient->Collision()->CheckRaceTile(PrevPos, Pos, CCollision::RACECHECK_TILES_MAIN);
	if(!IsFastCap(&ServerInfo) && m_pClient->Collision()->GetIndex(TilePos) == TILE_BEGIN)
		return true;
	if(IsFastCap(&ServerInfo) && m_pClient->m_aFlagPos[EnemyTeam] != vec2(-1, -1) && distance(Pos, m_pClient->m_aFlagPos[EnemyTeam]) < 32)
		return true;
	return false;
}

void CGhost::OnRender()
{
	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo) || !g_Config.m_ClRaceGhost || !m_pClient->m_Snap.m_pLocalCharacter || !m_pClient->m_Snap.m_pLocalPrevCharacter)
		return;

	// TODO: handle restart
	if(m_pClient->m_NewPredictedTick)
	{
		vec2 PrevPos = m_pClient->m_PredictedPrevChar.m_Pos;
		vec2 Pos = m_pClient->m_PredictedChar.m_Pos;
		if(!m_Rendering && IsStart(PrevPos, Pos))
			StartRender();
	}

	if(m_pClient->m_NewTick)
	{
		int PrevTick = m_pClient->m_Snap.m_pLocalPrevCharacter->m_Tick;
		vec2 PrevPos = vec2(m_pClient->m_Snap.m_pLocalPrevCharacter->m_X, m_pClient->m_Snap.m_pLocalPrevCharacter->m_Y);
		vec2 Pos = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_X, m_pClient->m_Snap.m_pLocalCharacter->m_Y);

		// detecting death, needed because teerace allows immediate respawning
		if(!m_Recording && IsStart(PrevPos, Pos) && (m_LastDeathTick == -1 || m_LastDeathTick < PrevTick))
				StartRecord();

		if(m_Recording)
		{
			// writing the tick into the file would be better than this
			// but it would require changes to the ghost file format
			CNetObj_Character NewChar = *m_pClient->m_Snap.m_pLocalCharacter;

			if(m_LastRecordTick != -1)
			{
				int NewTicks = Client()->GameTick() - m_LastRecordTick;
				CNetObj_Character LastRecordChar = m_CurGhost.m_lPath.all().back();
				if(LastRecordChar.m_Angle < 0 && NewChar.m_Angle > pi*256)
					LastRecordChar.m_Angle += 2*pi*256;
				else if(LastRecordChar.m_Angle > pi*256 && NewChar.m_Angle < 0)
					NewChar.m_Angle += 2*pi*256;
				// make sure that we have one item per tick
				for(int i = 1; i < NewTicks; i++)
				{
					float Intra = i / (float)NewTicks;
					CNetObj_Character TmpChar = LastRecordChar;
					vec2 Position = mix(vec2(TmpChar.m_X, TmpChar.m_Y), vec2(NewChar.m_X, NewChar.m_Y), Intra);
					vec2 HookPos = mix(vec2(TmpChar.m_HookX, TmpChar.m_HookY), vec2(NewChar.m_HookX, NewChar.m_HookY), Intra);
					TmpChar.m_X = round_to_int(Position.x);
					TmpChar.m_Y = round_to_int(Position.y);
					TmpChar.m_VelX = round_to_int(mix((float)TmpChar.m_VelX, (float)NewChar.m_VelX, Intra));
					TmpChar.m_VelY = round_to_int(mix((float)TmpChar.m_VelY, (float)NewChar.m_VelY, Intra));
					TmpChar.m_Angle = round_to_int(mix((float)TmpChar.m_Angle, (float)NewChar.m_Angle, Intra));
					TmpChar.m_HookX = round_to_int(HookPos.x);
					TmpChar.m_HookY = round_to_int(HookPos.y);
					AddInfos(TmpChar);
				}
			}

			m_LastRecordTick = Client()->GameTick();
			AddInfos(NewChar);
		}
	}

	// Play the ghost
	if(!m_Rendering)
		return;

	m_CurPos = Client()->PredGameTick()-m_StartRenderTick;
	if(m_CurPos < 0)
	{
		StopRender();
		return;
	}

	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
	{
		CGhostItem *pGhost = &m_aActiveGhosts[i];
		if(m_CurPos >= pGhost->m_lPath.size())
			continue;

		int PrevPos = (m_CurPos > 0) ? m_CurPos-1 : m_CurPos;
		CNetObj_Character Player = pGhost->m_lPath[m_CurPos];
		CNetObj_Character Prev = pGhost->m_lPath[PrevPos];

		if(!m_TickDiff[i] && Player.m_AttackTick != Prev.m_AttackTick)
			m_TickDiff[i] = Client()->GameTick() - Player.m_AttackTick;
		Player.m_AttackTick += m_TickDiff[i];

		m_pClient->m_pPlayers->RenderHook(&Prev, &Player, &pGhost->m_RenderInfo, -2);
		m_pClient->m_pPlayers->RenderPlayer(&Prev, &Player, &pGhost->m_RenderInfo, -2);
		RenderGhostNamePlate(Prev, Player, pGhost->m_aOwner);
	}
}

void CGhost::RenderGhostNamePlate(CNetObj_Character Prev, CNetObj_Character Player, const char *pName)
{
	if(!g_Config.m_ClGhostNamePlates)
		return;

	float IntraTick = Client()->PredIntraGameTick();

	vec2 Pos = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);

	float FontSize = 18.0f + 20.0f * g_Config.m_ClNameplatesSize / 100.0f;

	// render name plate
	float a = 0.5f;
	if(g_Config.m_ClGhostNameplatesAlways == 0)
		a = clamp(0.5f-powf(distance(m_pClient->m_pControls->m_TargetPos, Pos)/200.0f,16.0f), 0.0f, 0.5f);

	float tw = TextRender()->TextWidth(0, FontSize, pName, -1);
	
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.5f*a);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, a);
	TextRender()->Text(0, Pos.x-tw/2.0f, Pos.y-FontSize-38.0f, FontSize, pName, -1);

	// reset color;
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
}

void CGhost::InitRenderInfos(CTeeRenderInfo *pRenderInfo, const char *pSkinName, int UseCustomColor, int ColorBody, int ColorFeet)
{
	int SkinId = m_pClient->m_pSkins->Find(pSkinName);
	if(SkinId < 0)
	{
		SkinId = m_pClient->m_pSkins->Find("default");
		if(SkinId < 0)
			SkinId = 0;
	}

	pRenderInfo->m_ColorBody = m_pClient->m_pSkins->GetColorV4(ColorBody);
	pRenderInfo->m_ColorFeet = m_pClient->m_pSkins->GetColorV4(ColorFeet);

	if(UseCustomColor)
		pRenderInfo->m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_ColorTexture;
	else
	{
		pRenderInfo->m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_OrgTexture;
		pRenderInfo->m_ColorBody = vec4(1,1,1,1);
		pRenderInfo->m_ColorFeet = vec4(1,1,1,1);
	}

	pRenderInfo->m_ColorBody.a = 0.5f;
	pRenderInfo->m_ColorFeet.a = 0.5f;
	pRenderInfo->m_Size = 64;
}

void CGhost::StartRecord()
{
	m_Recording = true;
	m_LastRecordTick = -1;
	m_CurGhost.m_lPath.clear();
	CGameClient::CClientData ClientData = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID];
	str_copy(m_CurGhost.m_aOwner, g_Config.m_PlayerName, sizeof(m_CurGhost.m_aOwner));
	InitRenderInfos(&m_CurGhost.m_RenderInfo, ClientData.m_aSkinName, ClientData.m_UseCustomColor, ClientData.m_ColorBody, ClientData.m_ColorFeet);

	if(g_Config.m_ClRaceSaveGhost)
	{
		Client()->GhostRecorder_Start();

		CGhostSkin Skin;
		StrToInts(&Skin.m_Skin0, 6, ClientData.m_aSkinName);
		Skin.m_UseCustomColor = ClientData.m_UseCustomColor;
		Skin.m_ColorBody = ClientData.m_ColorBody;
		Skin.m_ColorFeet = ClientData.m_ColorFeet;
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_SKIN, (const char*)&Skin, sizeof(Skin));
	}
}

void CGhost::StopRecord(int Time)
{
	m_Recording = false;
	bool RecordingToFile = GhostRecorder()->IsRecording();

	if(RecordingToFile)
		GhostRecorder()->Stop(m_CurGhost.m_lPath.size(), Time);

	char aTmpFilename[128];
	Client()->Ghost_GetPath(aTmpFilename, sizeof(aTmpFilename));

	CMenus::CGhostItem *pOwnGhost = m_pClient->m_pMenus->GetOwnGhost();
	if(Time && (!pOwnGhost || Time < pOwnGhost->m_Time))
	{
		// add to active ghosts
		int Slot;
		if(pOwnGhost)
			Slot = pOwnGhost->m_Slot;
		else
			Slot = GetSlot();

		if(Slot != -1)
			m_aActiveGhosts[Slot] = m_CurGhost;

		char aFilename[128] = { 0 };
		if(RecordingToFile)
		{
			// remove old ghost
			if(pOwnGhost && pOwnGhost->HasFile())
				Storage()->RemoveFile(pOwnGhost->m_aFilename, IStorage::TYPE_SAVE);

			// save new ghost
			Client()->Ghost_GetPath(aFilename, sizeof(aFilename), Time);
			Storage()->RenameFile(aTmpFilename, aFilename, IStorage::TYPE_SAVE);
		}

		// create ghost item
		CMenus::CGhostItem Item;
		str_copy(Item.m_aFilename, aFilename, sizeof(Item.m_aFilename));
		str_copy(Item.m_aPlayer, g_Config.m_PlayerName, sizeof(Item.m_aPlayer));
		Item.m_Time = Time;
		Item.m_Slot = Slot;
		Item.m_Own = true;

		// add item to menu list
		if(pOwnGhost)
			*pOwnGhost = Item;
		else
			m_pClient->m_pMenus->m_lGhosts.add(Item);
		m_pClient->m_pMenus->m_lGhosts.sort_range();
	}
	else if(RecordingToFile) // no new record
		Storage()->RemoveFile(aTmpFilename, IStorage::TYPE_SAVE);

	m_CurGhost.m_lPath.clear();
}

void CGhost::StartRender()
{
	m_CurPos = 0;
	m_Rendering = true;
	m_StartRenderTick = Client()->PredGameTick();
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		m_TickDiff[i] = 0;
}

void CGhost::StopRender()
{
	m_Rendering = false;
}

int CGhost::Load(const char* pFilename)
{
	if(!Client()->GhostLoader_Load(pFilename))
		return false;

	// read header
	CGhostHeader Header = GhostLoader()->GetHeader();

	int NumTicks = GhostLoader()->GetTicks(Header);
	int Time = GhostLoader()->GetTime(Header);
	if(NumTicks <= 0 || Time <= 0)
	{
		GhostLoader()->Close();
		return false;
	}

	// create ghost
	CGhostItem Ghost;
	Ghost.m_lPath.set_size(NumTicks);

	// read client info
	str_copy(Ghost.m_aOwner, Header.m_aOwner, sizeof(Ghost.m_aOwner));

	int Index = 0;
	bool FoundSkin = false;

	// read data
	int Type;
	while(GhostLoader()->ReadNextType(&Type))
	{
		if(Type == GHOSTDATA_TYPE_SKIN)
		{
			CGhostSkin Skin;
			if(GhostLoader()->ReadData(Type, (char*)&Skin, sizeof(Skin)) && !FoundSkin)
			{
				FoundSkin = true;
				char aSkinName[64];
				IntsToStr(&Skin.m_Skin0, 6, aSkinName);
				InitRenderInfos(&Ghost.m_RenderInfo, aSkinName, Skin.m_UseCustomColor, Skin.m_ColorBody, Skin.m_ColorFeet);
			}
		}
		else if(Type == GHOSTDATA_TYPE_CHARACTER)
		{
			CGhostCharacter Char;
			if(GhostLoader()->ReadData(Type, (char*)&Char, sizeof(Char)))
				Ghost.m_lPath[Index++] = CGhostTools::GetNetObjCharacter(Char);
		}
	}

	GhostLoader()->Close();

	if(Index != NumTicks)
		return false;

	if(!FoundSkin)
		InitRenderInfos(&Ghost.m_RenderInfo, "default", 0, 0, 0);
	int Slot = GetSlot();
	if(Slot != -1)
		m_aActiveGhosts[Slot] = Ghost;
	return Slot;
}

void CGhost::Unload(int Slot)
{
	m_aActiveGhosts[Slot].m_lPath.clear();
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
	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);

	if(!IsRace(&ServerInfo) || m_pClient->m_Snap.m_SpecInfo.m_Active)
		return;

	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID)
		{
			if(m_Recording)
				StopRecord();
			StopRender();
			m_LastDeathTick = Client()->GameTick();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1 && m_Recording)
		{
			char aName[MAX_NAME_LENGTH];
			int CurTime = IRace::TimeFromFinishMessage(pMsg->m_pMessage, aName, sizeof(aName));
			if(CurTime && str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) == 0)
			{
				StopRecord(CurTime);
				StopRender();
			}
		}
	}
}

void CGhost::OnReset()
{
	StopRecord();
	StopRender();
	m_LastDeathTick = -1;
}

void CGhost::OnShutdown()
{
	OnReset();
}

void CGhost::OnMapLoad()
{
	OnReset();
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		Unload(i);

	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);

	if(IsRace(&ServerInfo))
		m_pClient->m_pMenus->GhostlistPopulate();
}
