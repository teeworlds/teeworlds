/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_GHOST_H
#define ENGINE_GHOST_H

#include "kernel.h"

class IGhostRecorder : public IInterface
{
	MACRO_INTERFACE("ghostrecorder", 0)
public:

	struct CGhostHeader
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
		char m_aOwner[MAX_NAME_LENGTH];
		char m_aSkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
		char m_aMap[64];
		unsigned char m_aCrc[4];
		int m_NumShots;
		float m_Time;
	};
	
	struct CGhostCharacter
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
	
	~IGhostRecorder() {}
	virtual bool IsRecording() const = 0;
	virtual void AddInfos(CGhostCharacter *pPlayer) = 0;
	virtual int Stop(float Time=0.0f) = 0;
};

#endif
