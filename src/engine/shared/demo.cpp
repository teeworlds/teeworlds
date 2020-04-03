/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/storage.h>

#include "compression.h"
#include "datafile.h"
#include "demo.h"
#include "memheap.h"
#include "network.h"
#include "snapshot.h"

static const unsigned char gs_aHeaderMarker[7] = {'T', 'W', 'D', 'E', 'M', 'O', 0};
static const unsigned char gs_ActVersion = 4;
static const int gs_LengthOffset = 152;
static const int gs_NumMarkersOffset = 176;

CDemoRecorder::CDemoRecorder(class CSnapshotDelta *pSnapshotDelta)
{
	m_File = 0;
	m_LastTickMarker = -1;
	m_pSnapshotDelta = pSnapshotDelta;
	m_Huffman.Init();
}

// Record
int CDemoRecorder::Start(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pNetVersion, const char *pMap, SHA256_DIGEST Sha256, unsigned Crc, const char *pType)
{
	CDemoHeader Header;
	if(m_File)
		return -1;

	m_pConsole = pConsole;

	// open mapfile
	char aMapFilename[128];
	// try the normal maps folder
	str_format(aMapFilename, sizeof(aMapFilename), "maps/%s.map", pMap);
	IOHANDLE MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL, 0, 0, CDataFileReader::CheckSha256, &Sha256);
	if(!MapFile)
	{
		// try the downloaded maps (sha256)
		char aSha256[SHA256_MAXSTRSIZE];
		sha256_str(Sha256, aSha256, sizeof(aSha256));
		str_format(aMapFilename, sizeof(aMapFilename), "downloadedmaps/%s_%s.map", pMap, aSha256);
		MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL, 0, 0, CDataFileReader::CheckSha256, &Sha256);
	}
	if(!MapFile)
	{
		// try the downloaded maps (crc)
		str_format(aMapFilename, sizeof(aMapFilename), "downloadedmaps/%s_%08x.map", pMap, Crc);
		MapFile = pStorage->OpenFile(aMapFilename, IOFLAG_READ, IStorage::TYPE_ALL, 0, 0, CDataFileReader::CheckSha256, &Sha256);
	}
	if(!MapFile)
	{
		// search for the map within subfolders
		char aBuf[IO_MAX_PATH_LENGTH];
		str_format(aMapFilename, sizeof(aMapFilename), "%s.map", pMap);
		if(pStorage->FindFile(aMapFilename, "maps", IStorage::TYPE_ALL, aBuf, sizeof(aBuf)))
			MapFile = pStorage->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL, 0, 0, CDataFileReader::CheckSha256, &Sha256);
	}
	if(!MapFile)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Unable to open mapfile '%s'", pMap);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", aBuf);
		return -1;
	}

	IOHANDLE DemoFile = pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!DemoFile)
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
	str_copy(Header.m_aMapName, pMap, sizeof(Header.m_aMapName));
	unsigned MapSize = io_length(MapFile);
	uint_to_bytes_be(Header.m_aMapSize, MapSize);
	uint_to_bytes_be(Header.m_aMapCrc, Crc);
	str_copy(Header.m_aType, pType, sizeof(Header.m_aType));
	// Header.m_Length - add this on stop
	str_timestamp(Header.m_aTimestamp, sizeof(Header.m_aTimestamp));
	// Header.m_aNumTimelineMarkers - add this on stop
	// Header.m_aTimelineMarkers - add this on stop
	io_write(DemoFile, &Header, sizeof(Header));

	// write map data
	unsigned char aChunk[1024*64];
	while(1)
	{
		int Bytes = io_read(MapFile, &aChunk, sizeof(aChunk));
		if(Bytes <= 0)
			break;
		io_write(DemoFile, &aChunk, Bytes);
	}
	io_close(MapFile);

	m_LastKeyFrame = -1;
	m_LastTickMarker = -1;
	m_FirstTick = -1;
	m_NumTimelineMarkers = 0;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Recording to '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", aBuf);
	m_File = DemoFile;

	return 0;
}

