#include <baselib/opengl.h>
#include <baselib/math.h>
#include <baselib/vmath.h>
#include <baselib/stream/file.h>

#include <engine/interface.h>

#include "pnglite/pnglite.h"

#include <string.h>

#include <engine/config.h>


using namespace baselib;

static opengl::context context;

struct custom_vertex
{
	vec3 pos;
	vec2 tex;
	vec4 color;
};

const int vertex_buffer_size = 2048*64;
//static custom_vertex vertices[4];
static custom_vertex *vertices = 0;
static int num_vertices = 0;
static vec4 color[4];
static vec2 texture[4];

static opengl::vertex_buffer vertex_buffer;
//static int screen_width = 800;
//static int screen_height = 600;
static float rotation = 0;
static int quads_drawing = 0;

static float screen_x0 = 0;
static float screen_y0 = 0;
static float screen_x1 = 0;
static float screen_y1 = 0;

struct texture_holder
{
	opengl::texture tex;
	int flags;
	int next;
};

static const int MAX_TEXTURES = 128;

static texture_holder textures[MAX_TEXTURES];
static int first_free_texture;

static const unsigned char null_texture_data[] = {
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
};

static void draw_quad(bool _bflush = false)
{
	if (!_bflush && ((num_vertices + 4) < vertex_buffer_size))
	{
		// Just add
		num_vertices += 4;
	}
	else if (num_vertices)
	{
		if (!_bflush)
			num_vertices += 4;
		if(GLEW_ARB_vertex_buffer_object)
		{
			// set the data
			vertex_buffer.data(vertices, num_vertices * sizeof(custom_vertex), GL_DYNAMIC_DRAW);
			opengl::stream_vertex(&vertex_buffer, 3, GL_FLOAT, sizeof(custom_vertex), 0);
			opengl::stream_texcoord(&vertex_buffer, 0, 2, GL_FLOAT,
					sizeof(custom_vertex),
					sizeof(vec3));
			opengl::stream_color(&vertex_buffer, 4, GL_FLOAT,
					sizeof(custom_vertex),
					sizeof(vec3)+sizeof(vec2));		
			opengl::draw_arrays(GL_QUADS, 0, num_vertices);
		}
		else
		{
			glVertexPointer(3, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)vertices);
			glTexCoordPointer(2, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)vertices + sizeof(vec3));
			glColorPointer(4, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)vertices + sizeof(vec3) + sizeof(vec2));
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glDrawArrays(GL_QUADS, 0, num_vertices);
		}
		
		// Reset pointer
		num_vertices = 0;
	}
}
struct batch
{
	opengl::vertex_buffer vb;
	int num;
};

void gfx_quads_draw_batch(void *in_b)
{
	batch *b = (batch*)in_b;
	
	if(GLEW_ARB_vertex_buffer_object)
	{
		// set the data
		opengl::stream_vertex(&b->vb, 3, GL_FLOAT, sizeof(custom_vertex), 0);
		opengl::stream_texcoord(&b->vb, 0, 2, GL_FLOAT,
				sizeof(custom_vertex),
				sizeof(vec3));
		opengl::stream_color(&b->vb, 4, GL_FLOAT,
				sizeof(custom_vertex),
				sizeof(vec3)+sizeof(vec2));		
		opengl::draw_arrays(GL_QUADS, 0, b->num);
	}
	/*
	else
	{
		glVertexPointer(3, GL_FLOAT,
				sizeof(custom_vertex),
				(char*)vertices);
		glTexCoordPointer(2, GL_FLOAT,
				sizeof(custom_vertex),
				(char*)vertices + sizeof(vec3));
		glColorPointer(4, GL_FLOAT,
				sizeof(custom_vertex),
				(char*)vertices + sizeof(vec3) + sizeof(vec2));
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glDrawArrays(GL_QUADS, 0, b->num);
	}*/	
}


