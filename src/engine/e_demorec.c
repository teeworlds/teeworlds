
#include <base/system.h>
#include "e_demorec.h"
#include "e_memheap.h"
#include "e_snapshot.h"
#include "e_compression.h"
#include "e_network.h"
#include "e_if_other.h"

static IOHANDLE record_file = 0;
static const unsigned char header_marker[8] = {'T', 'W', 'D', 'E', 'M', 'O', 0, 1};

/* Record */
static int record_lasttickmarker = -1;
static int record_lastkeyframe;
static unsigned char record_lastsnapshotdata[MAX_SNAPSHOT_SIZE];

int demorec_isrecording() { return record_file != 0; }

int demorec_record_start(const char *filename, const char *netversion, const char *map, int crc, const char *type)
{
	DEMOREC_HEADER header;
	if(record_file)
		return -1;

	record_file = io_open(filename, IOFLAG_WRITE);
	
	if(!record_file)
	{
		dbg_msg("demorec/record", "Unable to open '%s' for recording", filename);
		return -1;
	}
	
	/* write header */
	mem_zero(&header, sizeof(header));
	mem_copy(header.marker, header_marker, sizeof(header.marker));
	str_copy(header.netversion, netversion, sizeof(header.netversion));
	str_copy(header.map, map, sizeof(header.map));
	str_copy(header.type, type, sizeof(header.type));
	header.crc[0] = (crc>>24)&0xff;
	header.crc[1] = (crc>>16)&0xff;
	header.crc[2] = (crc>>8)&0xff;
	header.crc[3] = (crc)&0xff;
	io_write(record_file, &header, sizeof(header));
	
	record_lastkeyframe = -1;
	record_lasttickmarker = -1;
	
	dbg_msg("demorec/record", "Recording to '%s'", filename);
	return 0;
}

/*
	Tickmarker
		7   = Always set
		6   = Keyframe flag
		0-5 = Delta tick
	
	Normal
		7   = Not set
		5-6 = Type
		0-4 = Size
*/

enum
{
	CHUNKTYPEFLAG_TICKMARKER = 0x80,
	CHUNKTICKFLAG_KEYFRAME = 0x40, /* only when tickmarker is set*/
	
	CHUNKMASK_TICK = 0x3f,
	CHUNKMASK_TYPE = 0x60,
	CHUNKMASK_SIZE = 0x1f,
	
	CHUNKTYPE_SNAPSHOT = 1,
	CHUNKTYPE_MESSAGE = 2,
	CHUNKTYPE_DELTA = 3,

	CHUNKFLAG_BIGSIZE = 0x10
};

static void demorec_record_write_tickmarker(int tick, int keyframe)
{
	if(record_lasttickmarker == -1 || tick-record_lasttickmarker > 63 || keyframe)
	{
		unsigned char chunk[5];
		chunk[0] = CHUNKTYPEFLAG_TICKMARKER;
		chunk[1] = (tick>>24)&0xff;
		chunk[2] = (tick>>16)&0xff;
		chunk[3] = (tick>>8)&0xff;
		chunk[4] = (tick)&0xff;

		if(keyframe)
			chunk[0] |= CHUNKTICKFLAG_KEYFRAME;
		
		io_write(record_file, chunk, sizeof(chunk));
	}
	else
	{
		unsigned char chunk[1];
		chunk[0] = CHUNKTYPEFLAG_TICKMARKER | (tick-record_lasttickmarker);
		io_write(record_file, chunk, sizeof(chunk));
	}	

	record_lasttickmarker = tick;
}

static void demorec_record_write(int type, const void *data, int size)
{
	char buffer[64*1024];
	char buffer2[64*1024];
	unsigned char chunk[3];
	
	if(!record_file)
		return;
		
	size = intpack_compress(data, size, buffer);
	size = netcommon_compress(buffer, size, buffer2, sizeof(buffer2));
	
	
	chunk[0] = ((type&0x3)<<5);
	if(size < 30)
	{
		chunk[0] |= size;
		io_write(record_file, chunk, 1);
	}
	else
	{
		if(size < 256)
		{
			chunk[0] |= 30;
			chunk[1] = size&0xff;
			io_write(record_file, chunk, 2);
		}
		else
		{
			chunk[0] |= 31;
			chunk[1] = size&0xff;
			chunk[2] = size>>8;
			io_write(record_file, chunk, 3);
		}
	}
	
	io_write(record_file, buffer2, size);
}

