/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>
#include <engine/loader.h>

class CSound : public IEngineSound
{
	int m_SoundEnabled;

	struct SLoadSoundJobInfo
	{
		CSound *m_pSound;
		int m_Id;

		const char *m_pFilename;
		void *m_pWVData;
		unsigned m_WVDataSize;

		void *m_pWaveData;
		unsigned m_WaveDataSize;
	};

	static int Job_LoadSound_LoadFile(CJobHandler *pJobHandler, void *pData);
	static int Job_LoadSound_ParseWV(CJobHandler *pJobHandler, void *pData);
public:
	IEngineGraphics *m_pGraphics;
	IStorage *m_pStorage;

	virtual int Init();

	int Update();
	int Shutdown();
	int AllocID();

	static void RateConvert(int SampleID);

	// TODO: Refactor: clean this mess up
	static IOHANDLE ms_File;
	static int ReadData(void *pBuffer, int Size);

	virtual bool IsSoundEnabled() { return m_SoundEnabled != 0; }

	virtual int LoadWV(const char *pFilename);

	virtual void SetListenerPos(float x, float y);
	virtual void SetChannel(int ChannelID, float Vol, float Pan);

	int Play(int ChannelID, int SampleID, int Flags, float x, float y);
	virtual int PlayAt(int ChannelID, int SampleID, int Flags, float x, float y);
	virtual int Play(int ChannelID, int SampleID, int Flags);
	virtual void Stop(int SampleID);
	virtual void StopAll();
};

#endif