/*
	Tickmarker
		7	= Always set
		6	= Keyframe flag
		0-5	= Delta tick

	Normal
		7 = Not set
		5-6	= Type
		0-4	= Size
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
		uint_to_bytes_be(aChunk+1, Tick);

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
	if(!m_File)
		return;

	char aBuffer[64*1024];
	char aBuffer2[64*1024];

	/* pad the data with 0 so we get an alignment of 4,
	else the compression won't work and miss some bytes */
	mem_copy(aBuffer2, pData, Size);
	while(Size&3)
		aBuffer2[Size++] = 0;
	Size = CVariableInt::Compress(aBuffer2, Size, aBuffer, sizeof(aBuffer)); // buffer2 -> buffer
	if(Size < 0)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_recorder", "error during intpack compression");
		return;
	}
	Size = m_Huffman.Compress(aBuffer, Size, aBuffer2, sizeof(aBuffer2)); // buffer -> buffer2
	if(Size < 0)
	{
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_recorder", "error during network compression");
		return;
	}

	unsigned char aChunk[3];
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
	char aTmpData[CSnapshot::MAX_SIZE];

	if(m_LastKeyFrame == -1 || (Tick-m_LastKeyFrame) > SERVER_TICK_SPEED*5)
	{
		// write full tickmarker
		WriteTickMarker(Tick, 1);

		// write snapshot
		int SnapSize = ((CSnapshot*)pData)->Serialize(aTmpData);
		Write(CHUNKTYPE_SNAPSHOT, aTmpData, SnapSize);

		m_LastKeyFrame = Tick;
		mem_copy(m_aLastSnapshotData, pData, Size);
	}
	else
	{
		// write tickmarker
		WriteTickMarker(Tick, 0);

		// create delta
		int DeltaSize = m_pSnapshotDelta->CreateDelta((CSnapshot*)m_aLastSnapshotData, (CSnapshot*)pData, &aTmpData);
		if(DeltaSize)
		{
			// record delta
			Write(CHUNKTYPE_DELTA, aTmpData, DeltaSize);
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

	// add the demo length to the header
	io_seek(m_File, gs_LengthOffset, IOSEEK_START);
	unsigned char aLength[4];
	uint_to_bytes_be(aLength, Length());
	io_write(m_File, aLength, sizeof(aLength));

	// add the timeline markers to the header
	io_seek(m_File, gs_NumMarkersOffset, IOSEEK_START);
	unsigned char aNumMarkers[4];
	uint_to_bytes_be(aNumMarkers, m_NumTimelineMarkers);
	io_write(m_File, aNumMarkers, sizeof(aNumMarkers));
	for(int i = 0; i < m_NumTimelineMarkers; i++)
	{
		unsigned char aMarker[4];
		uint_to_bytes_be(aMarker, m_aTimelineMarkers[i]);
		io_write(m_File, aMarker, sizeof(aMarker));
	}

	io_close(m_File);
	m_File = 0;
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", "Stopped recording");

	return 0;
}

void CDemoRecorder::AddDemoMarker()
{
	if(m_LastTickMarker < 0 || m_NumTimelineMarkers >= MAX_TIMELINE_MARKERS)
		return;

	// not more than 1 marker in a second
	if(m_NumTimelineMarkers > 0)
	{
		int Diff = m_LastTickMarker - m_aTimelineMarkers[m_NumTimelineMarkers-1];
		if(Diff < SERVER_TICK_SPEED*1.0f)
			return;
	}

	m_aTimelineMarkers[m_NumTimelineMarkers++] = m_LastTickMarker;

	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_recorder", "Added timeline marker");
}



CDemoPlayer::CDemoPlayer(class CSnapshotDelta *pSnapshotDelta)
{
	m_Huffman.Init();
	m_File = 0;
	m_aErrorMsg[0] = 0;
	m_pKeyFrames = 0;

	m_pSnapshotDelta = pSnapshotDelta;
	m_LastSnapshotDataSize = -1;
}

void CDemoPlayer::SetListener(IListener *pListener)
{
	m_pListener = pListener;
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
			unsigned char aTickData[4];
			if(io_read(m_File, aTickData, sizeof(aTickData)) != sizeof(aTickData))
				return -1;
			*pTick = bytes_be_to_uint(aTickData);
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
			unsigned char aSizeData[1];
			if(io_read(m_File, aSizeData, sizeof(aSizeData)) != sizeof(aSizeData))
				return -1;
			*pSize = aSizeData[0];
		}
		else if(*pSize == 31)
		{
			unsigned char aSizeData[2];
			if(io_read(m_File, aSizeData, sizeof(aSizeData)) != sizeof(aSizeData))
				return -1;
			*pSize = (aSizeData[1]<<8) | aSizeData[0];
		}
	}

	return 0;
}

