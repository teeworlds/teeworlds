/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <base/system.h>
#include <base/math.hpp>

#include <stdlib.h>
#include <math.h>
#include "array.hpp"
#include "../mapitems.hpp"
#include "../client/render.hpp"

extern "C" {
	#include <engine/e_client_interface.h>
	#include <engine/e_datafile.h>
	#include <engine/e_config.h>
}

#include <game/client/ui.hpp>

typedef void (*INDEX_MODIFY_FUNC)(int *index);

// EDITOR SPECIFIC
template<typename T>
void swap(T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

enum
{
	MODE_LAYERS=0,
	MODE_IMAGES,
	
	DIALOG_NONE=0,
	DIALOG_FILE,
};

typedef struct
{
	POINT position;
	int type;
} ENTITY;

class ENVELOPE
{
public:
	int channels;
	array<ENVPOINT> points;
	char name[32];
	float bottom, top;
	
	ENVELOPE(int chan)
	{
		channels = chan;
		name[0] = 0;
		bottom = 0;
		top = 0;
	}
	
	static int sort_comp(const void *v0, const void *v1)
	{
		const ENVPOINT *p0 = (const ENVPOINT *)v0;
		const ENVPOINT *p1 = (const ENVPOINT *)v1;
		if(p0->time < p1->time)
			return -1;
		if(p0->time > p1->time)
			return 1;
		return 0;
	}
	
	void resort()
	{
		qsort(points.getptr(), points.len(), sizeof(ENVPOINT), sort_comp);
		find_top_bottom(0xf);
	}

	void find_top_bottom(int channelmask)
	{
		top = -1000000000.0f;
		bottom = 1000000000.0f;
		for(int i = 0; i < points.len(); i++)
		{
			for(int c = 0; c < channels; c++)
			{
				if(channelmask&(1<<c))
				{
					float v = fx2f(points[i].values[c]);
					if(v > top) top = v;
					if(v < bottom) bottom = v;
				}
			}
		}
	}
	
	int eval(float time, float *result)
	{
		render_eval_envelope(points.getptr(), points.len(), channels, time, result);
		return channels;
	}
	
	void add_point(int time, int v0, int v1=0, int v2=0, int v3=0)
	{
		ENVPOINT p;
		p.time = time;
		p.values[0] = v0;
		p.values[1] = v1;
		p.values[2] = v2;
		p.values[3] = v3;
		p.curvetype = CURVETYPE_LINEAR;
		points.add(p);
		resort();
	}
	
	float end_time()
	{
		if(points.len())
			return points[points.len()-1].time*(1.0f/1000.0f);
		return 0;
	}
};


class LAYER;
class LAYERGROUP;
class MAP;

class LAYER
{
public:
	LAYER()
	{
		type = LAYERTYPE_INVALID;
		type_name = "(invalid)";
		visible = true;
		readonly = false;
		flags = 0;
	}
	
	virtual ~LAYER()
	{
	}
	
	
	virtual void brush_selecting(RECT rect) {}
	virtual int brush_grab(LAYERGROUP *brush, RECT rect) { return 0; }
	virtual void brush_draw(LAYER *brush, float x, float y) {}
	virtual void brush_place(LAYER *brush, float x, float y) {}
	virtual void brush_flip_x() {}
	virtual void brush_flip_y() {}
	virtual void brush_rotate(float amount) {}
	
	virtual void render() {}
	virtual int render_properties(RECT *toolbox) { return 0; }
	
	virtual void modify_image_index(INDEX_MODIFY_FUNC func) {}
	virtual void modify_envelope_index(INDEX_MODIFY_FUNC func) {}
	
	virtual void get_size(float *w, float *h) { *w = 0; *h = 0;}
	
	const char *type_name;
	int type;
	int flags;

	bool readonly;
	bool visible;
};

class LAYERGROUP
{
public:
	array<LAYER*> layers;
	
	int offset_x;
	int offset_y;

	int parallax_x;
	int parallax_y;
	
	int use_clipping;
	int clip_x;
	int clip_y;
	int clip_w;
	int clip_h;
	
	const char *name;
	bool game_group;
	bool visible;
	
	LAYERGROUP();
	~LAYERGROUP();
	
	void convert(RECT *rect);
	void render();
	void mapscreen();
	void mapping(float *points);
	
	bool is_empty() const;
	void clear();
	void add_layer(LAYER *l);

	void get_size(float *w, float *h);
	
	void delete_layer(int index);
	int swap_layers(int index0, int index1);

	void modify_image_index(INDEX_MODIFY_FUNC func)
	{
		for(int i = 0; i < layers.len(); i++)
			layers[i]->modify_image_index(func);
	}
	
	void modify_envelope_index(INDEX_MODIFY_FUNC func)
	{
		for(int i = 0; i < layers.len(); i++)
			layers[i]->modify_envelope_index(func);
	}
};

class EDITOR_IMAGE : public IMAGE_INFO
{
public:
	EDITOR_IMAGE()
	{
		tex_id = -1;
		name[0] = 0;
		external = 0;
		width = 0;
		height = 0;
		data = 0;
		format = 0;
	}
	
	~EDITOR_IMAGE()
	{
		gfx_unload_texture(tex_id);
	}
	
	void analyse_tileflags();
	
	int tex_id;
	int external;
	char name[128];
	unsigned char tileflags[256];
};

class MAP
{
	void make_game_group(LAYERGROUP *group);
	void make_game_layer(LAYER *layer);
public:
	MAP()
	{
		clean();
	}

	array<LAYERGROUP*> groups;
	array<EDITOR_IMAGE*> images;
	array<ENVELOPE*> envelopes;
	
	class LAYER_GAME *game_layer;
	LAYERGROUP *game_group;
	
	ENVELOPE *new_envelope(int channels)
	{
		ENVELOPE *e = new ENVELOPE(channels);
		envelopes.add(e);
		return e;
	}
	
	LAYERGROUP *new_group()
	{
		LAYERGROUP *g = new LAYERGROUP;
		groups.add(g);
		return g;
	}
	
	int swap_groups(int index0, int index1)
	{
		if(index0 < 0 || index0 >= groups.len()) return index0;
		if(index1 < 0 || index1 >= groups.len()) return index0;
		if(index0 == index1) return index0;
		swap(groups[index0], groups[index1]);
		return index1;
	}
	
	void delete_group(int index)
	{
		if(index < 0 || index >= groups.len()) return;
		delete groups[index];
		groups.removebyindex(index);
	}
	
	void modify_image_index(INDEX_MODIFY_FUNC func)
	{
		for(int i = 0; i < groups.len(); i++)
			groups[i]->modify_image_index(func);
	}
	
	void modify_envelope_index(INDEX_MODIFY_FUNC func)
	{
		for(int i = 0; i < groups.len(); i++)
			groups[i]->modify_envelope_index(func);
	}
	
	void clean();
	void create_default(int entities_texture);

	// io	
	int save(const char *filename);
	int load(const char *filename);
};


struct PROPERTY
{
	const char *name;
	int value;
	int type;
	int min;
	int max;
};

enum
{
	PROPTYPE_NULL=0,
	PROPTYPE_BOOL,
	PROPTYPE_INT_STEP,
	PROPTYPE_INT_SCROLL,
	PROPTYPE_COLOR,
	PROPTYPE_IMAGE,
	PROPTYPE_ENVELOPE,
};

class EDITOR
{
public:	
	EDITOR()
	{
		mode = MODE_LAYERS;
		dialog = 0;
		tooltip = 0;

		world_offset_x = 0;
		world_offset_y = 0;
		editor_offset_x = 0.0f;
		editor_offset_y = 0.0f;
		
		world_zoom = 1.0f;
		zoom_level = 100;
		lock_mouse = false;
		show_mouse_pointer = true;
		mouse_delta_x = 0;
		mouse_delta_y = 0;
		mouse_delta_wx = 0;
		mouse_delta_wy = 0;
		
		gui_active = true;
		proof_borders = false;
		
		show_detail = true;
		animate = false;
		animate_start = 0;
		animate_time = 0;
		animate_speed = 1;
		
		show_envelope_editor = 0;
	}
	
	void invoke_file_dialog(int listdir_type, const char *title, const char *button_text,
		const char *basepath, const char *default_name,
		void (*func)(const char *filename));
	
	void reset(bool create_default=true);
	int save(const char *filename);
	int load(const char *filename);
	int append(const char *filename);
	void render();

	QUAD *get_selected_quad();
	LAYER *get_selected_layer_type(int index, int type);
	LAYER *get_selected_layer(int index);
	LAYERGROUP *get_selected_group();
	
	int do_properties(RECT *toolbox, PROPERTY *props, int *ids, int *new_val);
	
	int mode;
	int dialog;
	const char *tooltip;

	float world_offset_x;
	float world_offset_y;
	float editor_offset_x;
	float editor_offset_y;
	float world_zoom;
	int zoom_level;
	bool lock_mouse;
	bool show_mouse_pointer;
	bool gui_active;
	bool proof_borders;
	float mouse_delta_x;
	float mouse_delta_y;
	float mouse_delta_wx;
	float mouse_delta_wy;
	
	bool show_detail;
	bool animate;
	int64 animate_start;
	float animate_time;
	float animate_speed;
	
	int show_envelope_editor;
	
	int selected_layer;
	int selected_group;
	int selected_quad;
	int selected_points;
	int selected_envelope;
	int selected_image;
	
	MAP map;
};

extern EDITOR editor;

typedef struct
{
	int x, y;
	int w, h;
} RECTi;

class LAYER_TILES : public LAYER
{
public:
	LAYER_TILES(int w, int h);
	~LAYER_TILES();

	void resize(int new_w, int new_h);

	void make_palette();
	virtual void render();

	int convert_x(float x) const;
	int convert_y(float y) const;
	void convert(RECT rect, RECTi *out);
	void snap(RECT *rect);
	void clamp(RECTi *rect);

	virtual void brush_selecting(RECT rect);
	virtual int brush_grab(LAYERGROUP *brush, RECT rect);
	virtual void brush_draw(LAYER *brush, float wx, float wy);
	virtual void brush_flip_x();
	virtual void brush_flip_y();
	
	virtual int render_properties(RECT *toolbox);

	virtual void modify_image_index(INDEX_MODIFY_FUNC func);
	virtual void modify_envelope_index(INDEX_MODIFY_FUNC func);
	
	void prepare_for_save();

	void get_size(float *w, float *h) { *w = width*32.0f;  *h = height*32.0f; }
	
	int tex_id;
	int game;
	int image;
	int width;
	int height;
	TILE *tiles;
};

class LAYER_QUADS : public LAYER
{
public:
	LAYER_QUADS();
	~LAYER_QUADS();

	virtual void render();
	QUAD *new_quad();

	virtual void brush_selecting(RECT rect);
	virtual int brush_grab(LAYERGROUP *brush, RECT rect);
	virtual void brush_place(LAYER *brush, float wx, float wy);
	virtual void brush_flip_x();
	virtual void brush_flip_y();
	virtual void brush_rotate(float amount);
	
	virtual int render_properties(RECT *toolbox);

	virtual void modify_image_index(INDEX_MODIFY_FUNC func);
	virtual void modify_envelope_index(INDEX_MODIFY_FUNC func);
	
	void get_size(float *w, float *h);
	
	int image;
	array<QUAD> quads;
};

class LAYER_GAME : public LAYER_TILES
{
public:
	LAYER_GAME(int w, int h);
	~LAYER_GAME();

	virtual int render_properties(RECT *toolbox);
};

int do_editor_button(const void *id, const char *text, int checked, const RECT *r, ui_draw_button_func draw_func, int flags, const char *tooltip);
void draw_editor_button(const void *id, const char *text, int checked, const RECT *r, const void *extra);
void draw_editor_button_menuitem(const void *id, const char *text, int checked, const RECT *r, const void *extra);

void ui_invoke_popup_menu(void *id, int flags, float x, float y, float w, float h, int (*func)(RECT rect), void *extra=0);
void ui_do_popup_menu();

int popup_group(RECT view);
int popup_layer(RECT view);
int popup_quad(RECT view);
int popup_point(RECT view);

void popup_select_image_invoke(int current, float x, float y);
int popup_select_image_result();
