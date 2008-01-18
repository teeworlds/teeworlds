/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
	Title: Engine Interface
*/

#include "e_keys.h"
#include "e_system.h"

enum 
{
	MAX_CLIENTS=12,
	SERVER_TICK_SPEED=50, /* TODO: this should be removed */
	SNAP_CURRENT=0,
	SNAP_PREV=1,
	
	IMG_AUTO=-1,
	IMG_RGB=0,
	IMG_RGBA=1,
	IMG_ALPHA=2,
	
	MASK_NONE=0,
	MASK_SET,
	MASK_ZERO,

	SNDFLAG_LOOP=1,
	SNDFLAG_POS=2,
	SNDFLAG_ALL=3,
	
	CLIENTSTATE_OFFLINE=0,
	CLIENTSTATE_CONNECTING,
	CLIENTSTATE_LOADING,
	CLIENTSTATE_ONLINE,
	CLIENTSTATE_QUITING,
	
	BROWSESORT_NAME = 0,
	BROWSESORT_PING,
	BROWSESORT_MAP,
	BROWSESORT_GAMETYPE,
	BROWSESORT_PROGRESSION,
	BROWSESORT_NUMPLAYERS
};

typedef struct
{
	int type;
	int id;
} SNAP_ITEM;

typedef struct
{
	const char *name;
	int latency;
} CLIENT_INFO;

typedef struct
{
	int width, height;
	int format;
	void *data;
} IMAGE_INFO;

typedef struct 
{
	int width, height;
	int red, green, blue;
} VIDEO_MODE;

typedef struct
{
	int sorted_index;
	int server_index;
	
	int progression;
	int game_type;
	int max_players;
	int num_players;
	int flags;
	int latency; /* in ms */
	char name[64];
	char map[32];
	char version[32];
	char address[24];
	char player_names[16][48];
	int player_scores[16];
} SERVER_INFO;

typedef struct PERFORMACE_INFO_t
{
	const char *name;
	struct PERFORMACE_INFO_t *parent;
	struct PERFORMACE_INFO_t *first_child;
	struct PERFORMACE_INFO_t *next_child;
	int tick;
	int64 start;
	int64 total;
	int64 biggest;
	int64 last_delta;
} PERFORMACE_INFO;

void perf_init();
void perf_next();
void perf_start(PERFORMACE_INFO *info);
void perf_end();
void perf_dump();

int gfx_init();
void gfx_shutdown();
void gfx_swap();
int gfx_window_active();
int gfx_window_open();

void gfx_set_vsync(int val);
void gfx_mask_op(int mask, int write);
void gfx_clear_mask(int fill);

int snd_init();
int snd_shutdown();
int snd_update();

int map_load(const char *mapname);
void map_unload();

void map_set(void *m);


/*
	Group: Graphics
*/


/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float gfx_screenaspect();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int gfx_get_video_modes(VIDEO_MODE *list, int maxcount);

/* image loaders */

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int gfx_load_png(IMAGE_INFO *img, const char *filename);

/* textures */
/*
	Function: gfx_load_texture
		Loads a texture from a file. TGA and PNG supported.
	
	Arguments:
		filename - Null terminated string to the file to load.
		store_format - What format to store on gfx card as.
	
	Returns:
		An ID to the texture. -1 on failure.

	See Also:
		<gfx_unload_texture>
*/
int gfx_load_texture(const char *filename, int store_format);

/*
	Function: gfx_load_texture_raw
		Loads a texture from memory.
	
	Arguments:
		w - Width of the texture.
		h - Height of the texture.
		data - Pointer to the pixel data.
		format - Format of the pixel data.
		store_format - The format to store the texture on the graphics card.
	
	Returns:
		An ID to the texture. -1 on failure.
		
	Remarks:
		The pixel data should be in RGBA format with 8 bit per component.
		So the total size of the data should be w*h*4.
		
	See Also:
		<gfx_unload_texture>
*/
int gfx_load_texture_raw(int w, int h, int format, const void *data, int store_format);

/*
	Function: gfx_texture_set
		Sets the active texture.
	
	Arguments:
		id - ID to the texture to set.
*/
void gfx_texture_set(int id);

/*
	Function: gfx_unload_texture
		Unloads a texture.
	
	Arguments:
		id - ID to the texture to unload.
		
	See Also:
		<gfx_load_texture_tga>, <gfx_load_texture_raw>
		
	Remarks:
		NOT IMPLEMENTED
*/
int gfx_unload_texture(int id);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_clear(float r, float g, float b);

/*
	Function: gfx_screenwidth
		Returns the screen width.
	
	See Also:
		<gfx_screenheight>
*/
int gfx_screenwidth();

/*
	Function: gfx_screenheight
		Returns the screen height.
	
	See Also:
		<gfx_screenwidth>
*/
int gfx_screenheight();

