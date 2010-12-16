/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/sound.h>
#include <engine/demo.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/serverbrowser.h>
#include <engine/shared/demo.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/localization.h>
#include <game/version.h>
#include "render.h"

#include "gameclient.h"

#include "components/binds.h"
#include "components/broadcast.h"
#include "components/camera.h"
#include "components/chat.h"
#include "components/console.h"
#include "components/controls.h"
#include "components/damageind.h"
#include "components/debughud.h"
#include "components/effects.h"
#include "components/emoticon.h"
#include "components/flow.h"
#include "components/hud.h"
#include "components/items.h"
#include "components/killmessages.h"
#include "components/mapimages.h"
#include "components/maplayers.h"
#include "components/menus.h"
#include "components/motd.h"
#include "components/particles.h"
#include "components/players.h"
#include "components/nameplates.h"
#include "components/scoreboard.h"
#include "components/skins.h"
#include "components/sounds.h"
#include "components/voting.h"

CGameClient g_GameClient;

// instanciate all systems
static CKillMessages gs_KillMessages;
static CCamera gs_Camera;
static CChat gs_Chat;
static CMotd gs_Motd;
static CBroadcast gs_Broadcast;
static CGameConsole gs_GameConsole;
static CBinds gs_Binds;
static CParticles gs_Particles;
static CMenus gs_Menus;
static CSkins gs_Skins;
static CFlow gs_Flow;
static CHud gs_Hud;
static CDebugHud gs_DebugHud;
static CControls gs_Controls;
static CEffects gs_Effects;
static CScoreboard gs_Scoreboard;
static CSounds gs_Sounds;
static CEmoticon gs_Emoticon;
static CDamageInd gsDamageInd;
static CVoting gs_Voting;

static CPlayers gs_Players;
static CNamePlates gs_NamePlates;
static CItems gs_Items;
static CMapImages gs_MapImages;

static CMapLayers gs_MapLayersBackGround(CMapLayers::TYPE_BACKGROUND);
static CMapLayers gs_MapLayersForeGround(CMapLayers::TYPE_FOREGROUND);

CGameClient::CStack::CStack() { m_Num = 0; }
void CGameClient::CStack::Add(class CComponent *pComponent) { m_paComponents[m_Num++] = pComponent; }

static int gs_LoadCurrent;
static int gs_LoadTotal;

/*static void load_sounds_thread(void *do_render)
{
	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
	{
		if(do_render)
			gameclient.menus->render_loading(load_current/(float)load_total);
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
		{
			int id = Sound()->LoadWV(data->sounds[s].sounds[i].filename);
			data->sounds[s].sounds[i].id = id;
		}

		if(do_render)
			load_current++;
	}
}*/

static void ConServerDummy(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	dbg_msg("client", "this command is not available on the client");
}

#include <base/tl/sorted_array.h>

const char *CGameClient::Version() { return GAME_VERSION; }
const char *CGameClient::NetVersion() { return GAME_NETVERSION; }
const char *CGameClient::GetItemName(int Type) { return m_NetObjHandler.GetObjName(Type); }

