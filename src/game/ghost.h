#ifndef GAME_GHOST_H
#define GAME_GHOST_H

enum
{
	GHOSTDATA_TYPE_SKIN = 0,
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

struct CGhostCharacter
{
	int m_X;
	int m_Y;
	int m_VelX;
	//int m_VelY;
	int m_Angle;
	int m_Direction;
	int m_Weapon;
	int m_HookState;
	int m_HookX;
	int m_HookY;
	int m_AttackTick;
};

class CGhostTools
{
public:
	static CGhostCharacter GetGhostCharacter(CNetObj_Character Char)
	{
		CGhostCharacter Player;
		Player.m_X = Char.m_X;
		Player.m_Y = Char.m_Y;
		Player.m_VelX = Char.m_VelX;
		//Player.m_VelY = Char.m_VelY;
		Player.m_Angle = Char.m_Angle;
		Player.m_Direction = Char.m_Direction;
		Player.m_Weapon = Char.m_Weapon;
		Player.m_HookState = Char.m_HookState;
		Player.m_HookX = Char.m_HookX;
		Player.m_HookY = Char.m_HookY;
		Player.m_AttackTick = Char.m_AttackTick;

		return Player;
	}
};

#endif

