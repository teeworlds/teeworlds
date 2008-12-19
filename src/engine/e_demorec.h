/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef _DEMOREC_H
#define _DEMOREC_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct DEMOREC_HEADER
{
	char marker[8];
	char netversion[64];
	char map[64];
	unsigned char crc[4];
	char type[8];
} DEMOREC_HEADER;

typedef struct DEMOREC_CHUNK
{
	char type[2];
	unsigned short size;
} DEMOREC_CHUNK;

typedef struct DEMOREC_TICKMARKER
{
	int tick;
} DEMOREC_TICKMARKER;

typedef struct DEMOREC_PLAYBACKINFO
{
	DEMOREC_HEADER header;
	
	int paused;
	float speed;
	
	int64 last_update;
	int64 current_time;
	
	int first_tick;
	int last_tick;
	
	int seekable_points;
	
	int next_tick;
	int current_tick;
	int previous_tick;
	
	float intratick;
	float ticktime;
} DEMOREC_PLAYBACKINFO;

int demorec_record_start(const char *filename, const char *netversion, const char *map, int map_crc, const char *type);
int demorec_isrecording();
void demorec_record_snapshot(int tick, const void *data, int size);
void demorec_record_message(const void *data, int size);
int demorec_record_stop();

typedef void (*DEMOREC_PLAYCALLBACK)(void *data, int size);

int demorec_playback_registercallbacks(DEMOREC_PLAYCALLBACK snapshot_cb, DEMOREC_PLAYCALLBACK message_cb);
int demorec_playback_load(const char *filename);
int demorec_playback_nextframe();
int demorec_playback_play();
void demorec_playback_pause();
void demorec_playback_unpause();
void demorec_playback_setspeed(float speed);
int demorec_playback_set(float precent);
int demorec_playback_update();
const DEMOREC_PLAYBACKINFO *demorec_playback_info();
int demorec_isplaying();
int demorec_playback_stop();

#ifdef __cplusplus
}
#endif

#endif
