#include "ed_editor.hpp"
#include <game/g_math.h>
#include <game/generated/gc_data.h>
#include <game/client/gc_render.h>

LAYER_QUADS::LAYER_QUADS()
{
	type = LAYERTYPE_QUADS;
	type_name = "Quads";
	image = -1;
}

LAYER_QUADS::~LAYER_QUADS()
{
}

void LAYER_QUADS::render()
{
	gfx_texture_set(-1);
	if(image >= 0 && image < editor.map.images.len())
		gfx_texture_set(editor.map.images[image]->tex_id);
		
	render_quads(quads.getptr(), quads.len());
}

QUAD *LAYER_QUADS::new_quad()
{
	QUAD *q = &quads[quads.add(QUAD())];

	q->pos_env = -1;
	q->color_env = -1;
	q->pos_env_offset = 0;
	q->color_env_offset = 0;
	int x = 0, y = 0;
	q->points[0].x = x;
	q->points[0].y = y;
	q->points[1].x = x+64;
	q->points[1].y = y;
	q->points[2].x = x;
	q->points[2].y = y+64;
	q->points[3].x = x+64;
	q->points[3].y = y+64;

	q->points[4].x = x+32; // pivot
	q->points[4].y = y+32;
	
	for(int i = 0; i < 5; i++)
	{
		q->points[i].x <<= 10;
		q->points[i].y <<= 10;
	}
	

	q->texcoords[0].x = 0;
	q->texcoords[0].y = 0;
	
	q->texcoords[1].x = 1<<10;
	q->texcoords[1].y = 0;
	
	q->texcoords[2].x = 0;
	q->texcoords[2].y = 1<<10;
	
	q->texcoords[3].x = 1<<10;
	q->texcoords[3].y = 1<<10;
	
	q->colors[0].r = 255; q->colors[0].g = 255; q->colors[0].b = 255; q->colors[0].a = 255;
	q->colors[1].r = 255; q->colors[1].g = 255; q->colors[1].b = 255; q->colors[1].a = 255;
	q->colors[2].r = 255; q->colors[2].g = 255; q->colors[2].b = 255; q->colors[2].a = 255;
	q->colors[3].r = 255; q->colors[3].g = 255; q->colors[3].b = 255; q->colors[3].a = 255;

	return q;
}

void LAYER_QUADS::brush_selecting(RECT rect)
{
	// draw selection rectangle
	gfx_texture_set(-1);
	gfx_lines_begin();
	gfx_lines_draw(rect.x, rect.y, rect.x+rect.w, rect.y);
	gfx_lines_draw(rect.x+rect.w, rect.y, rect.x+rect.w, rect.y+rect.h);
	gfx_lines_draw(rect.x+rect.w, rect.y+rect.h, rect.x, rect.y+rect.h);
	gfx_lines_draw(rect.x, rect.y+rect.h, rect.x, rect.y);
	gfx_lines_end();
}

int LAYER_QUADS::brush_grab(LAYERGROUP *brush, RECT rect)
{
	// create new layers
	LAYER_QUADS *grabbed = new LAYER_QUADS();
	grabbed->image = image;
	brush->add_layer(grabbed);
	
	//dbg_msg("", "%f %f %f %f", rect.x, rect.y, rect.w, rect.h);
	for(int i = 0; i < quads.len(); i++)
	{
		QUAD *q = &quads[i];
		float px = fx2f(q->points[4].x);
		float py = fx2f(q->points[4].y);
		
		if(px > rect.x && px < rect.x+rect.w && py > rect.y && py < rect.y+rect.h)
		{
			dbg_msg("", "grabbed one");
			QUAD n;
			n = *q;
			
			for(int p = 0; p < 5; p++)
			{
				n.points[p].x -= f2fx(rect.x);
				n.points[p].y -= f2fx(rect.y);
			}
			
			grabbed->quads.add(n);
		}
	}
	
	return grabbed->quads.len()?1:0;
}

void LAYER_QUADS::brush_place(LAYER *brush, float wx, float wy)
{
	LAYER_QUADS *l = (LAYER_QUADS *)brush;
	for(int i = 0; i < l->quads.len(); i++)
	{
		QUAD n = l->quads[i];
		
		for(int p = 0; p < 5; p++)
		{
			n.points[p].x += f2fx(wx);
			n.points[p].y += f2fx(wy);
		}
			
		quads.add(n);
	}
}

