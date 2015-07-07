/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include "entities/character.h"
#include "entities/projectile.h"
#include "gamecontext.h"
#include "player.h"

#include "weapon.h"

CWeapon::CWeapon(CCharacter *pOwner, int WeaponID, bool AlwaysActive, bool Holdable, int Ammo)
{
	m_pGameServer = pOwner->GameServer();
	m_pOwner = pOwner;
	m_WeaponID = WeaponID;
	m_AlwaysActive = AlwaysActive;
	m_Holdable = Holdable;
	m_Maxammo = g_pData->m_Weapons.m_aId[WeaponID].m_Maxammo;
	m_Ammo = Ammo;
	m_AmmoRegenStart = -1;
}

CWeapon::~CWeapon()
{
}

IServer *CWeapon::Server() { return m_pGameServer->Server(); }

bool CWeapon::IncreaseAmmo(int Ammo)
{
	if(m_Ammo == -1)
		return false;

	if(Ammo == -1)
	{
		m_Ammo = -1;
		return true;
	}

	if(m_Ammo < m_Maxammo)
	{
		m_Ammo = min(m_Ammo+Ammo, m_Maxammo);
		return true;
	}
	else
		return false;
}

void CWeapon::SetActive()
{
	m_AmmoRegenStart = -1;
}

void CWeapon::Fire(vec2 Direction, vec2 ProjStartPos)
{
	// check ammo
	if(m_Ammo == 0)
		return;

	// fire!
	OnFire(Direction, ProjStartPos);

	// set reload timer
	if(!GetOwner()->IsReloading())
	{
		int ReloadTimer = g_pData->m_Weapons.m_aId[m_WeaponID].m_Firedelay * Server()->TickSpeed() / 1000;
		GetOwner()->SetReloadTimer(ReloadTimer);
	}

	// reset regen
	m_AmmoRegenStart = -1;

	// decrease ammo
	if(m_Ammo > 0)
		m_Ammo--;
}

void CWeapon::Tick()
{
	// ammo regen
	if(m_pOwner->IsReloading())
	{
		int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_WeaponID].m_Ammoregentime;
		if(AmmoRegenTime > 0 && m_Ammo != -1)
		{
			if(m_AmmoRegenStart < 0)
				m_AmmoRegenStart = Server()->Tick();

			if((Server()->Tick() - m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// add some ammo
				IncreaseAmmo(1);
				m_AmmoRegenStart = -1;
			}
		}
	}

	OnTick();
}

void CWeapon::TickPaused()
{
	if(m_AmmoRegenStart > -1)
		m_AmmoRegenStart++;
}

int CWeapon::SnapAmmo()
{
	if(m_Ammo == -1)
		return 0;
	else
		return m_Ammo;
}

void CWeapon::SendProjectiles(const CProjectile **ppProjectiles, int NumProjectiles)
{
	// pack the projectiles and send them to the client directly
	CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
	Msg.AddInt(NumProjectiles);

	for(int p = 0; p < NumProjectiles; p++)
	{
		CNetObj_Projectile Obj;
		ppProjectiles[p]->FillInfo(&Obj);

		for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
			Msg.AddInt(((int *) &Obj)[i]);
	}

	Server()->SendMsg(&Msg, 0, m_pOwner->GetPlayer()->GetCID());
}

void CWeapon::SendProjectile(const CProjectile *pProjectile)
{
	SendProjectiles(&pProjectile, 1);
}
