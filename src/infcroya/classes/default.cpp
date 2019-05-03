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
	SetName("Random choice");
}

CDefault::~CDefault()
{
}

void CDefault::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
	pChr->SetNormalEmote(EMOTE_NORMAL);
}

void CDefault::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	CGameWorld* pGameWorld = pChr->GameWorld();
	CGameContext* pGameServer = pChr->GameServer();

	switch (Weapon) {
	case WEAPON_GUN: {
		new CProjectile(pGameWorld, WEAPON_GUN,
			ClientID,
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pGameServer->Tuning()->m_GunLifetime),
			g_pData->m_Weapons.m_Gun.m_pBase->m_Damage, false, 0, -1, WEAPON_GUN);

		pGameServer->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
	} break;
	}
}