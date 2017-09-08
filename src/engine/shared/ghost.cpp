/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/console.h>
#include <engine/storage.h>

#include "ghost.h"
#include "compression.h"
#include "network.h"

static const unsigned char gs_aHeaderMarker[8] = {'T', 'W', 'G', 'H', 'O', 'S', 'T', 0};
static const unsigned char gs_ActVersion = 4;
static const int gs_NumTicksOffset = 93;

CGhostRecorder::CGhostRecorder()
{
	m_File = 0;
	ResetBuffer();
}

// Record
int CGhostRecorder::Start(IStorage *pStorage, IConsole *pConsole, const char *pFilename, const char *pMap, unsigned Crc, const char* pName)
{
	m_pConsole = pConsole;

	m_File = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!m_File)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Unable to open '%s' for ghost recording", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_recorder", aBuf);
		return -1;
	}
	
	// write header
	CGhostHeader Header;
	mem_zero(&Header, sizeof(Header));
	mem_copy(Header.m_aMarker, gs_aHeaderMarker, sizeof(Header.m_aMarker));
	Header.m_Version = gs_ActVersion;
	str_copy(Header.m_aOwner, pName, sizeof(Header.m_aOwner));
	str_copy(Header.m_aMap, pMap, sizeof(Header.m_aMap));
	Header.m_aCrc[0] = (Crc>>24)&0xff;
	Header.m_aCrc[1] = (Crc>>16)&0xff;
	Header.m_aCrc[2] = (Crc>>8)&0xff;
	Header.m_aCrc[3] = (Crc)&0xff;
	io_write(m_File, &Header, sizeof(Header));

	m_LastItem.Reset();
	ResetBuffer();
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Ghost recording to '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_recorder", aBuf);
	return 0;
}

void CGhostRecorder::ResetBuffer()
{
	m_pBufferPos = m_aBuffer;
	m_BufferNumItems = 0;
}

static void DiffItem(int *pPast, int *pCurrent, int *pOut, int Size)
{
	while(Size)
	{
		*pOut = *pCurrent - *pPast;
		pOut++;
		pPast++;
		pCurrent++;
		Size--;
	}
}

void CGhostRecorder::WriteData(int Type, const char *pData, int Size)
{
	if(!m_File || (unsigned)Size > MAX_ITEM_SIZE || Size <= 0 || Type == -1)
		return;

	CGhostItem Data(Type);
	mem_copy(Data.m_aData, pData, Size);

	if(m_LastItem.m_Type == Data.m_Type)
		DiffItem((int*)m_LastItem.m_aData, (int*)Data.m_aData, (int*)m_pBufferPos, Size/4);
	else
	{
		FlushChunk();
		mem_copy(m_pBufferPos, Data.m_aData, Size);
	}

	m_LastItem = Data;
	m_pBufferPos += Size;
	m_BufferNumItems++;
	if(m_BufferNumItems >= NUM_ITEMS_PER_CHUNK)
		FlushChunk();
}

