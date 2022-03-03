/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"
#include <game/server/gamecontext.h>


CGameControllerDM::CGameControllerDM(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "DM";
}

/* Race podium support example
void CGameControllerDM::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
	// Send Info about who carries which podium place
	if(Server()->GetClientVersion(SnappingClient) >= GameServer()->MIN_RACE_GIMMIC_CLIENTVERSION)
	{
		CNetObj_GameDataRaceFlag *pGameDataRaceFlag = static_cast<CNetObj_GameDataRaceFlag *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATARACEFLAG, 0, sizeof(CNetObj_GameDataRaceFlag)));
		if(!pGameDataRaceFlag)
			return;
		pGameDataRaceFlag->m_FlagCarrierRaceGold = 0;
		pGameDataRaceFlag->m_FlagCarrierRaceSilver = 1;
		pGameDataRaceFlag->m_FlagCarrierRaceBronze = 2;
	}
}*/