void LAYER_QUADS::brush_flip_x()
{
}

void LAYER_QUADS::brush_flip_y()
{
}

void LAYER_QUADS::get_size(float *w, float *h)
{
	*w = 0; *h = 0;
	
	for(int i = 0; i < quads.len(); i++)
	{
		for(int p = 0; p < 5; p++)
		{
			*w = max(*w, fx2f(quads[i].points[p].x));
			*h = max(*h, fx2f(quads[i].points[p].y));
		}
	}
}

extern int selected_points;

int LAYER_QUADS::render_properties(RECT *toolbox)
{
	// layer props
	if(editor.props == PROPS_LAYER)
	{
		enum
		{
			PROP_IMAGE=0,
			NUM_PROPS,
		};
		
		PROPERTY props[] = {
			{"Image", image, PROPTYPE_IMAGE, -1, 0},
			{0},
		};
		
		static int ids[NUM_PROPS] = {0};
		int new_val = 0;
		int prop = editor.do_properties(toolbox, props, ids, &new_val);		
		
		if(prop == PROP_IMAGE)
		{
			if(new_val >= 0)
				image = new_val%editor.map.images.len();
			else
				image = -1;
		}
	}

	// quad props
	QUAD *quad = editor.get_selected_quad();
	if(editor.props == PROPS_QUAD)
	{
		if(quad)
		{
			RECT slot;
			ui_hsplit_t(toolbox, 15.0f, &slot, toolbox);
			ui_do_label(&slot, "Quad Props", 12.0f, -1, -1);
			
			enum
			{
				PROP_POS_ENV=0,
				PROP_POS_ENV_OFFSET,
				PROP_COLOR_ENV,
				PROP_COLOR_ENV_OFFSET,
				NUM_PROPS,
			};
			
			PROPERTY props[] = {
				{"Pos. Env", quad->pos_env, PROPTYPE_INT_STEP, -1, editor.map.envelopes.len()},
				{"Pos. TO", quad->pos_env_offset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
				{"Color Env", quad->color_env, PROPTYPE_INT_STEP, -1, editor.map.envelopes.len()},
				{"Color TO", quad->color_env_offset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
				
				{0},
			};
			
			static int ids[NUM_PROPS] = {0};
			int new_val = 0;
			int prop = editor.do_properties(toolbox, props, ids, &new_val);		
			
			if(prop == PROP_POS_ENV) quad->pos_env = new_val;
			if(prop == PROP_POS_ENV_OFFSET) quad->pos_env_offset = new_val;
			if(prop == PROP_COLOR_ENV) quad->color_env = new_val;
			if(prop == PROP_COLOR_ENV_OFFSET) quad->color_env_offset = new_val;
		}
	}
	
	// point props
	if(editor.props == PROPS_QUAD_POINT && quad && selected_points)
	{
		RECT slot;
		ui_hsplit_t(toolbox, 15.0f, &slot, toolbox);
		ui_do_label(&slot, "Point Props", 14.0f, -1, -1);
		
		enum
		{
			PROP_COLOR=0,
			NUM_PROPS,
		};
		
		int color = 0;
	
		for(int v = 0; v < 4; v++)
		{
			if(selected_points&(1<<v))
			{
				color = 0;
				color |= quad->colors[v].r<<24;
				color |= quad->colors[v].g<<16;
				color |= quad->colors[v].b<<8;
				color |= quad->colors[v].a;
			}
		}
		
		
		PROPERTY props[] = {
			{"Color", color, PROPTYPE_COLOR, -1, editor.map.envelopes.len()},
			{0},
		};
		
		static int ids[NUM_PROPS] = {0};
		int new_val = 0;
		int prop = editor.do_properties(toolbox, props, ids, &new_val);		
		if(prop == PROP_COLOR)
		{
			for(int v = 0; v < 4; v++)
			{
				if(selected_points&(1<<v))
				{
					color = 0;
					quad->colors[v].r = (new_val>>24)&0xff;
					quad->colors[v].g = (new_val>>16)&0xff;
					quad->colors[v].b = (new_val>>8)&0xff;
					quad->colors[v].a = new_val&0xff;
				}
			}
		}
	}	
	
	return 0;
}