void *gfx_quads_create_batch()
{
	batch *b = new batch;
	b->num = num_vertices;
	b->vb.data(vertices, num_vertices*sizeof(custom_vertex), GL_STATIC_DRAW);
	num_vertices = 0;
	gfx_quads_end();
	return b;
}

	
bool gfx_init()
{
	if(!context.create(config.screen_width, config.screen_height, 24, 8, 16, 0, config.fullscreen?opengl::context::FLAG_FULLSCREEN:0))
	{
		dbg_msg("game", "failed to create gl context");
		return false;
	}

	
	// Init vertices
	if (vertices)
		mem_free(vertices);
	vertices = (custom_vertex*)mem_alloc(sizeof(custom_vertex) * vertex_buffer_size, 1);
	num_vertices = 0;

	context.set_title("---");

	/*
	dbg_msg("gfx", "OpenGL version %d.%d.%d", context.version_major(),
											  context.version_minor(),
											  context.version_rev());*/

	gfx_mapscreen(0,0,config.screen_width, config.screen_height);
	
	// TODO: make wrappers for this
	glEnable(GL_BLEND);
	
	// model
	mat4 mat = mat4::identity;
	opengl::matrix_modelview(&mat);
	
	// Set all z to -5.0f
	for (int i = 0; i < vertex_buffer_size; i++)
		vertices[i].pos.z = -5.0f;

	if(GLEW_ARB_vertex_buffer_object)
	{
		// set the streams
		vertex_buffer.data(vertices, sizeof(vertex_buffer_size), GL_DYNAMIC_DRAW);
		opengl::stream_vertex(&vertex_buffer, 3, GL_FLOAT, sizeof(custom_vertex), 0);
		opengl::stream_texcoord(&vertex_buffer, 0, 2, GL_FLOAT,
				sizeof(custom_vertex),
				sizeof(vec3));
		opengl::stream_color(&vertex_buffer, 4, GL_FLOAT,
				sizeof(custom_vertex),
				sizeof(vec3)+sizeof(vec2));		
	}

	// init textures
	first_free_texture = 0;
	for(int i = 0; i < MAX_TEXTURES; i++)
		textures[i].next = i+1;
	textures[MAX_TEXTURES-1].next = -1;
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	// create null texture, will get id=0
	gfx_load_texture_raw(4,4,IMG_RGBA,null_texture_data);

	// set vsync as needed
	gfx_set_vsync(config.vsync);

	return true;
}

video_mode fakemodes[] = {
	{320,240,8,8,8}, {400,300,8,8,8}, {640,480,8,8,8},
	{720,400,8,8,8}, {768,576,8,8,8}, {800,600,8,8,8},
	{1024,600,8,8,8}, {1024,768,8,8,8}, {1152,864,8,8,8},
	{1280,768,8,8,8}, {1280,800,8,8,8}, {1280,960,8,8,8},
	{1280,1024,8,8,8}, {1368,768,8,8,8}, {1400,1050,8,8,8},
	{1440,900,8,8,8}, {1440,1050,8,8,8}, {1600,1000,8,8,8},
	{1600,1200,8,8,8}, {1680,1050,8,8,8}, {1792,1344,8,8,8},
	{1800,1440,8,8,8}, {1856,1392,8,8,8}, {1920,1080,8,8,8},
	{1920,1200,8,8,8}, {1920,1440,8,8,8}, {1920,2400,8,8,8},
	{2048,1536,8,8,8},
		
	{320,240,5,6,5}, {400,300,5,6,5}, {640,480,5,6,5},
	{720,400,5,6,5}, {768,576,5,6,5}, {800,600,5,6,5},
	{1024,600,5,6,5}, {1024,768,5,6,5}, {1152,864,5,6,5},
	{1280,768,5,6,5}, {1280,800,5,6,5}, {1280,960,5,6,5},
	{1280,1024,5,6,5}, {1368,768,5,6,5}, {1400,1050,5,6,5},
	{1440,900,5,6,5}, {1440,1050,5,6,5}, {1600,1000,5,6,5},
	{1600,1200,5,6,5}, {1680,1050,5,6,5}, {1792,1344,5,6,5},
	{1800,1440,5,6,5}, {1856,1392,5,6,5}, {1920,1080,5,6,5},
	{1920,1200,5,6,5}, {1920,1440,5,6,5}, {1920,2400,5,6,5},
	{2048,1536,5,6,5}
};

int gfx_get_video_modes(video_mode *list, int maxcount)
{
	if(config.display_all_modes)
	{
		mem_copy(list, fakemodes, sizeof(fakemodes));
		return min((int)(sizeof(fakemodes)/sizeof(video_mode)), maxcount);
	}
	return context.getvideomodes((opengl::videomode *)list, maxcount);
}

void gfx_set_vsync(int val)
{
	context.set_vsync(val);
}

int gfx_unload_texture(int index)
{
	textures[index].tex.clear();
	textures[index].next = first_free_texture;
	first_free_texture = index;
	return 0;
}

