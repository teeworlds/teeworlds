/* (c) Rajh, Redix and Sushi. */

#include <engine/ghost.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/teerace.h>

#include "controls.h"
#include "players.h"
#include "skins.h"
#include "ghost.h"

CGhost::CGhost() : m_StartRenderTick(-1), m_LastDeathTick(-1), m_Recording(false), m_Rendering(false), m_SymmetricMap(false) {}

void CGhost::CGhostPath::Copy(const CGhostPath &Other)
{
	Reset(Other.m_ChunkSize);
	SetSize(Other.Size());
	for(int i = 0; i < m_lChunks.size(); i++)
		mem_copy(m_lChunks[i], Other.m_lChunks[i], sizeof(CGhostCharacter) * m_ChunkSize);
}

void CGhost::CGhostPath::Reset(int ChunkSize)
{
	for(int i = 0; i < m_lChunks.size(); i++)
		mem_free(m_lChunks[i]);
	m_lChunks.clear();
	m_ChunkSize = ChunkSize;
	m_NumItems = 0;
}

void CGhost::CGhostPath::SetSize(int Items)
{
	int Chunks = m_lChunks.size();
	int NeededChunks = (Items + m_ChunkSize - 1) / m_ChunkSize;

	if(NeededChunks > Chunks)
	{
		m_lChunks.set_size(NeededChunks);
		for(int i = Chunks; i < NeededChunks; i++)
			m_lChunks[i] = (CGhostCharacter*)mem_alloc(sizeof(CGhostCharacter) * m_ChunkSize, 1);
	}

	m_NumItems = Items;
}

void CGhost::CGhostPath::Add(CGhostCharacter Char)
{
	SetSize(m_NumItems + 1);
	*Get(m_NumItems - 1) = Char;
}

CGhostCharacter *CGhost::CGhostPath::Get(int Index)
{
	if(Index < 0 || Index >= m_NumItems)
		return 0;

	int Chunk = Index / m_ChunkSize;
	int Pos = Index % m_ChunkSize;
	return &m_lChunks[Chunk][Pos];
}

void CGhost::AddInfos(const CNetObj_Character *pChar)
{
	// do not start writing to file as long as we still touch the start line
	int NumTicks = m_CurGhost.m_Path.Size();
	if(g_Config.m_ClRaceSaveGhost && !GhostRecorder()->IsRecording() && NumTicks > 0)
	{
		Client()->GhostRecorder_Start(m_CurGhost.m_aPlayer);

		GhostRecorder()->WriteData(GHOSTDATA_TYPE_START_TICK, (const char*)&m_CurGhost.m_StartTick, sizeof(int));
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_SKIN, (const char*)&m_CurGhost.m_Skin, sizeof(CGhostSkin));
		for(int i = 0; i < NumTicks; i++)
			GhostRecorder()->WriteData(GHOSTDATA_TYPE_CHARACTER, (const char*)m_CurGhost.m_Path.Get(i), sizeof(CGhostCharacter));
	}

	CGhostCharacter GhostChar;
	CGhostTools::GetGhostCharacter(&GhostChar, pChar);
	m_CurGhost.m_Path.Add(GhostChar);
	if(GhostRecorder()->IsRecording())
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_CHARACTER, (const char*)&GhostChar, sizeof(CGhostCharacter));
}

int CGhost::GetSlot() const
{
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		if(m_aActiveGhosts[i].Empty())
			return i;
	return -1;
}

int CGhost::FreeSlot() const
{
	int Num = 0;
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		if(m_aActiveGhosts[i].Empty())
			Num++;
	return Num;
}

void CGhost::MirrorChar(CNetObj_Character *pChar, int Middle)
{
	pChar->m_HookDx = -pChar->m_HookDx;
	pChar->m_VelX = -pChar->m_VelX;
	pChar->m_HookX = 2 * Middle - pChar->m_HookX;
	pChar->m_X = 2 * Middle - pChar->m_X;
	pChar->m_Angle = -pChar->m_Angle - pi*256.f;
	pChar->m_Direction = -pChar->m_Direction;
}

