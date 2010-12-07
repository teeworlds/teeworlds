/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/console.h>
#include <engine/shared/protocol.h>
#include <engine/storage.h>
#include "demo.h"
#include "memheap.h"
#include "snapshot.h"
#include "compression.h"
#include "network.h"
#include "engine.h"

static const unsigned char gs_aHeaderMarker[7] = {'T', 'W', 'D', 'E', 'M', 'O', 0};
static const unsigned char gs_ActVersion = 2;
static const unsigned char gs_VersionWithMap = 2;

//Versions :
//1 : 0.5.0
//2 : 0.5.3/0.6.0, includes the map


CDemoRecorder::CDemoRecorder(class CSnapshotDelta *pSnapshotDelta)
{
	m_File = 0;
	m_LastTickMarker = -1;
	m_pSnapshotDelta = pSnapshotDelta;
}

//static IOHANDLE m_File = 0;

// Record
int CDemoRecorder::Start(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pNetVersion, const char *pMap, int Crc, const char *pType)
{
	CDemoHeader Header;
	if(m_File)
		return -1;

	m_pConsole = pConsole;

	// open mapfile
	char aMapFilename[128];
	// try the normal maps folder
	str_format(aMapFilename, sizeof(aMapFilename), "maps/%s.map", pMap);
	IOHANDLE MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!MapFile)
	{
		// try the downloaded maps
		str_format(aMapFilename, sizeof(aMapFilename), "downloadedmaps/%s_%08x.map", pMap, Crc);
		MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	}
	if(!MapFile)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Unable to open mapfile '%s'", pMap);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", aBuf);
		return -1;
	}

	m_File = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!m_File)
	{
		io_close(MapFile);
		MapFile = 0;
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Unable to open '%s' for recording", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", aBuf);
		return -1;
	}
	
	// write header
	mem_zero(&Header, sizeof(Header));
	mem_copy(Header.m_aMarker, gs_aHeaderMarker, sizeof(Header.m_aMarker));
	Header.m_Version = gs_ActVersion;
	str_copy(Header.m_aNetversion, pNetVersion, sizeof(Header.m_aNetversion));
	str_copy(Header.m_aMap, pMap, sizeof(Header.m_aMap));
	str_copy(Header.m_aType, pType, sizeof(Header.m_aType));
	Header.m_aCrc[0] = (Crc>>24)&0xff;
	Header.m_aCrc[1] = (Crc>>16)&0xff;
	Header.m_aCrc[2] = (Crc>>8)&0xff;
	Header.m_aCrc[3] = (Crc)&0xff;
	io_write(m_File, &Header, sizeof(Header));
	
	
	// write map
	// write map size
	int MapSize = io_length(MapFile);
	unsigned char aBufMapSize[4];
	aBufMapSize[0] = (MapSize>>24)&0xff;
	aBufMapSize[1] = (MapSize>>16)&0xff;
	aBufMapSize[2] = (MapSize>>8)&0xff;
	aBufMapSize[3] = (MapSize)&0xff;
	io_write(m_File, &aBufMapSize, sizeof(aBufMapSize));
		
	// write map data
	while(1)
	{
		unsigned char aChunk[1024*64];
		int Bytes = io_read(MapFile, &aChunk, sizeof(aChunk));
		if(Bytes <= 0)
			break;
		io_write(m_File, &aChunk, Bytes);
	}
	io_close(MapFile);
	
	m_LastKeyFrame = -1;
	m_LastTickMarker = -1;
	m_FirstTick = -1;
	
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Recording to '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", aBuf);
	return 0;
}

/*
	Tickmarker
		7   = Always set
		6   = Keyframe flag
		0-5 = Delta tick
	
	Normal
		7   = Not set
		5-6 = Type
		0-4 = Size
*/

enum
{
	CHUNKTYPEFLAG_TICKMARKER = 0x80,
	CHUNKTICKFLAG_KEYFRAME = 0x40, // only when tickmarker is set
	
	CHUNKMASK_TICK = 0x3f,
	CHUNKMASK_TYPE = 0x60,
	CHUNKMASK_SIZE = 0x1f,
	