void CGameClient::OnConsoleInit()
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pSound = Kernel()->RequestInterface<ISound>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pDemoPlayer = Kernel()->RequestInterface<IDemoPlayer>();
	m_pDemoRecorder = Kernel()->RequestInterface<IDemoRecorder>();
	m_pServerBrowser = Kernel()->RequestInterface<IServerBrowser>();
	
	// setup pointers
	m_pBinds = &::gs_Binds;
	m_pGameConsole = &::gs_GameConsole;
	m_pParticles = &::gs_Particles;
	m_pMenus = &::gs_Menus;
	m_pSkins = &::gs_Skins;
	m_pChat = &::gs_Chat;
	m_pFlow = &::gs_Flow;
	m_pCamera = &::gs_Camera;
	m_pControls = &::gs_Controls;
	m_pEffects = &::gs_Effects;
	m_pSounds = &::gs_Sounds;
	m_pMotd = &::gs_Motd;
	m_pDamageind = &::gsDamageInd;
	m_pMapimages = &::gs_MapImages;
	m_pVoting = &::gs_Voting;
	m_pScoreboard = &::gs_Scoreboard;
	
	// make a list of all the systems, make sure to add them in the corrent render order
	m_All.Add(m_pSkins);
	m_All.Add(m_pMapimages);
	m_All.Add(m_pEffects); // doesn't render anything, just updates effects
	m_All.Add(m_pParticles);
	m_All.Add(m_pBinds);
	m_All.Add(m_pControls);
	m_All.Add(m_pCamera);
	m_All.Add(m_pSounds);
	m_All.Add(m_pVoting);
	m_All.Add(m_pParticles); // doesn't render anything, just updates all the particles
	
	m_All.Add(&gs_MapLayersBackGround); // first to render
	m_All.Add(&m_pParticles->m_RenderTrail);
	m_All.Add(&gs_Items);
	m_All.Add(&gs_Players);
	m_All.Add(&gs_MapLayersForeGround);
	m_All.Add(&m_pParticles->m_RenderExplosions);
	m_All.Add(&gs_NamePlates);
	m_All.Add(&m_pParticles->m_RenderGeneral);
	m_All.Add(m_pDamageind);
	m_All.Add(&gs_Hud);
	m_All.Add(&gs_Emoticon);
	m_All.Add(&gs_KillMessages);
	m_All.Add(m_pChat);
	m_All.Add(&gs_Broadcast);
	m_All.Add(&gs_DebugHud);
	m_All.Add(&gs_Scoreboard);
	m_All.Add(m_pMotd);
	m_All.Add(m_pMenus);
	m_All.Add(m_pGameConsole);
	
	// build the input stack
	m_Input.Add(&m_pMenus->m_Binder); // this will take over all input when we want to bind a key
	m_Input.Add(&m_pBinds->m_SpecialBinds);
	m_Input.Add(m_pGameConsole);
	m_Input.Add(m_pChat); // chat has higher prio due to tha you can quit it by pressing esc
	m_Input.Add(m_pMotd); // for pressing esc to remove it
	m_Input.Add(m_pMenus);
	m_Input.Add(&gs_Emoticon);
	m_Input.Add(m_pControls);
	m_Input.Add(m_pBinds);
	
	// add the some console commands
	Console()->Register("team", "i", CFGFLAG_CLIENT, ConTeam, this, "Switch team", 0);
	Console()->Register("kill", "", CFGFLAG_CLIENT, ConKill, this, "Kill yourself", 0);
	
	// register server dummy commands for tab completion
	Console()->Register("tune", "si", CFGFLAG_SERVER, ConServerDummy, 0, "Tune variable to value", 0);
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, ConServerDummy, 0, "Reset tuning", 0);
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, ConServerDummy, 0, "Dump tuning", 0);
	Console()->Register("change_map", "?r", CFGFLAG_SERVER, ConServerDummy, 0, "Change map", 0);
	Console()->Register("restart", "?i", CFGFLAG_SERVER, ConServerDummy, 0, "Restart in x seconds", 0);
	Console()->Register("broadcast", "r", CFGFLAG_SERVER, ConServerDummy, 0, "Broadcast message", 0);
	Console()->Register("say", "r", CFGFLAG_SERVER, ConServerDummy, 0, "Say in chat", 0);
	Console()->Register("set_team", "ii", CFGFLAG_SERVER, ConServerDummy, 0, "Set team of player to team", 0);
	Console()->Register("addvote", "r", CFGFLAG_SERVER, ConServerDummy, 0, "Add a voting option", 0);
	Console()->Register("vote", "r", CFGFLAG_SERVER, ConServerDummy, 0, "Force a vote to yes/no", 0);


	// propagate pointers
	m_UI.SetGraphics(Graphics(), TextRender());
	m_RenderTools.m_pGraphics = Graphics();
	m_RenderTools.m_pUI = UI();
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->m_pClient = this;
	
	// let all the other components register their console commands
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnConsoleInit();
	
	
	//
	Console()->Chain("player_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_use_custom_color", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_body", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_feet", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_skin", ConchainSpecialInfoupdate, this);
	
	//
	m_SuppressEvents = false;
}

