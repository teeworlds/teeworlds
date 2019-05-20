/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "mod.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <vector>
#include <algorithm>
#include <infcroya/croyaplayer.h>
#include <infcroya/classes/default.h>
#include <infcroya/classes/biologist.h>
#include <infcroya/classes/smoker.h>
#include <infcroya/classes/engineer.h>
#include <infcroya/classes/soldier.h>
#include <infcroya/classes/scientist.h>
#include <infcroya/classes/medic.h>
#include <infcroya/classes/mercenary.h>
#include <engine/shared/config.h>
#include <infcroya/localization/localization.h>
#include <engine/storage.h>
#include <infcroya/entities/inf-circle.h>
#include <infcroya/entities/circle.h>
#include <infcroya/lualoader/lualoader.h>
#ifdef CONF_GEOLOCATION
	#include <infcroya/geolocation/geolocation.h>
#endif

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	m_pGameType = "InfCroya";

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode

#ifdef CONF_GEOLOCATION
	geolocation = new Geolocation("mmdb/GeoLite2-Country.mmdb");
#endif
	m_pGameWorld = nullptr;
	IGameController::m_MOD = this; // temporarily, todo: avoid this
	lua = nullptr;

	m_ExplosionStarted = false;
	m_InfectedStarted = false;
	m_MapWidth = GameServer()->Collision()->GetWidth();
	m_MapHeight = GameServer()->Collision()->GetHeight();
	m_GrowingMap = new int[m_MapWidth * m_MapHeight];

	for (int j = 0; j < m_MapHeight; j++)
	{
		for (int i = 0; i < m_MapWidth; i++)
		{
			vec2 TilePos = vec2(16.0f, 16.0f) + vec2(i * 32.0f, j * 32.0f);
			if (GameServer()->Collision()->CheckPoint(TilePos))
			{
				m_GrowingMap[j * m_MapWidth + i] = 4;
			}
			else
			{
				m_GrowingMap[j * m_MapWidth + i] = 1;
			}
		}
	}
	m_GrowingMap[56 * m_MapWidth + 23] = 6;

	classes[Class::DEFAULT] = new CDefault();
	classes[Class::BIOLOGIST] = new CBiologist();
	classes[Class::ENGINEER] = new CEngineer();
	classes[Class::MEDIC] = new CMedic();
	classes[Class::SOLDIER] = new CSoldier();
	classes[Class::SCIENTIST] = new CScientist();
	classes[Class::MERCENARY] = new CMercenary();
	classes[Class::SMOKER] = new CSmoker();
}

CGameControllerMOD::~CGameControllerMOD()
{
#ifdef CONF_GEOLOCATION
	delete geolocation;
#endif
	for (const auto& c : classes) {
		delete c.second; // delete classes[i];
	}
	if (m_GrowingMap) delete[] m_GrowingMap;

	delete lua;
}

void CGameControllerMOD::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}

void CGameControllerMOD::OnRoundStart()
{
	if (!lua) { // not sure yet if OnRoundStart is run only once
		std::string path_to_lua("maps/");
		path_to_lua += g_Config.m_SvMap;
		path_to_lua += ".lua";
		lua = new LuaLoader();
		lua->load(path_to_lua.c_str());
		lua->init(GetRealPlayerNum());

		// positions.size() should be equal to radiuses.size()
		// safezone circles
		auto positions = lua->get_circle_positions();
		auto radiuses = lua->get_circle_radiuses();
		for (size_t i = 0; i < positions.size(); i++) {
			const int TILE_SIZE = 32;
			int x = positions[i].x * TILE_SIZE;
			int y = positions[i].y * TILE_SIZE;
			circles.push_back(new CCircle(m_pGameWorld, vec2(x, y), -1, radiuses[i]));
		}

		// infection zone circles
		auto inf_positions = lua->get_inf_circle_positions();
		auto inf_radiuses = lua->get_inf_circle_radiuses();
		for (size_t i = 0; i < positions.size(); i++) {
			const int TILE_SIZE = 32;
			int x = inf_positions[i].x * TILE_SIZE;
			int y = inf_positions[i].y * TILE_SIZE;
			inf_circles.push_back(new CInfCircle(m_pGameWorld, vec2(x, y), -1, inf_radiuses[i]));
		}
	}
	StartInitialInfection();
	m_InfectedStarted = true;
	TurnDefaultIntoRandomHuman();
}

