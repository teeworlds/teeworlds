/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_GHOST_H
#define ENGINE_SHARED_GHOST_H

#include <engine/ghost.h>

enum
{
	MAX_ITEM_SIZE = 128,
	NUM_ITEMS_PER_CHUNK = 50,
};

struct CGhostItem
{
public:
	char m_aData[MAX_ITEM_SIZE];
	int m_Type;

	CGhostItem();
	CGhostItem(int Type);
	bool Compare(CGhostItem Other);
};

class CGhostRecorder : public IGhostRecorder
{
	IOHANDLE m_File;
	class IConsole *m_pConsole;

	CGhostItem m_LastItem;

	char m_aBuffer[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	char *m_pBufferPos;
	int m_BufferNumItems;

	void ResetBuffer();
	void FlushChunk(int Type);
	
public:	
	CGhostRecorder();
	
	int Start(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pMap, unsigned MapCrc, const char *pName);
	int Stop(int Ticks, int Time);

	void WriteData(int Type, const char *pData, int Size);
	bool IsRecording() const { return m_File != 0; }
};

class CGhostLoader : public IGhostLoader
{
	IOHANDLE m_File;
	class IConsole *m_pConsole;

	CGhostHeader m_Header;

	CGhostItem m_LastItem;

	char m_aBuffer[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	char *m_pBufferPos;
	int m_BufferNumItems;
	int m_BufferCurItem;

	void ResetBuffer();
	int ReadChunk(int *pType);

public:
	CGhostLoader();

	int Load(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pMap, unsigned Crc);
	void Close();
	CGhostHeader GetHeader() { return m_Header; }

	bool ReadNextType(int *pType);
	bool ReadData(int Type, char *pData, int Size);

	bool GetGhostInfo(class IStorage *pStorage, const char *pFilename, CGhostHeader *pGhostHeader, const char *pMap, unsigned Crc) const;
	int GetTime(CGhostHeader Header);
	int GetTicks(CGhostHeader Header);
};

#endif