void CGameClient::OnInit()
{
	//m_pServerBrowser = Kernel()->RequestInterface<IServerBrowser>();
	
	// set the language
	g_Localization.Load(g_Config.m_ClLanguagefile, Storage(), Console());
	
	// init all components
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnInit();
	
	// setup item sizes
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Client()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));
	
	int64 Start = time_get();
	
	// load default font	
	static CFont *pDefaultFont;
	//default_font = gfx_font_load("data/fonts/sazanami-gothic.ttf");

	char aFilename[512];
	IOHANDLE File = Storage()->OpenFile("fonts/DejaVuSans.ttf", IOFLAG_READ, IStorage::TYPE_ALL, aFilename, sizeof(aFilename));
	if(File)
		io_close(File);
	pDefaultFont = TextRender()->LoadFont(aFilename);
	TextRender()->SetDefaultFont(pDefaultFont);

	g_Config.m_ClThreadsoundloading = 0;

	// setup load amount
	gs_LoadTotal = g_pData->m_NumImages;
	gs_LoadCurrent = 0;
	if(!g_Config.m_ClThreadsoundloading)
		gs_LoadTotal += g_pData->m_NumSounds;
	
	// load textures
	for(int i = 0; i < g_pData->m_NumImages; i++)
	{
		g_GameClient.m_pMenus->RenderLoading(gs_LoadCurrent/(float)gs_LoadTotal);
		g_pData->m_aImages[i].m_Id = Graphics()->LoadTexture(g_pData->m_aImages[i].m_pFilename, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		gs_LoadCurrent++;
	}

	::gs_Skins.Init();
	
	
	// TODO: Refactor: fix threaded loading of sounds again
	// load sounds
	{
		bool DoRender = true;
		for(int s = 0; s < g_pData->m_NumSounds; s++)
		{
			if(DoRender)
				g_GameClient.m_pMenus->RenderLoading(gs_LoadCurrent/(float)gs_LoadTotal);
			for(int i = 0; i < g_pData->m_aSounds[s].m_NumSounds; i++)
			{
				int id = Sound()->LoadWV(g_pData->m_aSounds[s].m_aSounds[i].m_pFilename);
				g_pData->m_aSounds[s].m_aSounds[i].m_Id = id;
			}

			if(DoRender)
				gs_LoadCurrent++;
		}
	}
		
	/*if(config.cl_threadsoundloading)
		thread_create(load_sounds_thread, 0);
	else
		load_sounds_thread((void*)1);*/

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();
	
	int64 End = time_get();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "initialisation finished after %.2fms", ((End-Start)*1000)/(float)time_freq());
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "gameclient", aBuf);
	
	m_ServerMode = SERVERMODE_PURE;
	
	m_DDRaceMsgSent = false;
}

void CGameClient::DispatchInput()
{
	// handle mouse movement
	float x = 0.0f, y = 0.0f;
	Input()->MouseRelative(&x, &y);
	if(x != 0.0f || y != 0.0f)
	{
		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnMouseMove(x, y))
				break;
		}
	}
	
	// handle key presses
	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);
		
		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnInput(e))
			{
				//dbg_msg("", "%d char=%d key=%d flags=%d", h, e.ch, e.key, e.flags);
				break;
			}
		}
	}
	
	// clear all events for this frame
	Input()->ClearEvents();	
}


int CGameClient::OnSnapInput(int *pData)
{
	return m_pControls->SnapInput(pData);
}

void CGameClient::OnConnected()
{
	m_Layers.Init(Kernel());
	m_Collision.Init(Layers());
	
	RenderTools()->RenderTilemapGenerateSkip(Layers());

	for(int i = 0; i < m_All.m_Num; i++)
	{
		m_All.m_paComponents[i]->OnMapLoad();
		m_All.m_paComponents[i]->OnReset();
	}
	
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	
	m_ServerMode = SERVERMODE_PURE;
	m_LastSendInfo = 0;
	
	// send the inital info
	SendInfo(true);
}

void CGameClient::OnReset()
{
	// clear out the invalid pointers
	m_LastNewPredictedTick = -1;
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aClients[i].m_aName[0] = 0;
		m_aClients[i].m_SkinId = 0;
		m_aClients[i].m_Team = 0;
		m_aClients[i].m_Angle = 0;
		m_aClients[i].m_Emoticon = 0;
		m_aClients[i].m_EmoticonStart = -1;
		m_aClients[i].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(0)->m_ColorTexture;
		m_aClients[i].m_SkinInfo.m_ColorBody = vec4(1,1,1,1);
		m_aClients[i].m_SkinInfo.m_ColorFeet = vec4(1,1,1,1);
		m_aClients[i].UpdateRenderInfo();
	}
	
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();
	m_Teams.Reset();
	m_DDRaceMsgSent = false;
}


void CGameClient::UpdateLocalCharacterPos()
{
	if(g_Config.m_ClPredict && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(!m_Snap.m_pLocalCharacter || (m_Snap.m_pLocalCharacter->m_Health < 0) || (m_Snap.m_pGameobj && m_Snap.m_pGameobj->m_GameOver))
		{
			// don't use predicted
		}
		else
			m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
	}
	else if(m_Snap.m_pLocalCharacter && m_Snap.m_pLocalPrevCharacter)
	{
		m_LocalCharacterPos = mix(
			vec2(m_Snap.m_pLocalPrevCharacter->m_X, m_Snap.m_pLocalPrevCharacter->m_Y),
			vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y), Client()->IntraGameTick());
	}
}


