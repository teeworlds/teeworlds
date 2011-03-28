/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"
#include "character.h"
#include "laser.h"
#include "weapon.h"

CWeapon::CWeapon(CGameWorld *pGameworld, CCharacter *pCarrier, int WeaponType)
{
	m_pGameWorld = pGameworld;
	m_pCarrier = pCarrier;
	m_Type = WeaponType;

	m_AmmoRegenStart = -1;
	m_AmmoRegenTime = g_pData->m_Weapons.m_aId[WeaponType].m_Ammoregentime;

//TODO put a field AmmoRegenValue in g_pdata
	m_AmmoRegenValue = 1;

	m_Maxammo = g_pData->m_Weapons.m_aId[WeaponType].m_Maxammo;
	m_Ammo = 0;
	m_AmmoCost = 1;
	m_Damage = g_pData->m_Weapons.m_aId[WeaponType].m_Damage;
	m_Explosive = false;
	m_Force = 0.f;
	m_SpeedFactor = 1.f;

	m_SoundImpact = -1;
	m_SoundFire = -1;

	m_LifeTime = 0;

	m_FireDelay = g_pData->m_Weapons.m_aId[WeaponType].m_Firedelay * Server()->TickSpeed() / 1000;

	m_Got = false;
	m_FullAuto = false;


}

CGun::CGun(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_GUN) 
{
	m_LifeTime = (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime);
	m_SoundFire = SOUND_GUN_FIRE;
}

CNinja::CNinja(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_NINJA)
{
	m_CurrentMoveTime = -1;
	m_NumObjectsHit = 0;
	m_SoundFire = SOUND_NINJA_FIRE;
	m_MoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
	m_Duration = g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
	m_Velocity = g_pData->m_Weapons.m_Ninja.m_Velocity;
}

CHammer::CHammer(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_HAMMER)
{
	m_Ammo = -1;
	m_Maxammo = -1;
}

CGrenadeLauncher::CGrenadeLauncher(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_GRENADE)
{
	m_FullAuto = true;
	m_LifeTime = (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime);
	m_Explosive = true;
	m_SoundImpact = SOUND_GRENADE_EXPLODE;
	m_SoundFire = SOUND_GRENADE_FIRE;
}

CShotgun::CShotgun(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_SHOTGUN)
{
	m_FullAuto = true;
	m_LifeTime = (int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime);
	m_SoundFire = SOUND_SHOTGUN_FIRE;
	m_ShotSpread = 2;
	m_ShotgunSpeeddiff = (float)GameServer()->Tuning()->m_ShotgunSpeeddiff;
}

CLaserRifle::CLaserRifle(CGameWorld *pGameworld, CCharacter *pCarrier) : CWeapon(pGameworld,pCarrier,WEAPON_RIFLE)
{
	m_FullAuto = true;
	m_Damage = GameServer()->Tuning()->m_LaserDamage;
	m_Reach = GameServer()->Tuning()->m_LaserReach;
	m_SoundFire = SOUND_RIFLE_FIRE;
}

int
CWeapon::FireWeapon(vec2 ProjStartPos, vec2 Direction)
{
	CProjectile *pProj = new CProjectile(GameWorld(), m_Type,
					     m_pCarrier->GetPlayer()->GetCID(), ProjStartPos,
					     Direction * m_SpeedFactor,
					     m_LifeTime,
					     m_Damage, m_Explosive, m_Force,
					     m_SoundImpact, m_Type);
	
	// pack the Projectile and send it to the client Directly
	CNetObj_Projectile p;
	pProj->FillInfo(&p);
	
	CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
	Msg.AddInt(1);
	for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
		Msg.AddInt(((int *)&p)[i]);

	Server()->SendMsg(&Msg, 0, m_pCarrier->GetPlayer()->GetCID());

	GameServer()->CreateSound(m_pCarrier->m_Pos, m_SoundFire);

	ConsumeAmmo();
	return m_FireDelay;
}

bool
CWeapon::CanFire()
{
	//-1 is infinite ammo
	return m_Ammo == -1 || (m_Ammo > 0 && m_Ammo >= m_AmmoCost);
}