	CHUNKTYPE_SNAPSHOT = 1,
	CHUNKTYPE_MESSAGE = 2,
	CHUNKTYPE_DELTA = 3,

	CHUNKFLAG_BIGSIZE = 0x10
};

void CDemoRecorder::WriteTickMarker(int Tick, int Keyframe)
{
	if(m_LastTickMarker == -1 || Tick-m_LastTickMarker > 63 || Keyframe)
	{
		unsigned char aChunk[5];
		aChunk[0] = CHUNKTYPEFLAG_TICKMARKER;
		aChunk[1] = (Tick>>24)&0xff;
		aChunk[2] = (Tick>>16)&0xff;
		aChunk[3] = (Tick>>8)&0xff;
		aChunk[4] = (Tick)&0xff;

		if(Keyframe)
			aChunk[0] |= CHUNKTICKFLAG_KEYFRAME;
		
		io_write(m_File, aChunk, sizeof(aChunk));
	}
	else
	{
		unsigned char aChunk[1];
		aChunk[0] = CHUNKTYPEFLAG_TICKMARKER | (Tick-m_LastTickMarker);
		io_write(m_File, aChunk, sizeof(aChunk));
	}	

	m_LastTickMarker = Tick;
	if(m_FirstTick < 0)
		m_FirstTick = Tick;
}

void CDemoRecorder::Write(int Type, const void *pData, int Size)
{
	char aBuffer[64*1024];
	char aBuffer2[64*1024];
	unsigned char aChunk[3];
	
	if(!m_File)
		return;

	/* pad the data with 0 so we get an alignment of 4,
	else the compression won't work and miss some bytes */
	mem_copy(aBuffer2, pData, Size);
	while(Size&3)
		aBuffer2[Size++] = 0;
	Size = CVariableInt::Compress(aBuffer2, Size, aBuffer); // buffer2 -> buffer
	Size = CNetBase::Compress(aBuffer, Size, aBuffer2, sizeof(aBuffer2)); // buffer -> buffer2
	
	
	aChunk[0] = ((Type&0x3)<<5);
	if(Size < 30)
	{
		aChunk[0] |= Size;
		io_write(m_File, aChunk, 1);
	}
	else
	{
		if(Size < 256)
		{
			aChunk[0] |= 30;
			aChunk[1] = Size&0xff;
			io_write(m_File, aChunk, 2);
		}
		else
		{
			aChunk[0] |= 31;
			aChunk[1] = Size&0xff;
			aChunk[2] = Size>>8;
			io_write(m_File, aChunk, 3);
		}
	}
	
	io_write(m_File, aBuffer2, Size);
}

void CDemoRecorder::RecordSnapshot(int Tick, const void *pData, int Size)
{
	if(m_LastKeyFrame == -1 || (Tick-m_LastKeyFrame) > SERVER_TICK_SPEED*5)
	{
		// write full tickmarker
		WriteTickMarker(Tick, 1);
		
		// write snapshot
		Write(CHUNKTYPE_SNAPSHOT, pData, Size);
			
		m_LastKeyFrame = Tick;
		mem_copy(m_aLastSnapshotData, pData, Size);
	}
	else
	{
		// create delta, prepend tick
		char aDeltaData[CSnapshot::MAX_SIZE+sizeof(int)];
		int DeltaSize;

		// write tickmarker
		WriteTickMarker(Tick, 0);
		
		DeltaSize = m_pSnapshotDelta->CreateDelta((CSnapshot*)m_aLastSnapshotData, (CSnapshot*)pData, &aDeltaData);
		if(DeltaSize)
		{
			// record delta
			Write(CHUNKTYPE_DELTA, aDeltaData, DeltaSize);
			mem_copy(m_aLastSnapshotData, pData, Size);
		}
	}
}

void CDemoRecorder::RecordMessage(const void *pData, int Size)
{
	Write(CHUNKTYPE_MESSAGE, pData, Size);
}