void demorec_record_snapshot(int tick, const void *data, int size)
{
	record_emptydeltatick = -1;
	
	if(record_lastkeyframe == -1 || (tick-record_lastkeyframe) > SERVER_TICK_SPEED*5)
	{
		/* write full tickmarker */
		demorec_record_write_tickmarker(tick, 1);
		
		/* write snapshot */
		demorec_record_write(CHUNKTYPE_SNAPSHOT, data, size);
			
		record_lastkeyframe = tick;
		mem_copy(record_lastsnapshotdata, data, size);
	}
	else
	{
		/* create delta, prepend tick */
		char delta_data[MAX_SNAPSHOT_SIZE+sizeof(int)];
		int delta_size;

		/* write tickmarker */
		demorec_record_write_tickmarker(tick, 0);
		
		delta_size = snapshot_create_delta((SNAPSHOT*)record_lastsnapshotdata, (SNAPSHOT*)data, &delta_data);
		if(delta_size)
		{
			/* record delta */
			demorec_record_write(CHUNKTYPE_DELTA, delta_data, delta_size);
			mem_copy(record_lastsnapshotdata, data, size);
		}
	}
}

void demorec_record_message(const void *data, int size)
{
	demorec_record_write(CHUNKTYPE_MESSAGE, data, size);
}

int demorec_record_stop()
{
	if(!record_file)
		return -1;
		
	dbg_msg("demorec/record", "Stopped recording");
	io_close(record_file);
	record_file = 0;
	return 0;
}

/* Playback */
typedef struct KEYFRAME
{
	long filepos;
	int tick;
} KEYFRAME;

typedef struct KEYFRAME_SEARCH
{
	KEYFRAME frame;
	struct KEYFRAME_SEARCH *next;
} KEYFRAME_SEARCH;

static IOHANDLE play_file = 0;
static DEMOREC_PLAYCALLBACK play_callback_snapshot = 0;
static DEMOREC_PLAYCALLBACK play_callback_message = 0;
static KEYFRAME *keyframes = 0;

static DEMOREC_PLAYBACKINFO playbackinfo;
static unsigned char playback_lastsnapshotdata[MAX_SNAPSHOT_SIZE];
static int playback_lastsnapshotdata_size = -1;


const DEMOREC_PLAYBACKINFO *demorec_playback_info() { return &playbackinfo; }
int demorec_isplaying() { return play_file != 0; }

int demorec_playback_registercallbacks(DEMOREC_PLAYCALLBACK snapshot_cb, DEMOREC_PLAYCALLBACK message_cb)
{
	play_callback_snapshot = snapshot_cb;
	play_callback_message = message_cb;
	return 0;
}

static int read_chunk_header(int *type, int *size, int *tick)
{
	unsigned char chunk = 0;
	
	*size = 0;
	*type = 0;
	
	if(io_read(play_file, &chunk, sizeof(chunk)) != sizeof(chunk))
		return -1;
		
	if(chunk&CHUNKTYPEFLAG_TICKMARKER)
	{
		/* decode tick marker */
		int tickdelta = chunk&(CHUNKMASK_TICK);
		*type = chunk&(CHUNKTYPEFLAG_TICKMARKER|CHUNKTICKFLAG_KEYFRAME);
		
		if(tickdelta == 0)
		{
			unsigned char tickdata[4];
			if(io_read(play_file, tickdata, sizeof(tickdata)) != sizeof(tickdata))
				return -1;
			*tick = (tickdata[0]<<24) | (tickdata[1]<<16) | (tickdata[2]<<8) | tickdata[3];
		}
		else
		{
			*tick += tickdelta;
		}
		
	}
	else
	{
		/* decode normal chunk */
		*type = (chunk&CHUNKMASK_TYPE)>>5;
		*size = chunk&CHUNKMASK_SIZE;
		
		if(*size == 30)
		{
			unsigned char sizedata[1];
			if(io_read(play_file, sizedata, sizeof(sizedata)) != sizeof(sizedata))
				return -1;
			*size = sizedata[0];
			
		}
		else if(*size == 31)
		{
			unsigned char sizedata[2];
			if(io_read(play_file, sizedata, sizeof(sizedata)) != sizeof(sizedata))
				return -1;
			*size = (sizedata[1]<<8) | sizedata[0];
		}
	}
	
	return 0;
}

