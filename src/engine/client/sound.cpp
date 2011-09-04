/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "SDL.h"

#include "sound.h"

extern "C" { // wavpack
	#include <engine/external/wavpack/wavpack.h>
}
#include <math.h>

enum
{
	NUM_SAMPLES = 512,
	NUM_VOICES = 64,
	NUM_CHANNELS = 16,

	MAX_FRAMES = 1024
};

struct CChannel
{
	int m_Vol;
	int m_Pan;
} ;

struct CVoice
{
	CSound::CResource_Sample *m_pSample;
	CChannel *m_pChannel;
	int m_Tick;
	int m_Vol; // 0 - 255
	int m_Flags;
	int m_X, m_Y;
} ;

static CVoice m_aVoices[NUM_VOICES] = { {0} };
static CChannel m_aChannels[NUM_CHANNELS] = { {255, 0} };

static LOCK m_SoundLock = 0;

static int m_CenterX = 0;
static int m_CenterY = 0;

static int m_MixingRate = 48000;
static volatile int m_SoundVolume = 100;

static int m_NextVoice = 0;


// TODO: there should be a faster way todo this
static short Int2Short(int i)
{
	if(i > 0x7fff)
		return 0x7fff;
	else if(i < -0x7fff)
		return -0x7fff;
	return i;
}

static int IntAbs(int i)
{
	if(i<0)
		return -i;
	return i;
}

static void Mix(short *pFinalOut, unsigned Frames)
{
	int aMixBuffer[MAX_FRAMES*2] = {0};
	int MasterVol;

	// aquire lock while we are mixing
	lock_wait(m_SoundLock);

	MasterVol = m_SoundVolume;

	for(unsigned i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample)
		{
			// mix voice
			CVoice *v = &m_aVoices[i];
			int *pOut = aMixBuffer;

			int Step = v->m_pSample->m_Channels; // setup input sources
			short *pInL = &v->m_pSample->m_pData[v->m_Tick*Step];
			short *pInR = &v->m_pSample->m_pData[v->m_Tick*Step+1];

			unsigned End = v->m_pSample->m_NumFrames-v->m_Tick;

			int Rvol = v->m_pChannel->m_Vol;
			int Lvol = v->m_pChannel->m_Vol;

			// make sure that we don't go outside the sound data
			if(Frames < End)
				End = Frames;

			// check if we have a mono sound
			if(v->m_pSample->m_Channels == 1)
				pInR = pInL;

			// volume calculation
			if(v->m_Flags&ISound::FLAG_POS && v->m_pChannel->m_Pan)
			{
				// TODO: we should respect the channel panning value
				const int Range = 1500; // magic value, remove
				int dx = v->m_X - m_CenterX;
				int dy = v->m_Y - m_CenterY;
				int Dist = (int)sqrtf((float)dx*dx+dy*dy); // float here. nasty
				int p = IntAbs(dx);
				if(Dist >= 0 && Dist < Range)
				{
					// panning
					if(dx > 0)
						Lvol = ((Range-p)*Lvol)/Range;
					else
						Rvol = ((Range-p)*Rvol)/Range;

					// falloff
					Lvol = (Lvol*(Range-Dist))/Range;
					Rvol = (Rvol*(Range-Dist))/Range;
				}
				else
				{
					Lvol = 0;
					Rvol = 0;
				}
			}

			// process all frames
			for(unsigned s = 0; s < End; s++)
			{
				*pOut++ += (*pInL)*Lvol;
				*pOut++ += (*pInR)*Rvol;
				pInL += Step;
				pInR += Step;
				v->m_Tick++;
			}

			// free voice if not used any more
			if(v->m_Tick == v->m_pSample->m_NumFrames)
			{
				if(v->m_Flags&ISound::FLAG_LOOP)
					v->m_Tick = 0;
				else
					v->m_pSample = 0;
			}
		}
	}


	// release the lock
	lock_release(m_SoundLock);

	{
		// clamp accumulated values
		// TODO: this seams slow
		for(unsigned i = 0; i < Frames; i++)
		{
			int j = i<<1;
			int vl = ((aMixBuffer[j]*MasterVol)/101)>>8;
			int vr = ((aMixBuffer[j+1]*MasterVol)/101)>>8;

			pFinalOut[j] = Int2Short(vl);
			pFinalOut[j+1] = Int2Short(vr);
		}
	}

#if defined(CONF_ARCH_ENDIAN_BIG)
	swap_endian(pFinalOut, sizeof(short), Frames * 2);
#endif
}

static void SdlCallback(void *pUnused, Uint8 *pStream, int Len)
{
	(void)pUnused;
	Mix((short *)pStream, Len/2/2);
}


