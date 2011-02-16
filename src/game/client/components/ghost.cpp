/* (c) Rajh, Redix and Sushi. */

#include <cstdio>

#include <engine/storage.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/compression.h>
#include <engine/shared/network.h>

#include <game/generated/client_data.h>
#include <game/client/animstate.h>

#include "skins.h"
#include "menus.h"
#include "ghost.h"

/*
Note:
Freezing fucks up the ghost
the ghost isnt really sync
don't really get the client tick system for prediction
can used PrevChar and PlayerChar and it would be fluent and accurate but won't be predicted
so it will be affected by lags
*/

static const unsigned char gs_aHeaderMarker[8] = {'T', 'W', 'G', 'H', 'O', 'S', 'T', 0};
static const unsigned char gs_ActVersion = 2;

CGhost::CGhost()
{
	m_lGhosts.clear();
	m_CurGhost.m_Path.clear();
	m_CurGhost.m_ID = -1;
	m_CurPos = 0;
	m_Recording = false;
	m_Rendering = false;
	m_RaceState = RACE_NONE;
	m_NewRecord = false;
	m_BestTime = -1;
	m_StartRenderTick = -1;
}

void CGhost::AddInfos(CGhostCharacter Player)
{
	if(!m_Recording)
		return;
	
	// Just to be sure it doesnt eat too much memory, the first test should be enough anyway
	if(m_CurGhost.m_Path.size() > Client()->GameTickSpeed()*60*20)
	{
		dbg_msg("ghost", "20 minutes elapsed. Stopping ghost record");
		StopRecord();
		m_CurGhost.m_Path.clear();
		return;
	}
	
	m_CurGhost.m_Path.add(Player);
}

void CGhost::OnRender()
{
	// only for race
	if(!m_pClient->m_IsRace || !g_Config.m_ClRaceGhost)
		return;
	
	// Check if the race line is crossed then start the render of the ghost if one
	if(m_RaceState != RACE_STARTED && ((m_pClient->Collision()->GetCollisionRace(m_pClient->Collision()->GetIndex(m_pClient->m_PredictedPrevChar.m_Pos, m_pClient->m_LocalCharacterPos)) == TILE_BEGIN) ||
		(m_pClient->m_IsFastCap && m_pClient->m_FlagPos != vec2(-1, -1) && distance(m_pClient->m_LocalCharacterPos, m_pClient->m_FlagPos) < 32)))
	{
		//dbg_msg("ghost", "race started");
		m_RaceState = RACE_STARTED;
		StartRender();
		StartRecord();
	}

	if(m_RaceState == RACE_FINISHED)
	{
		if(m_NewRecord)
		{
			// search for own ghost
			array<CGhostItem>::range r = find_linear(m_lGhosts.all(), m_CurGhost);
			m_NewRecord = false;
			if(r.empty())
				m_lGhosts.add(m_CurGhost);
			else
				r.front() = m_CurGhost;
			
			Save();
		}
		StopRecord();
		StopRender();
		OnReset();
	}
	
	CNetObj_Character Char = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalClientID].m_Cur;
	m_pClient->m_PredictedChar.Write(&Char);
	
	if(m_pClient->m_NewPredictedTick)
		AddInfos(GetGhostCharacter(Char));

	// Play the ghost
	if(!m_Rendering || !g_Config.m_ClRaceShowGhost)
		return;
	
	m_CurPos = Client()->PredGameTick()-m_StartRenderTick;

	if(m_lGhosts.size() == 0 || m_CurPos < 0)
	{
		//dbg_msg("ghost", "Ghost path done");
		StopRender();
		return;
	}
	
	for(int i = 0; i < m_lGhosts.size(); i++)
	{
		CGhostItem *pGhost = &m_lGhosts[i];
		if(m_CurPos >= pGhost->m_Path.size())
			continue;
		
		int PrevPos = (m_CurPos > 0) ? m_CurPos-1 : m_CurPos;
		CGhostCharacter Player = pGhost->m_Path[m_CurPos];
		CGhostCharacter Prev = pGhost->m_Path[PrevPos];
		CNetObj_ClientInfo Info = pGhost->m_Info;
		
		RenderGhostHook(Player, Prev);
		RenderGhost(Player, Prev, Info);
	}
}

