
#include <base/system.h>
#include "e_demorec.h"
#include "e_memheap.h"
#include "e_if_other.h"

static IOHANDLE record_file = 0;
static const unsigned char header_marker[8] = {'T', 'W', 'D', 'E', 'M', 'O', 0, 1};

/* Record */

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
	
	dbg_msg("demorec/record", "Recording to '%s'", filename);
	return 0;
}

void demorec_record_write(const char *type, int size, const void *data)
{
	DEMOREC_CHUNK chunk;
	if(!record_file)
		return;
	
	chunk.type[0] = type[0];
	chunk.type[1] = type[1];
	chunk.type[2] = type[2];
	chunk.type[3] = type[3];
	chunk.size = size;
	swap_endian(&chunk.size, sizeof(int), 1);
	io_write(record_file, &chunk, sizeof(chunk));
	io_write(record_file, data, size);
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
static DEMOREC_PLAYCALLBACK play_callback = 0;
static KEYFRAME *keyframes = 0;

static DEMOREC_PLAYBACKINFO playbackinfo;
const DEMOREC_PLAYBACKINFO *demorec_playback_info() { return &playbackinfo; }

int demorec_isplaying() { return play_file != 0; }

int demorec_playback_registercallback(DEMOREC_PLAYCALLBACK cb)
{
	play_callback = cb;
	return 0;
}

static int read_chunk_header(DEMOREC_CHUNK *chunk)
{
	if(io_read(play_file, chunk, sizeof(*chunk)) != sizeof(*chunk))
		return -1;
	swap_endian(&chunk->size, sizeof(int), 1);
	return 0;
}

static void scan_file()
{
	long start_pos;
	HEAP *heap = 0;
	KEYFRAME_SEARCH *first_key = 0;
	KEYFRAME_SEARCH *current_key = 0;
	DEMOREC_CHUNK chunk;
	int i;
	
	heap = memheap_create();

	start_pos = io_tell(play_file);
	playbackinfo.seekable_points = 0;

	while(1)
	{
		long current_pos = io_tell(play_file);

		if(read_chunk_header(&chunk))
			break;
		
		/* read the chunk */
			
		if(mem_comp(chunk.type, "TICK", 4) == 0)
		{
			KEYFRAME_SEARCH *key;
			DEMOREC_TICKMARKER marker;
			io_read(play_file, &marker, chunk.size);
			swap_endian(&marker.tick, sizeof(int), 1);
			
			/* save the position */
			key = memheap_allocate(heap, sizeof(KEYFRAME_SEARCH));
			key->frame.filepos = current_pos;
			key->frame.tick = marker.tick;
			key->next = 0;
			if(current_key)
				current_key->next = key;
			if(!first_key)
				first_key = key;
			current_key = key;
			
			if(playbackinfo.first_tick == -1)
				playbackinfo.first_tick = marker.tick;
			playbackinfo.last_tick = marker.tick;
			playbackinfo.seekable_points++;
		}
		else
			io_skip(play_file, chunk.size);
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
	static char buffer[64*1024];
	DEMOREC_CHUNK chunk;

	/* update ticks */
	playbackinfo.previous_tick = playbackinfo.current_tick;
	playbackinfo.current_tick = playbackinfo.next_tick;

	while(1)
	{
		int r = read_chunk_header(&chunk);
		if(chunk.size > sizeof(buffer))
		{
			dbg_msg("demorec/playback", "chunk is too big %d", chunk.size);
			r = 1;
		}
		
		if(r)
		{
			/* stop on error or eof */
			demorec_playback_stop();
			break;
		}
		
		/* read the chunk */
		io_read(play_file, buffer, chunk.size);
			
		if(mem_comp(chunk.type, "TICK", 4) == 0)
		{
			DEMOREC_TICKMARKER marker = *(DEMOREC_TICKMARKER *)buffer;
			swap_endian(&marker.tick, sizeof(int), 1);
			playbackinfo.next_tick = marker.tick;
			break;
		}
		else if(play_callback)
			play_callback(chunk, buffer);
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