static void Evolve(CNetObj_Character *pCharacter, int Tick)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;
	CTeamsCore TempTeams;
	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, g_GameClient.Collision(), &TempTeams);
	TempCore.Read(pCharacter);
	
	while(pCharacter->m_Tick < Tick)
	{
		pCharacter->m_Tick++;
		TempCore.Tick(false);
		TempCore.Move();
		TempCore.Quantize();
	}

	TempCore.Write(pCharacter);
}


void CGameClient::OnRender()
{
	/*Graphics()->Clear(1,0,0);
	
	menus->render_background();
	return;*/
	/*
	Graphics()->Clear(1,0,0);
	Graphics()->MapScreen(0,0,100,100);
	
	Graphics()->QuadsBegin();
		Graphics()->SetColor(1,1,1,1);
		Graphics()->QuadsDraw(50, 50, 30, 30);
	Graphics()->QuadsEnd();
	
	return;*/
	
	// update the local character position
	UpdateLocalCharacterPos();
	
	// dispatch all input to systems
	DispatchInput();
	
	// render all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRender();
		
	// clear new tick flags
	m_NewTick = false;
	m_NewPredictedTick = false;

	// check if client info has to be resent
	if(m_LastSendInfo && Client()->State() == IClient::STATE_ONLINE && !m_pMenus->IsActive() && m_LastSendInfo+time_freq()*5 < time_get())
	{
		// resend if client info differs
		if(str_comp(g_Config.m_PlayerName, m_aClients[m_Snap.m_LocalCid].m_aName) ||
			str_comp(g_Config.m_PlayerSkin, m_aClients[m_Snap.m_LocalCid].m_aSkinName) ||
			(g_GameClient.m_Snap.m_pGameobj && !(g_GameClient.m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS) &&	// no teamgame?
			(g_Config.m_PlayerUseCustomColor != m_aClients[m_Snap.m_LocalCid].m_UseCustomColor ||
			g_Config.m_PlayerColorBody != m_aClients[m_Snap.m_LocalCid].m_ColorBody ||
			g_Config.m_PlayerColorFeet != m_aClients[m_Snap.m_LocalCid].m_ColorFeet)))
		{
			SendInfo(false);
		}
		m_LastSendInfo = 0;
	}
}

void CGameClient::OnRelease()
{
	// release all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRelease();
}

void CGameClient::OnMessage(int MsgId, CUnpacker *pUnpacker)
{
	
	// special messages
	if(MsgId == NETMSGTYPE_SV_EXTRAPROJECTILE)
	{
		/*
		int num = msg_unpack_int();
		
		for(int k = 0; k < num; k++)
		{
			NETOBJ_PROJECTILE proj;
			for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
				((int *)&proj)[i] = msg_unpack_int();
				
			if(msg_unpack_error())
				return;
				
			if(extraproj_num != MAX_EXTRA_PROJECTILES)
			{
				extraproj_projectiles[extraproj_num] = proj;
				extraproj_num++;
			}
		}
		
		return;*/
	}
	else if(MsgId == NETMSGTYPE_SV_TUNEPARAMS)
	{
		// unpack the new tuning
		CTuningParams NewTuning;
		int *pParams = (int *)&NewTuning;
		for(unsigned i = 0; i < sizeof(CTuningParams)/sizeof(int); i++)
			pParams[i] = pUnpacker->GetInt();

		// check for unpacking errors
		if(pUnpacker->Error())
			return;
		
		m_ServerMode = SERVERMODE_PURE;
			
		// apply new tuning
		m_Tuning = NewTuning;
		return;
	}
	
	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	if(!pRawMsg)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
		return;
	}

	// TODO: this should be done smarter
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnMessage(MsgId, pRawMsg);
	
	if(MsgId == NETMSGTYPE_SV_READYTOENTER)
	{
		Client()->EnterGame();
	}
	else if (MsgId == NETMSGTYPE_SV_EMOTICON)
	{
		CNetMsg_Sv_Emoticon *pMsg = (CNetMsg_Sv_Emoticon *)pRawMsg;

		// apply
		m_aClients[pMsg->m_Cid].m_Emoticon = pMsg->m_Emoticon;
		m_aClients[pMsg->m_Cid].m_EmoticonStart = Client()->GameTick();
	}
	else if(MsgId == NETMSGTYPE_SV_SOUNDGLOBAL)
	{
		if(m_SuppressEvents)
			return;
		
		// don't enqueue pseudo-global sounds from demos (created by PlayAndRecord)
		CNetMsg_Sv_SoundGlobal *pMsg = (CNetMsg_Sv_SoundGlobal *)pRawMsg;
		if(pMsg->m_Soundid == SOUND_CTF_DROP || pMsg->m_Soundid == SOUND_CTF_RETURN ||
			pMsg->m_Soundid == SOUND_CTF_CAPTURE || pMsg->m_Soundid == SOUND_CTF_GRAB_EN ||
			pMsg->m_Soundid == SOUND_CTF_GRAB_PL)
			g_GameClient.m_pSounds->Enqueue(pMsg->m_Soundid);
		else
			g_GameClient.m_pSounds->Play(CSounds::CHN_GLOBAL, pMsg->m_Soundid, 1.0f, vec2(0,0));
	}
	else if(MsgId == NETMSGTYPE_CL_TEAMSSTATE) {
		CNetMsg_Cl_TeamsState *pMsg = (CNetMsg_Cl_TeamsState *)pRawMsg;
		m_Teams.Team(0, pMsg->m_Tee0);
		m_Teams.Team(1, pMsg->m_Tee1);
		m_Teams.Team(2, pMsg->m_Tee2);
		m_Teams.Team(3, pMsg->m_Tee3);
		m_Teams.Team(4, pMsg->m_Tee4);
		m_Teams.Team(5, pMsg->m_Tee5);
		m_Teams.Team(6, pMsg->m_Tee6);
		m_Teams.Team(7, pMsg->m_Tee7);
		m_Teams.Team(8, pMsg->m_Tee8);
		m_Teams.Team(9, pMsg->m_Tee9);
		m_Teams.Team(10, pMsg->m_Tee10);
		m_Teams.Team(11, pMsg->m_Tee11);
		m_Teams.Team(12, pMsg->m_Tee12);
		m_Teams.Team(13, pMsg->m_Tee13);
		m_Teams.Team(14, pMsg->m_Tee14);
		m_Teams.Team(15, pMsg->m_Tee15);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			dbg_msg1("Teams", "Team = %d", m_Teams.Team(i));
		}
	} else if(MsgId == NETMSGTYPE_SV_PLAYERTIME) {
		CNetMsg_Sv_PlayerTime *pMsg = (CNetMsg_Sv_PlayerTime *)pRawMsg;
		m_aClients[pMsg->m_Cid].m_Score = pMsg->m_Time;
	}
	
}

