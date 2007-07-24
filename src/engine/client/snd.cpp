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
static float master_volume = 1.0f;

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

static class mixer : public audio_stream
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

	void fill_mono(short *out, unsigned long frames, channel *c, float dv = 0.0f)
	{
		for(unsigned long i = 0; i < frames; i++)
		{
			float p = (1.0f-(c->pan+1.0f)*0.5f);
			int val = (int)(p*c->vol * master_volume * c->data->data[c->tick]);
			out[i<<1] += (short)val;
			out[(i<<1)+1] += (short)val;
			c->tick++;
			c->vol += dv;
			if(c->vol < 0.0f) c->vol = 0.0f;
		}
	}

	void fill_stereo(short *out, unsigned long frames, channel *c, float dv = 0.0f)
	{
		for(unsigned long i = 0; i < frames; i++)
		{
			float pl = c->pan<0.0f?-c->pan:1.0f;
			float pr = c->pan>0.0f?1.0f-c->pan:1.0f;
			int vl = (int)(pl*c->vol * master_volume * c->data->data[c->tick]);
			int vr = (int)(pr*c->vol * master_volume * c->data->data[c->tick + 1]);
			out[i<<1] += (short)vl;
			out[(i<<1)+1] += (short)vr;
			c->tick += 2;
			c->vol += dv;
			if(c->vol < 0.0f) c->vol = 0.0f;
		}
	}

	virtual void fill(void *output, unsigned long frames)
	{
		short *out = (short*)output;

		for(unsigned long i = 0; i < frames; i++)
		{
			out[i<<1] = 0;
			out[(i<<1)+1] = 0;
		}

		for(int c = 0; c < MAX_CHANNELS; c++)
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
					fill_mono(out, to_fill, &channels[c], dv);
				else
					fill_stereo(out, to_fill, &channels[c], dv);

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
				channels[c].stop = -1;
				channels[c].lerp = -1;
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
} mixer;

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
				file.skip(chunk_size);
		}
		else if(state == 3)
		{
			if(head[0] == 's' && head[1] == 'm' && head[2] == 'p' && head[3] == 'l')
			{
				unsigned char smpl[36];
				unsigned char loop[24];
				dbg_msg("sound/wav", "got sustain");

				file.read(smpl, sizeof(smpl));
				unsigned num_loops = (smpl[28] | (smpl[29]<<8) | (smpl[30]<<16) | (smpl[31]<<24));
				unsigned skip = (smpl[32] | (smpl[33]<<8) | (smpl[34]<<16) | (smpl[35]<<24));

				if(num_loops > 0)
				{
					file.read(loop, sizeof(loop));
					unsigned start = (loop[8] | (loop[9]<<8) | (loop[10]<<16) | (loop[11]<<24));
					unsigned end = (loop[12] | (loop[13]<<8) | (loop[14]<<16) | (loop[15]<<24));
					sounds[id].sound.sustain_start = start * sounds[id].sound.channels;
					sounds[id].sound.sustain_end = end * sounds[id].sound.channels;
				}

				if(num_loops > 1)
					file.skip((num_loops-1) * sizeof(loop));

				file.skip(skip);
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
