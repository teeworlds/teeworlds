/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "ninja.h"

CWeaponNinja::CWeaponNinja(CCharacter *pOwner, int Ammo)
: CWeapon(pOwner, WEAPON_NINJA, true, false, Ammo)
{
	m_ActivationTick = Server()->Tick();
	m_CurrentMoveTime = -1;
}

void CWeaponNinja::OnFire(vec2 Direction, vec2 ProjStartPos)
{
	m_NumHitObjects = 0;
	m_ActivationDir = Direction;
	m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
	m_OldVelAmount = length(GetOwner()->GetCore()->m_Vel);

	GameServer()->CreateSound(GetOwner()->GetPos(), SOUND_NINJA_FIRE);
}

void CWeaponNinja::OnTick()
{
	CCharacterCore *pCore = GetOwner()->GetCore();

	if((Server()->Tick() - m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// reset velocity
		if(m_CurrentMoveTime > 0)
			pCore->m_Vel = m_ActivationDir*m_OldVelAmount;

		// return weapon
		GetOwner()->RemoveWeapon(WEAPON_NINJA);
		return;
	}

	m_CurrentMoveTime--;

	if(m_CurrentMoveTime == 0)
	{
		// reset velocity
		pCore->m_Vel = m_ActivationDir*m_OldVelAmount;
	}
	else if(m_CurrentMoveTime > 0)
	{
		// set velocity
		pCore->m_Vel = m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = GetOwner()->GetPos();
		int CharRadius = GetOwner()->GetProximityRadius();
		GameServer()->Collision()->MoveBox(&pCore->m_Pos, &pCore->m_Vel, vec2(CharRadius, CharRadius), 0.0f);

		// reset velocity so the client doesn't predict stuff
		pCore->m_Vel = vec2(0.0f, 0.0f);

		// check if we hit anything along the way
		CCharacter *apEnts[MAX_CLIENTS];
		vec2 Dir = GetOwner()->GetPos() - OldPos;
		float Radius = CharRadius * 2.0f;
		vec2 Center = OldPos + Dir * 0.5f;
		int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for(int i = 0; i < Num; i++)
		{
			if(apEnts[i] == GetOwner())
				continue;

			// make sure we haven't hit this object before
			if(AlreadyHit(apEnts[i]))
				continue;

			// check that we are close enough
			if(distance(apEnts[i]->GetPos(), GetOwner()->GetPos()) > Radius)
				continue;

			// hit a player, give him damage and stuffs...
			GameServer()->CreateSound(apEnts[i]->GetPos(), SOUND_NINJA_HIT);

			// set his velocity to fast upward
			if(m_NumHitObjects < MAX_CLIENTS)
				m_apHitObjects[m_NumHitObjects++] = apEnts[i];

			apEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, GetOwner()->GetPlayer()->GetCID(), WEAPON_NINJA);
		}
	}
}

void CWeaponNinja::OnTickPaused()
{
	m_ActivationTick++;
}

int CWeaponNinja::SnapAmmo()
{
	return m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}

bool CWeaponNinja::AlreadyHit(CEntity *pEntity)
{
	for(int i = 0; i < m_NumHitObjects; i++)
	{
		if(m_apHitObjects[i] == pEntity)
			return true;
	}
	return false;
}