void CGhostRecorder::FlushChunk()
{
	static char s_aBuffer[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	static char s_aBuffer2[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	unsigned char aChunk[4];

	int Size = m_pBufferPos - m_aBuffer;
	int Type = m_LastItem.m_Type;

	if(!m_File || Size == 0)
		return;

	while(Size&3)
		m_aBuffer[Size++] = 0;
	Size = CVariableInt::Compress(m_aBuffer, Size, s_aBuffer, sizeof(s_aBuffer));
	Size = CNetBase::Compress(s_aBuffer, Size, s_aBuffer2, sizeof(s_aBuffer2));

	aChunk[0] = Type&0xff;
	aChunk[1] = m_BufferNumItems&0xff;
	aChunk[2] = (Size>>8)&0xff;
	aChunk[3] = (Size)&0xff;

	io_write(m_File, aChunk, sizeof(aChunk));
	io_write(m_File, s_aBuffer2, Size);

	ResetBuffer();
}

int CGhostRecorder::Stop(int Ticks, int Time)
{
	if(!m_File)
		return -1;
		
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_recorder", "Stopped ghost recording");

	FlushChunk();

	unsigned char aNumTicks[4];
	unsigned char aTime[4];

	aNumTicks[0] = (Ticks>>24)&0xff;
	aNumTicks[1] = (Ticks>>16)&0xff;
	aNumTicks[2] = (Ticks>>8)&0xff;
	aNumTicks[3] = (Ticks)&0xff;

	aTime[0] = (Time>>24)&0xff;
	aTime[1] = (Time>>16)&0xff;
	aTime[2] = (Time>>8)&0xff;
	aTime[3] = (Time)&0xff;
	
	// write down num shots and time
	io_seek(m_File, gs_NumTicksOffset, IOSEEK_START);
	io_write(m_File, &aNumTicks, sizeof(aNumTicks));
	io_write(m_File, &aTime, sizeof(aTime));
	
	io_close(m_File);
	m_File = 0;
	return 0;
}

CGhostLoader::CGhostLoader()
{
	m_File = 0;
	ResetBuffer();
}

void CGhostLoader::ResetBuffer()
{
	m_pBufferPos = m_aBuffer;
	m_BufferNumItems = 0;
	m_BufferCurItem = 0;
	m_BufferPrevItem = -1;
}

int CGhostLoader::Load(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pMap, unsigned Crc)
{
	m_pConsole = pConsole;
	m_File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!m_File)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "could not open '%s'", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_loader", aBuf);
		return -1;
	}

	// read the header
	mem_zero(&m_Header, sizeof(m_Header));
	io_read(m_File, &m_Header, sizeof(CGhostHeader));
	if(mem_comp(m_Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "'%s' is not a ghost file", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_loader", aBuf);
		io_close(m_File);
		m_File = 0;
		return -1;
	}

	if(m_Header.m_Version != gs_ActVersion)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "ghost version %d is not supported", m_Header.m_Version);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_loader", aBuf);
		io_close(m_File);
		m_File = 0;
		return -1;
	}

	unsigned GhostMapCrc = (m_Header.m_aCrc[0] << 24) | (m_Header.m_aCrc[1] << 16) | (m_Header.m_aCrc[2] << 8) | (m_Header.m_aCrc[3]);
	if(str_comp(m_Header.m_aMap, pMap) != 0 || GhostMapCrc != Crc)
	{
		io_close(m_File);
		m_File = 0;
		return -1;
	}

	m_LastItem.Reset();
	ResetBuffer();

	return 0;
}

int CGhostLoader::ReadChunk(int *pType)
{
	static char s_aCompresseddata[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	static char s_aDecompressed[MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK];
	unsigned char aChunk[4];

	ResetBuffer();

	if(io_read(m_File, aChunk, sizeof(aChunk)) != sizeof(aChunk))
		return -1;

	*pType = aChunk[0];
	int Size = (aChunk[2] << 8) | aChunk[3];
	m_BufferNumItems = aChunk[1];

	if(Size > MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK || Size <= 0)
		return -1;

	if(io_read(m_File, s_aCompresseddata, Size) != (unsigned)Size)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error reading chunk");
		return -1;
	}

	Size = CNetBase::Decompress(s_aCompresseddata, Size, s_aDecompressed, sizeof(s_aDecompressed));
	if(Size < 0)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error during network decompression");
		return -1;
	}

	Size = CVariableInt::Decompress(s_aDecompressed, Size, m_aBuffer, sizeof(m_aBuffer));
	if(Size < 0)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost", "error during intpack decompression");
		return -1;
	}

	return 0;
}

bool CGhostLoader::ReadNextType(int *pType)
{
	if(!m_File)
		return false;

	if(m_BufferCurItem != m_BufferPrevItem && m_BufferCurItem < m_BufferNumItems)
	{
		*pType = m_LastItem.m_Type;
	}
	else
	{
		if(ReadChunk(pType))
			return false; // error or eof
	}

	m_BufferPrevItem = m_BufferCurItem;

	return true;
}

static void UndiffItem(int *pPast, int *pDiff, int *pOut, int Size)
{
	while(Size)
	{
		*pOut = *pPast + *pDiff;
		pOut++;
		pPast++;
		pDiff++;
		Size--;
	}
}

