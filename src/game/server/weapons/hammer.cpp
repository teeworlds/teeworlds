/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "hammer.h"

CWeaponHammer::CWeaponHammer(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_HAMMER, false, false, Ammo)
{
}

void CWeaponHammer::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	CCharacter *apEnts[MAX_CLIENTS];
	float HammerRadius = GetOwner()->GetProximityRadius() * 0.5f;
	int Num = GameServer()->m_World.FindEntities(ProjStartPos, HammerRadius, (CEntity**)apEnts,
													MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	int Hits = 0;
	for(int i = 0; i < Num; i++)
	{
		CCharacter *pTarget = apEnts[i];
		vec2 TargetPos = pTarget->GetPos();

		// check valid target
		if(pTarget == GetOwner() || GameServer()->Collision()->IntersectLine(ProjStartPos, TargetPos, 0, 0))
			continue;

		// hammer hit event
		vec2 ProjDir = TargetPos - ProjStartPos;
		vec2 HammerHitPos;
		if(length(ProjDir) > 0.0f)
			HammerHitPos = TargetPos - normalize(ProjDir) * HammerRadius;
		else
			HammerHitPos = ProjStartPos;
		GameServer()->CreateHammerHit(HammerHitPos);

		// damage dir
		vec2 CharsDir = TargetPos - GetOwner()->GetPos();
		vec2 DamageDir;
		if(length(CharsDir) > 0.0f)
			DamageDir = normalize(CharsDir);
		else
			DamageDir = vec2(0.0f, -1.0f);

		// deal damage
		vec2 Force = vec2(0.0f, -1.0f) + normalize(DamageDir + vec2(0.0f, -1.1f)) * 10.0f;
		int Damage = g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage;
		pTarget->TakeDamage(Force, Damage, GetOwner()->GetPlayer()->GetCID(), WEAPON_HAMMER);

		Hits++;
	}

	// if we hit anything, we have to wait for the reload
	if(Hits)
		GetOwner()->SetReloadTimer(Server()->TickSpeed() / 3);

	GameServer()->CreateSound(GetOwner()->GetPos(), SOUND_HAMMER_FIRE);
}
