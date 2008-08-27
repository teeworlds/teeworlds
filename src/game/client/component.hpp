#ifndef GAME_CLIENT_GAMESYSTEM_H
#define GAME_CLIENT_GAMESYSTEM_H

#include <engine/e_client_interface.h>

class GAMECLIENT;

class COMPONENT
{
public:
	~COMPONENT() {}
	GAMECLIENT *client;
	
	virtual void on_statechange(int new_state, int old_state) {};
	virtual void on_init() {};
	virtual void on_reset() {};
	virtual void on_render() {};
	virtual void on_message(int msg, void *rawmsg) {}
	virtual bool on_mousemove(float x, float y) { return false; }
	virtual bool on_input(INPUT_EVENT e) { return false; }
};

#endif
