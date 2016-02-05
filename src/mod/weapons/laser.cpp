#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

#include <mod/entities/character.h>
#include <mod/entities/laser.h>

#include "laser.h"

CMod_Weapon_Laser::CMod_Weapon_Laser(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MOD_WEAPON_LASER, WEAPON_LASER, pCharacter, Ammo)
{
	
}

void CMod_Weapon_Laser::CreateProjectile(vec2 Pos, vec2 Direction)
{
	new CLaser(GameWorld(), Pos, Direction, GameServer()->Tuning()->m_LaserReach, Player()->GetCID());
	
	CModAPI_WorldEvent_Sound(GameServer(), WorldID())
		.Send(Character()->GetPos(), SOUND_LASER_FIRE);
}
