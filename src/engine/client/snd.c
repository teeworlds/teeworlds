#include <engine/system.h>
#include <engine/interface.h>
#include <engine/config.h>

#include <engine/external/portaudio/portaudio.h>
#include <engine/external/wavpack/wavpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

enum
{
	NUM_SOUNDS = 512,
	NUM_VOICES = 64,
	NUM_CHANNELS = 4,
};

enum
{
	MAX_FRAMES = 1024
};

static struct sound
{
	short *data;
	int num_samples;
	int rate;
	int channels;
	int loop_start;
	int loop_end;
} sounds[NUM_SOUNDS] = { {0x0, 0, 0, 0, -1, -1} };

static struct voice
{
	volatile struct sound *sound;
	int tick;
	int stop;
	int loop;
	float vol;
	float pan;
	float x;
	float y;
	volatile struct voice *prev;
	volatile struct voice *next;
} voices[NUM_VOICES] = { {0x0, 0, -1, -1, 1.0f, 0.0f, 0.0f, 0.0f, 0x0, 0x0} };

#define CHANNEL_POSITION_VOLUME 1
#define CHANNEL_POSITION_PAN 2

static struct channel
{
	volatile struct voice *first_voice;
	float vol;
	float pan;
	int flags;
} channels[NUM_CHANNELS] = { {0x0, 1.0f, 0.0f, 0} };

static float center_x = 0.0f;
static float center_y = 0.0f;
static float master_vol = 1.0f;
static float master_pan = 0.0f;
static float pan_deadzone = 256.0f;
static float pan_falloff = 1.0f;
static float volume_deadzone = 256.0f;
static float volume_falloff = 1.0f;

static inline short int2short(int i)
{
	if(i > 0x7fff)
		return 0x7fff;
	else if(i < -0x7fff)
		return -0x7fff;
	return i;
}

static inline float sgn(float f)
{
	if(f < 0.0f)
		return -1.0f;
	return 1.0f;
}

static void reset_voice(struct voice *v)
{
	v->sound = 0x0;
	v->tick = 0;
	v->stop = -1;
	v->loop = -1;
	v->vol = 1.0f;
	v->pan = 0.0f;
	v->x = 0.0f;
	v->y = 0.0f;
	v->next = 0x0;
	v->prev = 0x0;
}

static inline void fill_mono(int *out, unsigned frames, struct voice *v, float fvol, float fpan)
{
	int ivol = (int) (31.0f * fvol);
	int ipan = (int) (31.0f * ipan);

	unsigned i;
	for(i = 0; i < frames; i++)
	{
		unsigned j = i<<1;
		int val = v->sound->data[v->tick] * ivol;
		out[j] += val;
		out[j+1] += val;
		v->tick++;
	}
}

static inline void fill_stereo(int *out, unsigned frames, struct voice *v, float fvol, float fpan)
{
	int ivol = (int) (31.0f * fvol);
	int ipan = (int) (31.0f * ipan);

	unsigned i;
	for(i = 0; i < frames; i++)
	{
		unsigned j = i<<1;
		out[j] += v->sound->data[v->tick] * ivol;
		out[j+1] += v->sound->data[v->tick+1] * ivol;
		v->tick += 2;
	}
}