void CGameClient::OnStateChange(int NewState, int OldState)
{
	// reset everything when not already connected (to keep gathered stuff)
	if(NewState < IClient::STATE_ONLINE)
		OnReset();
	
	// then change the state
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnStateChange(NewState, OldState);
}

void CGameClient::OnShutdown() {}
void CGameClient::OnEnterGame() {}

void CGameClient::OnGameOver()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->AutoScreenshot_Start();
}

void CGameClient::OnRconLine(const char *pLine)
{
	m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_REMOTE, pLine);
}

void CGameClient::ProcessEvents()
{
	if(m_SuppressEvents)
		return;
	
	int SnapType = IClient::SNAP_CURRENT;
	int Num = Client()->SnapNumItems(SnapType);
	for(int Index = 0; Index < Num; Index++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(SnapType, Index, &Item);

		if(Item.m_Type == NETEVENTTYPE_DAMAGEIND)
		{
			NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)pData;
			g_GameClient.m_pEffects->DamageIndicator(vec2(ev->m_X, ev->m_Y), GetDirection(ev->m_Angle));
		}
		else if(Item.m_Type == NETEVENTTYPE_EXPLOSION)
		{
			NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)pData;
			g_GameClient.m_pEffects->Explosion(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_HAMMERHIT)
		{
			NETEVENT_HAMMERHIT *ev = (NETEVENT_HAMMERHIT *)pData;
			g_GameClient.m_pEffects->HammerHit(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_SPAWN)
		{
			NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)pData;
			g_GameClient.m_pEffects->PlayerSpawn(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_DEATH)
		{
			NETEVENT_DEATH *ev = (NETEVENT_DEATH *)pData;
			g_GameClient.m_pEffects->PlayerDeath(vec2(ev->m_X, ev->m_Y), ev->m_ClientId);
		}
		else if(Item.m_Type == NETEVENTTYPE_SOUNDWORLD)
		{
			NETEVENT_SOUNDWORLD *ev = (NETEVENT_SOUNDWORLD *)pData;
			g_GameClient.m_pSounds->Play(CSounds::CHN_WORLD, ev->m_SoundId, 1.0f, vec2(ev->m_X, ev->m_Y));
		}
	}
}

