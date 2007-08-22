#include <engine/system.h>
#include <engine/interface.h>
#include <engine/config.h>

#include <engine/external/portaudio/portaudio.h>
#include <engine/external/wavpack/wavpack.h>

enum
{
	NUM_FRAMES_STOP = 512,
	NUM_FRAMES_LERP = 512,
};

static const float NUM_FRAMES_STOP_INV = 1.0f/(float)NUM_FRAMES_STOP;
static const float NUM_FRAMES_LERP_INV = 1.0f/(float)NUM_FRAMES_LERP;

static const float GLOBAL_VOLUME_SCALE = 0.75f;
static float master_volume = 1.0f;

static const float GLOBAL_SOUND_DELAY = 0.05f;

// --- sound ---
typedef struct
{
	/*
public:
	sound_data() :
		data(0x0),
		num_samples(0),
		rate(0),
		channels(0),
		sustain_start(-1),
		sustain_end(-1),
		last_played(0)
	{ }*/

	short *data;
	int num_samples;
	int rate;
	int channels;
	int sustain_start;
	int sustain_end;
	int64 last_played;
} SOUND_DATA;


static float clampf(float val, float lower, float upper)
{
	if(val > upper)
		return upper;
	if(val < lower)
		return lower;
	return val;
}


static int clampi(int val, int lower, int upper)
{
	if(val > upper)
		return upper;
	if(val < lower)
		return lower;
	return val;
}
/*
template<typename T>
inline const T clamp(const T val, const T lower, const T upper)
{
	if(val > upper)
		return upper;
	if(val < lower)
		return lower;
	return val;
}*/


typedef struct
{
/*
public:
	channel()
	{ data = 0; lerp = -1; stop = -1; }
*/
	
	SOUND_DATA *data;
	int tick;
	int loop;
	float pan;
	float vol;
	float old_vol;
	float new_vol;
	int lerp;
	int stop;
} MIXER_CHANNEL;

enum
{
	MAX_CHANNELS=32,
	MAX_FILL_FRAMES=256,
};

static MIXER_CHANNEL channels[MAX_CHANNELS];
static int buffer[MAX_FILL_FRAMES*2];

static void mixer_fill_mono(int *out, unsigned long frames, MIXER_CHANNEL *c, float dv)
{
	float pl = clampf(1.0f - c->pan, 0.0f, 1.0f);
	float pr = clampf(1.0f + c->pan, 0.0f, 1.0f);
	unsigned long i;

	for(i = 0; i < frames; i++)
	{
		float val = c->vol * master_volume * c->data->data[c->tick];

		out[i<<1] += (int)(pl*val);
		out[(i<<1)+1] += (int)(pr*val);
		c->tick++;
		c->vol += dv;
		if(c->vol < 0.0f) c->vol = 0.0f;
	}
}

static void mixer_fill_stereo(int *out, unsigned long frames, MIXER_CHANNEL *c, float dv)
{
	float pl = clampf(1.0f - c->pan, 0.0f, 1.0f);
	float pr = clampf(1.0f + c->pan, 0.0f, 1.0f);
	unsigned long i;

	for(i = 0; i < frames; i++)
	{
		int vl = (int)(pl*c->vol * master_volume * c->data->data[c->tick]);
		int vr = (int)(pr*c->vol * master_volume * c->data->data[c->tick + 1]);
		out[i<<1] += vl;
		out[(i<<1)+1] += vr;
		c->tick += 2;
		c->vol += dv;
		if(c->vol < 0.0f) c->vol = 0.0f;
	}
}

static void mixer_fill(void *output, unsigned long frames)
{
	short *out = (short*)output;

	dbg_assert(frames <= MAX_FILL_FRAMES, "not enough fill frames in buffer");
	unsigned long i;
	int c;

	for(i = 0; i < frames; i++)
	{
		buffer[i<<1] = 0;
		buffer[(i<<1)+1] = 0;
	}

	for(c = 0; c < MAX_CHANNELS; c++)
	{
		unsigned long filled = 0;
		while(channels[c].data && filled < frames)
		{
			unsigned long frames_left = (channels[c].data->num_samples - channels[c].tick) >> (channels[c].data->channels-1);
			unsigned long to_fill = frames>frames_left?frames_left:frames;
			float dv = 0.0f;

			if(channels[c].stop >= 0)
				to_fill = (unsigned)channels[c].stop>frames_left?frames:channels[c].stop;
			if(channels[c].loop >= 0 &&
					channels[c].data->sustain_start >= 0)
			{
				unsigned long tmp = channels[c].data->sustain_end - channels[c].tick;
				to_fill = tmp>frames?frames:tmp;
			}

			if(channels[c].lerp >= 0)
			{
					dv = (channels[c].new_vol - channels[c].old_vol) * NUM_FRAMES_LERP_INV;
			}

			if(channels[c].data->channels == 1)
				mixer_fill_mono(buffer, to_fill, &channels[c], dv);
			else
				mixer_fill_stereo(buffer, to_fill, &channels[c], dv);

			if(channels[c].loop >= 0 &&
					channels[c].data->sustain_start >= 0 &&
					channels[c].tick >= channels[c].data->sustain_end)
				channels[c].tick = channels[c].data->sustain_start;

			if(channels[c].stop >= 0)
				channels[c].stop -=  to_fill;
			if(channels[c].tick >= channels[c].data->num_samples ||
					channels[c].stop == 0)
				channels[c].data = 0;

			channels[c].lerp -= to_fill;
			if(channels[c].lerp < 0)
				channels[c].lerp = -1;


			filled += to_fill;
		}
	}

	for(i = 0; i < frames; i++)
	{
		out[i<<1] = (short)clampi(buffer[i<<1], -0x7fff, 0x7fff);
		out[(i<<1)+1] = (short)clampi(buffer[(i<<1)+1], -0x7fff, 0x7fff);
	}
}

