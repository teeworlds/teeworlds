/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>

class CSound : public IEngineSound
{
	int m_SoundEnabled;

public:
	IEngineGraphics *m_pGraphics;
	IStorage *m_pStorage;

	virtual int Init();

	int Update();
	int Shutdown();
	int AllocID();

	static void RateConvert(int SampleID);

	virtual bool IsSoundEnabled() { return m_SoundEnabled != 0; }

	virtual CSampleHandle LoadWV(const char *pFilename);

	virtual void SetListenerPos(float x, float y);
	virtual void SetChannelVolume(int ChannelID, float Vol);
	virtual void SetMaxDistance(float Distance);

	int Play(int ChannelID, CSampleHandle SampleID, int Flags, float x, float y);
	virtual int PlayAt(int ChannelID, CSampleHandle SampleID, int Flags, float x, float y);
	virtual int Play(int ChannelID, CSampleHandle SampleID, int Flags);
	virtual void Stop(CSampleHandle SampleID);
	virtual void StopAll();
	virtual bool IsPlaying(CSampleHandle SampleID);
};

#endif
