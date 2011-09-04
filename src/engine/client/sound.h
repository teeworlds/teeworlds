/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>
#include <engine/loader.h>

class CSound : public IEngineSound
{
	int m_SoundEnabled;

	/*
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
	*/

	class CResource_Sound : public IResources::IResource
	{
	public:
		CResource_Sound(int Slot)
		{
			m_Slot = Slot;
			//m_pWaveData = 0x0;
			//m_WaveDataSize = 0;
			//m_TexSlot = 0;
			//mem_zero(&m_ImageInfo, sizeof(m_ImageInfo));
		}

		int m_Slot;
		//void *m_pWaveData;
		//unsigned m_WaveDataSize;



		//GLuint m_TexId;
		//int m_TexSlot;

		// used for loading the texture
		// TODO: should perhaps just be stored at load time
		//CImageInfo m_ImageInfo;
	};

	class CResourceHandler : public IResources::IHandler
	{
	public:
		CSound *m_pSound;
		//static unsigned int PngReadFunc(void *pOutput, unsigned long size, unsigned long numel, void *pUserPtr);
		virtual IResources::IResource *Create(IResources::CResourceId Id);
		virtual bool Load(IResources::IResource *pResource, void *pData, unsigned DataSize);
		virtual bool Insert(IResources::IResource *pResource);
	};

	CResourceHandler m_ResourceHandler;

public:
	IResources *m_pResources;
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
