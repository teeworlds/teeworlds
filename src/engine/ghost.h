/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_GHOST_H
#define ENGINE_GHOST_H

#include <engine/shared/protocol.h>

#include "kernel.h"

struct CGhostHeader
{
	unsigned char m_aMarker[8];
	unsigned char m_Version;
	char m_aOwner[MAX_NAME_LENGTH];
	char m_aMap[64];
	unsigned char m_aCrc[4];
	unsigned char m_aNumTicks[4];
	unsigned char m_aTime[4];
};

class IGhostRecorder : public IInterface
{
	MACRO_INTERFACE("ghostrecorder", 0)
public:
	virtual ~IGhostRecorder() {}
	virtual int Stop(int Ticks, int Time) = 0;

	virtual void WriteData(int Type, const char *pData, int Size) = 0;
	virtual bool IsRecording() const = 0;
};

class IGhostLoader : public IInterface
{
	MACRO_INTERFACE("ghostloader", 0)
public:
	virtual ~IGhostLoader() {}
	virtual void Close() = 0;

	virtual const CGhostHeader *GetHeader() const = 0;

	virtual bool ReadNextType(int *pType) = 0;
	virtual bool ReadData(int Type, char *pData, int Size) = 0;

	virtual int GetTime(const CGhostHeader *pHeader) const = 0;
	virtual int GetTicks(const CGhostHeader *pHeader) const = 0;
};

#endif