void CGhost::OnRender()
{
	// only for race
	CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);
	if(!IsRace(&ServerInfo) || !g_Config.m_ClRaceGhost || !m_pClient->m_Snap.m_pGameInfoObj)
		return;

	if(m_pClient->m_Snap.m_pLocalCharacter && m_pClient->m_Snap.m_pLocalPrevCharacter)
	{
		bool RaceFlag = m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME;
		bool ServerControl = RaceFlag && g_Config.m_ClRaceGhostServerControl;
		int RaceTick = -m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer;

		static int s_NewRenderTick = -1;
		int RenderTick = s_NewRenderTick;

		if(!ServerControl && m_pClient->m_NewPredictedTick)
		{
			vec2 PrevPos = m_pClient->m_PredictedPrevChar.m_Pos;
			vec2 Pos = m_pClient->m_PredictedChar.m_Pos;
			bool Rendering = m_Rendering || RenderTick != -1;
			if((!Rendering || m_AllowRestart) && m_pClient->IsRaceStart(PrevPos, Pos))
				RenderTick = Client()->PredGameTick();
		}

		if(m_pClient->m_NewTick)
		{
			static int s_LastRaceTick = -1;

			if(ServerControl && s_LastRaceTick != RaceTick)
			{
				if(m_Recording && s_LastRaceTick != -1)
					m_AllowRestart = true;
				if(GhostRecorder()->IsRecording())
					GhostRecorder()->Stop(0, -1);
				int StartTick = RaceTick;
				if(IsDDRace(&ServerInfo))
					StartTick--;
				StartRecord(StartTick);
				RenderTick = StartTick;
			}
			else if(!ServerControl)
			{
				int PrevTick = m_pClient->m_Snap.m_pLocalPrevCharacter->m_Tick;
				int CurTick = m_pClient->m_Snap.m_pLocalCharacter->m_Tick;
				vec2 PrevPos = vec2(m_pClient->m_Snap.m_pLocalPrevCharacter->m_X, m_pClient->m_Snap.m_pLocalPrevCharacter->m_Y);
				vec2 Pos = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_X, m_pClient->m_Snap.m_pLocalCharacter->m_Y);

				// detecting death, needed because race allows immediate respawning
				if((!m_Recording || m_AllowRestart) && m_LastDeathTick < PrevTick)
				{
					// estimate the exact start tick
					int RecordTick = -1;
					int TickDiff = CurTick - PrevTick;
					for(int i = 0; i < TickDiff; i++)
					{
						if(m_pClient->IsRaceStart(mix(PrevPos, Pos, (float)i / TickDiff), mix(PrevPos, Pos, (float)(i + 1) / TickDiff)))
						{
							RecordTick = PrevTick + i + 1;
							if(!m_AllowRestart)
								break;
						}
					}
					if(RecordTick != -1)
					{
						if(GhostRecorder()->IsRecording())
							GhostRecorder()->Stop(0, -1);
						StartRecord(RecordTick);
					}
				}
			}

			if(m_Recording)
				AddInfos(m_pClient->m_Snap.m_pLocalCharacter);

			s_LastRaceTick = RaceFlag ? RaceTick : -1;
		}

		if((ServerControl && m_pClient->m_NewTick) || (!ServerControl && m_pClient->m_NewPredictedTick))
		{
			// only restart rendering if it did not change since last tick to prevent stuttering
			if(s_NewRenderTick != -1 && s_NewRenderTick == RenderTick)
			{
				StartRender(RenderTick);
				RenderTick = -1;
			}
			s_NewRenderTick = RenderTick;
		}
	}

	// Play the ghost
	if(!m_Rendering)
		return;

	int ActiveGhosts = 0;
	int PlaybackTick = Client()->PredGameTick() - m_StartRenderTick;

	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
	{
		CGhostItem *pGhost = &m_aActiveGhosts[i];
		if(pGhost->Empty())
			continue;

		bool End = false;
		int GhostTick = pGhost->m_StartTick + PlaybackTick;
		while(pGhost->m_Path.Get(pGhost->m_PlaybackPos)->m_Tick < GhostTick && !End)
		{
			if(pGhost->m_PlaybackPos < pGhost->m_Path.Size() - 1)
				pGhost->m_PlaybackPos++;
			else
				End = true;
		}

		if(End)
			continue;

		ActiveGhosts++;

		int CurPos = pGhost->m_PlaybackPos;
		int PrevPos = max(0, CurPos - 1);
		if(pGhost->m_Path.Get(PrevPos)->m_Tick > GhostTick)
			continue;

		CNetObj_Character Player, Prev;
		CGhostTools::GetNetObjCharacter(&Player, pGhost->m_Path.Get(CurPos));
		CGhostTools::GetNetObjCharacter(&Prev, pGhost->m_Path.Get(PrevPos));

		if(pGhost->m_Mirror && IsFastCap(&ServerInfo))
		{
			vec2 FlagPosRed = m_pClient->Collision()->GetPos(m_pClient->m_aFlagIndex[TEAM_RED]);
			vec2 FlagPosBlue = m_pClient->Collision()->GetPos(m_pClient->m_aFlagIndex[TEAM_BLUE]);
			int Middle = (FlagPosRed.x + FlagPosBlue.x) / 2;
			MirrorChar(&Player, Middle);
			MirrorChar(&Prev, Middle);
		}

		int TickDiff = Player.m_Tick - Prev.m_Tick;
		float IntraTick = 0.f;
		if(TickDiff > 0)
			IntraTick = (GhostTick - Prev.m_Tick - 1 + Client()->PredIntraGameTick()) / TickDiff;

		Player.m_AttackTick += Client()->GameTick() - GhostTick;

		m_pClient->m_pPlayers->RenderHook(&Prev, &Player, &pGhost->m_RenderInfo, -2, IntraTick);
		m_pClient->m_pPlayers->RenderPlayer(&Prev, &Player, &pGhost->m_RenderInfo, -2, IntraTick);
		if(g_Config.m_ClGhostNamePlates)
			RenderGhostNamePlate(&Prev, &Player, IntraTick, pGhost->m_aPlayer);
	}

	if(!ActiveGhosts)
		StopRender();
}

