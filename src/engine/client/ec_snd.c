/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <base/system.h>
#include <engine/e_client_interface.h>
#include <engine/e_config.h>

#ifdef CONFIG_NO_SDL
	#include <portaudio.h>
#else
	#include "SDL.h"
#endif


#include <engine/external/wavpack/wavpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

enum
{
	NUM_SAMPLES = 512,
	NUM_VOICES = 64,
	NUM_CHANNELS = 16,
	
	MAX_FRAMES = 1024
};

typedef struct
{
	short *data;
	int num_frames;
	int rate;
	int channels;
	int loop_start;
	int loop_end;
} SAMPLE;

typedef struct
{
	int vol;
	int pan;
} CHANNEL;

typedef struct VOICE_t
{
	SAMPLE *snd;
	CHANNEL *channel;
	int tick;
	int vol; /* 0 - 255 */
	int flags;
	int x, y;
} VOICE;

static SAMPLE samples[NUM_SAMPLES] = { {0} };
static VOICE voices[NUM_VOICES] = { {0} };
static CHANNEL channels[NUM_CHANNELS] = { {255, 0} };

static LOCK sound_lock = 0;
static int sound_enabled = 0;

static int center_x = 0;
static int center_y = 0;

static int mixing_rate = 48000;
static volatile int sound_volume = 100;

void snd_set_channel(int cid, float vol, float pan)
{
	channels[cid].vol = (int)(vol*255.0f);
	channels[cid].pan = (int)(pan*255.0f); /* TODO: this is only on and off right now */
}

static int play(int cid, int sid, int flags, float x, float y)
{
	int vid = -1;
	int i;
	
	lock_wait(sound_lock);
	
	/* search for voice */
	/* TODO: fix this linear search */
	for(i = 0; i < NUM_VOICES; i++)
	{
		if(!voices[i].snd)
		{
			vid = i;
			break;
		}
	}
	
	/* voice found, use it */
	if(vid != -1)
	{
		voices[vid].snd = &samples[sid];
		voices[vid].channel = &channels[cid];
		voices[vid].tick = 0;
		voices[vid].vol = 255;
		voices[vid].flags = flags;
		voices[vid].x = (int)x;
		voices[vid].y = (int)y;
	}
	
	lock_release(sound_lock);
	return vid;
}

int snd_play_at(int cid, int sid, int flags, float x, float y)
{
	return play(cid, sid, flags|SNDFLAG_POS, x, y);
}

int snd_play(int cid, int sid, int flags)
{
	return play(cid, sid, flags, 0, 0);
}

void snd_stop(int vid)
{
	/* TODO: a nice fade out */
	lock_wait(sound_lock);
	voices[vid].snd = 0;
	lock_release(sound_lock);
}

/* TODO: there should be a faster way todo this */
static short int2short(int i)
{
	if(i > 0x7fff)
		return 0x7fff;
	else if(i < -0x7fff)
		return -0x7fff;
	return i;
}

static int iabs(int i)
{
	if(i<0)
		return -i;
	return i;
}

static void mix(short *final_out, unsigned frames)
{
	int mix_buffer[MAX_FRAMES*2] = {0};
	int i, s;
	int master_vol;

	/* aquire lock while we are mixing */
	lock_wait(sound_lock);
	
	master_vol = sound_volume;
	
	for(i = 0; i < NUM_VOICES; i++)
	{
		if(voices[i].snd)
		{
			/* mix voice */
			VOICE *v = &voices[i];
			int *out = mix_buffer;

			int step = v->snd->channels; /* setup input sources */
			short *in_l = &v->snd->data[v->tick*step];
			short *in_r = &v->snd->data[v->tick*step+1];
			
			int end = v->snd->num_frames-v->tick;

			int rvol = v->channel->vol;
			int lvol = v->channel->vol;

			/* make sure that we don't go outside the sound data */
			if(frames < end)
				end = frames;
			
			/* check if we have a mono sound */
			if(v->snd->channels == 1)
				in_r = in_l;

			/* volume calculation */
			if(v->flags&SNDFLAG_POS && v->channel->pan)
			{
				/* TODO: we should respect the channel panning value */
				const int range = 1500; /* magic value, remove */
				int dx = v->x - center_x;
				int dy = v->y - center_y;
				int dist = sqrt(dx*dx+dy*dy); /* double here. nasty */
				int p = iabs(dx);
				if(dist < range)
				{
					/* panning */
					if(dx > 0)
						lvol = ((range-p)*lvol)/range;
					else
						rvol = ((range-p)*rvol)/range;
					
					/* falloff */
					lvol = (lvol*(range-dist))/range;
					rvol = (rvol*(range-dist))/range;
				}
				else
				{
					lvol = 0;
					rvol = 0;
				}
			}

			/* process all frames */
			for(s = 0; s < end; s++)
			{
				*out++ += (*in_l)*lvol;
				*out++ += (*in_r)*rvol;
				in_l += step;
				in_r += step;
				v->tick++;
			}
			
			/* free voice if not used any more */
			if(v->tick == v->snd->num_frames)
				v->snd = 0;
			
		}
	}
	
	
	/* release the lock */
	lock_release(sound_lock);

	{
		/* clamp accumulated values */
		/* TODO: this seams slow */
		for(i = 0; i < frames; i++)
		{
			int j = i<<1;
			int vl = ((mix_buffer[j]*master_vol)/101)>>8;
			int vr = ((mix_buffer[j+1]*master_vol)/101)>>8;

			final_out[j] = int2short(vl);
			final_out[j+1] = int2short(vr);
		}
	}
}

