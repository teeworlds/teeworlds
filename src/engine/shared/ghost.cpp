/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/console.h>
#include <engine/storage.h>
#include "ghost.h"
#include "memheap.h"
#include "compression.h"
#include "network.h"
#include "engine.h"

static const unsigned char gs_aHeaderMarker[8] = {'T', 'W', 'G', 'H', 'O', 'S', 'T', 0};
static const unsigned char gs_ActVersion = 3;

CGhostRecorder::CGhostRecorder()
{
	m_File = 0;
	m_AddedInfo = 0;
	m_NumShots = 0;
}

// Record
int CGhostRecorder::Start(IStorage *pStorage, IConsole *pConsole, const char *pFilename, const char *pMap, int Crc, const char* pName, const char *pSkinName, int UseCustomColor, int ColorBody, int ColorFeet)
{
	CGhostHeader Header;
	if(m_File)
		return -1;

	// reset values
	m_AddedInfo = 0;
	m_NumShots = 0;
	
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
	mem_zero(&Header, sizeof(Header));
	mem_copy(Header.m_aMarker, gs_aHeaderMarker, sizeof(Header.m_aMarker));
	Header.m_Version = gs_ActVersion;
	str_copy(Header.m_aOwner, pName, sizeof(Header.m_aOwner));
	str_copy(Header.m_aSkinName, pSkinName, sizeof(Header.m_aSkinName));
	Header.m_UseCustomColor = UseCustomColor;
	Header.m_ColorBody = ColorBody;
	Header.m_ColorFeet = ColorFeet;
	str_copy(Header.m_aMap, pMap, sizeof(Header.m_aMap));
	Header.m_aCrc[0] = (Crc>>24)&0xff;
	Header.m_aCrc[1] = (Crc>>16)&0xff;
	Header.m_aCrc[2] = (Crc>>8)&0xff;
	Header.m_aCrc[3] = (Crc)&0xff;
	Header.m_NumShots = 0;
	Header.m_Time = 0.0f;
	io_write(m_File, &Header, sizeof(Header));
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Ghost recording to '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_recorder", aBuf);
	return 0;
}

void CGhostRecorder::WriteData()
{
	if(!m_File)
		return;
	
	// write data
	CGhostCharacter *Data = &m_aInfoBuf[0];
	
	char aBuffer[100*500];
	char aBuffer2[100*500];
	unsigned char aSize[4];
	
	int Size = sizeof(CGhostCharacter)*m_AddedInfo;
	mem_copy(aBuffer2, Data, Size);
	
	Size = CVariableInt::Compress(aBuffer2, Size, aBuffer);
	Size = CNetBase::Compress(aBuffer, Size, aBuffer2, sizeof(aBuffer2));
	
	aSize[0] = (Size>>24)&0xff;
	aSize[1] = (Size>>16)&0xff;
	aSize[2] = (Size>>8)&0xff;
	aSize[3] = (Size)&0xff;
	
	io_write(m_File, aSize, sizeof(aSize));
	io_write(m_File, aBuffer2, Size);
	
	m_NumShots += m_AddedInfo;
}

int CGhostRecorder::Stop(float Time)
{
	if(!m_File)
		return -1;
		
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ghost_recorder", "Stopped ghost recording");
	
	// save the rest of the ghost
	WriteData();
	
	// write down num shots and time
	io_seek(m_File, sizeof(CGhostHeader)-8, IOSEEK_START);
	io_write(m_File, &m_NumShots, sizeof(m_NumShots));
	io_write(m_File, &Time, sizeof(Time));
	
	io_close(m_File);
	m_File = 0;
	return 0;
}

void CGhostRecorder::AddInfos(CGhostCharacter *pPlayer)
{
	if(m_AddedInfo == 500)
	{
		WriteData();
		m_AddedInfo = 0;
	}
	
	m_aInfoBuf[m_AddedInfo++] = *pPlayer;
}