void CGameClient::OnNewSnapshot()
{
	m_NewTick = true;
	
	// clear out the invalid pointers
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));
	m_Snap.m_LocalCid = -1;

	// secure snapshot
	{
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int Index = 0; Index < Num; Index++)
		{
			IClient::CSnapItem Item;
			void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
			if(m_NetObjHandler.ValidateObj(Item.m_Type, pData, Item.m_DataSize) != 0)
			{
				if(g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "invalidated index=%d type=%d (%s) size=%d id=%d", Index, Item.m_Type, m_NetObjHandler.GetObjName(Item.m_Type), Item.m_DataSize, Item.m_Id);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				}
				Client()->SnapInvalidateItem(IClient::SNAP_CURRENT, Index);
			}
		}
	}
		
	ProcessEvents();

	if(g_Config.m_DbgStress)
	{
		if((Client()->GameTick()%100) == 0)
		{
			char aMessage[64];
			int MsgLen = rand()%(sizeof(aMessage)-1);
			for(int i = 0; i < MsgLen; i++)
				aMessage[i] = 'a'+(rand()%('z'-'a'));
			aMessage[MsgLen] = 0;
				
			CNetMsg_Cl_Say Msg;
			Msg.m_Team = rand()&1;
			Msg.m_pMessage = aMessage;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}
	}

	// go trough all the items in the snapshot and gather the info we want
	{
		m_Snap.m_aTeamSize[0] = m_Snap.m_aTeamSize[1] = 0;
		
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int i = 0; i < Num; i++)
		{
			IClient::CSnapItem Item;
			const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

			if(Item.m_Type == NETOBJTYPE_CLIENTINFO)
			{
				const CNetObj_ClientInfo *pInfo = (const CNetObj_ClientInfo *)pData;
				int Cid = Item.m_Id;
				IntsToStr(&pInfo->m_Name0, 6, m_aClients[Cid].m_aName);
				IntsToStr(&pInfo->m_Skin0, 6, m_aClients[Cid].m_aSkinName);
				
				m_aClients[Cid].m_UseCustomColor = pInfo->m_UseCustomColor;
				m_aClients[Cid].m_ColorBody = pInfo->m_ColorBody;
				m_aClients[Cid].m_ColorFeet = pInfo->m_ColorFeet;
				
				// prepare the info
				if(m_aClients[Cid].m_aSkinName[0] == 'x' || m_aClients[Cid].m_aSkinName[1] == '_')
					str_copy(m_aClients[Cid].m_aSkinName, "default", 64);
					
				m_aClients[Cid].m_SkinInfo.m_ColorBody = m_pSkins->GetColorV4(m_aClients[Cid].m_ColorBody);
				m_aClients[Cid].m_SkinInfo.m_ColorFeet = m_pSkins->GetColorV4(m_aClients[Cid].m_ColorFeet);
				m_aClients[Cid].m_SkinInfo.m_Size = 64;
				
				// find new skin
				m_aClients[Cid].m_SkinId = g_GameClient.m_pSkins->Find(m_aClients[Cid].m_aSkinName);
				if(m_aClients[Cid].m_SkinId < 0)
				{
					m_aClients[Cid].m_SkinId = g_GameClient.m_pSkins->Find("default");
					if(m_aClients[Cid].m_SkinId < 0)
						m_aClients[Cid].m_SkinId = 0;
				}
				
				if(m_aClients[Cid].m_UseCustomColor)
					m_aClients[Cid].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[Cid].m_SkinId)->m_ColorTexture;
				else
				{
					m_aClients[Cid].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[Cid].m_SkinId)->m_OrgTexture;
					m_aClients[Cid].m_SkinInfo.m_ColorBody = vec4(1,1,1,1);
					m_aClients[Cid].m_SkinInfo.m_ColorFeet = vec4(1,1,1,1);
				}

				m_aClients[Cid].UpdateRenderInfo();
				g_GameClient.m_Snap.m_NumPlayers++;
				
			}
			else if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
			{
				const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;
				
				m_aClients[pInfo->m_ClientId].m_Team = pInfo->m_Team;
				m_Snap.m_paPlayerInfos[pInfo->m_ClientId] = pInfo;
				
				if(pInfo->m_Local)
				{
					m_Snap.m_LocalCid = Item.m_Id;
					m_Snap.m_pLocalInfo = pInfo;
					
					if (pInfo->m_Team == -1)
						m_Snap.m_Spectate = true;
				}
				
				// calculate team-balance
				if(pInfo->m_Team != -1)
					m_Snap.m_aTeamSize[pInfo->m_Team]++;
				
			}
			else if(Item.m_Type == NETOBJTYPE_CHARACTER)
			{
				const void *pOld = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, Item.m_Id);
				if(pOld)
				{
					m_Snap.m_aCharacters[Item.m_Id].m_Active = true;
					m_Snap.m_aCharacters[Item.m_Id].m_Prev = *((const CNetObj_Character *)pOld);
					m_Snap.m_aCharacters[Item.m_Id].m_Cur = *((const CNetObj_Character *)pData);

					if(m_Snap.m_aCharacters[Item.m_Id].m_Prev.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_Id].m_Prev, Client()->PrevGameTick());
					if(m_Snap.m_aCharacters[Item.m_Id].m_Cur.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_Id].m_Cur, Client()->GameTick());
				}
			}
			else if(Item.m_Type == NETOBJTYPE_GAME)
			{
				static int s_GameOver = 0;
				m_Snap.m_pGameobj = (CNetObj_Game *)pData;
				if(s_GameOver == 0 && m_Snap.m_pGameobj->m_GameOver != 0)
					OnGameOver();
				s_GameOver = m_Snap.m_pGameobj->m_GameOver;
			}
			else if(Item.m_Type == NETOBJTYPE_FLAG)
				m_Snap.m_paFlags[Item.m_Id%2] = (const CNetObj_Flag *)pData;
		}
	}
	
	// setup local pointers
	if(m_Snap.m_LocalCid >= 0)
	{
		CSnapState::CCharacterInfo *c = &m_Snap.m_aCharacters[m_Snap.m_LocalCid];
		if(c->m_Active)
		{
			m_Snap.m_pLocalCharacter = &c->m_Cur;
			m_Snap.m_pLocalPrevCharacter = &c->m_Prev;
			m_LocalCharacterPos = vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y);
		}
		else if(Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, m_Snap.m_LocalCid))
		{
			// player died
			m_pControls->OnPlayerDeath();
		}
	}
	else
		m_Snap.m_Spectate = true;
	
	CTuningParams StandardTuning;
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	if(CurrentServerInfo.m_aGameType[0] != '0')
	{
		if(str_comp(CurrentServerInfo.m_aGameType, "DM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "TDM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "CTF") != 0)
			m_ServerMode = SERVERMODE_MOD;
		else if(mem_comp(&StandardTuning, &m_Tuning, sizeof(CTuningParams)) == 0)
			m_ServerMode = SERVERMODE_PURE;
		else
			m_ServerMode = SERVERMODE_PUREMOD;
	}
	
	if(!m_DDRaceMsgSent && m_Snap.m_pLocalInfo)
	{
		CNetMsg_Cl_IsDDRace Msg;
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		m_DDRaceMsgSent = true;
	}

}

