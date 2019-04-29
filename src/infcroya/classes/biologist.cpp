#include "biologist.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <infcroya/entities/biologist-mine.h>
#include <generated/server_data.h>
#include <game/server/entities/projectile.h>
#include <base/math.h>
#include <infcroya\entities\bouncing-bullet.h>
#include <engine/message.h>

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
	pChr->GiveWeapon(WEAPON_SHOTGUN, 10);
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

			pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Dir * -1, 20,
				pChr->GetPlayer()->GetCID(), pChr->GetActiveWeapon());
			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if (Hits)
			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 3);
	} break;

	case WEAPON_GUN: {
		new CProjectile(pChr->GameWorld(), WEAPON_GUN,
			pChr->GetPlayer()->GetCID(),
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pChr->GameServer()->Tuning()->m_GunLifetime),
			g_pData->m_Weapons.m_Gun.m_pBase->m_Damage, false, 0, -1, WEAPON_GUN);

		pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_GUN_FIRE);
	} break;

	case WEAPON_SHOTGUN: {
		int ShotSpread = 2;
		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(ShotSpread * 2 + 1);

		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = { -0.185f, -0.070f, 0, 0.070f, 0.185f };
			float a = angle(Direction);
			a += Spreading[i + 2];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)pChr->GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			
			CBouncingBullet* pProj = new CBouncingBullet(pChr->GameWorld(), pChr->GetPlayer()->GetCID(), ProjStartPos, vec2(cosf(a), sinf(a)) * Speed);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);
			for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
				Msg.AddInt(((int*)& p)[i]);
		}

		pChr->GameServer()->CreateSound(pChr->GetPos(), SOUND_SHOTGUN_FIRE);
	} break;

	case WEAPON_LASER: {
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
	} break;
	}
}