#ifdef CONFIG_NO_SDL
static PaStream *stream;
static int pacallback(const void *in, void *out, unsigned long frames, const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags status, void *user)
{
	mix(out, frames);
	return 0;
}
#else
static void sdlcallback(void *unused, Uint8 *stream, int len)
{
	mix((short *)stream, len/2/2);
}
#endif


int snd_init()
{
#ifdef CONFIG_NO_SDL
	PaStreamParameters params;
	PaError err = Pa_Initialize();
	
	sound_lock = lock_create();
	
	if(!config.snd_enable)
		return 0;
	
	mixing_rate = config.snd_rate;

	{
		int num = Pa_GetDeviceCount();
		int i;
		const PaDeviceInfo *info;
		
		for(i = 0; i < num; i++)
		{
			info = Pa_GetDeviceInfo(i);
			dbg_msg("snd", "device #%d name='%s'", i, info->name);
		}
	}

	params.device = config.snd_device;
	if(params.device == -1)
	{
		params.device = Pa_GetDefaultOutputDevice();
		if(params.device < 0)
		{
			dbg_msg("snd", "no default device (%d)", params.device);
			return 1;
		}
	}

	if(params.device < 0)
		return 1;
	dbg_msg("snd", "device = %d", params.device);
	params.channelCount = 2;
	params.sampleFormat = paInt16;
	params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
	params.hostApiSpecificStreamInfo = 0x0;

	err = Pa_OpenStream(
			&stream,        /* passes back stream pointer */
			0,              /* no input channels */
			&params,                /* pointer to parameters */
			mixing_rate,          /* sample rate */
			128,            /* frames per buffer */
			paClipOff,              /* no clamping */
			pacallback,             /* specify our custom callback */
			0x0); /* pass our data through to callback */
	err = Pa_StartStream(stream);
#else
    SDL_AudioSpec format;
	
	sound_lock = lock_create();
	
	if(!config.snd_enable)
		return 0;
	
	mixing_rate = config.snd_rate;

    /* Set 16-bit stereo audio at 22Khz */
    format.freq = config.snd_rate;
    format.format = AUDIO_S16;
    format.channels = 2;
    format.samples = 512;        /* A good value for games */
    format.callback = sdlcallback;
    format.userdata = NULL;

    /* Open the audio device and start playing sound! */
    if(SDL_OpenAudio(&format, NULL) < 0)
	{
        dbg_msg("client/sound", "unable to open audio: %s", SDL_GetError());
		return -1;
    }
	else
        dbg_msg("client/sound", "sound init successful");

	SDL_PauseAudio(0);
	
#endif
	sound_enabled = 1;
	snd_update(); /* update the volume */
	return 0;
}

int snd_update()
{
	/* update volume */
	int wanted_volume = config.snd_volume;
	
	if(!gfx_window_active() && config.snd_nonactive_mute)
		wanted_volume = 0;
	
	if(wanted_volume != sound_volume)
	{
		lock_wait(sound_lock);
		sound_volume = wanted_volume;
		lock_release(sound_lock);
	}
	
	return 0;
}

