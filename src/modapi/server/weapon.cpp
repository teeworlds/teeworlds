#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <mod/entities/character.h>
#include <mod/entities/projectile.h>
#include <mod/entities/laser.h>

#include "weapon.h"

CModAPI_Weapon::CModAPI_Weapon(int ID, CCharacter* pCharacter) :
	m_ID(ID),
	m_pCharacter(pCharacter)
{
	
}

CCharacter* CModAPI_Weapon::Character()
{
	return m_pCharacter;
}

CPlayer* CModAPI_Weapon::Player()
{
	return m_pCharacter->GetPlayer();
}

CGameWorld* CModAPI_Weapon::GameWorld()
{
	return m_pCharacter->GameWorld();
}

CGameContext* CModAPI_Weapon::GameServer()
{
	return m_pCharacter->GameServer();
}

IServer* CModAPI_Weapon::Server()
{
	return m_pCharacter->Server();
}

const IServer* CModAPI_Weapon::Server() const
{
	return m_pCharacter->Server();
}
	

/* HAMMER *********************************************************** */

CModAPI_Weapon_Hammer::CModAPI_Weapon_Hammer(CCharacter* pCharacter) :
	CModAPI_Weapon(MODAPI_WEAPON_HAMMER07, pCharacter)
{
	m_ReloadTimer = 0;
}

bool CModAPI_Weapon_Hammer::TickPreFire(bool IsActive)
{
	if(IsActive && m_ReloadTimer > 0)
		m_ReloadTimer--;

	return true;
}

bool CModAPI_Weapon_Hammer::IsWeaponSwitchLocked() const
{
	return m_ReloadTimer != 0;
}

bool CModAPI_Weapon_Hammer::OnFire(vec2 Direction)
{
	if(m_ReloadTimer > 0)
		return false;
	
	vec2 ProjStartPos = Character()->GetPos() + Direction * Character()->GetProximityRadius()*0.75f;
	
	GameServer()->CreateSound(Character()->GetPos(), SOUND_HAMMER_FIRE);

	CCharacter *apEnts[MAX_CLIENTS];
	int Hits = 0;
	int Num = GameServer()->m_World.FindEntities(ProjStartPos, Character()->GetProximityRadius()*0.5f, (CEntity**)apEnts,
												MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; ++i)
	{
		CCharacter *pTarget = apEnts[i];

		if ((pTarget == Character()) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->GetPos(), NULL, NULL))
			continue;

		// set his velocity to fast upward (for now)
		if(length(pTarget->GetPos()-ProjStartPos) > 0.0f)
			GameServer()->CreateHammerHit(pTarget->GetPos()-normalize(pTarget->GetPos()-ProjStartPos)*Character()->GetProximityRadius()*0.5f);
		else
			GameServer()->CreateHammerHit(ProjStartPos);

		vec2 Dir;
		if (length(pTarget->GetPos() - Character()->GetPos()) > 0.0f)
		{
			Dir = normalize(pTarget->GetPos() - Character()->GetPos());
		}
		else
		{
			Dir = vec2(0.f, -1.f);
		}

		pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
			Player()->GetCID(), MODAPI_WEAPON_HAMMER07);
			
		Hits++;
	}

	// if we Hit anything, we have to wait for the reload
	if(Hits)
		m_ReloadTimer = Server()->TickSpeed()/3;
	
	return true;
}

/* GENERIC GUN 07 *************************************************** */