static void mix(short *out, unsigned frames)
{
	static int main_buffer[MAX_FRAMES*2];

	dbg_assert(frames <= MAX_FRAMES, "too many frames to fill");

	unsigned i;
	for(i = 0; i < frames; i++)
	{
		unsigned j = i<<1;
		main_buffer[j] = 0;
		main_buffer[j+1] = 0;
	}

	unsigned cid;
	for(cid = 0; cid < NUM_CHANNELS; cid++)
	{
		struct channel *c = &channels[cid];
		struct voice *v = (struct voice*)c->first_voice;

		while(v)
		{
			unsigned filled = 0;
			while(v->sound && filled < frames)
			{
				// calculate maximum frames to fill
				unsigned frames_left = (v->sound->num_samples - v->tick) >> (v->sound->channels-1);
				unsigned long to_fill = frames>frames_left?frames_left:frames;
				float vol = 1.0f;
				float pan = 0.0f;

				// clamp to_fill if voice should stop
				if(v->stop >= 0)
					to_fill = (unsigned)v->stop>frames_left?frames:v->stop;

				// clamp to_fill if we are about to loop
				if(v->loop >= 0 && v->sound->loop_start >= 0)
				{
					unsigned tmp = v->sound->loop_end - v->tick;
					to_fill = tmp>to_fill?to_fill:tmp;
				}

				// calculate voice volume and delta
				if(c->flags & CHANNEL_POSITION_VOLUME)
				{
					float dx = v->x - center_x;
					float dy = v->y - center_y;
					float dist = dx*dx + dy*dy;
					if(dist < volume_deadzone*volume_deadzone)
						vol = master_vol * c->vol;
					else
						vol = master_vol * c->vol / ((dist - volume_deadzone*volume_deadzone)*volume_falloff); //TODO: use some fast 1/x^2
				}
				else
				{
					vol = master_vol * c->vol * v->vol;
				}

				// calculate voice pan and delta
				if(c->flags & CHANNEL_POSITION_PAN)
				{
					float dx = v->x - center_x;
					if(fabs(dx) < pan_deadzone)
						pan = master_pan + c->pan;
					else
						pan = master_pan + c->pan + sgn(dx)*(fabs(dx) - pan_deadzone)/pan_falloff;
				}
				else
				{
					pan = master_pan + c->pan + v->pan;
				}

				// fill the main buffer
				if(v->sound->channels == 1)
					fill_mono(&main_buffer[filled], to_fill, v, vol, pan);
				else
					fill_stereo(&main_buffer[filled], to_fill, v, vol, pan);

				// reset tick of we hit loop point
				if(v->loop >= 0 &&
						v->sound->loop_start >= 0 &&
						v->tick >= v->sound->loop_end)
					v->tick = v->sound->loop_start;

				// stop sample if nessecary
				if(v->stop >= 0)
					v->stop -=  to_fill;
				if(v->tick >= v->sound->num_samples || v->stop == 0)
				{
					if(v->next)
						v->next->prev = v->prev;

					if(v->prev)
						v->prev->next = v->next;
					else
						channels[cid].first_voice = v->next;

					dbg_msg("snd", "sound stopped");

					reset_voice(v);
				}

				filled += to_fill;
			}

			v = (struct voice*)v->next;
		}
	}

	// clamp accumulated values
	for(i = 0; i < frames; i++)
	{
		int j = i<<1;
		int vl = main_buffer[j];
		int vr = main_buffer[j+1];

		out[j] = int2short(vl>>5);
		out[j+1] = int2short(vr>>5);
	}
}

static int pacallback(const void *in, void *out, unsigned long frames, const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags status, void *user)
{
	mix(out, frames);
	return 0;
}

static PaStream *stream;

int snd_init()
{
	PaStreamParameters params;
	PaError err = Pa_Initialize();
	params.device = Pa_GetDefaultOutputDevice();
	params.channelCount = 2;
	params.sampleFormat = paInt16;
	params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
	params.hostApiSpecificStreamInfo = 0x0;

	err = Pa_OpenStream(
			&stream,        /* passes back stream pointer */
			0,              /* no input channels */
			&params,                /* pointer to parameters */
			44100,          /* sample rate */
			128,            /* frames per buffer */
			paClipOff,              /* no clamping */
			pacallback,             /* specify our custom callback */
			0x0); /* pass our data through to callback */
	err = Pa_StartStream(stream);

	return 0;
}

int snd_shutdown()
{
	Pa_StopStream(stream);
	Pa_Terminate();

	return 0;
}

void snd_set_center(int x, int y)
{
	center_x = x;
	center_y = y;
}

