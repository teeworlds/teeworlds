#include <engine/server.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <generated/server_data.h>

#include <modapi/server/event.h>

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

int CModAPI_Weapon::WorldID()
{
	return Player()->GetWorldID();
}

/* GENERIC GUN 07 *************************************************** */

CModAPI_Weapon_GenericGun07::CModAPI_Weapon_GenericGun07(int WID, int TW07ID, CCharacter* pCharacter, int Ammo) :
	CModAPI_Weapon(WID, pCharacter)
{
	m_Ammo = Ammo;
	m_LastNoAmmoSound = -1;
	m_ReloadTimer = 0;
	m_TW07ID = TW07ID;
}
	
int CModAPI_Weapon_GenericGun07::GetMaxAmmo() const
{
	return g_pData->m_Weapons.m_aId[m_TW07ID].m_Maxammo;
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
	
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_TW07ID].m_Ammoregentime;
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
			CModAPI_Event_Sound(GameServer()).World(WorldID())
				.Send(Character()->GetPos(), SOUND_WEAPON_NOAMMO);
		
			m_LastNoAmmoSound = Server()->Tick();
		}
		return false;
	}
	
	vec2 ProjStartPos = Character()->GetPos() + Direction * Character()->GetProximityRadius()*0.75f;
	
	CreateProjectile(ProjStartPos, Direction);
	
	m_Ammo--;
	
	m_ReloadTimer = g_pData->m_Weapons.m_aId[m_TW07ID].m_Firedelay * Server()->TickSpeed() / 1000;
	
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
	
void CModAPI_Weapon_GenericGun07::Snap06(int Snapshot, int SnappingClient, class CTW06_NetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = m_TW07ID;
	pCharNetObj->m_AmmoCount = (m_Ammo > 0 ? m_Ammo : 0);
}
	
void CModAPI_Weapon_GenericGun07::Snap07(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = m_TW07ID;
	pCharNetObj->m_AmmoCount = (m_Ammo > 0 ? m_Ammo : 0);
}
	
void CModAPI_Weapon_GenericGun07::Snap07ModAPI(int Snapshot, int SnappingClient, class CNetObj_Character* pCharNetObj)
{
	pCharNetObj->m_Weapon = m_TW07ID;
	pCharNetObj->m_AmmoCount = (m_Ammo > 0 ? m_Ammo : 0);
}
