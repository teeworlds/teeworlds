/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>
#include <engine/storage.h>
#include <engine/graphics.h>
#include <engine/shared/engine.h>

class CSound : public IEngineSound
{
public:
	IEngineGraphics *m_pGraphics;
	IStorage *m_pStorage;

	virtual int Init();

	int Update();
	int Shutdown();
	int AllocId();

	static void RateConvert(int SampleId);

	// TODO: Refactor: clean this mess up
	static IOHANDLE ms_File;
	static int ReadData(void *pBuffer, int Size);

	virtual int LoadWV(const char *pFilename);

	virtual void SetListenerPos(float x, float y);
	virtual void SetChannel(int ChannelId, float Vol, float Pan);

	int Play(int ChannelId, int SampleId, int Flags, float x, float y);
	virtual int PlayAt(int ChannelId, int SampleId, int Flags, float x, float y);
	virtual int Play(int ChannelId, int SampleId, int Flags);
	virtual void Stop(int VoiceId);
	virtual void StopAll();
};

#endif