int snd_alloc_id()
{
	unsigned sid;
	for(sid = 0; sid < NUM_SOUNDS; sid++)
	{
		if(sounds[sid].data == 0x0)
		{
			return sid;
		}
	}

	return -1;
}

static FILE *file = NULL;

static int read_data(void *buffer, int size)
{
	return fread(buffer, 1, size, file);	
}

int snd_load_wv(const char *filename)
{
	struct sound *snd;
	int sid = -1;
	char error[100];

	sid = snd_alloc_id();
	if(sid < 0)
		return -1;
	snd = &sounds[sid];

	file = fopen(filename, "rb"); // TODO: use system.h stuff for this

	WavpackContext *context = WavpackOpenFileInput(read_data, error);
	if (context)
	{
		int samples = WavpackGetNumSamples(context);
		int bitspersample = WavpackGetBitsPerSample(context);
		unsigned int samplerate = WavpackGetSampleRate(context);
		int channels = WavpackGetNumChannels(context);

		snd->channels = channels;
		snd->rate = samplerate;

		if(snd->channels > 2)
		{
			dbg_msg("sound/wv", "file is not mono or stereo. filename='%s'", filename);
			return -1;
		}

		if(snd->rate != 44100)
		{
			dbg_msg("sound/wv", "file is %d Hz, not 44100 Hz. filename='%s'", snd->rate, filename);
			return -1;
		}
		
		if(bitspersample != 16)
		{
			dbg_msg("sound/wv", "bps is %d, not 16, filname='%s'", bitspersample, filename);
			return -1;
		}

		int *data = (int *)mem_alloc(4*samples*channels, 1);
		WavpackUnpackSamples(context, data, samples); // TODO: check return value
		int *src = data;
		
		snd->data = (short *)mem_alloc(2*samples*channels, 1);
		short *dst = snd->data;

		int i;
		for (i = 0; i < samples*channels; i++)
			*dst++ = (short)*src++;

		mem_free(data);

		snd->num_samples = samples;
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

	return sid;
}

int snd_load_wav(const char *filename)
{
	// open file for reading
	IOHANDLE file;
	file = io_open(filename, IOFLAG_READ);
	if(!file)
	{
		dbg_msg("sound/wav", "failed to open file. filename='%s'", filename);
		return -1;
	}

	struct sound *snd;
	int sid = -1;

	sid = snd_alloc_id();
	if(sid < 0)
		return -1;
	snd = &sounds[sid];

	int state = 0;
	while(1)
	{
		// read chunk header
		unsigned char head[8];
		if(io_read(file, head, sizeof(head)) != 8)
		{
			break;
		}
		
		int chunk_size = head[4] | (head[5]<<8) | (head[6]<<16) | (head[7]<<24);
		head[4] = 0;
			
		if(state == 0)
		{
			// read the riff and wave headers
			if(head[0] != 'R' || head[1] != 'I' || head[2] != 'F' || head[3] != 'F')
			{
				dbg_msg("sound/wav", "not a RIFF file. filename='%s'", filename);
				return -1;
			}
			
			unsigned char type[4];
			io_read(file, type, 4);

			if(type[0] != 'W' || type[1] != 'A' || type[2] != 'V' || type[3] != 'E')
			{
				dbg_msg("sound/wav", "RIFF file is not a WAVE. filename='%s'", filename);
				return -1;
			}
			
			state++;
		}
		else if(state == 1)
		{
			// read the format chunk
			if(head[0] == 'f' && head[1] == 'm' && head[2] == 't' && head[3] == ' ')
			{
				unsigned char fmt[16];
				if(io_read(file, fmt, sizeof(fmt)) !=  sizeof(fmt))
				{
					dbg_msg("sound/wav", "failed to read format. filename='%s'", filename);
					return -1;
				}
				
				// decode format
				int compression_code = fmt[0] | (fmt[1]<<8);
				snd->channels = fmt[2] | (fmt[3]<<8);
				snd->rate = fmt[4] | (fmt[5]<<8) | (fmt[6]<<16) | (fmt[7]<<24);

				if(compression_code != 1)
				{
					dbg_msg("sound/wav", "file is not uncompressed. filename='%s'", filename);
					return -1;
				}
				
				if(snd->channels > 2)
				{
					dbg_msg("sound/wav", "file is not mono or stereo. filename='%s'", filename);
					return -1;
				}

				if(snd->rate != 44100)
				{
					dbg_msg("sound/wav", "file is %d Hz, not 44100 Hz. filename='%s'", snd->rate, filename);
					return -1;
				}
				
				int bps = fmt[14] | (fmt[15]<<8);
				if(bps != 16)
				{
					dbg_msg("sound/wav", "bps is %d, not 16, filname='%s'", bps, filename);
					return -1;
				}
				
				// next state
				state++;
			}
			else
				io_skip(file, chunk_size);
		}
		else if(state == 2)
		{
			// read the data
			if(head[0] == 'd' && head[1] == 'a' && head[2] == 't' && head[3] == 'a')
			{
				snd->data = (short*)mem_alloc(chunk_size, 1);
				io_read(file, snd->data, chunk_size);
				snd->num_samples = chunk_size/(2);
#if defined(CONF_ARCH_ENDIAN_BIG)
				swap_endian(snd->data, sizeof(short), snd->num_samples);
#endif
				snd->loop_start = -1;
				snd->loop_end = -1;
				state++;
			}
			else
				io_skip(file, chunk_size);
		}
		else if(state == 3)
		{
			if(head[0] == 's' && head[1] == 'm' && head[2] == 'p' && head[3] == 'l')
			{
				unsigned char smpl[36];
				unsigned char loop[24];
				if(config.debug)
					dbg_msg("sound/wav", "got sustain");

				io_read(file, smpl, sizeof(smpl));
				unsigned num_loops = (smpl[28] | (smpl[29]<<8) | (smpl[30]<<16) | (smpl[31]<<24));
				unsigned skip = (smpl[32] | (smpl[33]<<8) | (smpl[34]<<16) | (smpl[35]<<24));

				if(num_loops > 0)
				{
					io_read(file, loop, sizeof(loop));
					unsigned start = (loop[8] | (loop[9]<<8) | (loop[10]<<16) | (loop[11]<<24));
					unsigned end = (loop[12] | (loop[13]<<8) | (loop[14]<<16) | (loop[15]<<24));
					snd->loop_start = start * snd->channels;
					snd->loop_end = end * snd->channels;
				}

				if(num_loops > 1)
					io_skip(file, (num_loops-1) * sizeof(loop));

				io_skip(file, skip);
				state++;
			}
			else
				io_skip(file, chunk_size);
		}
		else
			io_skip(file, chunk_size);
	}

	if(config.debug)
		dbg_msg("sound/wav", "loaded %s", filename);

	return sid;
}

int snd_play(int cid, int sid, int loop, int x, int y)
{
	int vid;
	for(vid = 0; vid < NUM_VOICES; vid++)
	{
		if(voices[vid].sound == 0x0)
		{
			voices[vid].tick = 0;
			voices[vid].x = x;
			voices[vid].y = y;
			voices[vid].sound = &sounds[sid];
			if(loop == SND_LOOP)
				voices[vid].loop = voices[vid].sound->loop_end;
			else
				voices[vid].loop = -1;

			// add voice to channel last, to avoid threding errors
			voices[vid].next = channels[cid].first_voice;
			if(channels[cid].first_voice)
				channels[cid].first_voice->prev = &voices[vid];
			channels[cid].first_voice = &voices[vid];
			return vid;
		}
	}

	return -1;
}

void snd_set_master_volume(float val)
{
	master_vol = val;
}

void snd_stop(int vid)
{
	//TODO: lerp volume to 0
	voices[vid].stop = 0;
}