void CGameControllerMOD::Tick()
{
	IGameController::Tick();

	if (RoundJustStarted()) { // not sure if it is executed only once
		OnRoundStart();
	}

	if (!IsCroyaWarmup() && IsEveryoneInfected() && !IsGameEnd() && m_InfectedStarted) {
		OnRoundEnd();
	}

	if (!IsCroyaWarmup() && !IsGameEnd() && GetZombieCount() < 1 && !m_InfectedStarted) {
		StartInitialInfection();
		m_InfectedStarted = true;
	}

	bool IsGameStarted = !IsCroyaWarmup() && !IsGameEnd();

	if (IsGameStarted && GetRealPlayerNum() < 2) {
		OnRoundEnd();
	}

	if (IsGameStarted) {
		for (auto each : players) {
			if (!each)
				continue;

			each->Tick();
		}
	}

	// FINAL EXPLOSION BEGIN, todo: write a function for this?
	//infclass 0.6 copypaste
	//Start the final explosion if the time is over
	if (m_InfectedStarted && !m_ExplosionStarted && g_Config.m_SvTimelimit > 0 && (Server()->Tick() - m_GameStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60)
	{
		for (CCharacter* p = (CCharacter*)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter*)p->TypeNext())
		{
			if (p->IsZombie())
			{
				GameServer()->SendEmoticon(p->GetPlayer()->GetCID(), EMOTICON_GHOST);
			}
			else
			{
				GameServer()->SendEmoticon(p->GetPlayer()->GetCID(), EMOTICON_EYES);
			}
		}
		m_ExplosionStarted = true;
	}

	//Do the final explosion
	if (m_ExplosionStarted)
	{
		bool NewExplosion = false;

		for (int j = 0; j < m_MapHeight; j++)
		{
			for (int i = 0; i < m_MapWidth; i++)
			{
				if ((m_GrowingMap[j * m_MapWidth + i] & 1) && (
					(i > 0 && m_GrowingMap[j * m_MapWidth + i - 1] & 2) ||
					(i < m_MapWidth - 1 && m_GrowingMap[j * m_MapWidth + i + 1] & 2) ||
					(j > 0 && m_GrowingMap[(j - 1) * m_MapWidth + i] & 2) ||
					(j < m_MapHeight - 1 && m_GrowingMap[(j + 1) * m_MapWidth + i] & 2)
					))
				{
					NewExplosion = true;
					m_GrowingMap[j * m_MapWidth + i] |= 8;
					m_GrowingMap[j * m_MapWidth + i] &= ~1;
					if (random_prob(0.1f))
					{
						vec2 TilePos = vec2(16.0f, 16.0f) + vec2(i * 32.0f, j * 32.0f);
						GameServer()->CreateExplosion(TilePos, -1, WEAPON_GAME, true);
						GameServer()->CreateSound(TilePos, SOUND_GRENADE_EXPLODE);
					}
				}
			}
		}

		for (int j = 0; j < m_MapHeight; j++)
		{
			for (int i = 0; i < m_MapWidth; i++)
			{
				if (m_GrowingMap[j * m_MapWidth + i] & 8)
				{
					m_GrowingMap[j * m_MapWidth + i] &= ~8;
					m_GrowingMap[j * m_MapWidth + i] |= 2;
				}
			}
		}

		for (CCharacter* p = (CCharacter*)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER); p; p = (CCharacter*)p->TypeNext())
		{
			if (p->IsHuman())
				continue;

			int tileX = static_cast<int>(round(p->GetPos().x)) / 32;
			int tileY = static_cast<int>(round(p->GetPos().y)) / 32;

			if (tileX < 0) tileX = 0;
			if (tileX >= m_MapWidth) tileX = m_MapWidth - 1;
			if (tileY < 0) tileY = 0;
			if (tileY >= m_MapHeight) tileY = m_MapHeight - 1;

			if (m_GrowingMap[tileY * m_MapWidth + tileX] & 2 && p->GetPlayer())
			{
				p->Die(p->GetPlayer()->GetCID(), WEAPON_GAME);
			}
		}

		//If no more explosions, game over, decide who win
		if (!NewExplosion)
		{
			if (GameServer()->GetHumanCount())
			{
				int NumHumans = GameServer()->GetHumanCount();
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "%d humans won the round", NumHumans);
				GameServer()->SendChatTarget(-1, aBuf);

				for (CPlayer* each : GameServer()->m_apPlayers) {
					if (!each)
						continue;
					if (each->GetCroyaPlayer()->IsHuman())
					{
						GameServer()->SendChatTarget(-1, "You have survived, +5 points");
					}
				}
			}
			else
			{
				int Seconds = g_Config.m_SvTimelimit * 60;
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Infected won the round in %d seconds", Seconds);
				GameServer()->SendChatTarget(-1, aBuf);
			}
			OnRoundEnd();
		}
	}
	// FINAL EXPLOSION END
}

