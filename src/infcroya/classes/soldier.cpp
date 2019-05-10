#include "soldier.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <generated/server_data.h>
#include <infcroya/entities/soldier-bomb.h>

CSoldier::CSoldier()
{
	CSkin skin;
	skin.SetBodyName("bear");
	skin.SetBodyColor(16, 133, 121);
	skin.SetMarkingName("bear");
	skin.SetMarkingColor(17, 110, 168);
	skin.SetDecorationName("hair");
	skin.SetHandsColor(16, 133, 121);
	skin.SetFeetColor(17, 129, 38);
	SetSkin(skin);
	SetInfectedClass(false);
	SetName("Soldier");
}

void CSoldier::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, 10);
	pChr->SetWeapon(WEAPON_GUN);
	pChr->SetNormalEmote(EMOTE_NORMAL);
}

void CSoldier::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	CGameWorld* pGameWorld = pChr->GameWorld();
	CGameContext* pGameServer = pChr->GameServer();

	switch (Weapon) {
	case WEAPON_HAMMER: {
		bool BombFound = false;
		for (CSoldierBomb* pBomb = (CSoldierBomb*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_SOLDIER_BOMB); pBomb; pBomb = (CSoldierBomb*)pBomb->TypeNext())
		{
			if (pBomb->m_Owner == ClientID)
			{
				pBomb->Explode();
				BombFound = true;
			}
		}

		if (!BombFound)
		{
			new CSoldierBomb(pGameWorld, ProjStartPos, ClientID);
			pGameServer->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
		}
	} break;

	case WEAPON_GUN: {
		new CProjectile(pGameWorld, WEAPON_GUN,
			ClientID,
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pGameServer->Tuning()->m_GunLifetime),
			g_pData->m_Weapons.m_Gun.m_pBase->m_Damage, false, 0, -1, WEAPON_GUN);

		pGameServer->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
	} break;

	case WEAPON_GRENADE: {
		new CProjectile(pGameWorld, WEAPON_GRENADE,
			ClientID,
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pGameServer->Tuning()->m_GrenadeLifetime),
			g_pData->m_Weapons.m_Grenade.m_pBase->m_Damage, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

		pGameServer->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);
	} break;
	}
}
