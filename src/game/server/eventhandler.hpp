#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

//
class EVENTHANDLER
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128*64;

	int types[MAX_EVENTS];  // TODO: remove some of these arrays
	int offsets[MAX_EVENTS];
	int sizes[MAX_EVENTS];
	int client_masks[MAX_EVENTS];
	char data[MAX_DATASIZE];
	
	int current_offset;
	int num_events;
public:
	EVENTHANDLER();
	void *create(int type, int size, int mask = -1);
	void clear();
	void snap(int snapping_client);
};

#endif
