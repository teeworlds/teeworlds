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

class CGhostItem
{
public:
	char m_aData[MAX_ITEM_SIZE];
	int m_Type;

	CGhostItem();
	CGhostItem(int Type);
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
	void FlushChunk();
	
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

	bool GetGhostInfo(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, CGhostHeader *pGhostHeader, const char *pMap, unsigned Crc) const;
	int GetTime(CGhostHeader Header);
	int GetTicks(CGhostHeader Header);
};

class CGhostUpdater
{
	// all
	struct CGhostHeaderMain
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
	};

	// version 2
	struct CGhostHeaderV2
	{
		unsigned char m_aMarker[8];
		unsigned char m_Version;
		char m_aOwner[MAX_NAME_LENGTH];
		char m_aMap[64];
		unsigned char m_aCrc[4];
		int m_NumShots;
		float m_Time;
	};

	static const int ms_SkinSizeV2 = 17 * sizeof(int);
	static const int ms_SkinOffsetV2 = 8 * sizeof(int);

	// version 3
	struct CGhostHeaderV3
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

	// actual version
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

	static const int ms_ChostCharacterSize = 11 * sizeof(int);

	static CGhostRecorder ms_Recorder;

public:
	static bool Update(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename);
};

#endif
