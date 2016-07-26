#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

#include <mod/entities/character.h>
#include <mod/entities/projectile.h>

#include "grenade.h"

CMod_Weapon_Grenade::CMod_Weapon_Grenade(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MOD_WEAPON_GRENADE, WEAPON_GRENADE, pCharacter, Ammo)
{
	
}

void CMod_Weapon_Grenade::CreateProjectile(vec2 Pos, vec2 Direction)
{
	new CProjectile(GameWorld(), WEAPON_GRENADE,
		Player()->GetCID(),
		Pos,
		Direction,
		(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
		g_pData->m_Weapons.m_Grenade.m_pBase->m_Damage, true, 0, SOUND_GRENADE_EXPLODE, MOD_WEAPON_GRENADE);

	CModAPI_Event_Sound(GameServer()).World(WorldID())
		.Send(Character()->GetPos(), SOUND_GRENADE_FIRE);
}