void CGameClient::OnPredict()
{
	// store the previous values so we can detect prediction errors
	CCharacterCore BeforePrevChar = m_PredictedPrevChar;
	CCharacterCore BeforeChar = m_PredictedChar;

	// we can't predict without our own id or own character
	if(m_Snap.m_LocalCid == -1 || !m_Snap.m_aCharacters[m_Snap.m_LocalCid].m_Active)
		return;
	
	// don't predict anything if we are paused
	if(m_Snap.m_pGameobj && m_Snap.m_pGameobj->m_Paused)
	{
		if(m_Snap.m_pLocalCharacter)
			m_PredictedChar.Read(m_Snap.m_pLocalCharacter);
		if(m_Snap.m_pLocalPrevCharacter)
			m_PredictedPrevChar.Read(m_Snap.m_pLocalPrevCharacter);
		return;
	}

	// repredict character
	CWorldCore World;
	World.m_Tuning = m_Tuning;

	// search for players
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_Snap.m_aCharacters[i].m_Active)
			continue;
			
		g_GameClient.m_aClients[i].m_Predicted.Init(&World, Collision(), &m_Teams);
		World.m_apCharacters[i] = &g_GameClient.m_aClients[i].m_Predicted;
		World.m_apCharacters[i]->m_Id = m_Snap.m_paPlayerInfos[i]->m_ClientId;
		g_GameClient.m_aClients[i].m_Predicted.Read(&m_Snap.m_aCharacters[i].m_Cur);
	}
	
	// predict
	for(int Tick = Client()->GameTick()+1; Tick <= Client()->PredGameTick(); Tick++)
	{
		// fetch the local
		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_Snap.m_LocalCid])
			m_PredictedPrevChar = *World.m_apCharacters[m_Snap.m_LocalCid];
		
		// first calculate where everyone should move
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			mem_zero(&World.m_apCharacters[c]->m_Input, sizeof(World.m_apCharacters[c]->m_Input));
			if(m_Snap.m_LocalCid == c)
			{
				// apply player input
				int *pInput = Client()->GetInput(Tick);
				if(pInput)
					World.m_apCharacters[c]->m_Input = *((CNetObj_PlayerInput*)pInput);
				World.m_apCharacters[c]->Tick(true);
			}
			else
				World.m_apCharacters[c]->Tick(false);

		}

		// move all players and quantize their data
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			World.m_apCharacters[c]->Move();
			World.m_apCharacters[c]->Quantize();
		}
		
		// check if we want to trigger effects
		if(Tick > m_LastNewPredictedTick)
		{
			m_LastNewPredictedTick = Tick;
			m_NewPredictedTick = true;
			
			if(m_Snap.m_LocalCid != -1 && World.m_apCharacters[m_Snap.m_LocalCid])
			{
				vec2 Pos = World.m_apCharacters[m_Snap.m_LocalCid]->m_Pos;
				int Events = World.m_apCharacters[m_Snap.m_LocalCid]->m_TriggeredEvents;
				if(Events&COREEVENT_GROUND_JUMP) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, Pos);
				
				/*if(events&COREEVENT_AIR_JUMP)
				{
					GameClient.effects->air_jump(pos);
					GameClient.sounds->play_and_record(SOUNDS::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
				}*/
				
				//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
				//if(events&COREEVENT_HOOK_ATTACH_PLAYER) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_PLAYER, 1.0f, pos);
				if(Events&COREEVENT_HOOK_ATTACH_GROUND) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, Pos);
				if(Events&COREEVENT_HOOK_HIT_NOHOOK) g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_NOATTACH, 1.0f, Pos);
				//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
			}
		}
		
		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_Snap.m_LocalCid])
			m_PredictedChar = *World.m_apCharacters[m_Snap.m_LocalCid];
	}
	
	if(g_Config.m_Debug && g_Config.m_ClPredict && m_PredictedTick == Client()->PredGameTick())
	{
		CNetObj_CharacterCore Before = {0}, Now = {0}, BeforePrev = {0}, NowPrev = {0};
		BeforeChar.Write(&Before);
		BeforePrevChar.Write(&BeforePrev);
		m_PredictedChar.Write(&Now);
		m_PredictedPrevChar.Write(&NowPrev);

		if(mem_comp(&Before, &Now, sizeof(CNetObj_CharacterCore)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "prediction error");
			for(unsigned i = 0; i < sizeof(CNetObj_CharacterCore)/sizeof(int); i++)
				if(((int *)&Before)[i] != ((int *)&Now)[i])
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "    %d %d %d  (%d %d)", i, ((int *)&Before)[i], ((int *)&Now)[i], ((int *)&BeforePrev)[i], ((int *)&NowPrev)[i]);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
				}
		}
	}
	
	m_PredictedTick = Client()->PredGameTick();
}

