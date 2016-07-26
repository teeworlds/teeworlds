#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

#include <mod/entities/character.h>

#include "ninja.h"

CMod_Weapon_Ninja::CMod_Weapon_Ninja(CCharacter* pCharacter) :
	CModAPI_Weapon(MOD_WEAPON_NINJA, pCharacter)
{
	m_ActivationTick = Server()->Tick();
	m_CurrentMoveTime = -1;
	m_ReloadTimer = 0;
}
	
int CMod_Weapon_Ninja::GetMaxAmmo() const
{
	return g_pData->m_Weapons.m_aId[WEAPON_NINJA].m_Maxammo;
}

int CMod_Weapon_Ninja::GetAmmo() const
{
	return m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}

bool CMod_Weapon_Ninja::OnFire(vec2 Direction)
{
	if(m_ReloadTimer > 0)
		return false;

	m_NumObjectsHit = 0;

	m_ActivationDir = Direction;
	m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
	m_ReloadTimer = g_pData->m_Weapons.m_aId[WEAPON_NINJA].m_Firedelay * Server()->TickSpeed() / 1000;
	m_OldVelAmount = length(Character()->GetVelocity());

	CModAPI_Event_Sound(GameServer()).World(WorldID())
		.Send(Character()->GetPos(), SOUND_NINJA_FIRE);
	
	return true;
}

bool CMod_Weapon_Ninja::TickPreFire(bool IsActiveWeapon)
{
	if(m_ReloadTimer > 0)
		m_ReloadTimer--;

	if ((Server()->Tick() - m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		if(m_CurrentMoveTime > 0)
			Character()->SetVelocity(m_ActivationDir*m_OldVelAmount);

		return false; // deletes the weapon from the character (remove ninja)
	}

	m_CurrentMoveTime--;

	if (m_CurrentMoveTime == 0)
	{
		// reset velocity
		Character()->SetVelocity(m_ActivationDir*m_OldVelAmount);
	}

	if (m_CurrentMoveTime > 0)
	{
		// set velocity
		Character()->SetVelocity(m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity);
		vec2 OldPos = Character()->GetPos();
		
		vec2 Pos = Character()->GetPos();
		vec2 Velocity = Character()->GetVelocity();
		float ProximityRadius = Character()->GetProximityRadius();
		
		GameServer()->Collision()->MoveBox(&Pos, &Velocity, vec2(ProximityRadius, ProximityRadius), 0.f);
		
		Character()->SetPos(Pos);
		Character()->SetVelocity(Velocity);

		// reset velocity so the client doesn't predict stuff
		Character()->SetVelocity(vec2(0.f, 0.f));

		// check if we hit anything along the way
		CCharacter *aEnts[MAX_CLIENTS];
		vec2 Dir = Character()->GetPos() - OldPos;
		float Radius = ProximityRadius * 2.0f;
		vec2 Center = OldPos + Dir * 0.5f;
		int Num = GameServer()->m_World[WorldID()].FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);

		for (int i = 0; i < Num; ++i)
		{
			if (aEnts[i] == Character())
				continue;

			// make sure we haven't hit this object before
			bool bAlreadyHit = false;
			for (int j = 0; j < m_NumObjectsHit; j++)
			{
				if (m_apHitObjects[j] == aEnts[i])
					bAlreadyHit = true;
			}
			if (bAlreadyHit)
				continue;

			// check so we are sufficiently close
			if (distance(aEnts[i]->GetPos(), Character()->GetPos()) > (ProximityRadius * 2.0f))
				continue;

			// hit a player, give him damage and stuffs...
			CModAPI_Event_Sound(GameServer()).World(WorldID())
				.Send(aEnts[i]->GetPos(), SOUND_NINJA_HIT);
				
			// set his velocity to fast upward (for now)
			if(m_NumObjectsHit < 10)
				m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

			aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, Player()->GetCID(), MOD_WEAPON_NINJA);
		}
	}

	return true;
}

bool CMod_Weapon_Ninja::TickPaused(bool IsActive)
{
	m_ActivationTick++;
	return false;
}
	
void CMod_Weapon_Ninja::Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_NINJA;
	pCharNetObj->m_AmmoCount = -1;
}
	
void CMod_Weapon_Ninja::Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_NINJA;
	pCharNetObj->m_AmmoCount = m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}
	
void CMod_Weapon_Ninja::Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_NINJA;
	pCharNetObj->m_AmmoCount = m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}