/*
	Function: gfx_mapscreen
		Specifies the coordinate system for the screen.
		
	Arguments:
		tl_x - Top-left X
		tl_y - Top-left Y
		br_x - Bottom-right X
		br_y - Bottom-right y
*/
void gfx_mapscreen(float tl_x, float tl_y, float br_x, float br_y);

/*
	Function: gfx_blend_normal
		Set the active blending mode to normal (src, 1-src).

	Remarks:
		This must be used before calling <gfx_quads_begin>.
		This is equal to glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).
	
	See Also:
		<gfx_blend_additive>
*/
void gfx_blend_normal();

/*
	Function: gfx_blend_additive
		Set the active blending mode to additive (src, one).

	Remarks:
		This must be used before calling <gfx_quads_begin>.
		This is equal to glBlendFunc(GL_SRC_ALPHA, GL_ONE).
	
	See Also:
		<gfx_blend_normal>
*/
void gfx_blend_additive();

/*
	Function: gfx_quads_begin
		Begins a quad drawing session.
		
	Remarks:
		This functions resets the rotation, color and subset.
		End the session by using <gfx_quads_end>.
		You can't change texture or blending mode during a session.
		
	See Also:
		<gfx_quads_end>
*/
void gfx_quads_begin();

/*
	Function: gfx_quads_end
		Ends a quad session.
		
	See Also:
		<gfx_quads_begin>
*/
void gfx_quads_end();

/*
	Function: gfx_quads_setrotation
		Sets the rotation to use when drawing a quad.
		
	Arguments:
		angle - Angle in radians.
		
	Remarks:
		The angle is reset when <gfx_quads_begin> is called.
*/
void gfx_quads_setrotation(float angle);

/*
	Function: gfx_quads_setcolorvertex
		Sets the color of a vertex.
		
	Arguments:
		i - Index to the vertex.
		r - Red value.
		g - Green value.
		b - Blue value.
		a - Alpha value.
		
	Remarks:
		The color values are from 0.0 to 1.0.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_setcolorvertex(int i, float r, float g, float b, float a);

/*
	Function: gfx_quads_setcolor
		Sets the color of all the vertices.
		
	Arguments:
		r - Red value.
		g - Green value.
		b - Blue value.
		a - Alpha value.
		
	Remarks:
		The color values are from 0.0 to 1.0.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_setcolor(float r, float g, float b, float a);

/*
	Function: gfx_quads_setsubset
		Sets the uv coordinates to use.
		
	Arguments:
		tl_u - Top-left U value.
		tl_v - Top-left V value.
		br_u - Bottom-right U value.
		br_v - Bottom-right V value.
		
	Remarks:
		O,0 is top-left of the texture and 1,1 is bottom-right.
		The color is reset when <gfx_quads_begin> is called.
*/
void gfx_quads_setsubset(float tl_u, float tl_v, float br_u, float br_v);

/*
	Function: gfx_quads_drawTL
		Draws a quad by specifying the top-left point.
		
	Arguments:
		x - X coordinate of the top-left corner.
		y - Y coordinate of the top-left corner.
		width - Width of the quad.
		height - Height of the quad.
		
	Remarks:
		Rotation still occurs from the center of the quad.
		You must call <gfx_quads_begin> before calling this function.

	See Also:
		<gfx_quads_draw>
*/
void gfx_quads_drawTL(float x, float y, float width, float height);