void CGhost::RenderGhostNamePlate(const CNetObj_Character *pPrev, const CNetObj_Character *pPlayer, float IntraTick, const char *pName)
{
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pPlayer->m_X, pPlayer->m_Y), IntraTick);
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

void CGhost::InitRenderInfos(CGhostItem *pGhost)
{
	char aSkinName[64];
	IntsToStr(&pGhost->m_Skin.m_Skin0, 6, aSkinName);
	CTeeRenderInfo *pRenderInfo = &pGhost->m_RenderInfo;

	int SkinId = m_pClient->m_pSkins->Find(aSkinName);
	if(SkinId < 0)
	{
		SkinId = m_pClient->m_pSkins->Find("default");
		if(SkinId < 0)
			SkinId = 0;
	}

	if(pGhost->m_Skin.m_UseCustomColor)
	{
		pRenderInfo->m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_ColorTexture;
		pRenderInfo->m_ColorBody = m_pClient->m_pSkins->GetColorV4(pGhost->m_Skin.m_ColorBody);
		pRenderInfo->m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pGhost->m_Skin.m_ColorFeet);
	}
	else
	{
		pRenderInfo->m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_OrgTexture;
		pRenderInfo->m_ColorBody = vec4(1, 1, 1, 1);
		pRenderInfo->m_ColorFeet = vec4(1, 1, 1, 1);
	}

	pRenderInfo->m_ColorBody.a = 0.5f;
	pRenderInfo->m_ColorFeet.a = 0.5f;
	pRenderInfo->m_Size = 64;
}

void CGhost::StartRecord(int Tick)
{
	m_Recording = true;
	m_CurGhost.Reset();
	m_CurGhost.m_StartTick = Tick;

	const CGameClient::CClientData *pData = &m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID];
	str_copy(m_CurGhost.m_aPlayer, g_Config.m_PlayerName, sizeof(m_CurGhost.m_aPlayer));
	CGhostTools::GetGhostSkin(&m_CurGhost.m_Skin, pData->m_aSkinName, pData->m_UseCustomColor, pData->m_ColorBody, pData->m_ColorFeet);
	InitRenderInfos(&m_CurGhost);
}