bool CGameControllerMOD::IsCroyaWarmup()
{
	if (IsWarmup())
		return true;
	if (m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) <= Server()->TickSpeed() * 10)
		return true;
	else
		return false;
}

bool CGameControllerMOD::RoundJustStarted()
{
	if (!IsWarmup() && m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) == Server()->TickSpeed() * 10)
		return true;
	else
		return false;
}

void CGameControllerMOD::StartInitialInfection()
{
	for (CroyaPlayer* each : players) {
		if (!each)
			continue;
		if (each->IsHuman() && each->GetPlayer()->GetTeam() != TEAM_SPECTATORS)
			humans.push_back(each->GetClientID());
	}
	if (GetRealPlayerNum() < 2) {
		humans.clear();
		return;
	}

	auto infect_random_human = [this](int n) {
		for (int i = 0; i < n; i++) {
			int HumanVecID = random_int_range(0, humans.size() - 1);
			int RandomHumanID = humans[HumanVecID];
			players[RandomHumanID]->SetOldClassNum(players[RandomHumanID]->GetClassNum());
			players[RandomHumanID]->TurnIntoRandomZombie();
			humans.erase(humans.begin() + HumanVecID);
		}
	};

	if (GetRealPlayerNum() <= 3 && GetZombieCount() < 1) {
		infect_random_human(1);
	}
	else if (GetRealPlayerNum() > 3 && GetZombieCount() < 2) {
		infect_random_human(2);
	}
	
	humans.clear();
}

void CGameControllerMOD::TurnDefaultIntoRandomHuman()
{
	for (CroyaPlayer* each : players) {
		if (!each)
			continue;
		if (!each->GetCharacter())
			continue;
		if (each->GetClassNum() == Class::DEFAULT && each->GetPlayer()->GetTeam() != TEAM_SPECTATORS) {
			each->TurnIntoRandomHuman();
			each->GetCharacter()->IncreaseArmor(10);
		}
	}
}

void CGameControllerMOD::OnRoundEnd()
{
	m_InfectedStarted = false;
	ResetFinalExplosion();
	delete lua;
	lua = nullptr;
	circles.clear();
	inf_circles.clear();

	IGameController::EndRound();
}

bool CGameControllerMOD::DoWincheckMatch()
{
	return false;
}

void CGameControllerMOD::DoWincheckRound()
{
}

void CGameControllerMOD::OnCharacterSpawn(CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();

	players[ClientID]->SetCharacter(pChr);
	players[ClientID]->OnCharacterSpawn(pChr);
	if (!m_pGameWorld) {
		m_pGameWorld = pChr->GameWorld();
	}

	if (pChr->IsZombie() && !IsCroyaWarmup() && !m_ExplosionStarted) {
		const int TILE_SIZE = 32;
		int RandomInfectionCircleID = random_int_range(0, inf_circles.size() - 1);
		vec2 NewPos = inf_circles[RandomInfectionCircleID]->GetPos();
		int RandomShiftX = random_int_range(-2, 2); // todo: make this configurable in .lua
		int RandomShiftY = random_int_range(-1, -3); // todo: make this configurable in .lua
		NewPos.x += RandomShiftX * TILE_SIZE;
		NewPos.y += RandomShiftY * TILE_SIZE;
		pChr->GetCharacterCore().m_Pos = NewPos;
		pChr->GameServer()->CreatePlayerSpawn(NewPos);
	}
}