int CSound::Init()
{
	m_SoundEnabled = 0;
	m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pResources = Kernel()->RequestInterface<IResources>();

	m_ResourceHandler.m_pSound = this;
	m_pResources->AssignHandler("wv", &m_ResourceHandler);

	SDL_AudioSpec Format;

	m_SoundLock = lock_create();

	if(!g_Config.m_SndEnable)
		return 0;

	m_MixingRate = g_Config.m_SndRate;

	// Set 16-bit stereo audio at 22Khz
	Format.freq = g_Config.m_SndRate; // ignore_convention
	Format.format = AUDIO_S16; // ignore_convention
	Format.channels = 2; // ignore_convention
	Format.samples = g_Config.m_SndBufferSize; // ignore_convention
	Format.callback = SdlCallback; // ignore_convention
	Format.userdata = NULL; // ignore_convention

	// Open the audio device and start playing sound!
	if(SDL_OpenAudio(&Format, NULL) < 0)
	{
		dbg_msg("client/sound", "unable to open audio: %s", SDL_GetError());
		return -1;
	}
	else
		dbg_msg("client/sound", "sound init successful");

	SDL_PauseAudio(0);

	m_SoundEnabled = 1;
	Update(); // update the volume
	return 0;
}

int CSound::Update()
{
	// update volume
	int WantedVolume = g_Config.m_SndVolume;

	if(!m_pGraphics->WindowActive() && g_Config.m_SndNonactiveMute)
		WantedVolume = 0;

	if(WantedVolume != m_SoundVolume)
	{
		lock_wait(m_SoundLock);
		m_SoundVolume = WantedVolume;
		lock_release(m_SoundLock);
	}

	return 0;
}

int CSound::Shutdown()
{
	SDL_CloseAudio();
	lock_destroy(m_SoundLock);
	return 0;
}

IResource *CSound::CResourceHandler::Create(IResources::CResourceId Id)
{
	return new CResource_Sample();
}


// Ugly TLS solution
__thread char *gt_pWVData;
__thread int gt_WVDataSize;
static int ThreadReadData(void *pBuffer, int ChunkSize)
{
	if(ChunkSize > gt_WVDataSize)
		ChunkSize = gt_WVDataSize;
	mem_copy(pBuffer, gt_pWVData, ChunkSize);
	gt_pWVData += ChunkSize;
	gt_WVDataSize -= ChunkSize;
	return ChunkSize;
}

bool CSound::CResourceHandler::Load(IResource *pResource, void *pData, unsigned DataSize)
{
	CResource_Sample *pSample = static_cast<CResource_Sample*>(pResource);

	char aError[100];
	gt_pWVData = (char*)pData;
	gt_WVDataSize = DataSize;
	WavpackContext *pContext = WavpackOpenFileInput(ThreadReadData, aError);
	if (pContext)
	{
		int NumSamples = WavpackGetNumSamples(pContext);
		int BitsPerSample = WavpackGetBitsPerSample(pContext);
		unsigned int SampleRate = WavpackGetSampleRate(pContext);
		int NumChannels = WavpackGetNumChannels(pContext);

		if(NumChannels > 2)
		{
			dbg_msg("sound/wv", "file is not mono or stereo. filename='%s'", pResource->m_Id.m_pName);
			return -1;
		}

		/*
		if(snd->rate != 44100)
		{
			dbg_msg("sound/wv", "file is %d Hz, not 44100 Hz. filename='%s'", snd->rate, filename);
			return -1;
		}*/

		if(BitsPerSample != 16)
		{
			dbg_msg("sound/wv", "bps is %d, not 16, filname='%s'", BitsPerSample, pResource->m_Id.m_pName);
			return -1;
		}

		short *pFinalData = (short *)mem_alloc(2*NumSamples*NumChannels, 1);

		{
			int *pTmpData = (int *)mem_alloc(4*NumSamples*NumChannels, 1);
			WavpackUnpackSamples(pContext, pTmpData, NumSamples); // TODO: check return value

			// convert int32 to int16
			{
				int *pSrc = pTmpData;
				short *pDst = pFinalData;
				for(int i = 0; i < NumSamples*NumChannels; i++)
					*pDst++ = (short)*pSrc++;
			}

			mem_free(pTmpData);
		}

		// do rate convert
		{
			int NewNumFrames = 0;
			short *pNewData = 0;

			// allocate new data
			NewNumFrames = (int)((NumSamples / (float)SampleRate)*m_MixingRate);
			pNewData = (short *)mem_alloc(NewNumFrames*NumChannels*sizeof(short), sizeof(void*));

			for(int i = 0; i < NewNumFrames; i++)
			{
				// resample TODO: this should be done better, like linear atleast
				float a = i/(float)NewNumFrames;
				int f = (int)(a*NumSamples);
				if(f >= NumSamples)
					f = NumSamples-1;

				// set new data
				if(NumChannels == 1)
					pNewData[i] = pFinalData[f];
				else if(NumChannels == 2)
				{
					pNewData[i*2] = pFinalData[f*2];
					pNewData[i*2+1] = pFinalData[f*2+1];
				}
			}

			// free old data and apply new
			mem_free(pFinalData);
			pFinalData = pNewData;
			NumSamples = NewNumFrames;
		}


		// insert it directly, we don't need to wait for anything
		pSample->m_Channels = NumChannels;
		pSample->m_Rate = SampleRate;
		pSample->m_LoopStart = -1;
		pSample->m_LoopEnd = -1;
		pSample->m_PausedAt = 0;
		pSample->m_pData = pFinalData;
		sync_barrier(); // make sure that all parameters are written before we say how large it is
		pSample->m_NumFrames = NumSamples;
	}
	else
	{
		dbg_msg("sound/wv", "failed to open %s: %s", pResource->m_Id.m_pName, aError);
	}

	//RateConvert(SampleID);
	return 0;
}

