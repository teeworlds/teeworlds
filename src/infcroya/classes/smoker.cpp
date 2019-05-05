#include "smoker.h"
#include "base/system.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

CSmoker::CSmoker() : IClass()
{
	CSkin skin;
	skin.SetBodyColor(58, 200, 79);
	skin.SetMarkingName("cammostripes");
	skin.SetMarkingColor(0, 0, 0);
	skin.SetFeetColor(0, 79, 70);
	SetSkin(skin);
	SetInfectedClass(true);
	SetName("Smoker");
}

CSmoker::~CSmoker()
{
}

void CSmoker::InitialWeaponsHealth(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);
	pChr->SetNormalEmote(EMOTE_ANGRY);
}

void CSmoker::OnWeaponFire(vec2 Direction, vec2 ProjStartPos, int Weapon, CCharacter* pChr)
{
	int ClientID = pChr->GetPlayer()->GetCID();
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
			if (pTarget->IsZombie() && pTarget != pChr) {
				pTarget->IncreaseOverallHp(4);
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

			if (pTarget->IsZombie() && !pTarget->IsHookProtected()) {
				const int DAMAGE = 0; // 0 = no damage, TakeDamage() is called below just for hammerfly etc
				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Dir * -1, DAMAGE,
					pChr->GetPlayer()->GetCID(), pChr->GetActiveWeapon());
			}
			if (pTarget->IsHuman())
				pTarget->Infect(ClientID);
			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if (Hits)
			pChr->SetReloadTimer(pChr->Server()->TickSpeed() / 3);
	} break;
	}
}