bool CGhostLoader::ReadData(int Type, char *pData, int Size)
{
	if(!m_File || Size > MAX_ITEM_SIZE || Size <= 0 || Type == -1)
		return false;

	CGhostItem Data(Type);

	if(m_LastItem.m_Type == Data.m_Type)
		UndiffItem((int*)m_LastItem.m_aData, (int*)m_pBufferPos, (int*)Data.m_aData, Size/4);
	else
		mem_copy(Data.m_aData, m_pBufferPos, Size);

	mem_copy(pData, Data.m_aData, Size);

	m_LastItem = Data;
	m_pBufferPos += Size;
	m_BufferCurItem++;
	return true;
}

void CGhostLoader::Close()
{
	if(!m_File)
		return;
	io_close(m_File);
	m_File = 0;
}

bool CGhostLoader::GetGhostInfo(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, CGhostHeader *pGhostHeader, const char *pMap, unsigned Crc) const
{
	if(!pGhostHeader)
		return false;

	mem_zero(pGhostHeader, sizeof(CGhostHeader));

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return false;

	io_read(File, pGhostHeader, sizeof(CGhostHeader));

	if(mem_comp(pGhostHeader->m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) == 0 && (pGhostHeader->m_Version == 2 || pGhostHeader->m_Version == 3))
	{
		io_close(File);
		// old version... try to update
		if(CGhostUpdater::Update(pStorage, pConsole, pFilename))
		{
			// try again
			File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
			io_read(File, pGhostHeader, sizeof(CGhostHeader));
		}
		else
			return false;
	}

	if(mem_comp(pGhostHeader->m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) || pGhostHeader->m_Version != gs_ActVersion)
	{
		io_close(File);
		return false;
	}

	unsigned GhostMapCrc = (pGhostHeader->m_aCrc[0] << 24) | (pGhostHeader->m_aCrc[1] << 16) | (pGhostHeader->m_aCrc[2] << 8) | (pGhostHeader->m_aCrc[3]);
	if(str_comp(pGhostHeader->m_aMap, pMap) != 0 || GhostMapCrc != Crc)
	{
		io_close(File);
		return false;
	}

	io_close(File);
	return true;
}

int CGhostLoader::GetTime(const CGhostHeader *pHeader) const
{
	return (pHeader->m_aTime[0] << 24) | (pHeader->m_aTime[1] << 16) | (pHeader->m_aTime[2] << 8) | (pHeader->m_aTime[3]);
}

int CGhostLoader::GetTicks(const CGhostHeader *pHeader) const
{
	return (pHeader->m_aNumTicks[0] << 24) | (pHeader->m_aNumTicks[1] << 16) | (pHeader->m_aNumTicks[2] << 8) | (pHeader->m_aNumTicks[3]);
}

inline void StrToInts(int *pInts, int Num, const char *pStr)
{
	int Index = 0;
	while (Num)
	{
		char aBuf[4] = { 0,0,0,0 };
		for (int c = 0; c < 4 && pStr[Index]; c++, Index++)
			aBuf[c] = pStr[Index];
		*pInts = ((aBuf[0] + 128) << 24) | ((aBuf[1] + 128) << 16) | ((aBuf[2] + 128) << 8) | (aBuf[3] + 128);
		pInts++;
		Num--;
	}

	// null terminate
	pInts[-1] &= 0xffffff00;
}

CGhostRecorder CGhostUpdater::ms_Recorder;