CModAPI_Weapon_GenericGun07::CModAPI_Weapon_GenericGun07(int WID, CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon(WID, pCharacter)
{
	m_Ammo = Ammo;
	m_LastNoAmmoSound = -1;
	m_ReloadTimer = 0;
}
	
int CModAPI_Weapon_GenericGun07::GetMaxAmmo() const
{
	return g_pData->m_Weapons.m_aId[GetID()].m_Maxammo;
}

int CModAPI_Weapon_GenericGun07::GetAmmo() const
{
	return m_Ammo;
}

bool CModAPI_Weapon_GenericGun07::AddAmmo(int Ammo)
{
	if(m_Ammo < GetMaxAmmo())
	{
		m_Ammo = min(m_Ammo + Ammo, GetMaxAmmo());
		return true;
	}
	else return false;
}

bool CModAPI_Weapon_GenericGun07::IsWeaponSwitchLocked() const
{
	return m_ReloadTimer != 0;
}

bool CModAPI_Weapon_GenericGun07::TickPreFire(bool IsActive)
{
	if(!IsActive)
		return true;
	
	if(m_ReloadTimer > 0) m_ReloadTimer--;
	
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[GetID()].m_Ammoregentime;
	if(AmmoRegenTime && m_Ammo >= 0)
	{
		// If equipped and not active, regen ammo?
		if(m_ReloadTimer <= 0)
		{
			if(m_AmmoRegenStart < 0)
			{
				m_AmmoRegenStart = Server()->Tick();
			}
			
			if((Server()->Tick() - m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_Ammo = min(m_Ammo + 1, GetMaxAmmo());
				m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_AmmoRegenStart = -1;
		}
	}
	
	return true;
}

bool CModAPI_Weapon_GenericGun07::OnFire(vec2 Direction)
{
	if(m_ReloadTimer > 0)
		return false;
		
	// check for ammo
	if(!m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(Character()->GetPos(), SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return false;
	}
	
	vec2 ProjStartPos = Character()->GetPos() + Direction * Character()->GetProximityRadius()*0.75f;
	
	CreateProjectile(ProjStartPos, Direction);
	
	m_Ammo--;
	
	m_ReloadTimer = g_pData->m_Weapons.m_aId[GetID()].m_Firedelay * Server()->TickSpeed() / 1000;
	
	return true;
}

void CModAPI_Weapon_GenericGun07::OnActivation()
{
	m_AmmoRegenStart = -1;
}

bool CModAPI_Weapon_GenericGun07::TickPaused(bool IsActive)
{
	if(!IsActive)
		return true;
		
	if(m_AmmoRegenStart > -1)
		++m_AmmoRegenStart;

	return false;
}
	
void CModAPI_Weapon_GenericGun07::Snap(int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	if(m_Ammo > 0)
		pCharNetObj->m_AmmoCount = m_Ammo;
}


/* GUN 07 *********************************************************** */

CModAPI_Weapon_Gun::CModAPI_Weapon_Gun(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MODAPI_WEAPON_GUN07, pCharacter, Ammo)
{
	
}

void CModAPI_Weapon_Gun::CreateProjectile(vec2 Pos, vec2 Direction)
{
	new CProjectile(GameWorld(), MODAPI_WEAPON_GUN07,
		Player()->GetCID(),
		Pos,
		Direction,
		(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
		g_pData->m_Weapons.m_Gun.m_pBase->m_Damage, false, 0, -1, MODAPI_WEAPON_GUN07);

	GameServer()->CreateSound(Character()->GetPos(), SOUND_GUN_FIRE);
}


/* SHOTGUN 07 ******************************************************* */

CModAPI_Weapon_Shotgun::CModAPI_Weapon_Shotgun(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MODAPI_WEAPON_SHOTGUN07, pCharacter, Ammo)
{
	
}

void CModAPI_Weapon_Shotgun::CreateProjectile(vec2 Pos, vec2 Direction)
{
	int ShotSpread = 2;

	for(int i = -ShotSpread; i <= ShotSpread; ++i)
	{
		float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
		float a = angle(Direction);
		a += Spreading[i+2];
		float v = 1-(absolute(i)/(float)ShotSpread);
		float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
		new CProjectile(GameWorld(), MODAPI_WEAPON_SHOTGUN07,
			Player()->GetCID(),
			Pos,
			vec2(cosf(a), sinf(a))*Speed,
			(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
			g_pData->m_Weapons.m_Shotgun.m_pBase->m_Damage, false, 0, -1, MODAPI_WEAPON_SHOTGUN07);
	}

	GameServer()->CreateSound(Character()->GetPos(), SOUND_SHOTGUN_FIRE);
}


/* GRENADE 07 ******************************************************* */

CModAPI_Weapon_Grenade::CModAPI_Weapon_Grenade(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MODAPI_WEAPON_GRENADE07, pCharacter, Ammo)
{
	
}

void CModAPI_Weapon_Grenade::CreateProjectile(vec2 Pos, vec2 Direction)
{
	new CProjectile(GameWorld(), WEAPON_GRENADE,
		Player()->GetCID(),
		Pos,
		Direction,
		(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
		g_pData->m_Weapons.m_Grenade.m_pBase->m_Damage, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

	GameServer()->CreateSound(Character()->GetPos(), SOUND_GRENADE_FIRE);
}


/* LASER 07 ********************************************************* */

CModAPI_Weapon_Laser::CModAPI_Weapon_Laser(CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon_GenericGun07(MODAPI_WEAPON_LASER07, pCharacter, Ammo)
{
	
}

void CModAPI_Weapon_Laser::CreateProjectile(vec2 Pos, vec2 Direction)
{
	new CLaser(GameWorld(), Pos, Direction, GameServer()->Tuning()->m_LaserReach, Player()->GetCID());
	GameServer()->CreateSound(Character()->GetPos(), SOUND_LASER_FIRE);
}


/* NINJA 07 ********************************************************* */

CModAPI_Weapon_Ninja::CModAPI_Weapon_Ninja(CCharacter* pCharacter) :
	CModAPI_Weapon(MODAPI_WEAPON_NINJA07, pCharacter)
{
	m_ActivationTick = Server()->Tick();
	m_CurrentMoveTime = -1;
	m_ReloadTimer = 0;
}
	
int CModAPI_Weapon_Ninja::GetMaxAmmo() const
{
	return g_pData->m_Weapons.m_aId[GetID()].m_Maxammo;
}

int CModAPI_Weapon_Ninja::GetAmmo() const
{
	return m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}

bool CModAPI_Weapon_Ninja::OnFire(vec2 Direction)
{
	if(m_ReloadTimer > 0)
		return false;

	m_NumObjectsHit = 0;

	m_ActivationDir = Direction;
	m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
	m_ReloadTimer = g_pData->m_Weapons.m_aId[GetID()].m_Firedelay * Server()->TickSpeed() / 1000;
	m_OldVelAmount = length(Character()->GetVelocity());

	GameServer()->CreateSound(Character()->GetPos(), SOUND_NINJA_FIRE);
	
	return true;
}

bool CModAPI_Weapon_Ninja::TickPreFire(bool IsActiveWeapon)
{
	if(m_ReloadTimer > 0)
		m_ReloadTimer--;

	if ((Server()->Tick() - m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		if(m_CurrentMoveTime > 0)
			Character()->SetVelocity(m_ActivationDir*m_OldVelAmount);

		return false; // deletes the weapon from the character (remove ninja)
	}

	m_CurrentMoveTime--;

	if (m_CurrentMoveTime == 0)
	{
		// reset velocity
		Character()->SetVelocity(m_ActivationDir*m_OldVelAmount);
	}

	if (m_CurrentMoveTime > 0)
	{
		// set velocity
		Character()->SetVelocity(m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity);
		vec2 OldPos = Character()->GetPos();
		
		vec2 Pos = Character()->GetPos();
		vec2 Velocity = Character()->GetVelocity();
		float ProximityRadius = Character()->GetProximityRadius();
		
		GameServer()->Collision()->MoveBox(&Pos, &Velocity, vec2(ProximityRadius, ProximityRadius), 0.f);
		
		Character()->SetPos(Pos);
		Character()->SetVelocity(Velocity);

		// reset velocity so the client doesn't predict stuff
		Character()->SetVelocity(vec2(0.f, 0.f));

		// check if we hit anything along the way
		CCharacter *aEnts[MAX_CLIENTS];
		vec2 Dir = Character()->GetPos() - OldPos;
		float Radius = ProximityRadius * 2.0f;
		vec2 Center = OldPos + Dir * 0.5f;
		int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, MOD_ENTTYPE_CHARACTER);

		for (int i = 0; i < Num; ++i)
		{
			if (aEnts[i] == Character())
				continue;

			// make sure we haven't hit this object before
			bool bAlreadyHit = false;
			for (int j = 0; j < m_NumObjectsHit; j++)
			{
				if (m_apHitObjects[j] == aEnts[i])
					bAlreadyHit = true;
			}
			if (bAlreadyHit)
				continue;

			// check so we are sufficiently close
			if (distance(aEnts[i]->GetPos(), Character()->GetPos()) > (ProximityRadius * 2.0f))
				continue;

			// hit a player, give him damage and stuffs...
			GameServer()->CreateSound(aEnts[i]->GetPos(), SOUND_NINJA_HIT);
			// set his velocity to fast upward (for now)
			if(m_NumObjectsHit < 10)
				m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

			aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, Player()->GetCID(), WEAPON_NINJA);
		}
	}

	return true;
}

bool CModAPI_Weapon_Ninja::TickPaused(bool IsActive)
{
	m_ActivationTick++;
	return false;
}
	
void CModAPI_Weapon_Ninja::Snap(int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_AmmoCount = m_ActivationTick + g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000;
}