static void scan_file()
{
	long start_pos;
	HEAP *heap = 0;
	KEYFRAME_SEARCH *first_key = 0;
	KEYFRAME_SEARCH *current_key = 0;
	/*DEMOREC_CHUNK chunk;*/
	int chunk_size, chunk_type, chunk_tick = 0;
	int i;
	
	heap = memheap_create();

	start_pos = io_tell(play_file);
	playbackinfo.seekable_points = 0;

	while(1)
	{
		long current_pos = io_tell(play_file);
		
		if(read_chunk_header(&chunk_type, &chunk_size, &chunk_tick))
			break;
		
		/* read the chunk */
		if(chunk_type&CHUNKTYPEFLAG_TICKMARKER)
		{
			if(chunk_type&CHUNKTICKFLAG_KEYFRAME)
			{
				KEYFRAME_SEARCH *key;
				
				/* save the position */
				key = memheap_allocate(heap, sizeof(KEYFRAME_SEARCH));
				key->frame.filepos = current_pos;
				key->frame.tick = chunk_tick;
				key->next = 0;
				if(current_key)
					current_key->next = key;
				if(!first_key)
					first_key = key;
				current_key = key;
				playbackinfo.seekable_points++;
			}
			
			if(playbackinfo.first_tick == -1)
				playbackinfo.first_tick = chunk_tick;
			playbackinfo.last_tick = chunk_tick;
			dbg_msg("", "%x %d", chunk_type, chunk_tick);
		}
		else if(chunk_size)
			io_skip(play_file, chunk_size);
			
	}

	/* copy all the frames to an array instead for fast access */
	keyframes = (KEYFRAME*)mem_alloc(playbackinfo.seekable_points*sizeof(KEYFRAME), 1);
	for(current_key = first_key, i = 0; current_key; current_key = current_key->next, i++)
		keyframes[i] = current_key->frame;
		
	/* destroy the temporary heap and seek back to the start */
	memheap_destroy(heap);
	io_seek(play_file, start_pos, IOSEEK_START);
}

static void do_tick()
{
	static char compresseddata[MAX_SNAPSHOT_SIZE];
	static char decompressed[MAX_SNAPSHOT_SIZE];
	static char data[MAX_SNAPSHOT_SIZE];
	int chunk_size, chunk_type, chunk_tick;
	int data_size;
	int got_snapshot = 0;

	/* update ticks */
	playbackinfo.previous_tick = playbackinfo.current_tick;
	playbackinfo.current_tick = playbackinfo.next_tick;
	chunk_tick = playbackinfo.current_tick;

	while(1)
	{
		if(read_chunk_header(&chunk_type, &chunk_size, &chunk_tick))
		{
			/* stop on error or eof */
			dbg_msg("demorec", "end of file");
			demorec_playback_stop();
			break;
		}
		
		/* read the chunk */
		if(chunk_size)
		{
			if(io_read(play_file, compresseddata, chunk_size) != chunk_size)
			{
				/* stop on error or eof */
				dbg_msg("demorec", "error reading chunk");
				demorec_playback_stop();
				break;
			}
			
			data_size = netcommon_decompress(compresseddata, chunk_size, decompressed, sizeof(decompressed));
			if(data_size < 0)
			{
				/* stop on error or eof */
				dbg_msg("demorec", "error during network decompression");
				demorec_playback_stop();
				break;
			}
			
			data_size = intpack_decompress(decompressed, data_size, data);

			if(data_size < 0)
			{
				dbg_msg("demorec", "error during intpack decompression");
				demorec_playback_stop();
				break;
			}
		}
			
		if(chunk_type == CHUNKTYPE_DELTA)
		{
			/* process delta snapshot */
			static char newsnap[MAX_SNAPSHOT_SIZE];
			
			got_snapshot = 1;
			
			data_size = snapshot_unpack_delta((SNAPSHOT*)playback_lastsnapshotdata, (SNAPSHOT*)newsnap, data, data_size);
			
			if(data_size >= 0)
			{
				if(play_callback_snapshot)
					play_callback_snapshot(newsnap, data_size);

				playback_lastsnapshotdata_size = data_size;
				mem_copy(playback_lastsnapshotdata, newsnap, data_size);
			}
			else
				dbg_msg("demorec", "error duing unpacking of delta, err=%d", data_size);
		}
		else if(chunk_type == CHUNKTYPE_SNAPSHOT)
		{
			/* process full snapshot */
			got_snapshot = 1;
			
			playback_lastsnapshotdata_size = data_size;
			mem_copy(playback_lastsnapshotdata, data, data_size);
			if(play_callback_snapshot)
				play_callback_snapshot(data, data_size);
		}
		else
		{
			/* if there were no snapshots in this tick, replay the last one */
			if(!got_snapshot && play_callback_snapshot && playback_lastsnapshotdata_size != -1)
			{
				got_snapshot = 1;
				play_callback_snapshot(playback_lastsnapshotdata, playback_lastsnapshotdata_size);
			}
			
			/* check the remaining types */
			if(chunk_type&CHUNKTYPEFLAG_TICKMARKER)
			{
				playbackinfo.next_tick = chunk_tick;
				break;
			}
			else if(chunk_type == CHUNKTYPE_MESSAGE)
			{
				if(play_callback_message)
					play_callback_message(data, data_size);
			}
		}
	}
}

