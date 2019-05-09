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
#include <engine/shared/config.h>
#include <infcroya/localization/localization.h>
#include <engine/storage.h>
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
	classes[Class::DEFAULT] = new CDefault();
	classes[Class::BIOLOGIST] = new CBiologist();
	classes[Class::ENGINEER] = new CEngineer();
	classes[Class::MEDIC] = new CMedic();
	classes[Class::SOLDIER] = new CSoldier();
	classes[Class::SCIENTIST] = new CScientist();
	classes[Class::SMOKER] = new CSmoker();
}

CGameControllerMOD::~CGameControllerMOD()
{
#ifdef CONF_GEOLOCATION
	delete geolocation;
#endif
	for (const auto& c : classes) {
		delete c.second;
	}
}

void CGameControllerMOD::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}

void CGameControllerMOD::Tick()
{
	IGameController::Tick();

	if (IsGameRunning() && IsEveryoneInfected()) {
		EndRound();
	}
}

/*
void CGameControllerMOD::StartRound()
{
}*/

void CGameControllerMOD::OnCharacterSpawn(CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();

	players[ClientID]->SetCharacter(pChr);
	players[ClientID]->OnCharacterSpawn(pChr);
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

bool CGameControllerMOD::IsEveryoneInfected() const
{
	bool EveryoneInfected = false;
	int ZombiesCount = 0;
	for (CPlayer* each : GameServer()->m_apPlayers) {
		if (each) {
			CCharacter* pChr = each->GetCharacter();
			if (pChr && pChr->IsZombie())
				ZombiesCount++;
		}
	}
	if (GetRealPlayerNum() >= 2 && GetRealPlayerNum() == ZombiesCount)
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
	if (!IsGameRunning()) {
		players[ClientID]->SetClass(classes[Class::DEFAULT]);
	}
	else {
		players[ClientID]->TurnIntoRandomZombie();
		players[ClientID]->SetOldClassNum(Class::MEDIC); // turn into medic on medic revive
	}
	GameServer()->SendChatTarget(ClientID, g_Config.m_SvWelcome);
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
