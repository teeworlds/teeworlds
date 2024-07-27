/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DEMO_H
#define ENGINE_SHARED_DEMO_H

#include <engine/demo.h>
#include <engine/shared/protocol.h>

#include "huffman.h"
#include "snapshot.h"

class CDemoRecorder : public IDemoRecorder
{
	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	CHuffman m_Huffman;
	IOHANDLE m_File;
	int m_LastTickMarker;
	int m_LastKeyFrame;
	int m_FirstTick;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	class CSnapshotDelta *m_pSnapshotDelta;
	int m_NumTimelineMarkers;
	int m_aTimelineMarkers[MAX_TIMELINE_MARKERS];

	void WriteTickMarker(int Tick, int Keyframe);
	void Write(int Type, const void *pData, int Size);
public:
	CDemoRecorder(class CSnapshotDelta *pSnapshotDelta);
	void Init(class IConsole *pConsole, class IStorage *pStorage);

	int Start(const char *pFilename, bool WithTimestamp, const char *pNetversion, const char *pMap, SHA256_DIGEST MapSha256, unsigned MapCrc, const char *pType);
	int Stop();
	void AddDemoMarker();

	void RecordSnapshot(int Tick, const void *pData, int Size);
	void RecordMessage(const void *pData, int Size);

	bool IsRecording() const { return m_File != 0; }

	int Length() const { return (m_LastTickMarker - m_FirstTick)/SERVER_TICK_SPEED; }
};

class CDemoPlayer : public IDemoPlayer
{
public:
	class IListener
	{
	public:
		virtual ~IListener() {}
		virtual void OnDemoPlayerSnapshot(void *pData, int Size) = 0;
		virtual void OnDemoPlayerMessage(void *pData, int Size) = 0;
	};

	struct CPlaybackInfo
	{
		CDemoHeader m_Header;

		IDemoPlayer::CInfo m_Info;

		int64 m_LastUpdate;
		int64 m_CurrentTime;

		int m_SeekablePoints;

		int m_NextTick;
		int m_PreviousTick;

		float m_IntraTick;
		float m_TickTime;
	};

private:
	static const float ms_aSpeeds[];

	IListener *m_pListener;


	// Playback
	struct CKeyFrame
	{
		long m_Filepos;
		int m_Tick;
	};

	struct CKeyFrameSearch
	{
		CKeyFrame m_Frame;
		CKeyFrameSearch *m_pNext;
	};

	class IConsole *m_pConsole;
	class IStorage *m_pStorage;
	CHuffman m_Huffman;
	IOHANDLE m_File;
	char m_aFilename[256];
	char m_aErrorMsg[256];
	CKeyFrame *m_pKeyFrames;

	CPlaybackInfo m_Info;
	int m_DemoType;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	int m_LastSnapshotDataSize;
	class CSnapshotDelta *m_pSnapshotDelta;

	int ReadChunkHeader(int *pType, int *pSize, int *pTick);
	void DoTick();
	void ScanFile();

public:

	CDemoPlayer(class CSnapshotDelta *pSnapshotDelta);
	void Init(class IConsole *pConsole, class IStorage *pStorage);
	void SetListener(IListener *pListener);

	const char *Load(const char *pFilename, int StorageType, const char *pNetversion);
	int Play();
	void Pause();
	void Unpause();
	int Stop();
	void SetSpeed(float Speed);
	void SetSpeedIndex(int Offset);
	int SetPos(float Percent);
	int SetPos(int WantedTick);
	const CInfo *BaseInfo() const { return &m_Info.m_Info; }
	void GetDemoName(char *pBuffer, int BufferSize) const;
	bool GetDemoInfo(const char *pFilename, int StorageType, CDemoHeader *pDemoHeader) const;
	int GetDemoType() const;

	int Update();

	const CPlaybackInfo *Info() const { return &m_Info; }
	int IsPlaying() const { return m_File != 0; }
};

#endif