int CGameControllerMOD::OnCharacterDeath(CCharacter* pVictim, CPlayer* pKiller, int Weapon)
{
	int ClientID = pVictim->GetPlayer()->GetCID();

	players[ClientID]->OnCharacterDeath(pVictim, pKiller, Weapon);
	return 0;
}

bool CGameControllerMOD::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if (players[ClientID1]->IsHuman() == players[ClientID2]->IsHuman() && ClientID1 != ClientID2)
		return true;
	return false;
}

int CGameControllerMOD::GetZombieCount() const
{
	int ZombieCount = 0;
	for (CroyaPlayer* each : players) {
		if (!each)
			continue;
		if (each->IsZombie() && each->GetPlayer()->GetTeam() != TEAM_SPECTATORS) {
			ZombieCount++;
		}
	}
	return ZombieCount;
}

int CGameControllerMOD::GetHumanCount() const
{
	int HumanCount = 0;
	for (CroyaPlayer* each : players) {
		if (!each)
			continue;
		if (each->IsHuman() && each->GetPlayer()->GetTeam() != TEAM_SPECTATORS) {
			HumanCount++;
		}
	}
	return HumanCount;
}

bool CGameControllerMOD::IsEveryoneInfected() const
{
	bool EveryoneInfected = false;
	int ZombieCount = GetZombieCount();
	if (GetRealPlayerNum() >= 2 && GetRealPlayerNum() == ZombieCount)
		EveryoneInfected = true;
	return EveryoneInfected;
}

void CGameControllerMOD::OnPlayerDisconnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerDisconnect(pPlayer);
	int ClientID = pPlayer->GetCID();

	delete players[ClientID];
	players[ClientID] = nullptr;
}

void CGameControllerMOD::OnPlayerConnect(CPlayer* pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientID = pPlayer->GetCID();

#ifdef CONF_GEOLOCATION
	char aAddrStr[NETADDR_MAXSTRSIZE];
	Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));
	std::string ip(aAddrStr);
	Server()->SetClientCountry(ClientID, geolocation->get_country_iso_numeric_code(ip));
#endif
	players[ClientID] = new CroyaPlayer(ClientID, pPlayer, GameServer(), this, classes);
	SetLanguageByCountry(Server()->ClientCountry(ClientID), ClientID);
	if (IsCroyaWarmup()) {
		players[ClientID]->SetClass(classes[Class::DEFAULT]);
	}
	else {
		players[ClientID]->TurnIntoRandomZombie();
		players[ClientID]->SetOldClassNum(Class::MEDIC); // turn into medic on medic revive
	}
	GameServer()->SendChatTarget(ClientID, g_Config.m_SvWelcome);
}

std::array<CroyaPlayer*, 64> CGameControllerMOD::GetCroyaPlayers()
{
	return players;
}

void CGameControllerMOD::ResetFinalExplosion()
{
	m_ExplosionStarted = false;

	for (int j = 0; j < m_MapHeight; j++)
	{
		for (int i = 0; i < m_MapWidth; i++)
		{
			if (!(m_GrowingMap[j * m_MapWidth + i] & 4))
			{
				m_GrowingMap[j * m_MapWidth + i] = 1;
			}
		}
	}
}

bool CGameControllerMOD::IsExplosionStarted() const
{
	return m_ExplosionStarted;
}

std::vector<class CCircle*>& CGameControllerMOD::GetCircles()
{
	return circles;
}

std::vector<class CInfCircle*>& CGameControllerMOD::GetInfCircles()
{
	return inf_circles;
}

