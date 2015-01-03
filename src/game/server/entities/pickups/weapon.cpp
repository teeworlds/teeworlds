/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "weapon.h"

CPickupWeapon::CPickupWeapon(CGameWorld *pGameWorld, int Type, int PickupSound, int Weapon, vec2 Pos)
: CPickup(pGameWorld, Type, PickupSound, SOUND_WEAPON_SPAWN, Pos)
{
	m_Weapon = Weapon;
}

bool CPickupWeapon::OnPickup(CCharacter *pChar)
{
	int Maxammo = g_pData->m_Weapons.m_aId[m_Weapon].m_Maxammo;

	bool Picked = pChar->GiveWeapon(m_Weapon, Maxammo);
	if(Picked)
		GameServer()->SendWeaponPickup(pChar->GetPlayer()->GetCID(), m_Weapon);

	return Picked;
}
