/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>
#include <engine/loader.h>

class CSound : public IEngineSound
{
	int m_SoundEnabled;
public:
	class CResource_Sample : public IResource
	{
	public:
		CResource_Sample()
		{
			m_pData = 0x0;
			m_NumFrames = 0;
			m_Rate = 0;
			m_Channels = 0;
			m_LoopStart = 0;
			m_LoopEnd = 0;
			m_PausedAt = 0;
		}

		short *m_pData;
		int m_NumFrames;
		int m_Rate;
		int m_Channels;
		int m_LoopStart;
		int m_LoopEnd;
		int m_PausedAt;
	};

	class CResourceHandler : public IResources::IHandler
	{
	public:
		CSound *m_pSound;
		virtual IResource *Create(IResources::CResourceId Id);
		virtual bool Load(IResource *pResource, void *pData, unsigned DataSize);
		virtual bool Insert(IResource *pResource);
	};

	CResourceHandler m_ResourceHandler;
		
	IResources *m_pResources;
	IEngineGraphics *m_pGraphics;
	IStorage *m_pStorage;

	virtual int Init();

	int Update();
	int Shutdown();

	// TODO: reimplement RateConvert
	//static void RateConvert(int SampleID);

	virtual bool IsSoundEnabled() { return m_SoundEnabled != 0; }

	virtual IResource *LoadWV(const char *pFilename);

	virtual void SetListenerPos(float x, float y);
	virtual void SetChannel(int ChannelID, float Vol, float Pan);

	int Play(int ChannelID, IResource *pSound, int Flags, float x, float y);
	virtual int PlayAt(int ChannelID, IResource *pSound, int Flags, float x, float y);
	virtual int Play(int ChannelID, IResource *pSound, int Flags);
	virtual void Stop(IResource *pSound);
	virtual void StopAll();
};

#endif