bool CSound::CResourceHandler::Insert(IResource *pResource)
{
	// sounds can be inserted directly when they are loaded
	return true;
}


IResource *CSound::LoadWV(const char *pFilename)
{
	return m_pResources->GetResource(pFilename);
}

void CSound::SetListenerPos(float x, float y)
{
	m_CenterX = (int)x;
	m_CenterY = (int)y;
}


void CSound::SetChannel(int ChannelID, float Vol, float Pan)
{
	m_aChannels[ChannelID].m_Vol = (int)(Vol*255.0f);
	m_aChannels[ChannelID].m_Pan = (int)(Pan*255.0f); // TODO: this is only on and off right now
}

int CSound::Play(int ChannelID, IResource *pSoundResource, int Flags, float x, float y)
{
	CResource_Sample *pSample = static_cast<CResource_Sample*>(pSoundResource);
	int VoiceID = -1;
	int i;

	lock_wait(m_SoundLock);

	// search for voice
	for(i = 0; i < NUM_VOICES; i++)
	{
		int id = (m_NextVoice + i) % NUM_VOICES;
		if(!m_aVoices[id].m_pSample)
		{
			VoiceID = id;
			m_NextVoice = id+1;
			break;
		}
	}

	// voice found, use it
	if(VoiceID != -1)
	{
		m_aVoices[VoiceID].m_pSample = pSample;
		m_aVoices[VoiceID].m_pChannel = &m_aChannels[ChannelID];
		if(Flags & FLAG_LOOP)
			m_aVoices[VoiceID].m_Tick = pSample->m_PausedAt;
		else
			m_aVoices[VoiceID].m_Tick = 0;
		m_aVoices[VoiceID].m_Vol = 255;
		m_aVoices[VoiceID].m_Flags = Flags;
		m_aVoices[VoiceID].m_X = (int)x;
		m_aVoices[VoiceID].m_Y = (int)y;
	}

	lock_release(m_SoundLock);
	return VoiceID;
}

int CSound::PlayAt(int ChannelID, IResource *pSoundResource, int Flags, float x, float y)
{
	return Play(ChannelID, pSoundResource, Flags|ISound::FLAG_POS, x, y);
}

int CSound::Play(int ChannelID, IResource *pSoundResource, int Flags)
{
	return Play(ChannelID, pSoundResource, Flags, 0, 0);
}

void CSound::Stop(IResource *pSoundResource)
{
	CResource_Sample *pSample = static_cast<CResource_Sample*>(pSoundResource);

	// TODO: a nice fade out
	lock_wait(m_SoundLock);
	for(int i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample == pSample)
		{
			if(m_aVoices[i].m_Flags & FLAG_LOOP)
				m_aVoices[i].m_pSample->m_PausedAt = m_aVoices[i].m_Tick;
			else
				m_aVoices[i].m_pSample->m_PausedAt = 0;
			m_aVoices[i].m_pSample = 0;
		}
	}
	lock_release(m_SoundLock);
}

void CSound::StopAll()
{
	// TODO: a nice fade out
	lock_wait(m_SoundLock);
	for(int i = 0; i < NUM_VOICES; i++)
	{
		if(m_aVoices[i].m_pSample)
		{
			if(m_aVoices[i].m_Flags & FLAG_LOOP)
				m_aVoices[i].m_pSample->m_PausedAt = m_aVoices[i].m_Tick;
			else
				m_aVoices[i].m_pSample->m_PausedAt = 0;
		}
		m_aVoices[i].m_pSample = 0;
	}
	lock_release(m_SoundLock);
}

IEngineSound *CreateEngineSound() { return new CSound; }