void CGhost::RenderGhost(CGhostCharacter Player, CGhostCharacter Prev, CNetObj_ClientInfo Info)
{
	char aSkinName[64];
	IntsToStr(&Info.m_Skin0, 6, aSkinName);
	int SkinId = m_pClient->m_pSkins->Find(aSkinName);
	if(SkinId < 0)
	{
		SkinId = m_pClient->m_pSkins->Find("default");
		if(SkinId < 0)
			SkinId = 0;
	}
	
	CTeeRenderInfo RenderInfo;
	RenderInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(Info.m_ColorBody);
	RenderInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(Info.m_ColorFeet);
	
	if(Info.m_UseCustomColor)
		RenderInfo.m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_ColorTexture;
	else
	{
		RenderInfo.m_Texture = m_pClient->m_pSkins->Get(SkinId)->m_OrgTexture;
		RenderInfo.m_ColorBody = vec4(1,1,1,1);
		RenderInfo.m_ColorFeet = vec4(1,1,1,1);
	}
	
	RenderInfo.m_ColorBody.a = 0.5f;
	RenderInfo.m_ColorFeet.a = 0.5f;
	RenderInfo.m_Size = 64;
	
	float IntraTick = Client()->PredIntraGameTick();
	
	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, IntraTick)/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);
	vec2 Vel = mix(vec2(Prev.m_VelX/256.0f, Prev.m_VelY/256.0f), vec2(Player.m_VelX/256.0f, Player.m_VelY/256.0f), IntraTick);
	
	bool Stationary = Player.m_VelX <= 1 && Player.m_VelX >= -1;
	bool InAir = !Collision()->CheckPoint(Player.m_X, Player.m_Y+16);
	bool WantOtherDir = (Player.m_Direction == -1 && Vel.x > 0) || (Player.m_Direction == 1 && Vel.x < 0);

	float WalkTime = fmod(absolute(Position.x), 100.0f)/100.0f;
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);

	if(InAir)
		State.Add(&g_pData->m_aAnimations[ANIM_INAIR], 0, 1.0f);
	else if(Stationary)
		State.Add(&g_pData->m_aAnimations[ANIM_IDLE], 0, 1.0f);
	else if(!WantOtherDir)
		State.Add(&g_pData->m_aAnimations[ANIM_WALK], WalkTime, 1.0f);
	
	if (Player.m_Weapon == WEAPON_GRENADE)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation(State.GetAttach()->m_Angle*pi*2+Angle);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);

		// normal weapons
		int iw = clamp(Player.m_Weapon, 0, NUM_WEAPONS-1);
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_pSpriteBody, Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
		
		vec2 Dir = Direction;
		float Recoil = 0.0f;
		// TODO: is this correct?
		float a = (Client()->PredGameTick()-Player.m_AttackTick+IntraTick)/5.0f;
		if(a < 1)
			Recoil = sinf(a*pi);
		
		vec2 p = Position + Dir * g_pData->m_Weapons.m_aId[iw].m_Offsetx - Direction*Recoil*10.0f;
		p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
		RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
		Graphics()->QuadsEnd();
	}

	// Render ghost
	RenderTools()->RenderTee(&State, &RenderInfo, 0, Direction, Position, true);
}

void CGhost::RenderGhostHook(CGhostCharacter Player, CGhostCharacter Prev)
{
	if (Prev.m_HookState<=0 || Player.m_HookState<=0)
		return;
	
	float IntraTick = Client()->PredIntraGameTick();

	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, IntraTick)/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Pos = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);

	vec2 HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Player.m_HookX, Player.m_HookY), IntraTick);
	float d = distance(Pos, HookPos);
	vec2 Dir = normalize(Pos-HookPos);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetRotation(GetAngle(Dir)+pi);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);

	// render head
	RenderTools()->SelectSprite(SPRITE_HOOK_HEAD);
	IGraphics::CQuadItem QuadItem(HookPos.x, HookPos.y, 24, 16);
	Graphics()->QuadsDraw(&QuadItem, 1);

	// render chain
	RenderTools()->SelectSprite(SPRITE_HOOK_CHAIN);
	IGraphics::CQuadItem Array[1024];
	int j = 0;
	for(float f = 24; f < d && j < 1024; f += 24, j++)
	{
		vec2 p = HookPos + Dir*f;
		Array[j] = IGraphics::CQuadItem(p.x, p.y, 24, 16);
	}

	Graphics()->QuadsDraw(Array, j);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

CGhost::CGhostCharacter CGhost::GetGhostCharacter(CNetObj_Character Char)
{
	CGhostCharacter Player;
	Player.m_X = Char.m_X;
	Player.m_Y = Char.m_Y;
	Player.m_VelX = Char.m_VelX;
	Player.m_VelY = Char.m_VelY;
	Player.m_Angle = Char.m_Angle;
	Player.m_Direction = Char.m_Direction;
	Player.m_Weapon = Char.m_Weapon;
	Player.m_HookState = Char.m_HookState;
	Player.m_HookX = Char.m_HookX;
	Player.m_HookY = Char.m_HookY;
	Player.m_AttackTick = Char.m_AttackTick;
	
	return Player;
}