int CDemoRecorder::Stop()
{
	if(!m_File)
		return -1;
		
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", "Stopped recording");
	io_close(m_File);
	m_File = 0;
	return 0;
}



CDemoPlayer::CDemoPlayer(class CSnapshotDelta *pSnapshotDelta)
{
	m_File = 0;
	m_pKeyFrames = 0;

	m_pSnapshotDelta = pSnapshotDelta;
	m_LastSnapshotDataSize = -1;
}

void CDemoPlayer::SetListner(IListner *pListner)
{
	m_pListner = pListner;
}


int CDemoPlayer::ReadChunkHeader(int *pType, int *pSize, int *pTick)
{
	unsigned char Chunk = 0;
	
	*pSize = 0;
	*pType = 0;
	
	if(io_read(m_File, &Chunk, sizeof(Chunk)) != sizeof(Chunk))
		return -1;
		
	if(Chunk&CHUNKTYPEFLAG_TICKMARKER)
	{
		// decode tick marker
		int Tickdelta = Chunk&(CHUNKMASK_TICK);
		*pType = Chunk&(CHUNKTYPEFLAG_TICKMARKER|CHUNKTICKFLAG_KEYFRAME);
		
		if(Tickdelta == 0)
		{
			unsigned char aTickdata[4];
			if(io_read(m_File, aTickdata, sizeof(aTickdata)) != sizeof(aTickdata))
				return -1;
			*pTick = (aTickdata[0]<<24) | (aTickdata[1]<<16) | (aTickdata[2]<<8) | aTickdata[3];
		}
		else
		{
			*pTick += Tickdelta;
		}
		
	}
	else
	{
		// decode normal chunk
		*pType = (Chunk&CHUNKMASK_TYPE)>>5;
		*pSize = Chunk&CHUNKMASK_SIZE;
		
		if(*pSize == 30)
		{
			unsigned char aSizedata[1];
			if(io_read(m_File, aSizedata, sizeof(aSizedata)) != sizeof(aSizedata))
				return -1;
			*pSize = aSizedata[0];
			
		}
		else if(*pSize == 31)
		{
			unsigned char aSizedata[2];
			if(io_read(m_File, aSizedata, sizeof(aSizedata)) != sizeof(aSizedata))
				return -1;
			*pSize = (aSizedata[1]<<8) | aSizedata[0];
		}
	}
	
	return 0;
}

void CDemoPlayer::ScanFile()
{
	long StartPos;
	CHeap Heap;
	CKeyFrameSearch *pFirstKey = 0;
	CKeyFrameSearch *pCurrentKey = 0;
	//DEMOREC_CHUNK chunk;
	int ChunkSize, ChunkType, ChunkTick = 0;
	int i;

	StartPos = io_tell(m_File);
	m_Info.m_SeekablePoints = 0;

	while(1)
	{
		long CurrentPos = io_tell(m_File);
		
		if(ReadChunkHeader(&ChunkType, &ChunkSize, &ChunkTick))
			break;
			
		// read the chunk
		if(ChunkType&CHUNKTYPEFLAG_TICKMARKER)
		{
			if(ChunkType&CHUNKTICKFLAG_KEYFRAME)
			{
				CKeyFrameSearch *pKey;
				
				// save the position
				pKey = (CKeyFrameSearch *)Heap.Allocate(sizeof(CKeyFrameSearch));
				pKey->m_Frame.m_Filepos = CurrentPos;
				pKey->m_Frame.m_Tick = ChunkTick;
				pKey->m_pNext = 0;
				if(pCurrentKey)
					pCurrentKey->m_pNext = pKey;
				if(!pFirstKey)
					pFirstKey = pKey;
				pCurrentKey = pKey;
				m_Info.m_SeekablePoints++;
			}
			
			if(m_Info.m_Info.m_FirstTick == -1)
				m_Info.m_Info.m_FirstTick = ChunkTick;
			m_Info.m_Info.m_LastTick = ChunkTick;
		}
		else if(ChunkSize)
			io_skip(m_File, ChunkSize);
			
	}

	// copy all the frames to an array instead for fast access
	m_pKeyFrames = (CKeyFrame*)mem_alloc(m_Info.m_SeekablePoints*sizeof(CKeyFrame), 1);
	for(pCurrentKey = pFirstKey, i = 0; pCurrentKey; pCurrentKey = pCurrentKey->m_pNext, i++)
		m_pKeyFrames[i] = pCurrentKey->m_Frame;
		
	// destroy the temporary heap and seek back to the start
	io_seek(m_File, StartPos, IOSEEK_START);
}