void CDemoPlayer::ScanFile()
{
	CHeap Heap;
	CKeyFrameSearch *pFirstKey = 0;
	CKeyFrameSearch *pCurrentKey = 0;
	int ChunkTick = 0;

	long StartPos = io_tell(m_File);
	m_Info.m_SeekablePoints = 0;

	while(1)
	{
		long CurrentPos = io_tell(m_File);

		int ChunkSize, ChunkType;
		if(ReadChunkHeader(&ChunkType, &ChunkSize, &ChunkTick))
			break;

		// read the chunk
		if(ChunkType&CHUNKTYPEFLAG_TICKMARKER)
		{
			if(ChunkType&CHUNKTICKFLAG_KEYFRAME)
			{
				// save the position
				CKeyFrameSearch *pKey = (CKeyFrameSearch *)Heap.Allocate(sizeof(CKeyFrameSearch));
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
	int i;
	m_pKeyFrames = (CKeyFrame*)mem_alloc(m_Info.m_SeekablePoints*sizeof(CKeyFrame), 1);
	for(pCurrentKey = pFirstKey, i = 0; pCurrentKey; pCurrentKey = pCurrentKey->m_pNext, i++)
		m_pKeyFrames[i] = pCurrentKey->m_Frame;

	// destroy the temporary heap and seek back to the start
	io_seek(m_File, StartPos, IOSEEK_START);
}

void CDemoPlayer::DoTick()
{
	static char aCompressedData[CSnapshot::MAX_SIZE];
	static char aDecompressed[CSnapshot::MAX_SIZE];
	static char aData[CSnapshot::MAX_SIZE];
	static char aNewSnap[CSnapshot::MAX_SIZE];
	bool GotSnapshot = false;

	// update ticks
	m_Info.m_PreviousTick = m_Info.m_Info.m_CurrentTick;
	m_Info.m_Info.m_CurrentTick = m_Info.m_NextTick;
	int ChunkTick = m_Info.m_Info.m_CurrentTick;

	while(1)
	{
		int DataSize = 0;
		int ChunkType, ChunkSize;
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
			if(io_read(m_File, aCompressedData, ChunkSize) != (unsigned)ChunkSize)
			{
				// stop on error or eof
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "error reading chunk");
				Stop();
				break;
			}

			DataSize = m_Huffman.Decompress(aCompressedData, ChunkSize, aDecompressed, sizeof(aDecompressed));
			if(DataSize < 0)
			{
				// stop on error or eof
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", "error during network decompression");
				Stop();
				break;
			}

			DataSize = CVariableInt::Decompress(aDecompressed, DataSize, aData, sizeof(aData));
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
			GotSnapshot = true;

			// only unpack the delta if we have a valid snapshot
			if(m_LastSnapshotDataSize == -1)
				continue;

			DataSize = m_pSnapshotDelta->UnpackDelta((CSnapshot*)m_aLastSnapshotData, (CSnapshot*)aNewSnap, aData, DataSize);
			if(DataSize >= 0)
			{
				if(m_pListener)
					m_pListener->OnDemoPlayerSnapshot(aNewSnap, DataSize);

				m_LastSnapshotDataSize = DataSize;
				mem_copy(m_aLastSnapshotData, aNewSnap, DataSize);
			}
			else
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "error during unpacking of delta, err=%d", DataSize);
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", aBuf);
			}
		}
		else if(ChunkType == CHUNKTYPE_SNAPSHOT)
		{
			// process full snapshot
			CSnapshotBuilder Builder;
			GotSnapshot = true;

			if(Builder.UnserializeSnap(aData, DataSize))
				DataSize = Builder.Finish(aNewSnap);
			else
				DataSize = -1;

			if(DataSize >= 0)
			{
				m_LastSnapshotDataSize = DataSize;
				mem_copy(m_aLastSnapshotData, aNewSnap, DataSize);
				if(m_pListener)
					m_pListener->OnDemoPlayerSnapshot(aNewSnap, DataSize);
			}
			else
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "error during unpacking of snapshot, err=%d", DataSize);
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", aBuf);
			}
		}
		else
		{
			// if there were no snapshots in this tick, replay the last one
			if(!GotSnapshot && m_pListener && m_LastSnapshotDataSize != -1)
			{
				GotSnapshot = true;
				m_pListener->OnDemoPlayerSnapshot(m_aLastSnapshotData, m_LastSnapshotDataSize);
			}

			// check the remaining types
			if(ChunkType&CHUNKTYPEFLAG_TICKMARKER)
			{
				m_Info.m_NextTick = ChunkTick;
				break;
			}
			else if(ChunkType == CHUNKTYPE_MESSAGE)
			{
				if(m_pListener)
					m_pListener->OnDemoPlayerMessage(aData, DataSize);
			}
		}
	}
}