void CGameControllerMOD::SetLanguageByCountry(int Country, int ClientID)
{
	switch (Country)
	{
		/* ar - Arabic ************************************/
	case 12: //Algeria
	case 48: //Bahrain
	case 262: //Djibouti
	case 818: //Egypt
	case 368: //Iraq
	case 400: //Jordan
	case 414: //Kuwait
	case 422: //Lebanon
	case 434: //Libya
	case 478: //Mauritania
	case 504: //Morocco
	case 512: //Oman
	case 275: //Palestine
	case 634: //Qatar
	case 682: //Saudi Arabia
	case 706: //Somalia
	case 729: //Sudan
	case 760: //Syria
	case 788: //Tunisia
	case 784: //United Arab Emirates
	case 887: //Yemen
		// set to arabic
		break;
		/* bg - Bulgarian *************************************/
	case 100: //Bulgaria
		// set to bulgarian
		break;
		/* bs - Bosnian *************************************/
	case 70: //Bosnia and Hercegovina
		// set to bosnian
		break;
		/* cs - Czech *************************************/
	case 203: //Czechia
		// set to czech
		break;
		/* de - German ************************************/
	case 40: //Austria
	case 276: //Germany
	case 438: //Liechtenstein
	case 756: //Switzerland
		players[ClientID]->SetLanguage("german");
		break;
		/* el - Greek ***********************************/
	case 300: //Greece
	case 196: //Cyprus
		// set to greek
		break;
		/* es - Spanish ***********************************/
	case 32: //Argentina
	case 68: //Bolivia
	case 152: //Chile
	case 170: //Colombia
	case 188: //Costa Rica
	case 192: //Cuba
	case 214: //Dominican Republic
	case 218: //Ecuador
	case 222: //El Salvador
	case 226: //Equatorial Guinea
	case 320: //Guatemala
	case 340: //Honduras
	case 484: //Mexico
	case 558: //Nicaragua
	case 591: //Panama
	case 600: //Paraguay
	case 604: //Peru
	case 630: //Puerto Rico
	case 724: //Spain
	case 858: //Uruguay
	case 862: //Venezuela
		// set to spanish
		break;
		/* fa - Farsi ************************************/
	case 364: //Islamic Republic of Iran
	case 4: //Afghanistan
		// set to farsi
		break;
		/* fr - French ************************************/
	case 204: //Benin
	case 854: //Burkina Faso
	case 178: //Republic of the Congo
	case 384: //Cote d’Ivoire
	case 266: //Gabon
	case 324: //Ginea
	case 466: //Mali
	case 562: //Niger
	case 686: //Senegal
	case 768: //Togo
	case 250: //France
	case 492: //Monaco
		// set to french
		break;
		/* hr - Croatian **********************************/
	case 191: //Croatia
		// set to croatian
		break;
		/* hu - Hungarian *********************************/
	case 348: //Hungary
		// set to hungarian
		break;
		/* it - Italian ***********************************/
	case 380: //Italy
		// set to italian
		break;
		/* ja - Japanese **********************************/
	case 392: //Japan
		// set to japanese
		break;
		/* la - Latin *************************************/
	case 336: //Vatican
		// set to latin
		break;
		/* nl - Dutch *************************************/
	case 533: //Aruba
	case 531: //Curaçao
	case 534: //Sint Maarten
	case 528: //Netherland
	case 740: //Suriname
	case 56: //Belgique
		// set to dutch
		break;
		/* pl - Polish *************************************/
	case 616: //Poland
		// set to polish
		break;
		/* pt - Portuguese ********************************/
	case 24: //Angola
	case 76: //Brazil
	case 132: //Cape Verde
	//case 226: //Equatorial Guinea: official language, but not national language
	//case 446: //Macao: official language, but spoken by less than 1% of the population
	case 508: //Mozambique
	case 626: //Timor-Leste
	case 678: //São Tomé and Príncipe
		// set to portuguese
		break;
		/* ru - Russian ***********************************/
	case 112: //Belarus
	case 643: //Russia
	case 398: //Kazakhstan
		players[ClientID]->SetLanguage("russian");
		break;
		/* sk - Slovak ************************************/
	case 703: //Slovakia
		// set to slovak
		break;
		/* sr - Serbian ************************************/
	case 688: //Serbia
		// set to serbian
		break;
		/* tl - Tagalog ************************************/
	case 608: //Philippines
		// set to tagalog
		break;
		/* tr - Turkish ************************************/
	case 31: //Azerbaijan
	case 792: //Turkey
		// set to turkish
		break;
		/* uk - Ukrainian **********************************/
	case 804: //Ukraine
		// set to ukrainian
		break;
		/* zh-Hans - Chinese (Simplified) **********************************/
	case 156: //People’s Republic of China
	case 344: //Hong Kong
	case 446: //Macau
		// set to chinese
		break;
	}
}