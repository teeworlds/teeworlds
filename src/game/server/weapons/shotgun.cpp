/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>
#include <game/server/entities/projectiles/shotgun.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "shotgun.h"

CWeaponShotgun::CWeaponShotgun(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_SHOTGUN, false, true, Ammo)
{
}

void CWeaponShotgun::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	static const int SHOTSPREAD = 2;
	static const int NUM_SHOTS = 2*SHOTSPREAD + 1;

	static const float s_aSpreading[NUM_SHOTS] = {-0.185f, -0.070f, 0.000f, 0.070f, 0.185f};
	const CProjectile *apProjs[NUM_SHOTS];

	for(int i = 0; i < NUM_SHOTS; i++)
	{
		// angle
		float a = angle(Direction) + s_aSpreading[i];

		// speed
		float v = 1.0f - (absolute(i-SHOTSPREAD) / (float) SHOTSPREAD);
		float Speed = mix((float) GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
		vec2 ShotVel = vec2(cosf(a), sinf(a)) * Speed;

		// projectile
		apProjs[i] = new CProjectileShotgun(&GameServer()->m_World,
			GetOwner()->GetPlayer()->GetCID(), ShotVel, ProjStartPos);
	}

	SendProjectiles(apProjs, NUM_SHOTS);

	GameServer()->CreateSound(GetOwner()->GetPos(), SOUND_SHOTGUN_FIRE);
}