void CGhost::StopRecord(int Time)
{
	m_Recording = false;
	bool RecordingToFile = GhostRecorder()->IsRecording();

	if(RecordingToFile)
		GhostRecorder()->Stop(m_CurGhost.m_Path.Size(), Time);

	char aTmpFilename[128];
	Client()->Ghost_GetPath(aTmpFilename, sizeof(aTmpFilename), m_CurGhost.m_aPlayer);

	CMenus::CGhostItem *pOwnGhost = m_pClient->m_pMenus->GetOwnGhost();
	if(Time > 0 && (!pOwnGhost || Time < pOwnGhost->m_Time))
	{
		if(pOwnGhost && pOwnGhost->Active())
			Unload(pOwnGhost->m_Slot);

		// add to active ghosts
		int Slot = GetSlot();
		if(Slot != -1)
			m_aActiveGhosts[Slot] = m_CurGhost;

		// create ghost item
		CMenus::CGhostItem Item;
		if(RecordingToFile)
			Client()->Ghost_GetPath(Item.m_aFilename, sizeof(Item.m_aFilename), m_CurGhost.m_aPlayer, Time);
		str_copy(Item.m_aPlayer, m_CurGhost.m_aPlayer, sizeof(Item.m_aPlayer));
		Item.m_Time = Time;
		Item.m_Slot = Slot;

		// save new ghost file
		if(Item.HasFile())
			Storage()->RenameFile(aTmpFilename, Item.m_aFilename, IStorage::TYPE_SAVE);

		// add item to menu list
		m_pClient->m_pMenus->UpdateOwnGhost(Item);
	}
	else if(RecordingToFile) // no new record
		Storage()->RemoveFile(aTmpFilename, IStorage::TYPE_SAVE);

	m_CurGhost.Reset();
}

void CGhost::StartRender(int Tick)
{
	m_Rendering = true;
	m_StartRenderTick = Tick;
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		m_aActiveGhosts[i].m_PlaybackPos = 0;
}

void CGhost::StopRender()
{
	m_Rendering = false;
}

int CGhost::Load(const char *pFilename)
{
	int Slot = GetSlot();
	if(Slot == -1)
		return -1;

	if(!Client()->GhostLoader_Load(pFilename))
		return -1;

	const CGhostHeader *pHeader = GhostLoader()->GetHeader();

	int NumTicks = GhostLoader()->GetTicks(pHeader);
	int Time = GhostLoader()->GetTime(pHeader);
	if(NumTicks <= 0 || Time <= 0)
	{
		GhostLoader()->Close();
		return -1;
	}

	// select ghost
	CGhostItem *pGhost = &m_aActiveGhosts[Slot];
	pGhost->m_Path.SetSize(NumTicks);

	str_copy(pGhost->m_aPlayer, pHeader->m_aOwner, sizeof(pGhost->m_aPlayer));

	int Index = 0;
	bool FoundSkin = false;
	bool NoTick = false;
	bool Error = false;
	pGhost->m_StartTick = -1;

	int Type;
	while(!Error && GhostLoader()->ReadNextType(&Type))
	{
		if(Index == NumTicks && (Type == GHOSTDATA_TYPE_CHARACTER || Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK))
		{
			Error = true;
			break;
		}

		if(Type == GHOSTDATA_TYPE_SKIN && !FoundSkin)
		{
			FoundSkin = true;
			if(!GhostLoader()->ReadData(Type, (char*)&pGhost->m_Skin, sizeof(CGhostSkin)))
				Error = true;
		}
		else if(Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK)
		{
			NoTick = true;
			if(!GhostLoader()->ReadData(Type, (char*)pGhost->m_Path.Get(Index++), sizeof(CGhostCharacter_NoTick)))
				Error = true;
		}
		else if(Type == GHOSTDATA_TYPE_CHARACTER)
		{
			if(!GhostLoader()->ReadData(Type, (char*)pGhost->m_Path.Get(Index++), sizeof(CGhostCharacter)))
				Error = true;
		}
		else if(Type == GHOSTDATA_TYPE_START_TICK)
		{
			if(!GhostLoader()->ReadData(Type, (char*)&pGhost->m_StartTick, sizeof(int)))
				Error = true;
		}
	}

	GhostLoader()->Close();

	if(Error || Index != NumTicks)
	{
		pGhost->Reset();
		return -1;
	}

	if(NoTick)
	{
		int StartTick = 0;
		for(int i = 1; i < NumTicks; i++) // estimate start tick
			if(pGhost->m_Path.Get(i)->m_AttackTick != pGhost->m_Path.Get(i - 1)->m_AttackTick)
				StartTick = pGhost->m_Path.Get(i)->m_AttackTick - i;
		for(int i = 0; i < NumTicks; i++)
			pGhost->m_Path.Get(i)->m_Tick = StartTick + i;
	}

	if(pGhost->m_StartTick == -1)
		pGhost->m_StartTick = pGhost->m_Path.Get(0)->m_Tick;

	if(!FoundSkin)
		CGhostTools::GetGhostSkin(&pGhost->m_Skin, "default", 0, 0, 0);
	InitRenderInfos(pGhost);

	static const int s_aTeamColors[2] = { 65387, 10223467 };
	for(int i = 0; i < 2; i++)
		if(pGhost->m_Skin.m_UseCustomColor && pGhost->m_Skin.m_ColorBody == s_aTeamColors[i] && pGhost->m_Skin.m_ColorFeet == s_aTeamColors[i])
			pGhost->m_Team = i;

	return Slot;
}

