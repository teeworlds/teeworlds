#ifndef GAME_CLIENT_GAMESYSTEM_H
#define GAME_CLIENT_GAMESYSTEM_H

#include <engine/e_client_interface.h>
#include "gameclient.hpp"

class GAMECLIENT;

class COMPONENT
{
protected:
	friend class GAMECLIENT;

	GAMECLIENT *client;
	
	// perhaps propagte pointers for these as well
	class IGraphics *Graphics() const { return client->Graphics(); }
	class CUI *UI() const { return client->UI(); }
	class CRenderTools *RenderTools() const { return client->RenderTools(); }
public:
	virtual ~COMPONENT() {}
	
	virtual void on_statechange(int new_state, int old_state) {};
	virtual void on_console_init() {};
	virtual void on_init() {};
	virtual void on_save() {};
	virtual void on_reset() {};
	virtual void on_render() {};
	virtual void on_mapload() {};
	virtual void on_message(int msg, void *rawmsg) {}
	virtual bool on_mousemove(float x, float y) { return false; }
	virtual bool on_input(INPUT_EVENT e) { return false; }
};

#endif