void
CWeapon::RegenAmmo(int ReloadTimer)
{
	if (m_Ammo == -1)
		return;

	if(m_AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (ReloadTimer <= 0)
		{
			if (m_AmmoRegenStart < 0)
				m_AmmoRegenStart = Server()->Tick();
			
			if ((Server()->Tick() - m_AmmoRegenStart) >= m_AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_Ammo = min(m_Ammo + m_AmmoRegenValue, m_Maxammo);
				m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_AmmoRegenStart = -1;
		}
	}
}

void
CWeapon::ConsumeAmmo()
{
	if (m_Ammo != -1)
	{
		m_Ammo = max(m_Ammo - m_AmmoCost, 0);
	}
}

bool
CWeapon::GiveWeapon(int Ammo)
{
	if (m_Ammo < m_Maxammo || !m_Got)
	{
		m_Got = true;
		if (Ammo == -1)
			m_Ammo = Ammo;
		else
			m_Ammo = min(m_Maxammo, m_Ammo  + Ammo);
		return true;
	}

	return false;
}

int
CHammer::FireWeapon(vec2 ProjStartPos, vec2 Direction)
{
	GameServer()->CreateSound(m_pCarrier->m_Pos, SOUND_HAMMER_FIRE);
			
	CCharacter *apEnts[MAX_CLIENTS];
	int Hits = 0;
	int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_pCarrier->m_ProximityRadius*0.5f, (CEntity**)apEnts, 
						     MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; ++i)
	{
		CCharacter *pTarget = apEnts[i];
				
		if ((pTarget == m_pCarrier) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
			continue;

		// set his velocity to fast upward (for now)
		if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
			GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_pCarrier->m_ProximityRadius*0.5f);
		else
			GameServer()->CreateHammerHit(ProjStartPos);
				
		vec2 Dir;
		if (length(pTarget->m_Pos - m_pCarrier->m_Pos) > 0.0f)
			Dir = normalize(pTarget->m_Pos - m_pCarrier->m_Pos);
		else
			Dir = vec2(0.f, -1.f);
					
		pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, m_Damage,
				    m_pCarrier->GetPlayer()->GetCID(), m_Type);
		Hits++;
	}
			
	// if we Hit anything, we have to wait for the reload
	if(Hits)
		return Server()->TickSpeed()/3;
	return m_FireDelay;
}


int
CShotgun::FireWeapon(vec2 ProjStartPos, vec2 Direction) 
{
	CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
	Msg.AddInt(m_ShotSpread*2+1);
	
	for(int i = -m_ShotSpread; i <= m_ShotSpread; ++i)
	{
		float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
		float a = GetAngle(Direction);
		a += Spreading[i+2];
		float v = 1-(absolute(i)/(float)m_ShotSpread);
		float Speed = mix(m_ShotgunSpeeddiff, 1.0f, v);
		CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						     m_pCarrier->GetPlayer()->GetCID(),
						     ProjStartPos,
						     vec2(cosf(a), sinf(a)) * Speed * m_SpeedFactor,
						     m_LifeTime,
						     m_Damage, m_Explosive, m_Force,
						     m_SoundImpact, WEAPON_SHOTGUN);
		
		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);
				
		for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
			Msg.AddInt(((int *)&p)[i]);
	}
	
	Server()->SendMsg(&Msg, 0,m_pCarrier->GetPlayer()->GetCID());					
	
	GameServer()->CreateSound(m_pCarrier->m_Pos, m_SoundFire);
	ConsumeAmmo();
	return m_FireDelay;
}

int
CLaserRifle::FireWeapon(vec2 ProjStartPos, vec2 Direction) 
{
	new CLaser(GameWorld(), m_pCarrier->m_Pos, Direction, m_Reach, m_Damage, m_pCarrier->GetPlayer()->GetCID());
	GameServer()->CreateSound(m_pCarrier->m_Pos, m_SoundFire);
	ConsumeAmmo();
	return m_FireDelay;
}


bool
CNinja::GiveWeapon(int Ammo)
{
	m_ActivationTick = Server()->Tick();
	return CWeapon::GiveWeapon(Ammo);
}

int
CNinja::FireWeapon(vec2 ProjStartPos, vec2 Direction) 
{
	m_NumObjectsHit = 0;
	m_ActivationDir = Direction;
	m_CurrentMoveTime = m_MoveTime;
	
	GameServer()->CreateSound(m_pCarrier->m_Pos, m_SoundFire);
	return m_FireDelay;
}

void
CNinja::Update() {

	//we dont have the ninja
	if(!m_Got)
		return;

	if ((Server()->Tick() - m_ActivationTick) > m_Duration)
	{
		// time's up, return
		m_pCarrier->EndNinja();
		return;
	}

	// force ninja Weapon
	m_pCarrier->SetWeapon(WEAPON_NINJA);

	m_CurrentMoveTime--;

	CCharacterCore *Core = m_pCarrier->GetCore();

	if (m_CurrentMoveTime == 0)
	{
		// reset velocity
		Core->m_Vel *= 0.2f;
	}

	if (m_CurrentMoveTime > 0)
	{

		// Set velocity
		Core->m_Vel = m_ActivationDir * m_Velocity * m_SpeedFactor;
		vec2 OldPos = Core->m_Pos;
		GameServer()->Collision()->MoveBox(&Core->m_Pos, &Core->m_Vel, vec2(m_pCarrier->m_ProximityRadius, m_pCarrier->m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		Core->m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_pCarrier->m_Pos - OldPos;
			float Radius = m_pCarrier->m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == m_pCarrier)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_pCarrier->m_Pos) > (m_pCarrier->m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, 10.0f), m_Damage, m_pCarrier->GetPlayer()->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;

}