void gfx_blend_normal()
{
	// TODO: wrapper for this
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gfx_blend_additive()
{
	// TODO: wrapper for this
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

int gfx_load_texture_raw(int w, int h, int format, const void *data)
{
	// grab texture
	int tex = first_free_texture;
	first_free_texture = textures[tex].next;
	textures[tex].next = -1;
	
	// set data and return
	// TODO: should be RGBA, not BGRA
	dbg_msg("gfx", "%d = %dx%d", tex, w, h);
	if(format == IMG_RGB)
		textures[tex].tex.data2d(w, h, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, data);
	else if(format == IMG_RGBA)
	{
		textures[tex].tex.data2d(w, h, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	
	return tex;
}
/*
int gfx_load_mip_texture_raw(int w, int h, int format, const void *data)
{
	// grab texture
	int tex = first_free_texture;
	first_free_texture = textures[tex].next;
	textures[tex].next = -1;
	
	// set data and return
	// TODO: should be RGBA, not BGRA
	dbg_msg("gfx", "%d = %dx%d", tex, w, h);
	dbg_assert(format == IMG_RGBA, "not an RGBA image");

	unsigned mip_w = w;
	unsigned mip_h = h;
	unsigned level = 0;
	const unsigned char *ptr = (const unsigned char*)data;
	while(mip_w > 0 && mip_h > 0)
	{
		dbg_msg("gfx mip", "%d = %dx%d", level, mip_w, mip_h);
		textures[tex].tex.data2d_mip(mip_w, mip_h, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, level, ptr);
		level++;
		ptr = ptr + mip_w*mip_h*4;
		mip_w = mip_w>>1;
		mip_h = mip_h>>1;
	}
	return tex;
}
*/
// simple uncompressed RGBA loaders
int gfx_load_texture(const char *filename)
{
	int l = strlen(filename);
	if(l < 3)
		return 0;
	image_info img;
	if(gfx_load_png(&img, filename))
	{
		int id = gfx_load_texture_raw(img.width, img.height, img.format, img.data);
		mem_free(img.data);
		return id;
	}
	
	return 0;
}

int gfx_load_png(image_info *img, const char *filename)
{
	// open file for reading
	png_init(0,0);

	png_t png;
	if(png_open_file(&png, filename) != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", filename);
		return 0;
	}
	
	if(png.depth != 8 || (png.color_type != PNG_TRUECOLOR && png.color_type != PNG_TRUECOLOR_ALPHA))
	{
		dbg_msg("game/png", "invalid format. filename='%s'", filename);
		png_close_file(&png);
	}
		
	unsigned char *buffer = (unsigned char *)mem_alloc(png.width * png.height * png.bpp, 1);
	png_get_data(&png, buffer);
	png_close_file(&png);
	
	img->width = png.width;
	img->height = png.height;
	if(png.color_type == PNG_TRUECOLOR)
		img->format = IMG_RGB;
	else if(png.color_type == PNG_TRUECOLOR_ALPHA)
		img->format = IMG_RGBA;
	img->data = buffer;
	return 1;
}

void gfx_shutdown()
{
	if (vertices)
		mem_free(vertices);
	context.destroy();
}

void gfx_swap()
{
	context.swap();
}

int gfx_screenwidth()
{
	return config.screen_width;
}

int gfx_screenheight()
{
	return config.screen_height;
}

void gfx_texture_set(int slot)
{
	dbg_assert(quads_drawing == 0, "called gfx_texture_set within quads_begin");
	if(slot == -1)
		opengl::texture_disable(0);
	else
		opengl::texture_2d(0, &textures[slot].tex);
}

void gfx_clear(float r, float g, float b)
{
	glClearColor(r,g,b,1.0f);
	opengl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gfx_mapscreen(float tl_x, float tl_y, float br_x, float br_y)
{
	screen_x0 = tl_x;
	screen_y0 = tl_y;
	screen_x1 = br_x;
	screen_y1 = br_y;
	mat4 mat;
	mat.ortho(tl_x, br_x, br_y, tl_y, 1.0f, 10.f);
	opengl::matrix_projection(&mat);
}

void gfx_getscreen(float *tl_x, float *tl_y, float *br_x, float *br_y)
{
	*tl_x = screen_x0;
	*tl_y = screen_y0;
	*br_x = screen_x1;
	*br_y = screen_y1;
}

void gfx_setoffset(float x, float y)
{
	//const float scale = 0.75f;
	const float scale = 1.0f;
	mat4 mat = mat4::identity;
	mat.m[0] = scale;
	mat.m[5] = scale;
	mat.m[10] = scale;
	mat.m[12] = x*scale;
	mat.m[13] = y*scale;
	opengl::matrix_modelview(&mat);
}




void gfx_quads_begin()
{
	dbg_assert(quads_drawing == 0, "called quads_begin twice");
	quads_drawing++;
	
	gfx_quads_setsubset(0,0,1,1);
	gfx_quads_setrotation(0);
	gfx_quads_setcolor(1,1,1,1);
}

void gfx_quads_end()
{
	dbg_assert(quads_drawing == 1, "called quads_end without quads_begin");
	draw_quad(true);
	quads_drawing--;
}


void gfx_quads_setrotation(float angle)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setrotation without quads_begin");
	rotation = angle;
}

void gfx_quads_setcolorvertex(int i, float r, float g, float b, float a)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setcolorvertex without quads_begin");
	color[i].r = r;
	color[i].g = g;
	color[i].b = b;
	color[i].a = a;
}

void gfx_quads_setcolor(float r, float g, float b, float a)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setcolor without quads_begin");
	color[0] = vec4(r,g,b,a);
	color[1] = vec4(r,g,b,a);
	color[2] = vec4(r,g,b,a);
	color[3] = vec4(r,g,b,a);
}

void gfx_quads_setsubset(float tl_u, float tl_v, float br_u, float br_v)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setsubset without quads_begin");

	texture[0].x = tl_u;
	texture[0].y = tl_v;
	//g_pVertices[g_iVertexEnd].tex.u = tl_u;
	//g_pVertices[g_iVertexEnd].tex.v = tl_v;

	texture[1].x = br_u;
	texture[1].y = tl_v;
	//g_pVertices[g_iVertexEnd + 2].tex.u = br_u;
	//g_pVertices[g_iVertexEnd + 2].tex.v = tl_v;

	texture[2].x = br_u;
	texture[2].y = br_v;
	//g_pVertices[g_iVertexEnd + 1].tex.u = tl_u;
	//g_pVertices[g_iVertexEnd + 1].tex.v = br_v;

	texture[3].x = tl_u;
	texture[3].y = br_v;
	//g_pVertices[g_iVertexEnd + 3].tex.u = br_u;
	//g_pVertices[g_iVertexEnd + 3].tex.v = br_v;
}

static void rotate(vec3 &center, vec3 &point)
{
	vec3 p = point-center;
	point.x = p.x * cosf(rotation) - p.y * sinf(rotation) + center.x;
	point.y = p.x * sinf(rotation) + p.y * cosf(rotation) + center.y;
}

void gfx_quads_draw(float x, float y, float w, float h)
{
	gfx_quads_drawTL(x-w/2, y-h/2,w,h);
}

void gfx_quads_drawTL(float x, float y, float width, float height)
{
	dbg_assert(quads_drawing == 1, "called quads_draw without quads_begin");
	
	vec3 center;
	center.x = x + width/2;
	center.y = y + height/2;
	center.z = 0;
	
	vertices[num_vertices].pos.x = x;
	vertices[num_vertices].pos.y = y;
	vertices[num_vertices].tex.u = texture[0].x;
	vertices[num_vertices].tex.v = texture[0].y;
	vertices[num_vertices].color = color[0];
	rotate(center, vertices[num_vertices].pos);

	vertices[num_vertices + 1].pos.x = x+width;
	vertices[num_vertices + 1].pos.y = y;
	vertices[num_vertices + 1].tex.u = texture[1].x;
	vertices[num_vertices + 1].tex.v = texture[1].y;
	vertices[num_vertices + 1].color = color[1];
	rotate(center, vertices[num_vertices + 1].pos);

	vertices[num_vertices + 2].pos.x = x + width;
	vertices[num_vertices + 2].pos.y = y+height;
	vertices[num_vertices + 2].tex.u = texture[2].x;
	vertices[num_vertices + 2].tex.v = texture[2].y;
	vertices[num_vertices + 2].color = color[2];
	rotate(center, vertices[num_vertices + 2].pos);

	vertices[num_vertices + 3].pos.x = x;
	vertices[num_vertices + 3].pos.y = y+height;
	vertices[num_vertices + 3].tex.u = texture[3].x;
	vertices[num_vertices + 3].tex.v = texture[3].y;
	vertices[num_vertices + 3].color = color[3];
	rotate(center, vertices[num_vertices + 3].pos);
	
	draw_quad();
}

void gfx_quads_draw_freeform(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3)
{
	dbg_assert(quads_drawing == 1, "called quads_draw_freeform without quads_begin");
	
	vertices[num_vertices].pos.x = x0;
	vertices[num_vertices].pos.y = y0;
	vertices[num_vertices].tex.u = texture[0].x;
	vertices[num_vertices].tex.v = texture[0].y;
	vertices[num_vertices].color = color[0];

	vertices[num_vertices + 1].pos.x = x1;
	vertices[num_vertices + 1].pos.y = y1;
	vertices[num_vertices + 1].tex.u = texture[1].x;
	vertices[num_vertices + 1].tex.v = texture[1].y;
	vertices[num_vertices + 1].color = color[1];

	vertices[num_vertices + 2].pos.x = x3;
	vertices[num_vertices + 2].pos.y = y3;
	vertices[num_vertices + 2].tex.u = texture[2].x;
	vertices[num_vertices + 2].tex.v = texture[2].y;
	vertices[num_vertices + 2].color = color[2];

	vertices[num_vertices + 3].pos.x = x2;
	vertices[num_vertices + 3].pos.y = y2;
	vertices[num_vertices + 3].tex.u = texture[3].x;
	vertices[num_vertices + 3].tex.v = texture[3].y;
	vertices[num_vertices + 3].color = color[3];
	
	draw_quad();
}

void gfx_quads_text(float x, float y, float size, const char *text)
{
	gfx_quads_begin();
	float startx = x;
	while(*text)
	{
		char c = *text;
		text++;
		
		if(c == '\n')
		{
			x = startx;
			y += size;
		}
		else
		{
			gfx_quads_setsubset(
				(c%16)/16.0f,
				(c/16)/16.0f,
				(c%16)/16.0f+1.0f/16.0f,
				(c/16)/16.0f+1.0f/16.0f);
			
			gfx_quads_drawTL(x,y,size,size);
			x += size/2;
		}
	}
	
	gfx_quads_end();
}

struct pretty_font
{
	float m_CharStartTable[256];
	float m_CharEndTable[256];
	int font_texture;
};

pretty_font default_font = 
{
{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0.421875, 0.359375, 0.265625, 0.25, 0.1875, 0.25, 0.4375, 0.390625, 0.390625, 0.34375, 0.28125, 0.421875, 0.390625, 0.4375, 0.203125, 
        0.265625, 0.28125, 0.28125, 0.265625, 0.25, 0.28125, 0.28125, 0.265625, 0.28125, 0.265625, 0.4375, 0.421875, 0.3125, 0.28125, 0.3125, 0.3125, 
        0.25, 0.234375, 0.28125, 0.265625, 0.265625, 0.296875, 0.3125, 0.25, 0.25, 0.421875, 0.28125, 0.265625, 0.328125, 0.171875, 0.234375, 0.25, 
        0.28125, 0.234375, 0.265625, 0.265625, 0.28125, 0.265625, 0.234375, 0.09375, 0.234375, 0.234375, 0.265625, 0.390625, 0.203125, 0.390625, 0.296875, 0.28125, 
        0.375, 0.3125, 0.3125, 0.3125, 0.296875, 0.3125, 0.359375, 0.296875, 0.3125, 0.4375, 0.390625, 0.328125, 0.4375, 0.203125, 0.3125, 0.296875, 
        0.3125, 0.296875, 0.359375, 0.3125, 0.328125, 0.3125, 0.296875, 0.203125, 0.296875, 0.296875, 0.328125, 0.375, 0.421875, 0.375, 0.28125, 0.3125, 
        0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 
        0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 
        0, 0.421875, 0.3125, 0.265625, 0.25, 0.25, 0.421875, 0.265625, 0.375, 0.21875, 0.375, 0.328125, 0.3125, 0, 0.21875, 0.28125, 
        0.359375, 0.28125, 0.34375, 0.34375, 0.421875, 0.3125, 0.265625, 0.421875, 0.421875, 0.34375, 0.375, 0.328125, 0.125, 0.125, 0.125, 0.296875, 
        0.234375, 0.234375, 0.234375, 0.234375, 0.234375, 0.234375, 0.109375, 0.265625, 0.296875, 0.296875, 0.296875, 0.296875, 0.375, 0.421875, 0.359375, 0.390625, 
        0.21875, 0.234375, 0.25, 0.25, 0.25, 0.25, 0.25, 0.296875, 0.21875, 0.265625, 0.265625, 0.265625, 0.265625, 0.234375, 0.28125, 0.3125, 
        0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.1875, 0.3125, 0.3125, 0.3125, 0.3125, 0.3125, 0.375, 0.421875, 0.359375, 0.390625, 
        0.3125, 0.3125, 0.296875, 0.296875, 0.296875, 0.296875, 0.296875, 0.28125, 0.28125, 0.3125, 0.3125, 0.3125, 0.3125, 0.296875, 0.3125, 0.296875, 
},
{
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0.2, 0.5625, 0.625, 0.71875, 0.734375, 0.796875, 0.765625, 0.546875, 0.59375, 0.59375, 0.65625, 0.703125, 0.546875, 0.59375, 0.5625, 0.6875, 
        0.71875, 0.609375, 0.703125, 0.703125, 0.71875, 0.703125, 0.703125, 0.6875, 0.703125, 0.703125, 0.5625, 0.546875, 0.671875, 0.703125, 0.671875, 0.671875, 
        0.734375, 0.75, 0.734375, 0.734375, 0.734375, 0.6875, 0.6875, 0.734375, 0.71875, 0.5625, 0.65625, 0.765625, 0.703125, 0.8125, 0.75, 0.734375, 
        0.734375, 0.765625, 0.71875, 0.71875, 0.703125, 0.71875, 0.75, 0.890625, 0.75, 0.75, 0.71875, 0.59375, 0.6875, 0.59375, 0.6875, 0.703125, 
        0.5625, 0.671875, 0.6875, 0.671875, 0.671875, 0.671875, 0.625, 0.671875, 0.671875, 0.5625, 0.546875, 0.703125, 0.5625, 0.78125, 0.671875, 0.671875, 
        0.6875, 0.671875, 0.65625, 0.671875, 0.65625, 0.671875, 0.6875, 0.78125, 0.6875, 0.671875, 0.65625, 0.609375, 0.546875, 0.609375, 0.703125, 0.671875, 
        0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 
        0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 
        0, 0.5625, 0.671875, 0.734375, 0.734375, 0.734375, 0.546875, 0.71875, 0.609375, 0.765625, 0.609375, 0.65625, 0.671875, 0, 0.765625, 0.703125, 
        0.625, 0.703125, 0.640625, 0.640625, 0.609375, 0.671875, 0.703125, 0.546875, 0.5625, 0.578125, 0.609375, 0.65625, 0.859375, 0.859375, 0.859375, 0.671875, 
        0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0.84375, 0.734375, 0.6875, 0.6875, 0.6875, 0.6875, 0.5625, 0.609375, 0.640625, 0.59375, 
        0.734375, 0.75, 0.734375, 0.734375, 0.734375, 0.734375, 0.734375, 0.6875, 0.765625, 0.71875, 0.71875, 0.71875, 0.71875, 0.75, 0.734375, 0.6875, 
        0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.796875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.5625, 0.609375, 0.625, 0.59375, 
        0.6875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.703125, 0.703125, 0.671875, 0.671875, 0.671875, 0.671875, 0.671875, 0.6875, 0.671875, 
},
0
};

double extra_kerning[256*256] = {0};

pretty_font *current_font = &default_font;

void gfx_pretty_text(float x, float y, float size, const char *text)
{
	const float spacing = 0.05f;
	gfx_texture_set(current_font->font_texture);
	gfx_quads_begin();
	
	float startx = x;
	
	while (*text)
	{
		const int c = *text;
		
		if(c == '\n')
		{
			x = startx;
			y += size;
		}
		else
		{
			const float width = current_font->m_CharEndTable[c] - current_font->m_CharStartTable[c];

			x -= size * current_font->m_CharStartTable[c];
			
			gfx_quads_setsubset(
				(c%16)/16.0f, // startx
				(c/16)/16.0f, // starty
				(c%16)/16.0f+1.0f/16.0f, // endx
				(c/16)/16.0f+1.0f/16.0f); // endy

			gfx_quads_drawTL(x, y, size, size);

			double x_nudge = 0;
			if (text[1])
				x_nudge = extra_kerning[text[0] + text[1] * 256];

			x += (width + current_font->m_CharStartTable[c] + spacing + x_nudge) * size;
		}

		text++;
	}

	gfx_quads_end();
}

float gfx_pretty_text_width(float size, const char *text)
{
	const float spacing = 0.05f;
	float w = 0.0f;

	while (*text)
	{
		const int c = *text;
		const float width = current_font->m_CharEndTable[c] - current_font->m_CharStartTable[c];

		double x_nudge = 0;
		if (text[1])
			x_nudge = extra_kerning[text[0] + text[1] * 256];

		w += (width + spacing + x_nudge) * size;

		text++;
	}

	return w;
}
