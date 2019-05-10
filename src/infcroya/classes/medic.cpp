#include "medic.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <generated/server_data.h>
#include <infcroya/croyaplayer.h>
#include <infcroya/entities/medic-laser.h>
#include <infcroya/entities/medic-grenade.h>

CMedic::CMedic()
{
	CSkin skin;
	skin.SetBodyColor(233, 158, 183);
	skin.SetMarkingName("duodonny");
	skin.SetMarkingColor(231, 146, 218);
	skin.SetDecorationName("twinbopp");
	skin.SetDecorationColor(233, 158, 183);
	skin.SetHandsColor(233, 158, 183);
	skin.SetFeetColor(0, 146, 224);
	SetSkin(skin);
	SetInfectedClass(false);
	SetName("Medic");
}

void CMedic::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->SetWeapon(WEAPON_GUN);
	pChr->GiveWeapon(WEAPON_SHOTGUN, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, 10);
	pChr->GiveWeapon(WEAPON_LASER, 10);
	pChr->SetNormalEmote(EMOTE_NORMAL);
}

void CMedic::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	CGameWorld* pGameWorld = pChr->GameWorld();
	CGameContext* pGameServer = pChr->GameServer();

	switch (Weapon) {
	case WEAPON_HAMMER: {
		// reset objects Hit
		pChr->SetNumObjectsHit(0);
		pGameServer->CreateSound(pChr->GetPos(), SOUND_HAMMER_FIRE);

		CCharacter* apEnts[MAX_CLIENTS];
		int Hits = 0;
		int Num = pGameServer->m_World.FindEntities(ProjStartPos, pChr->GetProximityRadius() * 0.5f, (CEntity * *)apEnts,
			MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for (int i = 0; i < Num; ++i)
		{
			CCharacter* pTarget = apEnts[i];
			if (pTarget->IsHuman() && pTarget != pChr) {
				pTarget->IncreaseArmor(4);
				pTarget->SetEmote(EMOTE_HAPPY, pChr->Server()->Tick() + pChr->Server()->TickSpeed());
			}
			if ((pTarget == pChr) || pGameServer->Collision()->IntersectLine(ProjStartPos, pTarget->GetPos(), NULL, NULL))
				continue;

			// set his velocity to fast upward (for now)
			if (length(pTarget->GetPos() - ProjStartPos) > 0.0f)
				pGameServer->CreateHammerHit(pTarget->GetPos() - normalize(pTarget->GetPos() - ProjStartPos) * pChr->GetProximityRadius() * 0.5f);
			else
				pGameServer->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if (length(pTarget->GetPos() - pChr->GetPos()) > 0.0f)
				Dir = normalize(pTarget->GetPos() - pChr->GetPos());
			else
				Dir = vec2(0.f, -1.f);

			const int DAMAGE = 20; // should kill
			if (pTarget->IsZombie()) {
				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Dir * -1, DAMAGE,
					ClientID, pChr->GetActiveWeapon());
			}
			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if (Hits)
			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 3);
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

	case WEAPON_SHOTGUN: {
		int ShotSpread = 3;
		float Force = 10.0f;

		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = { -0.185f, -0.070f, 0, 0.070f, 0.185f };
			float a = angle(Direction);
			a += Spreading[i + 3] * 2.0f * (0.25f + 0.75f * static_cast<float>(10 - pChr->m_aWeapons[WEAPON_SHOTGUN].m_Ammo) / 10.0f);
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)pGameServer->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			float LifeTime = pGameServer->Tuning()->m_ShotgunLifetime + 0.1f * static_cast<float>(pChr->m_aWeapons[WEAPON_SHOTGUN].m_Ammo) / 10.0f;
			new CProjectile(pGameWorld, WEAPON_SHOTGUN,
				ClientID,
				ProjStartPos,
				vec2(cosf(a), sinf(a)) * Speed,
				(int)(pChr->Server()->TickSpeed() * LifeTime),
				g_pData->m_Weapons.m_Shotgun.m_pBase->m_Damage, false, Force, -1, WEAPON_SHOTGUN);
		}

		pGameServer->CreateSound(pChr->GetPos(), SOUND_SHOTGUN_FIRE);
	} break;

	case WEAPON_GRENADE: {
		//Find bomb
		bool BombFound = false;
		for (CMedicGrenade* pGrenade = (CMedicGrenade*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_MEDIC_GRENADE); pGrenade; pGrenade = (CMedicGrenade*)pGrenade->TypeNext())
		{
			if (pGrenade->m_Owner != ClientID) continue;
			pGrenade->Explode();
			BombFound = true;
		}

		if (!BombFound && pChr->m_aWeapons[WEAPON_GRENADE].m_Ammo)
		{
			int ShotSpread = 0;

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(ShotSpread * 2 + 1);

			for (int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float a = angle(Direction) + frandom() / 5.0f;

				CMedicGrenade* pProj = new CMedicGrenade(pGameWorld, ClientID, pChr->GetPos(), vec2(cosf(a), sinf(a)));

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
					Msg.AddInt(((int*)& p)[i]);
				pChr->Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
			}

			pGameServer->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);

			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 4);
			pChr->m_aWeapons[WEAPON_GRENADE].m_Ammo++;
		}
	} break;

	case WEAPON_LASER: {
		new CMedicLaser(pGameWorld, pChr->GetPos(), Direction, pGameServer->Tuning()->m_LaserReach, pChr->GetPlayer()->GetCID());
		pGameServer->CreateSound(pChr->GetPos(), SOUND_LASER_FIRE);
	}
	}
}
