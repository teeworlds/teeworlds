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

CGhost::CGhost() : m_StartRenderTick(-1), m_LastDeathTick(-1), m_Rendering(false), m_Recording(false), m_SymmetricMap(false) {}

void CGhost::AddInfos(const CNetObj_Character *pChar)
{
	CGhostCharacter GhostChar;
	CGhostTools::GetGhostCharacter(&GhostChar, pChar);
	GhostChar.m_Tick = pChar->m_Tick;
	CNetObj_Character Data;
	CGhostTools::GetNetObjCharacter(&Data, &GhostChar);
	Data.m_Tick = GhostChar.m_Tick;
	m_CurGhost.m_lPath.add(Data);
	if(GhostRecorder()->IsRecording())
		GhostRecorder()->WriteData(GHOSTDATA_TYPE_CHARACTER, (const char*)&GhostChar, sizeof(GhostChar));
}

int CGhost::GetSlot()
{
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		if(m_aActiveGhosts[i].Empty())
			return i;
	return -1;
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
	if(!IsRace(&ServerInfo) || !g_Config.m_ClRaceGhost)
		return;

	if(m_pClient->m_Snap.m_pLocalCharacter && m_pClient->m_Snap.m_pLocalPrevCharacter)
	{
		// TODO: handle restart
		if(m_pClient->m_NewPredictedTick)
		{
			vec2 PrevPos = m_pClient->m_PredictedPrevChar.m_Pos;
			vec2 Pos = m_pClient->m_PredictedChar.m_Pos;
			if(!m_Rendering && m_pClient->IsRaceStart(PrevPos, Pos))
				StartRender();
		}

		if(m_pClient->m_NewTick)
		{
			int PrevTick = m_pClient->m_Snap.m_pLocalPrevCharacter->m_Tick;
			vec2 PrevPos = vec2(m_pClient->m_Snap.m_pLocalPrevCharacter->m_X, m_pClient->m_Snap.m_pLocalPrevCharacter->m_Y);
			vec2 Pos = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_X, m_pClient->m_Snap.m_pLocalCharacter->m_Y);

			// detecting death, needed because race allows immediate respawning
			if(!m_Recording && m_pClient->IsRaceStart(PrevPos, Pos) && m_LastDeathTick < PrevTick)
				StartRecord();

			if(m_Recording)
				AddInfos(m_pClient->m_Snap.m_pLocalCharacter);
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
		int GhostTick = pGhost->m_lPath[0].m_Tick + PlaybackTick;
		while(pGhost->m_lPath[pGhost->m_PlaybackPos].m_Tick < GhostTick && !End)
		{
			if(pGhost->m_PlaybackPos < pGhost->m_lPath.size() - 1)
				pGhost->m_PlaybackPos++;
			else
				End = true;
		}

		if(End)
			continue;

		ActiveGhosts++;

		int CurPos = pGhost->m_PlaybackPos;
		int PrevPos = max(0, CurPos - 1);
		CNetObj_Character Player = pGhost->m_lPath[CurPos];
		CNetObj_Character Prev = pGhost->m_lPath[PrevPos];

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
			RenderGhostNamePlate(&Prev, &Player, IntraTick, pGhost->m_aOwner);
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
	m_CurGhost.Reset();

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
		int Slot = pOwnGhost ? pOwnGhost->m_Slot : GetSlot();
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

	m_CurGhost.Reset();
}

void CGhost::StartRender()
{
	m_Rendering = true;
	m_StartRenderTick = Client()->PredGameTick();
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

	// read header
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
	pGhost->m_lPath.set_size(NumTicks);

	// read player info
	str_copy(pGhost->m_aOwner, pHeader->m_aOwner, sizeof(pHeader->m_aOwner));

	int Index = 0;
	bool FoundSkin = false;
	bool NoTick = false;

	// read data
	int Type;
	while(GhostLoader()->ReadNextType(&Type))
	{
		if(Index == NumTicks && (Type == GHOSTDATA_TYPE_CHARACTER || Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK))
		{
			Index = -1;
			break;
		}

		if(Type == GHOSTDATA_TYPE_SKIN)
		{
			CGhostSkin Skin;
			if(GhostLoader()->ReadData(Type, (char*)&Skin, sizeof(Skin)) && !FoundSkin)
			{
				FoundSkin = true;
				char aSkinName[64];
				IntsToStr(&Skin.m_Skin0, 6, aSkinName);
				InitRenderInfos(&pGhost->m_RenderInfo, aSkinName, Skin.m_UseCustomColor, Skin.m_ColorBody, Skin.m_ColorFeet);

				static const int s_aTeamColors[2] = { 65387, 10223467 };
				for(int i = 0; i < 2; i++)
					if(Skin.m_UseCustomColor && Skin.m_ColorBody == s_aTeamColors[i] && Skin.m_ColorFeet == s_aTeamColors[i])
						pGhost->m_Team = i;
			}
		}
		else if(Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK)
		{
			NoTick = true;
			CGhostCharacter_NoTick Char;
			if(GhostLoader()->ReadData(Type, (char*)&Char, sizeof(Char)))
				CGhostTools::GetNetObjCharacter(&pGhost->m_lPath[Index++], &Char);
		}
		else if(Type == GHOSTDATA_TYPE_CHARACTER)
		{
			CGhostCharacter Char;
			if(GhostLoader()->ReadData(Type, (char*)&Char, sizeof(Char)))
			{
				CGhostTools::GetNetObjCharacter(&pGhost->m_lPath[Index], &Char);
				pGhost->m_lPath[Index++].m_Tick = Char.m_Tick;
			}
		}
	}

	GhostLoader()->Close();

	if(Index != NumTicks)
	{
		pGhost->Reset();
		return -1;
	}

	if(NoTick)
	{
		int StartTick = 0;
		for(int i = 1; i < NumTicks; i++) // estimate start tick
			if(pGhost->m_lPath[i].m_AttackTick != pGhost->m_lPath[i-1].m_AttackTick)
				StartTick = pGhost->m_lPath[i].m_AttackTick - i;
		for(int i = 0; i < NumTicks; i++)
			pGhost->m_lPath[i].m_Tick = StartTick + i;
	}

	if(!FoundSkin)
		InitRenderInfos(&pGhost->m_RenderInfo, "default", 0, 0, 0);

	return Slot;
}

void CGhost::Unload(int Slot)
{
	m_aActiveGhosts[Slot].Reset();
}

void CGhost::ConGPlay(IConsole::IResult *pResult, void *pUserData)
{
	((CGhost *)pUserData)->StartRender();
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

void CGhost::OnMapLoad()
{
	OnReset();
	for(int i = 0; i < MAX_ACTIVE_GHOSTS; i++)
		Unload(i);

	// TODO: fix this
	/*CServerInfo ServerInfo;
	Client()->GetServerInfo(&ServerInfo);

	if(IsRace(&ServerInfo))*/
	m_pClient->m_pMenus->GhostlistPopulate();

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