bool CGhostUpdater::Update(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename)
{
	pStorage->CreateFolder("ghosts/backup", IStorage::TYPE_SAVE);

	const char *pExtractedName = pFilename;
	for(const char *pSrc = pFilename; *pSrc; pSrc++)
		if(*pSrc == '/' || *pSrc == '\\')
			pExtractedName = pSrc + 1;

	char aBackupFilename[512];
	str_format(aBackupFilename, sizeof(aBackupFilename), "ghosts/backup/%s", pExtractedName);
	if(!pStorage->RenameFile(pFilename, aBackupFilename, IStorage::TYPE_SAVE))
		return false;

	IOHANDLE File = pStorage->OpenFile(aBackupFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return false;

	// read header
	CGhostHeaderMain Header;
	io_read(File, &Header, sizeof(Header));
	if(mem_comp(Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0 || (Header.m_Version != 2 && Header.m_Version != 3))
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "error: no valid ghost file");
		io_close(File);
		return false;
	}

	io_seek(File, 0, IOSEEK_START);

	int Ticks, Time;
	if(Header.m_Version == 2)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "updating v2 ghost file");
		CGhostHeaderV2 ExtHeader;
		char aSkinData[ms_SkinSizeV2];
		io_read(File, &ExtHeader, sizeof(ExtHeader));
		io_read(File, aSkinData, sizeof(aSkinData));

		Ticks = ExtHeader.m_NumShots;
		Time = ExtHeader.m_Time * 1000;

		unsigned Crc = (ExtHeader.m_aCrc[0] << 24) | (ExtHeader.m_aCrc[1] << 16) | (ExtHeader.m_aCrc[2] << 8) | (ExtHeader.m_aCrc[3]);
		ms_Recorder.Start(pStorage, pConsole, pFilename, ExtHeader.m_aMap, Crc, ExtHeader.m_aOwner);

		CGhostSkin Skin;
		mem_copy(&Skin, aSkinData + ms_SkinOffsetV2, sizeof(Skin));
		ms_Recorder.WriteData(0 /* GHOSTDATA_TYPE_SKIN */, (const char*)&Skin, sizeof(Skin));
	}
	else
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "updating v3 ghost file");
		CGhostHeaderV3 ExtHeader;
		io_read(File, &ExtHeader, sizeof(ExtHeader));

		Ticks = ExtHeader.m_NumShots;
		Time = ExtHeader.m_Time * 1000;

		unsigned Crc = (ExtHeader.m_aCrc[0] << 24) | (ExtHeader.m_aCrc[1] << 16) | (ExtHeader.m_aCrc[2] << 8) | (ExtHeader.m_aCrc[3]);
		ms_Recorder.Start(pStorage, pConsole, pFilename, ExtHeader.m_aMap, Crc, ExtHeader.m_aOwner);

		CGhostSkin Skin;
		StrToInts(&Skin.m_Skin0, 6, ExtHeader.m_aSkinName);
		Skin.m_UseCustomColor = ExtHeader.m_UseCustomColor;
		Skin.m_ColorBody = ExtHeader.m_ColorBody;
		Skin.m_ColorFeet = ExtHeader.m_ColorFeet;
		ms_Recorder.WriteData(0 /* GHOSTDATA_TYPE_SKIN */, (const char*)&Skin, sizeof(Skin));
	}

	// read data
	int Index = 0;
	while(Index < Ticks)
	{
		static char s_aCompresseddata[100 * 500];
		static char s_aDecompressed[100 * 500];
		static char s_aData[100 * 500];

		unsigned char aSize[4];
		if(io_read(File, aSize, sizeof(aSize)) != sizeof(aSize))
			break;
		unsigned Size = (aSize[0] << 24) | (aSize[1] << 16) | (aSize[2] << 8) | aSize[3];

		if(io_read(File, s_aCompresseddata, Size) != Size)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "error reading chunk");
			break;
		}

		int DataSize = CNetBase::Decompress(s_aCompresseddata, Size, s_aDecompressed, sizeof(s_aDecompressed));
		if(DataSize < 0)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "error during network decompression");
			break;
		}

		DataSize = CVariableInt::Decompress(s_aDecompressed, DataSize, s_aData, sizeof(s_aData));
		if(DataSize < 0)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost/updater", "error during intpack decompression");
			break;
		}

		char *pTmp = s_aData;
		for(int i = 0; i < DataSize / ms_GhostCharacterSize; i++)
		{
			ms_Recorder.WriteData(1 /* GHOSTDATA_TYPE_CHARACTER_NO_TICK */, pTmp, ms_GhostCharacterSize);
			pTmp += ms_GhostCharacterSize;
			Index++;
		}
	}

	io_close(File);

	bool Error = Ticks != Index;
	ms_Recorder.Stop(Index, Error ? 0 : Time);
	return !Error;
}