void CGhost::StartRecord()
{
	m_Recording = true;
	m_CurGhost.m_Path.clear();
	CNetObj_ClientInfo *pInfo = (CNetObj_ClientInfo *) Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_CLIENTINFO, m_pClient->m_Snap.m_LocalClientID);
	m_CurGhost.m_Info = *pInfo;
}

void CGhost::StopRecord()
{
	m_Recording = false;
}

void CGhost::StartRender()
{
	m_CurPos = 0;
	m_Rendering = true;
	m_StartRenderTick = Client()->PredGameTick();
}

void CGhost::StopRender()
{
	m_Rendering = false;
}

void CGhost::Save()
{
	if(!g_Config.m_ClRaceSaveGhost)
		return;
	
	CGhostHeader Header;
	
	// check the player name
	char aName[MAX_NAME_LENGTH];
	str_copy(aName, g_Config.m_PlayerName, sizeof(aName));
	for(int i = 0; i < MAX_NAME_LENGTH; i++)
	{
		if(!aName[i])
			break;
		
		if(aName[i] == '\\' || aName[i] == '/' || aName[i] == '|' || aName[i] == ':' || aName[i] == '*' || aName[i] == '?' || aName[i] == '<' || aName[i] == '>' || aName[i] == '"')
			aName[i] = '%';
	}
	
	char aFilename[256];
	char aBuf[256];
	str_format(aFilename, sizeof(aFilename), "%s_%s_%.3f_%08x.gho", Client()->GetCurrentMap(), aName, m_BestTime, Client()->GetCurrentMapCrc());
	str_format(aBuf, sizeof(aBuf), "ghosts/%s", aFilename);
	IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;
		
	// write header
	int Crc = Client()->GetCurrentMapCrc();
	mem_zero(&Header, sizeof(Header));
	mem_copy(Header.m_aMarker, gs_aHeaderMarker, sizeof(Header.m_aMarker));
	Header.m_Version = gs_ActVersion;
	IntsToStr(&m_CurGhost.m_Info.m_Name0, 6, Header.m_aOwner);
	str_copy(Header.m_aMap, Client()->GetCurrentMap(), sizeof(Header.m_aMap));
	Header.m_aCrc[0] = (Crc>>24)&0xff;
	Header.m_aCrc[1] = (Crc>>16)&0xff;
	Header.m_aCrc[2] = (Crc>>8)&0xff;
	Header.m_aCrc[3] = (Crc)&0xff;
	Header.m_Time = m_BestTime;
	Header.m_NumShots = m_CurGhost.m_Path.size();
	io_write(File, &Header, sizeof(Header));
	
	// write client info
	io_write(File, &m_CurGhost.m_Info, sizeof(m_CurGhost.m_Info));
	
	// write data
	int ItemsPerPackage = 500; // 500 ticks per package
	int Num = Header.m_NumShots;
	CGhostCharacter *Data = &m_CurGhost.m_Path[0];
	
	while(Num)
	{
		int Items = min(Num, ItemsPerPackage);
		Num -= Items;
		
		char aBuffer[100*500];
		char aBuffer2[100*500];
		unsigned char aSize[4];
		
		int Size = sizeof(CGhostCharacter)*Items;
		mem_copy(aBuffer2, Data, Size);
		Data += Items;
		
		Size = CVariableInt::Compress(aBuffer2, Size, aBuffer);
		Size = CNetBase::Compress(aBuffer, Size, aBuffer2, sizeof(aBuffer2));
		
		aSize[0] = (Size>>24)&0xff;
		aSize[1] = (Size>>16)&0xff;
		aSize[2] = (Size>>8)&0xff;
		aSize[3] = (Size)&0xff;
		
		io_write(File, aSize, sizeof(aSize));
		io_write(File, aBuffer2, Size);
	}
	
	io_close(File);
	
	// remove old ghost from list (TODO: remove other ghosts?)
	if(m_pClient->m_pMenus->m_OwnGhost)
	{
		char aFile[256];
		str_format(aFile, sizeof(aFile), "ghosts/%s", m_pClient->m_pMenus->m_OwnGhost->m_aFilename);
		Storage()->RemoveFile(aFile, IStorage::TYPE_SAVE);
		
		m_pClient->m_pMenus->m_lGhosts.remove(*m_pClient->m_pMenus->m_OwnGhost);
	}
	
	CMenus::CGhostItem Item;
	str_copy(Item.m_aFilename, aFilename, sizeof(Item.m_aFilename));
	str_copy(Item.m_aPlayer, Header.m_aOwner, sizeof(Item.m_aPlayer));
	Item.m_Time = m_BestTime;
	Item.m_Active = true;
	Item.m_ID = -1;

	m_pClient->m_pMenus->m_lGhosts.add(Item);
	m_pClient->m_pMenus->m_OwnGhost = &find_linear(m_pClient->m_pMenus->m_lGhosts.all(), Item).front();
}

