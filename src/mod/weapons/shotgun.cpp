#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

#include <mod/entities/character.h>
#include <mod/entities/projectile.h>

#include "shotgun.h"

CMod_Weapon_Shotgun::CMod_Weapon_Shotgun(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MOD_WEAPON_SHOTGUN, WEAPON_SHOTGUN, pCharacter, Ammo)
{
	
}

void CMod_Weapon_Shotgun::CreateProjectile(vec2 Pos, vec2 Direction)
{
	int ShotSpread = 2;

	for(int i = -ShotSpread; i <= ShotSpread; ++i)
	{
		float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
		float a = angle(Direction);
		a += Spreading[i+2];
		float v = 1-(absolute(i)/(float)ShotSpread);
		float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
		new CProjectile(GameWorld(), WEAPON_SHOTGUN,
			Player()->GetCID(),
			Pos,
			vec2(cosf(a), sinf(a))*Speed,
			(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
			g_pData->m_Weapons.m_Shotgun.m_pBase->m_Damage, false, 0, -1, GetID());
	}

	CModAPI_Event_Sound(GameServer()).World(WorldID())
		.Send(Character()->GetPos(), SOUND_SHOTGUN_FIRE);
}
