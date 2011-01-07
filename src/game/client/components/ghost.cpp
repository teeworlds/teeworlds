/* (c) Rajh. */

#include <cstdio>

#include <engine/storage.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/compression.h>
#include <engine/shared/network.h>

#include <game/generated/client_data.h>
#include <game/client/animstate.h>
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

static const unsigned char gs_aHeaderMarker[8] = {'T', 'W', 'G', 'H', 'O', 'S', 'T', 0};
static const unsigned char gs_ActVersion = 1;

CGhost::CGhost()
{
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

void CGhost::AddInfos(CNetObj_Character Player)
{
	if(!m_Recording)
		return;

	// Just to be sure it doesnt eat too much memory, the first test should be enough anyway
	if((Client()->GameTick()-m_StartRecordTick) > Client()->GameTickSpeed()*60*10 || m_CurPath.size() > 50*15*60)
	{
		dbg_msg("ghost","10 minutes elapsed. Stopping ghost record");
		StopRecord();
		m_CurPath.clear();
		return;
	}
	
	// TODO: I don't know what the fuck is happening atm
	if(m_CurPath.size() == 0)
		m_CurPath.add(Player);
	m_CurPath.add(Player);
}

void CGhost::OnRender()
{
	CNetObj_Character Player = m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_LocalCid].m_Cur;
	m_pClient->m_PredictedChar.Write(&Player);
	
	if(m_pClient->m_NewPredictedTick)
		AddInfos(Player);
	
	// only for race
	if(!m_pClient->m_IsRace || !g_Config.m_ClGhost)
		return;
	
	// Check if the race line is crossed then start the render of the ghost if one
	if(m_RaceState != RACE_STARTED && ((m_pClient->Collision()->GetCollisionRace(m_pClient->Collision()->GetIndex(m_pClient->m_PredictedPrevChar.m_Pos, m_pClient->m_LocalCharacterPos)) == TILE_BEGIN) ||
		(m_pClient->m_IsFastCap && m_pClient->m_FlagPos != vec2(-1, -1) && distance(m_pClient->m_LocalCharacterPos, m_pClient->m_FlagPos) < 32)))
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
			m_BestPath.clear();
			m_BestPath = m_CurPath;
			m_GhostInfo = m_CurInfo;
			Save();
		}
		StopRecord();
		StopRender();
		m_RaceState = RACE_NONE;
	}

	// Play the ghost
	if(!m_Rendering)
		return;
	
	m_CurPos = Client()->PredGameTick()-m_StartRenderTick;

	if(m_BestPath.size() == 0 || m_CurPos < 0 || m_CurPos >= m_BestPath.size())
	{
		//dbg_msg("ghost","Ghost path done");
		m_Rendering = false;
		return;
	}

	RenderGhostHook();
	RenderGhost();
}

void CGhost::RenderGhost()
{
	CNetObj_Character Player = m_BestPath[m_CurPos];
	CNetObj_Character Prev = m_BestPath[m_CurPos];
	
	if(m_CurPos > 0)
		Prev = m_BestPath[m_CurPos-1];
	
	char aSkinName[64];
	IntsToStr(&m_GhostInfo.m_Skin0, 6, aSkinName);
	int SkinId = m_pClient->m_pSkins->Find(aSkinName);
	if(SkinId < 0)
	{
		SkinId = m_pClient->m_pSkins->Find("default");
		if(SkinId < 0)
			SkinId = 0;
	}
	
	CTeeRenderInfo RenderInfo;
	RenderInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(m_GhostInfo.m_ColorBody);
	RenderInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(m_GhostInfo.m_ColorFeet);
	
	if(m_GhostInfo.m_UseCustomColor)
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
	
	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, Client()->IntraGameTick())/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->PredIntraGameTick());
	vec2 Vel = mix(vec2(Prev.m_VelX/256.0f, Prev.m_VelY/256.0f), vec2(Player.m_VelX/256.0f, Player.m_VelY/256.0f), Client()->PredIntraGameTick());
	
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
		float a = (Client()->GameTick()-Player.m_AttackTick+Client()->PredIntraGameTick())/5.0f;
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