/*
	Function: gfx_quads_draw
		Draws a quad by specifying the center point.
		
	Arguments:
		x - X coordinate of the center.
		y - Y coordinate of the center.
		width - Width of the quad.
		height - Height of the quad.

	Remarks:
		You must call <gfx_quads_begin> before calling this function.

	See Also:
		<gfx_quads_drawTL>
*/
void gfx_quads_draw(float x, float y, float w, float h);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_quads_draw_freeform(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_quads_text(float x, float y, float size, const char *text);

/* sound (client) */

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void snd_set_channel(int cid, float vol, float pan);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int snd_load_wv(const char *filename);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int snd_play_at(int cid, int sid, int flags, float x, float y);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int snd_play(int cid, int sid, int flags);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void snd_stop(int id);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void snd_set_listener_pos(float x, float y);

/*
	Group: Input
*/

/*
	Function: inp_mouse_relative
		Fetches the mouse movements.
		
	Arguments:
		x - Pointer to the variable that should get the X movement.
		y - Pointer to the variable that should get the Y movement.
*/
void inp_mouse_relative(int *x, int *y);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_mouse_scroll();

/*
	Function: inp_key_pressed
		Checks if a key is pressed.
		
	Arguments:
		key - Index to the key to check
		
	Returns:
		Returns 1 if the button is pressed, otherwise 0.
	
	Remarks:
		Check keys.h for the keys.
*/
int inp_key_pressed(int key);

/*
	Group: Map
*/

/*
	Function: map_is_loaded
		Checks if a map is loaded.
		
	Returns:
		Returns 1 if the button is pressed, otherwise 0.
*/
int map_is_loaded();

/*
	Function: map_num_items
		Checks the number of items in the loaded map.
		
	Returns:
		Returns the number of items. 0 if no map is loaded.
*/
int map_num_items();

/*
	Function: map_find_item
		Searches the map for an item.
	
	Arguments:
		type - Item type.
		id - Item ID.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise it returns NULL.
*/
void *map_find_item(int type, int id);

/*
	Function: map_get_item
		Gets an item from the loaded map from index.
	
	Arguments:
		index - Item index.
		type - Pointer that recives the item type (can be NULL).
		id - Pointer that recives the item id (can be NULL).
	
	Returns:
		Returns a pointer to the item if it exists, otherwise it returns NULL.
*/
void *map_get_item(int index, int *type, int *id);

/*
	Function: map_get_type
		Gets the index range of an item type.
	
	Arguments:
		type - Item type to search for.
		start - Pointer that recives the starting index.
		num - Pointer that recives the number of items.
	
	Returns:
		If the item type is not in the map, start and num will be set to 0.
*/
void map_get_type(int type, int *start, int *num);

/*
	Function: map_get_data
		Fetches a pointer to a raw data chunk in the map.
	
	Arguments:
		index - Index to the data to fetch.
	
	Returns:
		A pointer to the raw data, otherwise 0.
*/
void *map_get_data(int index);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void *map_get_data_swapped(int index);

/*
	Group: Network (Server)
*/
/*
	Function: snap_new_item
		Creates a new item that should be sent.
	
	Arguments:
		type - Type of the item.
		id - ID of the item.
		size - Size of the item.
	
	Returns:
		A pointer to the item data, otherwise 0.
	
	Remarks:
		The item data should only consist pf 4 byte integers as
		they are subject to byte swapping. This means that the size
		argument should be dividable by 4.
*/
void *snap_new_item(int type, int id, int size);

/*
	Group: Network (Client)
*/
/*
	Function: snap_num_items
		Check the number of items in a snapshot.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
	
	Returns:
		The number of items in the snapshot.
*/
int snap_num_items(int snapid);

/*
	Function: snap_get_item
		Gets an item from a snapshot.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
		index - Index of the item.
		item - Pointer that recives the item info.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise NULL.
*/
const void *snap_get_item(int snapid, int index, SNAP_ITEM *item);

/*
	Function: snap_find_item
		Searches a snapshot for an item.
	
	Arguments:
		snapid - Snapshot ID to the data to fetch.
			* SNAP_PREV for previous snapshot.
			* SNAP_CUR for current snapshot.
		type - Type of the item.
		id - ID of the item.
	
	Returns:
		Returns a pointer to the item if it exists, otherwise NULL.
*/
const void *snap_find_item(int snapid, int type, int id);

/*
	Function: snap_input
		Sets the input data to send to the server.
	
	Arguments:
		data - Pointer to the data.
		size - Size of the data.

	Remarks:
		The data should only consist of 4 bytes integer as they are
		subject to byte swapping.
*/
void snap_input(void *data, int size);

/*
	Group: Server Callbacks
*/
/*
	Function: mods_init
		Called when the server is started.
	
	Remarks:
		It's called after the map is loaded so all map items are available.
*/
void mods_init();

/*
	Function: mods_shutdown
		Called when the server quits.
	
	Remarks:
		Should be used to clean up all resources used.
*/
void mods_shutdown();

/*
	Function: mods_client_enter
		Called when a client has joined the game.
		
	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
	
	Remarks:
		It's called when the client is finished loading and should enter gameplay.
*/
void mods_client_enter(int cid);

/*
	Function: mods_client_drop
		Called when a client drops from the server.

	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS
*/
void mods_client_drop(int cid);

/*
	Function: mods_client_input
		Called when the server recives new input from a client.

	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
		input - Pointer to the input data.
		size - Size of the data. (NOT IMPLEMENTED YET)
*/
void mods_client_input(int cid, void *input);

/*
	Function: mods_tick
		Called with a regular interval to progress the gameplay.
		
	Remarks:
		The SERVER_TICK_SPEED tells the number of ticks per second.
*/
void mods_tick();

/*
	Function: mods_presnap
		Called before the server starts to construct snapshots for the clients.
*/
void mods_presnap();

/*
	Function: mods_snap
		Called to create the snapshot for a client.
	
	Arguments:
		cid - Client ID. Is 0 - MAX_CLIENTS.
	
	Remarks:
		The game should make a series of calls to <snap_new_item> to construct
		the snapshot for the client.
*/
void mods_snap(int cid);

/*
	Function: mods_postsnap
		Called after the server is done sending the snapshots.
*/
void mods_postsnap();

/*
	Group: Client Callbacks
*/

/*
	Function: modc_preinit
		Called when the client starts, but before the engine is initialized.
	
	Remarks:
*/
void modc_preinit();

/*
	Function: modc_init
		Called when the client starts.
	
	Remarks:
		The game should load resources that are used during the entire
		time of the game. No map is loaded.
*/
void modc_init();

/*
	Function: modc_newsnapshot
		Called when the client progressed to a new snapshot.
	
	Remarks:
		The client can check for items in the snapshot and perform one time
		events like playing sounds, spawning client side effects etc.
*/
void modc_newsnapshot();

/*
	Function: modc_entergame
		Called when the client has successfully connect to a server and
		loaded a map.
	
	Remarks:
		The client can check for items in the map and load them.
*/
void modc_entergame();

/*
	Function: modc_shutdown
		Called when the client closes down.
*/
void modc_shutdown();

/*
	Function: modc_render
		Called every frame to let the game render it self.
*/
void modc_render();

/*
	Function: modc_statechange
		Called every time client changes state.
*/
void modc_statechange(int new_state, int old_state);

/* undocumented callbacks */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_connected();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_message(int msg);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void modc_predict();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int modc_snap_input(int *data);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void mods_message(int msg, int client_id);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void mods_connected(int client_id);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *modc_net_version();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *mods_net_version();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *mods_version();

/* server */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_getclientinfo(int client_id, CLIENT_INFO *info);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *server_clientname(int client_id);

/* grabs the latest input for the client. not withholding anything */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int *server_latestinput(int client_id, int *size);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setclientname(int client_id, const char *name);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setclientscore(int client_id, int score);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_setbrowseinfo(int game_type, int progression);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void server_kick(int client_id, const char *reason);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_tick();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_tickspeed();

/* input */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_was_pressed(int key);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_down(int key);



typedef struct
{
	char ch;
	int key;
} INPUTEVENT;

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_num_events();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
INPUTEVENT inp_get_event(int index);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void inp_clear_events();

void inp_update();
void inp_init();
void inp_mouse_mode_absolute();
void inp_mouse_mode_relative();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_mouse_doubleclick();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void inp_clear_key_states();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_presses(int key);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_releases(int key);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_state(int key);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *inp_key_name(int k);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int inp_key_code(const char *key_name);

/* message packing */
enum
{
	MSGFLAG_VITAL=1
};

void msg_pack_start_system(int msg, int flags);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_start(int msg, int flags);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_int(int i);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_string(const char *p, int limit);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_raw(const void *data, int size);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void msg_pack_end();

typedef struct
{
	int msg;
	int flags;
	const unsigned char *data;
	int size;
} MSG_INFO;

const MSG_INFO *msg_get_info();

/* message unpacking */
int msg_unpack_start(const void *data, int data_size, int *system);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int msg_unpack_int();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *msg_unpack_string();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const unsigned char *msg_unpack_raw(int size);

/* message sending */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int server_send_msg(int client_id); /* client_id == -1 == broadcast */

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_send_msg();

/* client */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_tick();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_predtick();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_intratick(); 

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_ticktime();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_intrapredtick();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_tickspeed();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_frametime();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float client_localtime();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_direct_input(int *input, int size);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_state();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
const char *client_error_string();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int *client_get_input(int tick);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_connection_problems();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_connect(const char *address);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_disconnect();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_quit();

void client_entergame();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_rcon(const char *cmd);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_serverbrowse_refresh(int lan);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
SERVER_INFO *client_serverbrowse_sorted_get(int index);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_sorted_num();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
SERVER_INFO *client_serverbrowse_get(int index);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_num();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int client_serverbrowse_num_requests();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void client_serverbrowse_update();

/* undocumented graphics stuff */

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_text(void *font, float x, float y, float size, const char *text, int max_width);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
float gfx_text_width(void *font, float size, const char *text, int length);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_text_color(float r, float g, float b, float a);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_text_set_default_font(void *font);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_getscreen(float *tl_x, float *tl_y, float *br_x, float *br_y);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int gfx_memory_usage();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_screenshot();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_begin();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_draw(float x0, float y0, float x1, float y1);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_lines_end();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_clip_enable(int x, int y, int w, int h);

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_clip_disable();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void gfx_quads_setsubset_free(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3);

/* server snap id */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
int snap_new_id();

/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void snap_free_id(int id);

/* other */
/*
	Function: TODO
	
	Arguments:
		arg1 - desc
	
	Returns:

	See Also:
		<other_func>
*/
void map_unload_data(int index);


#ifdef __cplusplus
}
#endif

#endif