void CDemoPlayer::Pause()
{
	m_Info.m_Info.m_Paused = true;
}

void CDemoPlayer::Unpause()
{
	m_Info.m_Info.m_Paused = false;
}

const char *CDemoPlayer::Load(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, int StorageType, const char *pNetversion)
{
	m_pConsole = pConsole;
	m_aErrorMsg[0] = 0;
	m_File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!m_File)
	{
		str_format(m_aErrorMsg, sizeof(m_aErrorMsg), "could not open '%s'", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", m_aErrorMsg);
		return m_aErrorMsg;
	}

	// store the filename
	str_copy(m_aFilename, pFilename, sizeof(m_aFilename));

	// clear the playback info
	mem_zero(&m_Info, sizeof(m_Info));
	m_Info.m_Info.m_FirstTick = -1;
	m_Info.m_Info.m_LastTick = -1;
	m_Info.m_NextTick = -1;
	m_Info.m_Info.m_CurrentTick = -1;
	m_Info.m_PreviousTick = -1;
	m_Info.m_Info.m_Speed = 1;

	m_LastSnapshotDataSize = -1;

	// read the header
	io_read(m_File, &m_Info.m_Header, sizeof(m_Info.m_Header));
	if(mem_comp(m_Info.m_Header.m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) != 0)
	{
		str_format(m_aErrorMsg, sizeof(m_aErrorMsg), "'%s' is not a demo file", pFilename);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", m_aErrorMsg);
		io_close(m_File);
		m_File = 0;
		return m_aErrorMsg;
	}

	if(m_Info.m_Header.m_Version != gs_ActVersion)
	{
		str_format(m_aErrorMsg, sizeof(m_aErrorMsg), "demo version %d is not supported", m_Info.m_Header.m_Version);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", m_aErrorMsg);
		io_close(m_File);
		m_File = 0;
		return m_aErrorMsg;
	}

	if(str_comp(m_Info.m_Header.m_aNetversion, pNetversion) != 0)
	{
		str_format(m_aErrorMsg, sizeof(m_aErrorMsg), "net version '%s' is not supported", m_Info.m_Header.m_aNetversion);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demo_player", m_aErrorMsg);
		io_close(m_File);
		m_File = 0;
		return m_aErrorMsg;
	}

	// get demo type
	if(!str_comp(m_Info.m_Header.m_aType, "client"))
		m_DemoType = DEMOTYPE_CLIENT;
	else if(!str_comp(m_Info.m_Header.m_aType, "server"))
		m_DemoType = DEMOTYPE_SERVER;
	else
		m_DemoType = DEMOTYPE_INVALID;

	// read map
	unsigned MapSize = bytes_be_to_uint(m_Info.m_Header.m_aMapSize);

	// check if we already have the map
	// TODO: improve map checking (maps folder, check crc)
	unsigned Crc = bytes_be_to_uint(m_Info.m_Header.m_aMapCrc);
	char aMapFilename[128];
	str_format(aMapFilename, sizeof(aMapFilename), "downloadedmaps/%s_%08x.map", m_Info.m_Header.m_aMapName, Crc);
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

	// get timeline markers
	m_Info.m_Info.m_NumTimelineMarkers = min(bytes_be_to_uint(m_Info.m_Header.m_aNumTimelineMarkers), unsigned(MAX_TIMELINE_MARKERS));
	for(int i = 0; i < m_Info.m_Info.m_NumTimelineMarkers; i++)
	{
		m_Info.m_Info.m_aTimelineMarkers[i] = bytes_be_to_uint(m_Info.m_Header.m_aTimelineMarkers[i]);
	}

	// scan the file for interesting points
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
	m_Info.m_CurrentTime = m_Info.m_PreviousTick*time_freq()/SERVER_TICK_SPEED;
	m_Info.m_LastUpdate = time_get();
	return 0;
}

