#ifndef GAME_GHOST_H
#define GAME_GHOST_H

#include <game/generated/protocol.h>

enum
{
	GHOSTDATA_TYPE_SKIN = 0,
	GHOSTDATA_TYPE_CHARACTER_NO_TICK,
	GHOSTDATA_TYPE_CHARACTER
};

struct CGhostSkin
{
	int m_Skin0;
	int m_Skin1;
	int m_Skin2;
	int m_Skin3;
	int m_Skin4;
	int m_Skin5;
	int m_UseCustomColor;
	int m_ColorBody;
	int m_ColorFeet;
};

struct CGhostCharacter_NoTick
{
	int m_X;
	int m_Y;
	int m_VelX;
	int m_VelY;
	int m_Angle;
	int m_Direction;
	int m_Weapon;
	int m_HookState;
	int m_HookX;
	int m_HookY;
	int m_AttackTick;
};

struct CGhostCharacter : public CGhostCharacter_NoTick
{
	int m_Tick;
};

class CGhostTools
{
public:
	static void GetGhostCharacter(CGhostCharacter_NoTick *pGhostChar, const CNetObj_Character *pChar)
	{
		pGhostChar->m_X = pChar->m_X;
		pGhostChar->m_Y = pChar->m_Y;
		pGhostChar->m_VelX = pChar->m_VelX;
		pGhostChar->m_VelY = 0;
		pGhostChar->m_Angle = pChar->m_Angle;
		pGhostChar->m_Direction = pChar->m_Direction;
		pGhostChar->m_Weapon = pChar->m_Weapon;
		pGhostChar->m_HookState = pChar->m_HookState;
		pGhostChar->m_HookX = pChar->m_HookX;
		pGhostChar->m_HookY = pChar->m_HookY;
		pGhostChar->m_AttackTick = pChar->m_AttackTick;
	}

	static void GetNetObjCharacter(CNetObj_Character *pChar, const CGhostCharacter_NoTick *pGhostChar)
	{
		mem_zero(pChar, sizeof(CNetObj_Character));
		pChar->m_X = pGhostChar->m_X;
		pChar->m_Y = pGhostChar->m_Y;
		pChar->m_VelX = pGhostChar->m_VelX;
		pChar->m_VelY = 0;
		pChar->m_Angle = pGhostChar->m_Angle;
		pChar->m_Direction = pGhostChar->m_Direction;
		pChar->m_Weapon = pGhostChar->m_Weapon == WEAPON_GRENADE ? WEAPON_GRENADE : WEAPON_GUN;
		pChar->m_HookState = pGhostChar->m_HookState;
		pChar->m_HookX = pGhostChar->m_HookX;
		pChar->m_HookY = pGhostChar->m_HookY;
		pChar->m_AttackTick = pGhostChar->m_AttackTick;

		pChar->m_HookedPlayer = -1;
	}
};

#endif

