#include "scientist.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <generated/server_data.h>
#include <infcroya/entities/scientist-mine.h>
#include <infcroya/entities/scientist-laser.h>
#include <engine/shared/config.h>

CScientist::CScientist()
{
	CSkin skin;
	skin.SetBodyColor(93, 95, 163);
	skin.SetMarkingName("toptri");
	skin.SetMarkingColor(0, 0, 255);
	skin.SetHandsColor(55, 141, 170);
	skin.SetFeetColor(88, 97, 119);
	SetSkin(skin);
	SetInfectedClass(false);
	SetName("Scientist");
}

CScientist::~CScientist()
{
}

void CScientist::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, 10);
	pChr->GiveWeapon(WEAPON_LASER, 10);
	pChr->SetWeapon(WEAPON_GUN);
	pChr->SetNormalEmote(EMOTE_NORMAL);
}

void CScientist::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	CGameWorld* pGameWorld = pChr->GameWorld();
	CGameContext* pGameServer = pChr->GameServer();

	switch (Weapon) {
	case WEAPON_HAMMER: {
		bool FreeSpace = true;
		int NbMine = 0;

		int OlderMineTick = pChr->Server()->Tick() + 1;
		CScientistMine* pOlderMine = 0;
		CScientistMine* pIntersectMine = 0;

		CScientistMine* p = (CScientistMine*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_SCIENTIST_MINE);
		while (p)
		{
			float d = distance(p->GetPos(), ProjStartPos);

			if (p->GetOwner() == ClientID)
			{
				if (OlderMineTick > p->m_StartTick)
				{
					OlderMineTick = p->m_StartTick;
					pOlderMine = p;
				}
				NbMine++;

				if (d < 2.0f * g_Config.m_InfMineRadius)
				{
					if (pIntersectMine)
						FreeSpace = false;
					else
						pIntersectMine = p;
				}
			}
			else if (d < 2.0f * g_Config.m_InfMineRadius)
				FreeSpace = false;

			p = (CScientistMine*)p->TypeNext();
		}

		if (FreeSpace)
		{
			if (pIntersectMine) //Move the mine
				pGameServer->m_World.DestroyEntity(pIntersectMine);
			else if (NbMine >= g_Config.m_InfMineLimit && pOlderMine)
				pGameServer->m_World.DestroyEntity(pOlderMine);

			new CScientistMine(pGameWorld, ProjStartPos, ClientID);

			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 2);
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

	case WEAPON_LASER: {
		int Damage = 6;
		new CScientistLaser(pGameWorld, pChr->GetPos(), Direction, pGameServer->Tuning()->m_LaserReach * 0.6f, ClientID, Damage);
		pGameServer->CreateSound(pChr->GetPos(), SOUND_LASER_FIRE);
	} break;
	}
}