int CDemoPlayer::SetPos(float Percent)
{
	if(!m_File)
		return -1;

	// -5 because we have to have a current tick and previous tick when we do the playback
	int WantedTick = m_Info.m_Info.m_FirstTick + (int)((m_Info.m_Info.m_LastTick-m_Info.m_Info.m_FirstTick)*Percent) - 5;

	int Keyframe = clamp((int)(m_Info.m_SeekablePoints*Percent), 0, m_Info.m_SeekablePoints-1);

	// get correct key frame
	if(m_pKeyFrames[Keyframe].m_Tick < WantedTick)
		while(Keyframe < m_Info.m_SeekablePoints-1 && m_pKeyFrames[Keyframe].m_Tick < WantedTick)
			Keyframe++;

	while(Keyframe && m_pKeyFrames[Keyframe].m_Tick > WantedTick)
		Keyframe--;

	// seek to the correct keyframe
	io_seek(m_File, m_pKeyFrames[Keyframe].m_Filepos, IOSEEK_START);

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

	if(!IsPlaying() || m_Info.m_Info.m_Paused)
		return 0;

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
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "tick error prev=%d cur=%d next=%d",
			m_Info.m_PreviousTick, m_Info.m_Info.m_CurrentTick, m_Info.m_NextTick);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "demo_player", aBuf);
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
	m_aFilename[0] = '\0';
	return 0;
}

void CDemoPlayer::GetDemoName(char *pBuffer, int BufferSize) const
{
	const char *pFileName = m_aFilename;
	const char *pExtractedName = pFileName;
	const char *pEnd = 0;
	for(; *pFileName; ++pFileName)
	{
		if(*pFileName == '/' || *pFileName == '\\')
			pExtractedName = pFileName+1;
		else if(*pFileName == '.')
			pEnd = pFileName;
	}

	int Length = pEnd > pExtractedName ? min(BufferSize, (int)(pEnd-pExtractedName+1)) : BufferSize;
	str_copy(pBuffer, pExtractedName, Length);
}

bool CDemoPlayer::GetDemoInfo(class IStorage *pStorage, const char *pFilename, int StorageType, CDemoHeader *pDemoHeader) const
{
	if(!pDemoHeader)
		return false;

	mem_zero(pDemoHeader, sizeof(CDemoHeader));

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType);
	if(!File)
		return false;

	io_read(File, pDemoHeader, sizeof(CDemoHeader));
	bool Valid = mem_comp(pDemoHeader->m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) == 0 && pDemoHeader->m_Version == gs_ActVersion;
	io_close(File);
	return Valid;
}

int CDemoPlayer::GetDemoType() const
{
	if(m_File)
		return m_DemoType;
	return DEMOTYPE_INVALID;
}
