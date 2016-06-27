#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

#include <mod/entities/character.h>

#include "hammer.h"

CMod_Weapon_Hammer::CMod_Weapon_Hammer(CCharacter* pCharacter) :
	CModAPI_Weapon(MOD_WEAPON_HAMMER, pCharacter)
{
	m_ReloadTimer = 0;
}

bool CMod_Weapon_Hammer::TickPreFire(bool IsActive)
{
	if(IsActive && m_ReloadTimer > 0)
		m_ReloadTimer--;

	return true;
}

bool CMod_Weapon_Hammer::IsWeaponSwitchLocked() const
{
	return m_ReloadTimer != 0;
}

bool CMod_Weapon_Hammer::OnFire(vec2 Direction)
{
	if(m_ReloadTimer > 0)
		return false;
	
	vec2 ProjStartPos = Character()->GetPos() + Direction * Character()->GetProximityRadius()*0.75f;
	
	CModAPI_Event_Sound(GameServer()).World(WorldID())
		.Send(Character()->GetPos(), SOUND_HAMMER_FIRE);

	CCharacter *apEnts[MAX_CLIENTS];
	int Hits = 0;
	int Num = GameServer()->m_World[WorldID()].FindEntities(ProjStartPos, Character()->GetProximityRadius()*0.5f, (CEntity**)apEnts,
												MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; ++i)
	{
		CCharacter *pTarget = apEnts[i];

		if ((pTarget == Character()) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->GetPos(), NULL, NULL))
			continue;

		// set his velocity to fast upward (for now)
		vec2 HammerHitPos;
		
		if(length(pTarget->GetPos()-ProjStartPos) > 0.0f)
			HammerHitPos = pTarget->GetPos()-normalize(pTarget->GetPos()-ProjStartPos)*Character()->GetProximityRadius()*0.5f;
		else
			HammerHitPos = ProjStartPos;
			
		CModAPI_Event_HammerHitEffect(GameServer()).World(WorldID())
			.Send(HammerHitPos);

		vec2 Dir;
		if (length(pTarget->GetPos() - Character()->GetPos()) > 0.0f)
		{
			Dir = normalize(pTarget->GetPos() - Character()->GetPos());
		}
		else
		{
			Dir = vec2(0.f, -1.f);
		}

		pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
			Player()->GetCID(), MOD_WEAPON_HAMMER);
			
		Hits++;
	}

	// if we Hit anything, we have to wait for the reload
	if(Hits)
		m_ReloadTimer = Server()->TickSpeed()/3;
	
	return true;
}
	
void CMod_Weapon_Hammer::Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_HAMMER;
	pCharNetObj->m_AmmoCount = -1;
}
	
void CMod_Weapon_Hammer::Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_HAMMER;
	pCharNetObj->m_AmmoCount = -1;
}
	
void CMod_Weapon_Hammer::Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = WEAPON_HAMMER;
	pCharNetObj->m_AmmoCount = -1;
}
