#include <baselib/system.h>
#include <baselib/audio.h>
#include <baselib/stream/file.h>

#include <engine/interface.h>

using namespace baselib;

static const int NUM_FRAMES_STOP = 512;
static const float NUM_FRAMES_STOP_INV = 1.0f/(float)NUM_FRAMES_STOP;
static const int NUM_FRAMES_LERP = 512;
static const float NUM_FRAMES_LERP_INV = 1.0f/(float)NUM_FRAMES_LERP;

static const float GLOBAL_VOLUME_SCALE = 0.75f;

static const int64 GLOBAL_SOUND_DELAY = 1000;

// --- sound ---
class sound_data
{
public:
	short *data;
	int num_samples;
	int rate;
	int channels;
	int sustain_start;
	int sustain_end;
	int64 last_played;
};

inline short clamp(int i)
{
	if(i > 0x7fff)
		return 0x7fff;
	if(i < -0x7fff)
		return -0x7fff;
	return i;
}

class mixer : public audio_stream
{
public:
	class channel
	{
	public:
		channel()
		{ data = 0; lerp = -1; stop = -1; }
		
		sound_data *data;
		int tick;
		int loop;
		float pan;
		float vol;
		float old_vol;
		float new_vol;
		int lerp;
		int stop;
	};
	
	enum
	{
		MAX_CHANNELS=8,
	};

	channel channels[MAX_CHANNELS];

	virtual void fill(void *output, unsigned long frames)
	{
		//dbg_msg("snd", "mixing!");
		
		short *out = (short*)output;
		bool clamp_flag = false;
		
		int active_channels = 0;
		for(unsigned long i = 0; i < frames; i++)
		{
			int left = 0;
			int right = 0;
			
			for(int c = 0; c < MAX_CHANNELS; c++)
			{
				if(channels[c].data)
				{
					if(channels[c].data->channels == 1)
					{
						left += (1.0f-(channels[c].pan+1.0f)*0.5f) * channels[c].vol * channels[c].data->data[channels[c].tick];
						right += (channels[c].pan+1.0f)*0.5f * channels[c].vol * channels[c].data->data[channels[c].tick];
						channels[c].tick++;
					}
					else
					{
						float pl = channels[c].pan<0.0f?-channels[c].pan:1.0f;
						float pr = channels[c].pan>0.0f?1.0f-channels[c].pan:1.0f;
						left += pl*channels[c].vol * channels[c].data->data[channels[c].tick];
						right += pr*channels[c].vol * channels[c].data->data[channels[c].tick + 1];
						channels[c].tick += 2;
					}
				
					if(channels[c].loop)
					{
						if(channels[c].data->sustain_start >= 0 && channels[c].tick >= channels[c].data->sustain_end)
							channels[c].tick = channels[c].data->sustain_start;
						else if(channels[c].tick > channels[c].data->num_samples)
							channels[c].tick = 0;
					}
					else if(channels[c].tick > channels[c].data->num_samples)
						channels[c].data = 0;

					if(channels[c].stop == 0)
					{
						channels[c].stop = -1;
						channels[c].data = 0;
					}
					else if(channels[c].stop > 0)
					{
						channels[c].vol = channels[c].old_vol * (float)channels[c].stop * NUM_FRAMES_STOP_INV;
						channels[c].stop--;
					}
					if(channels[c].lerp > 0)
					{
						channels[c].vol = (1.0f - (float)channels[c].lerp * NUM_FRAMES_LERP_INV) * channels[c].new_vol +
							(float)channels[c].lerp * NUM_FRAMES_LERP_INV * channels[c].old_vol;
						channels[c].lerp--;
					}
					active_channels++;
				}
			}

			// TODO: remove these

			*out = clamp(left); // left
			if(*out != left) clamp_flag = true;
			out++;
			*out = clamp(right); // right
			if(*out != right) clamp_flag = true;
			out++;
		}

		if(clamp_flag)
			dbg_msg("snd", "CLAMPED!");
	}
	
	int play(sound_data *sound, unsigned loop, float vol, float pan)
	{
		if(time_get() - sound->last_played < GLOBAL_SOUND_DELAY)
			return -1;

		for(int c = 0; c < MAX_CHANNELS; c++)
		{
			if(channels[c].data == 0)
			{
				channels[c].data = sound;
				channels[c].tick = 0;
				channels[c].loop = loop;
				channels[c].vol = vol * GLOBAL_VOLUME_SCALE;
				channels[c].pan = pan;
				sound->last_played = time_get();
				return c;
			}
		}

		return -1;
	}

	void stop(int id)
	{
		dbg_assert(id >= 0 && id < MAX_CHANNELS, "id out of bounds");
		channels[id].old_vol = channels[id].vol;
		channels[id].stop = NUM_FRAMES_STOP;
	}

	void set_vol(int id, float vol)
	{
		dbg_assert(id >= 0 && id < MAX_CHANNELS, "id out of bounds");
		channels[id].new_vol = vol * GLOBAL_VOLUME_SCALE;
		channels[id].old_vol = channels[id].vol;
		channels[id].lerp = NUM_FRAMES_LERP;
	}
};

static mixer mixer;
//static sound_data test_sound;

/*
extern "C" 
{
#include "wavpack/wavpack.h"
}*/