int snd_shutdown()
{
#ifdef CONFIG_NO_SDL	
	Pa_StopStream(stream);
	Pa_Terminate();
#else
	SDL_CloseAudio();
#endif
	lock_destroy(sound_lock);

	return 0;
}

int snd_alloc_id()
{
	/* TODO: linear search, get rid of it */
	unsigned sid;
	for(sid = 0; sid < NUM_SAMPLES; sid++)
	{
		if(samples[sid].data == 0x0)
			return sid;
	}

	return -1;
}

static void rate_convert(int sid)
{
	SAMPLE *snd = &samples[sid];
	int num_frames = 0;
	short *new_data = 0;
	int i;
	
	/* make sure that we need to convert this sound */
	if(!snd->data || snd->rate == mixing_rate)
		return;

	/* allocate new data */
	num_frames = (int)((snd->num_frames/(float)snd->rate)*mixing_rate);
	new_data = mem_alloc(num_frames*snd->channels*sizeof(short), 1);
	
	for(i = 0; i < num_frames; i++)
	{
		/* resample TODO: this should be done better, like linear atleast */
		float a = i/(float)num_frames;
		int f = (int)(a*snd->num_frames);
		if(f >= snd->num_frames)
			f = snd->num_frames-1;
		
		/* set new data */
		if(snd->channels == 1)
			new_data[i] = snd->data[f];
		else if(snd->channels == 2)
		{
			new_data[i*2] = snd->data[f*2];
			new_data[i*2+1] = snd->data[f*2+1];
		}
	}
	
	/* free old data and apply new */
	mem_free(snd->data);
	snd->data = new_data;
	snd->num_frames = num_frames;
}


static FILE *file = NULL;

static int read_data(void *buffer, int size)
{
	return fread(buffer, 1, size, file);	
}

int snd_load_wv(const char *filename)
{
	SAMPLE *snd;
	int sid = -1;
	char error[100];
	WavpackContext *context;
	
	/* don't waste memory on sound when we are stress testing */
	if(config.dbg_stress)
		return -1;
		
	/* no need to load sound when we are running with no sound */
	if(!sound_enabled)
		return 1;

	file = fopen(filename, "rb"); /* TODO: use system.h stuff for this */
	if(!file)
	{
		dbg_msg("sound/wv", "failed to open %s", filename);
		return -1;
	}

	sid = snd_alloc_id();
	if(sid < 0)
		return -1;
	snd = &samples[sid];

	context = WavpackOpenFileInput(read_data, error);
	if (context)
	{
		int samples = WavpackGetNumSamples(context);
		int bitspersample = WavpackGetBitsPerSample(context);
		unsigned int samplerate = WavpackGetSampleRate(context);
		int channels = WavpackGetNumChannels(context);
		int *data;
		int *src;
		short *dst;
		int i;

		snd->channels = channels;
		snd->rate = samplerate;

		if(snd->channels > 2)
		{
			dbg_msg("sound/wv", "file is not mono or stereo. filename='%s'", filename);
			return -1;
		}

		/*
		if(snd->rate != 44100)
		{
			dbg_msg("sound/wv", "file is %d Hz, not 44100 Hz. filename='%s'", snd->rate, filename);
			return -1;
		}*/
		
		if(bitspersample != 16)
		{
			dbg_msg("sound/wv", "bps is %d, not 16, filname='%s'", bitspersample, filename);
			return -1;
		}

		data = (int *)mem_alloc(4*samples*channels, 1);
		WavpackUnpackSamples(context, data, samples); /* TODO: check return value */
		src = data;
		
		snd->data = (short *)mem_alloc(2*samples*channels, 1);
		dst = snd->data;

		for (i = 0; i < samples*channels; i++)
			*dst++ = (short)*src++;

		mem_free(data);

		snd->num_frames = samples;
		snd->loop_start = -1;
		snd->loop_end = -1;
	}
	else
	{
		dbg_msg("sound/wv", "failed to open %s: %s", filename, error);
	}

	fclose(file);
	file = NULL;

	if(config.debug)
		dbg_msg("sound/wv", "loaded %s", filename);

	rate_convert(sid);
	return sid;
}

void snd_set_listener_pos(float x, float y)
{
	center_x = (int)x;
	center_y = (int)y;
}