int mixer_play(SOUND_DATA *sound, unsigned loop, float vol, float pan)
{
	if(time_get() - sound->last_played < (int64)(time_freq()*GLOBAL_SOUND_DELAY))
		return -1;

	int c;
	for(c = 0; c < MAX_CHANNELS; c++)
	{
		if(channels[c].data == 0)
		{
			channels[c].data = sound;
			channels[c].tick = 0;
			channels[c].loop = loop;
			channels[c].vol = vol * GLOBAL_VOLUME_SCALE;
			channels[c].pan = pan;
			channels[c].stop = -1;
			channels[c].lerp = -1;
			sound->last_played = time_get();
			return c;
		}
	}

	return -1;
}

static void mixer_stop(int id)
{
	dbg_assert(id >= 0 && id < MAX_CHANNELS, "id out of bounds");
	channels[id].old_vol = channels[id].vol;
	channels[id].stop = NUM_FRAMES_STOP;
}

static void mixer_set_vol(int id, float vol)
{
	dbg_assert(id >= 0 && id < MAX_CHANNELS, "id out of bounds");
	channels[id].new_vol = vol * GLOBAL_VOLUME_SCALE;
	channels[id].old_vol = channels[id].vol;
	channels[id].lerp = NUM_FRAMES_LERP;
}

typedef struct
{
	SOUND_DATA sound;
	int next;
} SOUND;

enum
{
	MAX_SOUNDS = 1024,
};

static SOUND sounds[MAX_SOUNDS];
static int first_free_sound;

static PaStream *stream = 0;

static int pacallback(const void *in, void *out, unsigned long frames, const PaStreamCallbackTimeInfo* time, PaStreamCallbackFlags status, void *user)
{
	mixer_fill(out, frames);
	return 0;
}

int snd_init()
{
	int i;
	first_free_sound = 0;
	for(i = 0; i < MAX_SOUNDS; i++)
		sounds[i].next = i+1;
	sounds[MAX_SOUNDS-1].next = -1;
	
	// init PA
	PaStreamParameters params;
	PaError err = Pa_Initialize();
	if(err != paNoError)
	{
		dbg_msg("audio_stream", "portaudio error: %s", Pa_GetErrorText(err));
		return 0;
	}

	params.device = Pa_GetDefaultOutputDevice();
	if(params.device == -1)
	{
		dbg_msg("audio_stream", "no default output device");
		return 0;
	}
	params.channelCount = 2;
	params.sampleFormat = paInt16;
	params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
	params.hostApiSpecificStreamInfo = 0x0;

	err = Pa_OpenStream(
		&stream,        /* passes back stream pointer */
		0,              /* no input channels */
		&params,		/* pointer to parameters */
		44100,          /* sample rate */
		128,            /* frames per buffer */
		paClipOff,		/* no clamping */
		pacallback,		/* specify our custom callback */
		0); /* pass our data through to callback */	

	if(err != paNoError)
	{
		dbg_msg("audio_stream", "portaudio error: %s", Pa_GetErrorText(err));
		return 0;
	}
	
	err = Pa_StartStream(stream);
	if(err != paNoError)
	{
		dbg_msg("audio_stream", "portaudio error: %s", Pa_GetErrorText(err));
		return 0;
	}
	
	return 1;	
}

int snd_shutdown()
{
	Pa_StopStream(stream);
	stream = NULL;
	Pa_Terminate();	
	return 1;
}

float snd_get_master_volume()
{
	return master_volume;
}

void snd_set_master_volume(float val)
{
	if(val < 0.0f)
		val = 0.0f;
	else if(val > 1.0f)
		val = 1.0f;

	master_volume = val;
}