void CGhost::Unload(int Slot)
{
	m_aActiveGhosts[Slot].Reset();
}

void CGhost::UnloadAll()
{
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		Unload(i);
}

void CGhost::SaveGhost(CMenus::CGhostItem *pItem)
{
	int Slot = pItem->m_Slot;
	if(!pItem->Active() || pItem->HasFile() || m_aActiveGhosts[Slot].Empty() || GhostRecorder()->IsRecording())
		return;

	CGhostItem *pGhost = &m_aActiveGhosts[Slot];

	int NumTicks = pGhost->m_Path.Size();
	Client()->GhostRecorder_Start(pItem->m_aPlayer, pItem->m_Time);

	GhostRecorder()->WriteData(GHOSTDATA_TYPE_START_TICK, (const char*)&pGhost->m_StartTick, sizeof(int));
	GhostRecorder()->WriteData(GHOSTDATA_TYPE_SKIN, (const char*)&pGhost->m_Skin, sizeof(CGhostSkin));
	for(int i = 0; i < NumTicks; i++)
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_CHARACTER, (const char*)pGhost->m_Path.Get(i), sizeof(CGhostCharacter));

	GhostRecorder()->Stop(NumTicks, pItem->m_Time);
	Client()->Ghost_GetPath(pItem->m_aFilename, sizeof(pItem->m_aFilename), pItem->m_aPlayer, pItem->m_Time);
}

void CGhost::ConGPlay(IConsole::IResult *pResult, void *pUserData)
{
	CGhost *pGhost = (CGhost *)pUserData;
	pGhost->StartRender(pGhost->Client()->PredGameTick());
}

void CGhost::OnConsoleInit()
{
	m_pGhostLoader = Kernel()->RequestInterface<IGhostLoader>();
	m_pGhostRecorder = Kernel()->RequestInterface<IGhostRecorder>();

	Console()->Register("gplay", "", CFGFLAG_CLIENT, ConGPlay, this, "");
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
			int Time = IRace::TimeFromFinishMessage(pMsg->m_pMessage, aName, sizeof(aName));
			if(Time > 0 && str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) == 0)
			{
				StopRecord(Time);
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

void CGhost::OnMapLoad()
{
	OnReset();
	UnloadAll();

	// TODO: fix this
	/*CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);

	if(IsRace(&ServerInfo))*/
	m_pClient->m_pMenus->GhostlistPopulate();

	m_AllowRestart = false;

	// symmetry check
	int Width = m_pClient->Collision()->GetWidth();
	int Height = m_pClient->Collision()->GetHeight();

	int RedIndex = m_pClient->m_aFlagIndex[TEAM_RED];
	int BlueIndex = m_pClient->m_aFlagIndex[TEAM_BLUE];
	ivec2 RedPos = ivec2(RedIndex % Width, RedIndex / Width);
	ivec2 BluePos = ivec2(BlueIndex % Width, BlueIndex / Width);
	int MiddleLeft = (RedPos.x + BluePos.x) / 2;
	int MiddleRight = (RedPos.x + BluePos.x + 1) / 2;
	int Half = min(MiddleLeft, Width - MiddleRight - 1);

	m_SymmetricMap = false;

	if(RedPos.y != BluePos.y)
		return;

	for(int y = 0; y < Height; y++)
	{
		int LeftOffset = y * Width + MiddleLeft;
		int RightOffset = y * Width + MiddleRight;
		for(int x = 0; x <= Half; x++)
		{
			int LeftIndex = m_pClient->Collision()->GetIndex(LeftOffset - x);
			int RightIndex = m_pClient->Collision()->GetIndex(RightOffset + x);
			if(LeftIndex != RightIndex && LeftIndex <= 128 && RightIndex <= 128)
				return;
		}
	}

	m_SymmetricMap = true;
}

void CGhost::OnGameJoin(int Team)
{
	if(!g_Config.m_ClGhostAutoMirror || Team < 0 || Team > 1 || (!m_SymmetricMap && !g_Config.m_ClGhostForceMirror))
		return;

	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		m_aActiveGhosts[i].m_Mirror = m_aActiveGhosts[i].m_Team != Team;
}