/*
static file_stream *read_func_filestream;
static int32_t read_func(void *buff, int32_t bcount)
{
    return read_func_filestream->read(buff, bcount);
}
static uchar *format_samples(int bps, uchar *dst, int32_t *src, uint32_t samcnt)
{
    int32_t temp;

    switch (bps) {

        case 1:
            while (samcnt--)
                *dst++ = *src++ + 128;

            break;

        case 2:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
            }

            break;

        case 3:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
                *dst++ = (uchar)(temp >> 16);
            }

            break;

        case 4:
            while (samcnt--) {
                *dst++ = (uchar)(temp = *src++);
                *dst++ = (uchar)(temp >> 8);
                *dst++ = (uchar)(temp >> 16);
                *dst++ = (uchar)(temp >> 24);
            }

            break;
    }

    return dst;
}*/

/*
struct sound_holder
{
	sound_data sound;
	int next;
};

static const int MAX_SOUNDS = 256;
static sound_holder sounds[MAX_SOUNDS];
static int first_free_sound;

bool snd_load_wv(const char *filename, sound_data *snd)
{
	// open file
	file_stream file;
	if(!file.open_r(filename))
	{
		dbg_msg("sound/wv", "failed to open file. filename='%s'", filename);
		return false;
	}
	read_func_filestream = &file;
	
	// get info
	WavpackContext *wpc;
	char error[128];
	wpc = WavpackOpenFileInput(read_func, error);
	if(!wpc)
	{
		dbg_msg("sound/wv", "failed to open file. err=%s filename='%s'", error, filename);
		return false;
	}


	snd->num_samples = WavpackGetNumSamples(wpc);
    int bps = WavpackGetBytesPerSample(wpc);
	int channels = WavpackGetReducedChannels(wpc);
	snd->rate = WavpackGetSampleRate(wpc);
	int bits = WavpackGetBitsPerSample(wpc);
	
	(void)bps;
	(void)channels;
	(void)bits;
	
	// decompress
	int datasize = snd->num_samples*2;
	snd->data = (short*)mem_alloc(datasize, 1);
	int totalsamples = 0;
	while(1)
	{
		int buffer[1024*4];
		int samples_unpacked = WavpackUnpackSamples(wpc, buffer, 1024*4);
		totalsamples += samples_unpacked;
		
		if(samples_unpacked)
		{
			// convert
		}
	}
	
	if(snd->num_samples != totalsamples)
	{
		dbg_msg("sound/wv", "wrong amount of samples. filename='%s'", filename);
		mem_free(snd->data);
		return false;;
	}
		
	return false;
}*/

struct sound_holder
{
	sound_data sound;
	int next;
};

static const int MAX_SOUNDS = 1024;
static sound_holder sounds[MAX_SOUNDS];
static int first_free_sound;

bool snd_init()
{
	first_free_sound = 0;
	for(int i = 0; i < MAX_SOUNDS; i++)
		sounds[i].next = i+1;
	sounds[MAX_SOUNDS-1].next = -1;
	return mixer.create();
}

bool snd_shutdown()
{
	mixer.destroy();
	return true;
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

int snd_load_wav(const char *filename)
{
	sound_data snd;
	
	// open file for reading
	file_stream file;
	if(!file.open_r(filename))
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
		if(file.read(head, sizeof(head)) != 8)
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
			file.read(type, 4);

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
				if(file.read(fmt, sizeof(fmt)) !=  sizeof(fmt))
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
				
				// skip extra bytes (not used for uncompressed)
				//int extra_bytes = fmt[14] | (fmt[15]<<8);
				//dbg_msg("sound/wav", "%d", extra_bytes);
				//file.skip(extra_bytes);
				
				// next state
				state++;
			}
			else
				file.skip(chunk_size);
		}
		else if(state == 2)
		{
			// read the data
			if(head[0] == 'd' && head[1] == 'a' && head[2] == 't' && head[3] == 'a')
			{
				snd.data = (short*)mem_alloc(chunk_size, 1);
				file.read(snd.data, chunk_size);
				snd.num_samples = chunk_size/(2);
				snd.sustain_start = -1;
				snd.sustain_end = -1;
				snd.last_played = 0;
				id = snd_alloc_sound();
				sounds[id].sound = snd;
				state++;
			}
			else
				file.skip(chunk_size);
		}
		else if(state == 3)
		{
			if(head[0] == 's' && head[1] == 'm' && head[2] == 'p' && head[3] == 'l')
			{
				int smpl[9];
				int loop[6];

				file.read(smpl, sizeof(smpl));

				if(smpl[7] > 0)
				{
					file.read(loop, sizeof(loop));
					sounds[id].sound.sustain_start = loop[2] * sounds[id].sound.channels;
					sounds[id].sound.sustain_end = loop[3] * sounds[id].sound.channels;
				}

				if(smpl[7] > 1)
					file.skip((smpl[7]-1) * sizeof(loop));

				file.skip(smpl[8]);
				state++;
			}
			else
				file.skip(chunk_size);
		}
		else
			file.skip(chunk_size);
	}

	if(id >= 0)
		dbg_msg("sound/wav", "loaded %s", filename);
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
	return mixer.play(&sounds[id].sound, loop, vol, pan);
}

void snd_stop(int id)
{
	if(id >= 0)
		mixer.stop(id);
}

void snd_set_vol(int id, float vol)
{
	if(id >= 0)
		mixer.set_vol(id, vol);
}