void CDemoPlayer::DoTick()
{
	static char aCompresseddata[CSnapshot::MAX_SIZE];
	static char aDecompressed[CSnapshot::MAX_SIZE];
	static char aData[CSnapshot::MAX_SIZE];
	int ChunkType, ChunkTick, ChunkSize;
	int DataSize = 0;
	int GotSnapshot = 0;

	// update ticks
	m_Info.m_PreviousTick = m_Info.m_Info.m_CurrentTick;
	m_Info.m_Info.m_CurrentTick = m_Info.m_NextTick;
	ChunkTick = m_Info.m_Info.m_CurrentTick;

	while(1)
	{
		if(ReadChunkHeader(&ChunkType, &ChunkSize, &ChunkTick))
		{
			// stop on error or eof
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "end of file");
			if(m_Info.m_PreviousTick == -1)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", "empty demo");
				Stop();
			}
			else
				Pause();
			break;
		}
		
		// read the chunk
		if(ChunkSize)
		{
			if(io_read(m_File, aCompresseddata, ChunkSize) != (unsigned)ChunkSize)
			{
				// stop on error or eof
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "error reading chunk");
				Stop();
				break;
			}
			
			DataSize = CNetBase::Decompress(aCompresseddata, ChunkSize, aDecompressed, sizeof(aDecompressed));
			if(DataSize < 0)
			{
				// stop on error or eof
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "error during network decompression");
				Stop();
				break;
			}
			
			DataSize = CVariableInt::Decompress(aDecompressed, DataSize, aData);

			if(DataSize < 0)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "error during intpack decompression");
				Stop();
				break;
			}
		}
			
		if(ChunkType == CHUNKTYPE_DELTA)
		{
			// process delta snapshot
			static char aNewsnap[CSnapshot::MAX_SIZE];
			
			GotSnapshot = 1;
			
			DataSize = m_pSnapshotDelta->UnpackDelta((CSnapshot*)m_aLastSnapshotData, (CSnapshot*)aNewsnap, aData, DataSize);
			
			if(DataSize >= 0)
			{
				if(m_pListner)
					m_pListner->OnDemoPlayerSnapshot(aNewsnap, DataSize);

				m_LastSnapshotDataSize = DataSize;
				mem_copy(m_aLastSnapshotData, aNewsnap, DataSize);
			}
			else
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "error during unpacking of delta, err=%d", DataSize);
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", aBuf);
			}
		}
		else if(ChunkType == CHUNKTYPE_SNAPSHOT)
		{
			// process full snapshot
			GotSnapshot = 1;
			
			m_LastSnapshotDataSize = DataSize;
			mem_copy(m_aLastSnapshotData, aData, DataSize);
			if(m_pListner)
				m_pListner->OnDemoPlayerSnapshot(aData, DataSize);
		}
		else
		{
			// if there were no snapshots in this tick, replay the last one
			if(!GotSnapshot && m_pListner && m_LastSnapshotDataSize != -1)
			{
				GotSnapshot = 1;
				m_pListner->OnDemoPlayerSnapshot(m_aLastSnapshotData, m_LastSnapshotDataSize);
			}
			
			// check the remaining types
			if(ChunkType&CHUNKTYPEFLAG_TICKMARKER)
			{
				m_Info.m_NextTick = ChunkTick;
				break;
			}
			else if(ChunkType == CHUNKTYPE_MESSAGE)
			{
				if(m_pListner)
					m_pListner->OnDemoPlayerMessage(aData, DataSize);
			}
		}
	}
}

void CDemoPlayer::Pause()
{
	m_Info.m_Info.m_Paused = 1;
}