void CGhost::RenderGhostHook()
{
	CNetObj_Character Player = m_BestPath[m_CurPos];
	CNetObj_Character Prev = m_BestPath[m_CurPos];
	
	if(m_CurPos > 0)
		Prev = m_BestPath[m_CurPos-1];

	if (Prev.m_HookState<=0 || Player.m_HookState<=0)
		return;

	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, Client()->IntraGameTick())/256.0f;
	vec2 Direction = GetDirection((int)(Angle*256.0f));
	vec2 Pos = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->PredIntraGameTick());

	vec2 HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Player.m_HookX, Player.m_HookY), Client()->PredIntraGameTick());
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
	int i = 0;
	for(float f = 24; f < d && i < 1024; f += 24, i++)
	{
		vec2 p = HookPos + Dir*f;
		Array[i] = IGraphics::CQuadItem(p.x, p.y, 24, 16);
	}

	Graphics()->QuadsDraw(Array, i);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

void CGhost::StartRecord()
{
	m_Recording = true;
	m_CurPath.clear();
	CNetObj_ClientInfo *pInfo = (CNetObj_ClientInfo *) Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_CLIENTINFO, m_pClient->m_Snap.m_LocalCid);
	m_CurInfo = *pInfo;
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
	m_StartRenderTick = Client()->PredGameTick();
}

void CGhost::StopRender()
{
	m_Rendering = false;
}

void CGhost::Save()
{
	CGhostHeader Header;
	
	char aFilename[128];
	str_format(aFilename, sizeof(aFilename), "ghost/%s_%08x.gho", Client()->GetCurrentMap(), Client()->GetCurrentMapCrc());
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;
	
	// write header
	// TODO: save netversion, mapname, crc ...?
	mem_zero(&Header, sizeof(Header));
	mem_copy(Header.m_aMarker, gs_aHeaderMarker, sizeof(Header.m_aMarker));
	Header.m_Version = gs_ActVersion;
	io_write(File, &Header, sizeof(Header));
	
	// write time
	io_write(File, &m_PrevTime, sizeof(m_PrevTime));
	
	// write client info
	io_write(File, &m_GhostInfo, sizeof(m_GhostInfo));
	
	// write data
	int ItemsPerPackage = 500; // 500 ticks per package
	int Num = m_BestPath.size();
	CNetObj_Character *Data = &m_BestPath[0];
	
	while(Num)
	{
		int Items = min(Num, ItemsPerPackage);
		Num -= Items;
		
		char aBuffer[100*500];
		char aBuffer2[100*500];
		unsigned char aSize[4];
		
		int Size = sizeof(CNetObj_Character)*Items;
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
}

void CGhost::Load()
{
	CGhostHeader Header;
	
	char aFilename[128];
	str_format(aFilename, sizeof(aFilename), "ghost/%s_%08x.gho", Client()->GetCurrentMap(), Client()->GetCurrentMapCrc());
	IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return;

	// read header
	io_read(File, &Header, sizeof(Header));
	if(mem_comp(Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "not a ghost file");
		io_close(File);
		return;
	}
	
	if(Header.m_Version > gs_ActVersion)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "wrong version");
		io_close(File);
		return;
	}
	
	// read time
	io_read(File, &m_PrevTime, sizeof(m_PrevTime));
	
	// read client info
	io_read(File, &m_GhostInfo, sizeof(m_GhostInfo));
	
	// read data
	m_BestPath.clear();
	
	while(1)
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
		
		CNetObj_Character *Tmp = (CNetObj_Character*)aData;
		for(int i = 0; i < Size/sizeof(CNetObj_Character); i++)
		{
			m_BestPath.add(*Tmp);
			Tmp++;
		}
	}
	
	io_close(File);
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
	Load();
}