void demorec_playback_pause()
{
	playbackinfo.paused = 1;
}

void demorec_playback_unpause()
{
	if(playbackinfo.paused)
	{
		/*playbackinfo.start_tick = playbackinfo.current_tick;
		playbackinfo.start_time = time_get();*/
		playbackinfo.paused = 0;
	}
}

int demorec_playback_load(const char *filename)
{
	play_file = io_open(filename, IOFLAG_READ);
	if(!play_file)
	{
		dbg_msg("demorec/playback", "could not open '%s'", filename);
		return -1;
	}
	
	/* clear the playback info */
	mem_zero(&playbackinfo, sizeof(playbackinfo));
	playbackinfo.first_tick = -1;
	playbackinfo.last_tick = -1;
	/*playbackinfo.start_tick = -1;*/
	playbackinfo.next_tick = -1;
	playbackinfo.current_tick = -1;
	playbackinfo.previous_tick = -1;
	playbackinfo.speed = 1;
	
	playback_lastsnapshotdata_size = -1;

	/* read the header */	
	io_read(play_file, &playbackinfo.header, sizeof(playbackinfo.header));
	if(mem_comp(playbackinfo.header.marker, header_marker, sizeof(header_marker)) != 0)
	{
		dbg_msg("demorec/playback", "'%s' is not a demo file", filename);
		io_close(play_file);
		play_file = 0;
		return -1;
	}
	
	/* scan the file for interessting points */
	scan_file();
	
	/* ready for playback */	
	return 0;
}

int demorec_playback_nextframe()
{
	do_tick();
	return demorec_isplaying();
}

int demorec_playback_play()
{
	/* fill in previous and next tick */
	while(playbackinfo.previous_tick == -1)
		do_tick();

	/* set start info */
	/*playbackinfo.start_tick = playbackinfo.previous_tick;
	playbackinfo.start_time = time_get();*/
	playbackinfo.current_time = playbackinfo.previous_tick*time_freq()/SERVER_TICK_SPEED;
	playbackinfo.last_update = time_get();
	return 0;
}

int demorec_playback_set(int keyframe)
{
	if(!play_file)
		return -1;
	if(keyframe < 0 || keyframe >= playbackinfo.seekable_points)
		return -1;
	
	io_seek(play_file, keyframes[keyframe].filepos, IOSEEK_START);

	/*playbackinfo.start_tick = -1;*/
	playbackinfo.next_tick = -1;
	playbackinfo.current_tick = -1;
	playbackinfo.previous_tick = -1;
	
	demorec_playback_play();
	
	return 0;
}

void demorec_playback_setspeed(float speed)
{
	playbackinfo.speed = speed;
}

int demorec_playback_update()
{
	int64 now = time_get();
	int64 deltatime = now-playbackinfo.last_update;
	playbackinfo.last_update = now;
	
	if(playbackinfo.paused)
	{
		
	}
	else
	{
		int64 freq = time_freq();
		playbackinfo.current_time += (int64)(deltatime*(double)playbackinfo.speed);
		
		while(1)
		{
			int64 curtick_start = (playbackinfo.current_tick)*freq/SERVER_TICK_SPEED;

			/* break if we are ready */		
			if(curtick_start > playbackinfo.current_time)
				break;
			
			/* do one more tick */
			do_tick();
		}

		/* update intratick */
		{	
			int64 curtick_start = (playbackinfo.current_tick)*freq/SERVER_TICK_SPEED;
			int64 prevtick_start = (playbackinfo.previous_tick)*freq/SERVER_TICK_SPEED;
			playbackinfo.intratick = (playbackinfo.current_time - prevtick_start) / (float)(curtick_start-prevtick_start);
			playbackinfo.ticktime = (playbackinfo.current_time - prevtick_start) / (float)freq;
		}
		
		if(playbackinfo.current_tick == playbackinfo.previous_tick ||
			playbackinfo.current_tick == playbackinfo.next_tick)
		{
			dbg_msg("demorec/playback", "tick error prev=%d cur=%d next=%d",
				playbackinfo.previous_tick, playbackinfo.current_tick, playbackinfo.next_tick);
		}
	}
	
	return 0;
}

int demorec_playback_stop(const char *filename)
{
	if(!play_file)
		return -1;
		
	dbg_msg("demorec/playback", "Stopped playback");
	io_close(play_file);
	play_file = 0;
	mem_free(keyframes);
	keyframes = 0;
	return 0;
}