void CDemoPlayer::Unpause()
{
	if(m_Info.m_Info.m_Paused)
	{
		/*m_Info.start_tick = m_Info.current_tick;
		m_Info.start_time = time_get();*/
		m_Info.m_Info.m_Paused = 0;
	}
}

int CDemoPlayer::Load(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, int StorageType)
{
	m_pConsole = pConsole;
	m_File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!m_File)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "could not open '%s'", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", aBuf);
		return -1;
	}
	
	// store the filename
	str_copy(m_aFilename, pFilename, sizeof(m_aFilename));

	// clear the playback info
	mem_zero(&m_Info, sizeof(m_Info));
	m_Info.m_Info.m_FirstTick = -1;
	m_Info.m_Info.m_LastTick = -1;
	//m_Info.start_tick = -1;
	m_Info.m_NextTick = -1;
	m_Info.m_Info.m_CurrentTick = -1;
	m_Info.m_PreviousTick = -1;
	m_Info.m_Info.m_Speed = 1;
	
	m_LastSnapshotDataSize = -1;

	// read the header
	io_read(m_File, &m_Info.m_Header, sizeof(m_Info.m_Header));
	if(mem_comp(m_Info.m_Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "'%s' is not a demo file", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", aBuf);
		io_close(m_File);
		m_File = 0;
		return -1;
	}
	
	
	// get map
	if(m_Info.m_Header.m_Version >= gs_VersionWithMap)
	{
		// get map size
		unsigned char aBufMapSize[4];
		io_read(m_File, &aBufMapSize, sizeof(aBufMapSize));
		int MapSize = (aBufMapSize[0]<<24) | (aBufMapSize[1]<<16) | (aBufMapSize[2]<<8) | (aBufMapSize[3]);
		
		// check if we already have the map
		// TODO: improve map checking (maps folder, check crc)
		int Crc = (m_Info.m_Header.m_aCrc[0]<<24) | (m_Info.m_Header.m_aCrc[1]<<16) | (m_Info.m_Header.m_aCrc[2]<<8) | (m_Info.m_Header.m_aCrc[3]);
		char aMapFilename[128];
		str_format(aMapFilename, sizeof(aMapFilename), "downloadedmaps/%s_%08x.map", m_Info.m_Header.m_aMap, Crc);
		IOHANDLE MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL);
		
		if(MapFile)
		{
			io_skip(m_File, MapSize);
			io_close(MapFile);
		}
		else if(MapSize > 0)
		{
			// get map data
			unsigned char *pMapData = (unsigned char *)mem_alloc(MapSize, 1);
			io_read(m_File, pMapData, MapSize);
			
			// save map
			MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
			io_write(MapFile, pMapData, MapSize);
			io_close(MapFile);
			
			// free data
			mem_free(pMapData);
		}
	}
	
	
	// scan the file for interessting points
	ScanFile();
	
	// ready for playback
	return 0;
}

int CDemoPlayer::NextFrame()
{
	DoTick();
	return IsPlaying();
}

int CDemoPlayer::Play()
{
	// fill in previous and next tick
	while(m_Info.m_PreviousTick == -1 && IsPlaying())
		DoTick();
		
	// set start info
	/*m_Info.start_tick = m_Info.previous_tick;
	m_Info.start_time = time_get();*/
	m_Info.m_CurrentTime = m_Info.m_PreviousTick*time_freq()/SERVER_TICK_SPEED;
	m_Info.m_LastUpdate = time_get();
	return 0;
}

