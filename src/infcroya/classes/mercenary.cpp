#include "mercenary.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <generated/server_data.h>
#include <game/server/entities/projectile.h>
#include <base/math.h>
#include <engine/message.h>
#include <infcroya/croyaplayer.h>
#include <infcroya/entities/merc-bomb.h>
#include <infcroya/entities/scatter-grenade.h>

CMercenary::CMercenary()
{
	CSkin skin;
	skin.SetBodyColor(155, 116, 122);
	skin.SetMarkingName("stripes");
	skin.SetMarkingColor(0, 0, 255);
	skin.SetFeetColor(29, 173, 87);
	SetSkin(skin);
	SetInfectedClass(false);
	SetName("Mercenary");
}

CMercenary::~CMercenary()
{
}

void CMercenary::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GiveWeapon(WEAPON_GUN, 10);
	pChr->GiveWeapon(WEAPON_GRENADE, 10);
	pChr->SetWeapon(WEAPON_GUN);
	pChr->SetNormalEmote(EMOTE_NORMAL);
}

void CMercenary::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
	CGameWorld* pGameWorld = pChr->GameWorld();
	CGameContext* pGameServer = pChr->GameServer();

	switch (Weapon) {
	case WEAPON_HAMMER: {
		CMercenaryBomb* pCurrentBomb = NULL;
		for (CMercenaryBomb* pBomb = (CMercenaryBomb*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_MERCENARY_BOMB); pBomb; pBomb = (CMercenaryBomb*)pBomb->TypeNext())
		{
			if (pBomb->m_Owner == ClientID)
			{
				pCurrentBomb = pBomb;
				break;
			}
		}

		if (pCurrentBomb)
		{
			if (pCurrentBomb->ReadyToExplode() || distance(pCurrentBomb->GetPos(), pChr->GetPos()) > 80.0f)
				pCurrentBomb->Explode();
			else
			{
				pCurrentBomb->IncreaseDamage();
				pGameServer->CreateSound(pChr->GetPos(), SOUND_PICKUP_ARMOR);
			}
		}
		else
		{
			new CMercenaryBomb(pGameWorld, pChr->GetPos(), ClientID);
			pGameServer->CreateSound(pChr->GetPos(), SOUND_PICKUP_ARMOR);
		}

		pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 4);
	} break;

	case WEAPON_GUN: {
		CProjectile* pProj = new CProjectile(pGameWorld, WEAPON_GUN,
			ClientID,
			ProjStartPos,
			Direction,
			(int)(pChr->Server()->TickSpeed() * pGameServer->Tuning()->m_GunLifetime),
			1, 0, 0, -1, WEAPON_GUN);

		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(1);
		for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
			Msg.AddInt(((int*)& p)[i]);

		pChr->Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);

		float MaxSpeed = pGameServer->Tuning()->m_GroundControlSpeed * 1.7f;
		vec2 Recoil = Direction * (-MaxSpeed / 5.0f);
		pChr->SaturateVelocity(Recoil, MaxSpeed);

		pGameServer->CreateSound(pChr->GetPos(), SOUND_HOOK_LOOP);
	} break;

	case WEAPON_GRENADE: {
		//Find bomb
		bool BombFound = false;
		for (CScatterGrenade* pGrenade = (CScatterGrenade*)pGameWorld->FindFirst(CGameWorld::ENTTYPE_SCATTER_GRENADE); pGrenade; pGrenade = (CScatterGrenade*)pGrenade->TypeNext())
		{
			if (pGrenade->m_Owner != ClientID) continue;
			pGrenade->Explode();
			BombFound = true;
		}

		if (!BombFound && pChr->m_aWeapons[WEAPON_GRENADE].m_Ammo)
		{
			int ShotSpread = 2;

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(ShotSpread * 2 + 1);

			for (int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float a = angle(Direction) + frandom() / 3.0f;

				CScatterGrenade* pProj = new CScatterGrenade(pGameWorld, ClientID, pChr->GetPos(), vec2(cosf(a), sinf(a)));

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
					Msg.AddInt(((int*)& p)[i]);
				pChr->Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
			}

			pGameServer->CreateSound(pChr->GetPos(), SOUND_GRENADE_FIRE);

			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 4);
		}
	} break;
	}
}