bool CGhost::GetHeader(IOHANDLE *pFile, CGhostHeader *pHeader)
{
	if(!*pFile)
		return 0;
	
	CGhostHeader Header;
	io_read(*pFile, &Header, sizeof(Header));
	
	*pHeader = Header;
	
	if(mem_comp(Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
		return 0;
	
	if(Header.m_Version != gs_ActVersion)
		return 0;
	
	int Crc = (Header.m_aCrc[0]<<24) | (Header.m_aCrc[1]<<16) | (Header.m_aCrc[2]<<8) | (Header.m_aCrc[3]);
	if(str_comp(Header.m_aMap, Client()->GetCurrentMap()) != 0 || Crc != Client()->GetCurrentMapCrc())
		return 0;
	
	return 1;
}

bool CGhost::GetInfo(const char* pFilename, CGhostHeader *pHeader)
{
	char aFilename[256];
	str_format(aFilename, sizeof(aFilename), "ghosts/%s", pFilename);
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return 0;
	
	bool Check = GetHeader(&File, pHeader);
	io_close(File);
	
	return Check;
}

void CGhost::Load(const char* pFilename, int ID)
{
	char aFilename[256];
	str_format(aFilename, sizeof(aFilename), "ghosts/%s", pFilename);
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return;

	// read header
	CGhostHeader Header;
	if(!GetHeader(&File, &Header))
	{
		io_close(File);
		return;
	}
	
	if(ID == -1)
		m_BestTime = Header.m_Time;
	
	int NumShots = Header.m_NumShots;
	
	// create ghost
	CGhostItem Ghost;
	Ghost.m_ID = ID;
	Ghost.m_Path.clear();
	Ghost.m_Path.set_size(NumShots);
	
	// read client info
	io_read(File, &Ghost.m_Info, sizeof(Ghost.m_Info));
	
	// read data
	int Index = 0;
	while(Index < NumShots)
	{
		static char aCompresseddata[100*500];
		static char aDecompressed[100*500];
		static char aData[100*500];
		
		unsigned char aSize[4];
		if(io_read(File, aSize, sizeof(aSize)) != sizeof(aSize))
			break;
		int Size = (aSize[0]<<24) | (aSize[1]<<16) | (aSize[2]<<8) | aSize[3];
		
		if(io_read(File, aCompresseddata, Size) != (unsigned)Size)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error reading chunk");
			break;
		}
		
		Size = CNetBase::Decompress(aCompresseddata, Size, aDecompressed, sizeof(aDecompressed));
		if(Size < 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error during network decompression");
			break;
		}
		
		Size = CVariableInt::Decompress(aDecompressed, Size, aData);
		if(Size < 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error during intpack decompression");
			break;
		}
		
		CGhostCharacter *Tmp = (CGhostCharacter*)aData;
		for(unsigned i = 0; i < Size/sizeof(CGhostCharacter); i++)
		{
			if(Index >= NumShots)
				break;
			
			Ghost.m_Path[Index] = *Tmp;
			Index++;
			Tmp++;
		}
	}
	
	io_close(File);
	
	m_lGhosts.add(Ghost);
}

void CGhost::Unload(int ID)
{
	CGhostItem Item;
	Item.m_ID = ID;
	m_lGhosts.remove_fast(Item);
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
	if(!m_pClient->m_IsRace || !g_Config.m_ClRaceGhost || m_pClient->m_Snap.m_Spectate)
		return;
	
	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID)
		{
			if(m_RaceState != RACE_FINISHED)
				OnReset();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1 && m_RaceState == RACE_STARTED)
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
			if(!str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) && sscanf(pMessage, " finished in: %d minute(s) %f", &Minutes, &Seconds) == 2)
			{
				m_RaceState = RACE_FINISHED;
				float CurTime = Minutes*60 + Seconds;
				if(m_Recording && (CurTime < m_BestTime || m_BestTime == -1))
				{
					m_NewRecord = true;
					m_BestTime = CurTime;
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
	m_CurGhost.m_Path.clear();
	m_StartRenderTick = -1;
}

void CGhost::OnMapLoad()
{
	OnReset();
	m_BestTime = -1;
	m_lGhosts.clear();
	m_pClient->m_pMenus->GhostlistPopulate();
}