int CDemoPlayer::SetPos(float Percent)
{
	int Keyframe;
	int WantedTick;
	if(!m_File)
		return -1;
	
	// -5 because we have to have a current tick and previous tick when we do the playback
	WantedTick = m_Info.m_Info.m_FirstTick + (int)((m_Info.m_Info.m_LastTick-m_Info.m_Info.m_FirstTick)*Percent) - 5;
	
	Keyframe = (int)(m_Info.m_SeekablePoints*Percent);

	if(Keyframe < 0 || Keyframe >= m_Info.m_SeekablePoints)
		return -1;
	
	// get correct key frame
	if(m_pKeyFrames[Keyframe].m_Tick < WantedTick)
		while(Keyframe < m_Info.m_SeekablePoints-1 && m_pKeyFrames[Keyframe].m_Tick < WantedTick)
			Keyframe++;

	while(Keyframe && m_pKeyFrames[Keyframe].m_Tick > WantedTick)
		Keyframe--;
	
	// seek to the correct keyframe
	io_seek(m_File, m_pKeyFrames[Keyframe].m_Filepos, IOSEEK_START);

	//m_Info.start_tick = -1;
	m_Info.m_NextTick = -1;
	m_Info.m_Info.m_CurrentTick = -1;
	m_Info.m_PreviousTick = -1;

	// playback everything until we hit our tick
	while(m_Info.m_PreviousTick < WantedTick)
		DoTick();
	
	Play();
	
	return 0;
}

void CDemoPlayer::SetSpeed(float Speed)
{
	m_Info.m_Info.m_Speed = Speed;
}

int CDemoPlayer::Update()
{
	int64 Now = time_get();
	int64 Deltatime = Now-m_Info.m_LastUpdate;
	m_Info.m_LastUpdate = Now;
	
	if(!IsPlaying())
		return 0;
	
	if(m_Info.m_Info.m_Paused)
	{
		
	}
	else
	{
		int64 Freq = time_freq();
		m_Info.m_CurrentTime += (int64)(Deltatime*(double)m_Info.m_Info.m_Speed);
		
		while(1)
		{
			int64 CurtickStart = (m_Info.m_Info.m_CurrentTick)*Freq/SERVER_TICK_SPEED;

			// break if we are ready
			if(CurtickStart > m_Info.m_CurrentTime)
				break;
			
			// do one more tick
			DoTick();
			
			if(m_Info.m_Info.m_Paused)
				return 0;
		}

		// update intratick
		{	
			int64 CurtickStart = (m_Info.m_Info.m_CurrentTick)*Freq/SERVER_TICK_SPEED;
			int64 PrevtickStart = (m_Info.m_PreviousTick)*Freq/SERVER_TICK_SPEED;
			m_Info.m_IntraTick = (m_Info.m_CurrentTime - PrevtickStart) / (float)(CurtickStart-PrevtickStart);
			m_Info.m_TickTime = (m_Info.m_CurrentTime - PrevtickStart) / (float)Freq;
		}
		
		if(m_Info.m_Info.m_CurrentTick == m_Info.m_PreviousTick ||
			m_Info.m_Info.m_CurrentTick == m_Info.m_NextTick)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "tick error prev=%d cur=%d next=%d",
				m_Info.m_PreviousTick, m_Info.m_Info.m_CurrentTick, m_Info.m_NextTick);
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", aBuf);
		}
	}
	
	return 0;
}

int CDemoPlayer::Stop()
{
	if(!m_File)
		return -1;
		
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", "Stopped playback");
	io_close(m_File);
	m_File = 0;
	mem_free(m_pKeyFrames);
	m_pKeyFrames = 0;
	str_copy(m_aFilename, "", sizeof(m_aFilename));
	return 0;
}

char *CDemoPlayer::GetDemoName()
{
	// get the name of the demo without its path
	char *pDemoShortName = &m_aFilename[0];
	for(int i = 0; i < str_length(m_aFilename)-1; i++)
	{
		if(m_aFilename[i] == '/' || m_aFilename[i] == '\\')
			pDemoShortName = &m_aFilename[i+1];
	}
	return pDemoShortName;
}

bool CDemoPlayer::GetDemoInfo(class IStorage *pStorage, const char *pFilename, int StorageType, char *pMap, int BufferSize) const
{
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!File)
		return false;
	
	CDemoHeader Header;
	io_read(File, &Header, sizeof(Header));
	if(mem_comp(Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
	{
		io_close(File);
		return false;
	}
	
	str_copy(pMap, Header.m_aMap, BufferSize);
	
	io_close(File);
	return true;
}
