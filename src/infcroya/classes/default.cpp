#include "default.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <generated/server_data.h>

CDefault::CDefault()
{
	SetSkin(CSkin());
	SetInfectedClass(false);
}

CDefault::~CDefault()
{
}

void CDefault::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

void CDefault::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	switch (Weapon) {
	case WEAPON_GUN: {
		new CProjectile(pChr->GameWorld(), WEAPON_GUN,
			pChr->GetPlayer()->GetCID(),
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pChr->GameServer()->Tuning()->m_GunLifetime),
			g_pData->m_Weapons.m_Gun.m_pBase->m_Damage, false, 0, -1, WEAPON_GUN);

		pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
	} break;
	}
}