#include "biologist.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <infcroya/entities/biologist-mine.h>
#include <generated/server_data.h>

CBiologist::CBiologist()
{
	CSkin skin;
	skin.SetBodyColor(52, 156, 124);
	skin.SetMarkingName("twintri");
	skin.SetMarkingColor(40, 222, 227);
	skin.SetFeetColor(147, 4, 72);
	SetSkin(skin);
	SetInfectedClass(false);
}

CBiologist::~CBiologist()
{
}

void CBiologist::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_LASER, 10);
	pChr->SetWeapon(WEAPON_GUN);
}

void CBiologist::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	switch (Weapon) {
	case WEAPON_HAMMER: {
		// reset objects Hit
		pChr->SetNumObjectsHit(0);
		pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_HAMMER_FIRE);

		CCharacter* apEnts[MAX_CLIENTS];
		int Hits = 0;
		int Num = pChr->GameServer()->m_World.FindEntities(ProjStartPos, pChr->GetProximityRadius() * 0.5f, (CEntity * *)apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for (int i = 0; i < Num; ++i)
		{
			CCharacter* pTarget = apEnts[i];

			if ((pTarget == pChr) || pChr->GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->GetPos(), NULL, NULL))
				continue;

			// set his velocity to fast upward (for now)
			if (length(pTarget->GetPos() - ProjStartPos) > 0.0f)
				pChr->GameServer()->CreateHammerHit(pTarget->GetPos() - normalize(pTarget->GetPos() - ProjStartPos) * pChr->GetProximityRadius() * 0.5f);
			else
				pChr->GameServer()->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if (length(pTarget->GetPos() - pChr->GetPos()) > 0.0f)
				Dir = normalize(pTarget->GetPos() - pChr->GetPos());
			else
				Dir = vec2(0.f, -1.f);

			pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Dir * -1, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
				pChr->GetPlayer()->GetCID(), pChr->GetActiveWeapon());
			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if (Hits)
			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 3);
	} break;

	case WEAPON_LASER:
		CGameWorld* pGameWorld = pChr->GameWorld();
		for (CBiologistMine* pMine = (CBiologistMine*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_BIOLOGIST_MINE); pMine; pMine = (CBiologistMine*)pMine->TypeNext())
		{
			if (pMine->m_Owner != pChr->GetPlayer()->GetCID()) continue;
			pChr->GameServer()->m_World.DestroyEntity(pMine);
		}

		vec2 To = pChr->GetPos() + Direction * 400.0f;
		if (pChr->GameServer()->Collision()->IntersectLine(pChr->GetPos(), To, 0x0, &To))
		{
			new CBiologistMine(pGameWorld, pChr->GetPos(), To, pChr->GetPlayer()->GetCID());
			pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_LASER_FIRE);
			pChr->m_aWeapons[WEAPON_LASER].m_Ammo = 0;
		}
		break;
	}
}
