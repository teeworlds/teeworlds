#ifndef GAME_GHOST_H
#define GAME_GHOST_H

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

#endif

