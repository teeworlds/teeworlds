/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DEMO_H
#define ENGINE_SHARED_DEMO_H

#include <engine/demo.h>
#include "snapshot.h"

struct CDemoHeader
{
	unsigned char m_aMarker[7];
	unsigned char m_Version;
	char m_aNetversion[64];
	char m_aMap[64];
	unsigned char m_aCrc[4];
	char m_aType[8];
};

class CDemoRecorder : public IDemoRecorder
{
	class IConsole *m_pConsole;
	IOHANDLE m_File;
	int m_LastTickMarker;
	int m_LastKeyFrame;
	int m_FirstTick;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	class CSnapshotDelta *m_pSnapshotDelta;
	
	void WriteTickMarker(int Tick, int Keyframe);
	void Write(int Type, const void *pData, int Size);
public:
	CDemoRecorder(class CSnapshotDelta *pSnapshotDelta);
	
	int Start(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, const char *pNetversion, const char *pMap, int MapCrc, const char *pType);
	int Stop();

	void RecordSnapshot(int Tick, const void *pData, int Size);
	void RecordMessage(const void *pData, int Size);

	bool IsRecording() const { return m_File != 0; }

	int TickCount() const { return m_LastTickMarker - m_FirstTick; }
};

class CDemoPlayer : public IDemoPlayer
{
public:
	class IListner
	{
	public:
		virtual ~IListner() {}
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
	IListner *m_pListner;


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
	IOHANDLE m_File;
	char m_aFilename[256];
	CKeyFrame *m_pKeyFrames;

	CPlaybackInfo m_Info;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	int m_LastSnapshotDataSize;
	class CSnapshotDelta *m_pSnapshotDelta;

	int ReadChunkHeader(int *pType, int *pSize, int *pTick);
	void DoTick();
	void ScanFile();
	int NextFrame();

public:
	
	CDemoPlayer(class CSnapshotDelta *m_pSnapshotDelta);
	
	void SetListner(IListner *pListner);
		
	int Load(class IStorage *pStorage, class IConsole *pConsole, const char *pFilename, int StorageType);
	int Play();
	void Pause();
	void Unpause();
	int Stop();	
	void SetSpeed(float Speed);
	int SetPos(float Precent);
	const CInfo *BaseInfo() const { return &m_Info.m_Info; }
	char *GetDemoName();
	bool GetDemoInfo(class IStorage *pStorage, const char *pFilename, int StorageType, char *pMap, int BufferSize) const;
	
	int Update();
	
	const CPlaybackInfo *Info() const { return &m_Info; }
	int IsPlaying() const { return m_File != 0; }
};

#endif