static int snd_alloc_sound()
{
	if(first_free_sound < 0)
		return -1;
	int id = first_free_sound;
	first_free_sound = sounds[id].next;
	sounds[id].next = -1;
	return id;
}

static FILE *file = NULL;

static int read_data(void *buffer, int size)
{
	return fread(buffer, 1, size, file);	
}

int snd_load_wv(const char *filename)
{
	SOUND_DATA snd;
	int id = -1;

	char error[100];

	file = fopen(filename, "rb"); // TODO: use system.h stuff for this

	WavpackContext *context = WavpackOpenFileInput(read_data, error);
	if (context)
	{
		int samples = WavpackGetNumSamples(context);
		int bitspersample = WavpackGetBitsPerSample(context);
		unsigned int samplerate = WavpackGetSampleRate(context);
		int channels = WavpackGetNumChannels(context);

		snd.channels = channels;
		snd.rate = samplerate;

		if(snd.channels > 2)
		{
			dbg_msg("sound/wv", "file is not mono or stereo. filename='%s'", filename);
			return -1;
		}

		if(snd.rate != 44100)
		{
			dbg_msg("sound/wv", "file is %d Hz, not 44100 Hz. filename='%s'", snd.rate, filename);
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
		
		snd.data = (short *)mem_alloc(2*samples*channels, 1);
		short *dst = snd.data;

		int i;
		for (i = 0; i < samples*channels; i++)
			*dst++ = (short)*src++;

		mem_free(data);

		snd.num_samples = samples;
		snd.sustain_start = -1;
		snd.sustain_end = -1;
		snd.last_played = 0;
		id = snd_alloc_sound();
		sounds[id].sound = snd;
	}
	else
	{
		dbg_msg("sound/wv", "failed to open %s: %s", filename, error);
	}

	fclose(file);
	file = NULL;

	if(id >= 0)
	{
		if(config.debug)
			dbg_msg("sound/wv", "loaded %s", filename);
	}
	else
	{
		dbg_msg("sound/wv", "failed to load %s", filename);
	}

	return id;
}

int snd_load_wav(const char *filename)
{
	SOUND_DATA snd;
	
	// open file for reading
	IOHANDLE file;
	file = io_open(filename, IOFLAG_READ);
	if(!file)
	{
		dbg_msg("sound/wav", "failed to open file. filename='%s'", filename);
		return -1;
	}

	int id = -1;
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
				snd.channels = fmt[2] | (fmt[3]<<8);
				snd.rate = fmt[4] | (fmt[5]<<8) | (fmt[6]<<16) | (fmt[7]<<24);

				if(compression_code != 1)
				{
					dbg_msg("sound/wav", "file is not uncompressed. filename='%s'", filename);
					return -1;
				}
				
				if(snd.channels > 2)
				{
					dbg_msg("sound/wav", "file is not mono or stereo. filename='%s'", filename);
					return -1;
				}

				if(snd.rate != 44100)
				{
					dbg_msg("sound/wav", "file is %d Hz, not 44100 Hz. filename='%s'", snd.rate, filename);
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
				snd.data = (short*)mem_alloc(chunk_size, 1);
				io_read(file, snd.data, chunk_size);
				snd.num_samples = chunk_size/(2);
#if defined(CONF_ARCH_ENDIAN_BIG)
				for(unsigned i = 0; i < (unsigned)snd.num_samples; i++)
				{
					unsigned j = i << 1;
					snd.data[i] = ((short)((char*)snd.data)[j]) + ((short)((char*)snd.data)[j+1] << 8);
				}
#endif
				snd.sustain_start = -1;
				snd.sustain_end = -1;
				snd.last_played = 0;
				id = snd_alloc_sound();
				sounds[id].sound = snd;
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
					sounds[id].sound.sustain_start = start * sounds[id].sound.channels;
					sounds[id].sound.sustain_end = end * sounds[id].sound.channels;
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

	if(id >= 0)
	{
		if(config.debug)
			dbg_msg("sound/wav", "loaded %s", filename);
	}
	else
		dbg_msg("sound/wav", "failed to load %s", filename);

	return id;
}

int snd_play(int id, int loop, float vol, float pan)
{
	if(id < 0)
	{
		dbg_msg("snd", "bad sound id");
		return -1;
	}

	dbg_assert(sounds[id].sound.data != 0, "null sound");
	dbg_assert(sounds[id].next == -1, "sound isn't allocated");
	return mixer_play(&sounds[id].sound, loop, vol, pan);
}

void snd_stop(int id)
{
	if(id >= 0)
		mixer_stop(id);
}

void snd_set_vol(int id, float vol)
{
	if(id >= 0)
		mixer_set_vol(id, vol);
}