void CGameClient::CClientData::UpdateRenderInfo()
{
	m_RenderInfo = m_SkinInfo;

	// force team colors
	if(g_GameClient.m_Snap.m_pGameobj && g_GameClient.m_Snap.m_pGameobj->m_Flags&GAMEFLAG_TEAMS)
	{
		const int TeamColors[2] = {65387, 10223467};
		if(m_Team >= 0 && m_Team <= 1)
		{
			m_RenderInfo.m_Texture = g_GameClient.m_pSkins->Get(m_SkinId)->m_ColorTexture;
			m_RenderInfo.m_ColorBody = g_GameClient.m_pSkins->GetColorV4(TeamColors[m_Team]);
			m_RenderInfo.m_ColorFeet = g_GameClient.m_pSkins->GetColorV4(TeamColors[m_Team]);
		}
	}		
}

void CGameClient::SendSwitchTeam(int Team)
{
	CNetMsg_Cl_SetTeam Msg;
	Msg.m_Team = Team;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);	
}

void CGameClient::SendInfo(bool Start)
{
	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pSkin = g_Config.m_PlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_PlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_PlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_PlayerColorFeet;
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);	
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pSkin = g_Config.m_PlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_PlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_PlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_PlayerColorFeet;
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

		// activate timer to resend the info if it gets filtered
		if(!m_LastSendInfo || m_LastSendInfo+time_freq()*5 < time_get())
			m_LastSendInfo = time_get();
	}
}

void CGameClient::SendKill(int ClientId)
{
	CNetMsg_Cl_Kill Msg;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);	
}

void CGameClient::ConTeam(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	((CGameClient*)pUserData)->SendSwitchTeam(pResult->GetInteger(0));
}

void CGameClient::ConKill(IConsole::IResult *pResult, void *pUserData, int ClientID)
{
	((CGameClient*)pUserData)->SendKill(-1);
}

void CGameClient::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData, -1);
	if(pResult->NumArguments())
		((CGameClient*)pUserData)->SendInfo(false);
}

IGameClient *CreateGameClient()
{
	return &g_GameClient;
}
