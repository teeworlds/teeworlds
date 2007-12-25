/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/external/glfw/include/GL/glfw.h>
#include <engine/external/pnglite/pnglite.h>

#include <engine/e_system.h>
#include <engine/e_interface.h>
#include <engine/e_engine.h>
#include <engine/e_config.h>
#include <engine/e_keys.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "ec_font.h"

/* compressed textures */
#define GL_COMPRESSED_RGB_ARB 0x84ED
#define GL_COMPRESSED_RGBA_ARB 0x84EE

enum
{
	DRAWING_QUADS=1,
	DRAWING_LINES=2
};

/* */
typedef struct { float x, y, z; } VEC3;
typedef struct { float u, v; } TEXCOORD;
typedef struct { float r, g, b, a; } COLOR;

typedef struct
{
	VEC3 pos;
	TEXCOORD tex;
	COLOR color;
} VERTEX;

const int vertex_buffer_size = 32*1024;
static VERTEX *vertices = 0;
static int num_vertices = 0;

static COLOR color[4];
static TEXCOORD texture[4];

static int do_screenshot = 0;

static int screen_width = -1;
static int screen_height = -1;
static float rotation = 0;
static int drawing = 0;

static float screen_x0 = 0;
static float screen_y0 = 0;
static float screen_x1 = 0;
static float screen_y1 = 0;

typedef struct
{
	GLuint tex;
	int memsize;
	int flags;
	int next;
} TEXTURE;


enum
{
	MAX_TEXTURES = 128
};

static TEXTURE textures[MAX_TEXTURES];
static int first_free_texture;
static int memory_usage = 0;

static const unsigned char null_texture_data[] = {
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
};


typedef struct
{
	float m_CharStartTable[256];
	float m_CharEndTable[256];
	int font_texture;
} pretty_font;

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

static void flush()
{
	if(num_vertices == 0)
		return;
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexPointer(3, GL_FLOAT,
			sizeof(VERTEX),
			(char*)vertices);
	glTexCoordPointer(2, GL_FLOAT,
			sizeof(VERTEX),
			(char*)vertices + sizeof(float)*3);
	glColorPointer(4, GL_FLOAT,
			sizeof(VERTEX),
			(char*)vertices + sizeof(float)*5);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	if(drawing == DRAWING_QUADS)
		glDrawArrays(GL_QUADS, 0, num_vertices);
	else if(drawing == DRAWING_LINES)
		glDrawArrays(GL_LINES, 0, num_vertices);
	
	/* Reset pointer */
	num_vertices = 0;	
}

static void draw_line()
{
	num_vertices += 2;
	if((num_vertices + 2) >= vertex_buffer_size)
		flush();
}

static void draw_quad()
{
	num_vertices += 4;
	if((num_vertices + 4) >= vertex_buffer_size)
		flush();
}

static void screen_resize(int width, int height)
{
	screen_width = width;
	screen_height = height;
	glViewport(0, 0, screen_width, screen_height);
}

int gfx_init()
{
	int i;

	screen_width = config.gfx_screen_width;
	screen_height = config.gfx_screen_height;

	glfwInit();

	if(config.dbg_stress)
	{
		screen_width = 320;
		screen_height = 240;
	}

	/* set antialiasing	*/
	if(config.gfx_fsaa_samples)
		glfwOpenWindowHint(GLFW_FSAA_SAMPLES, config.gfx_fsaa_samples);
	
	/* set refresh rate */
	if(config.gfx_refresh_rate)
		glfwOpenWindowHint(GLFW_REFRESH_RATE, config.gfx_refresh_rate);
	
	/* no resizing allowed */
	/* glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, 1); */
		
	/* open window */	
	if(config.gfx_fullscreen)
	{
		int result = glfwOpenWindow(screen_width, screen_height, 8, 8, 8, 0, 24, 0, GLFW_FULLSCREEN);
		if(result != GL_TRUE)
		{
			dbg_msg("game", "failed to create gl context");
			return 0;
		}
	}
	else
	{
		int result = glfwOpenWindow(screen_width, screen_height, 0, 0, 0, 0, 24, 0, GLFW_WINDOW);
		if(result != GL_TRUE)
		{
			dbg_msg("game", "failed to create gl context");
			return 0;
		}
	}

	
	glfwSetWindowSizeCallback(screen_resize);
	
	glGetIntegerv(GL_DEPTH_BITS, &i);
	dbg_msg("gfx", "depthbits = %d", i);
	
	glfwSetWindowTitle("Teewars");
	
	/* We don't want to see the window when we run the stress testing */
	if(config.dbg_stress)
		glfwIconifyWindow();
	
	/* Init vertices */
	if (vertices)
		mem_free(vertices);
	vertices = (VERTEX*)mem_alloc(sizeof(VERTEX) * vertex_buffer_size, 1);
	num_vertices = 0;


	/*
	dbg_msg("gfx", "OpenGL version %d.%d.%d", context.version_major(),
											  context.version_minor(),
											  context.version_rev());*/

	gfx_mapscreen(0,0,config.gfx_screen_width, config.gfx_screen_height);

	/* set some default settings */	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gfx_mask_op(MASK_NONE, 0);
	
	/* Set all z to -5.0f */
	for (i = 0; i < vertex_buffer_size; i++)
		vertices[i].pos.z = -5.0f;

	/* init textures */
	first_free_texture = 0;
	for(i = 0; i < MAX_TEXTURES; i++)
		textures[i].next = i+1;
	textures[MAX_TEXTURES-1].next = -1;
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	/* init input */
	inp_init();
	
	/* create null texture, will get id=0 */
	gfx_load_texture_raw(4,4,IMG_RGBA,null_texture_data);

	/* set vsync as needed */
	gfx_set_vsync(config.gfx_vsync);

	/* UGLY as hell, please remove */
    current_font->font_texture = gfx_load_texture("data/big_font.png");

	return 1;
}

float gfx_screenaspect()
{
    return gfx_screenwidth()/(float)gfx_screenheight();
}

void gfx_clear_mask(int fill)
{
	/*if(fill)
		glClearDepth(0.0f);
	else*/
	
	int i;
	glGetIntegerv(GL_DEPTH_WRITEMASK, &i);
	glDepthMask(GL_TRUE);
	glClearDepth(0.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glDepthMask(i);
}

void gfx_mask_op(int mask, int write)
{
	glEnable(GL_DEPTH_TEST);
	
	if(write)
		glDepthMask(GL_TRUE);
	else
		glDepthMask(GL_FALSE);
	
	if(mask == MASK_NONE)
		glDepthFunc(GL_ALWAYS);
	else if(mask == MASK_SET)
		glDepthFunc(GL_LEQUAL);
	else if(mask == MASK_ZERO)
		glDepthFunc(GL_NOTEQUAL);
}

int gfx_window_active()
{
	return glfwGetWindowParam(GLFW_ACTIVE) == GL_TRUE ? 1 : 0;
}

int gfx_window_open()
{
	return glfwGetWindowParam(GLFW_OPENED) == GL_TRUE ? 1 : 0;
}

VIDEO_MODE fakemodes[] = {
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

int gfx_get_video_modes(VIDEO_MODE *list, int maxcount)
{
	if(config.gfx_display_all_modes)
	{
		int count = sizeof(fakemodes)/sizeof(VIDEO_MODE);
		mem_copy(list, fakemodes, sizeof(fakemodes));
		if(maxcount < count)
			count = maxcount;
		return count;
	}
	
	return glfwGetVideoModes((GLFWvidmode *)list, maxcount);
}

void gfx_set_vsync(int val)
{
	glfwSwapInterval(val);
}

int gfx_unload_texture(int index)
{
	glDeleteTextures(1, &textures[index].tex);
	textures[index].next = first_free_texture;
	memory_usage -= textures[index].memsize;
	first_free_texture = index;
	return 0;
}

void gfx_blend_normal()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gfx_blend_additive()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

int gfx_memory_usage() { return memory_usage; }

static unsigned char sample(int w, int h, const unsigned char *data, int u, int v, int offset)
{
	return (data[(v*w+u)*4+offset]+
	data[(v*w+u+1)*4+offset]+
	data[((v+1)*w+u)*4+offset]+
	data[((v+1)*w+u+1)*4+offset])/4;
}

int gfx_load_texture_raw(int w, int h, int format, const void *data)
{
	int mipmap = 1;
	unsigned char *texdata = (unsigned char *)data;
	unsigned char *tmpdata = 0;
	int oglformat = 0;
	int tex = 0;
	
	/* don't waste memory on texture if we are stress testing */
	if(config.dbg_stress)
		return -1;
	
	/* grab texture */
	tex = first_free_texture;
	first_free_texture = textures[tex].next;
	textures[tex].next = -1;
	
	/* resample if needed */
	if(config.gfx_texture_quality==0)
	{
		if(w > 16 && h > 16 && format == IMG_RGBA)
		{
			unsigned char *tmpdata;
			int c = 0;
			int x, y;

			tmpdata = (unsigned char *)mem_alloc(w*h*4, 1);

			w/=2;
			h/=2;

			for(y = 0; y < h; y++)
				for(x = 0; x < w; x++, c++)
				{
					tmpdata[c*4] = sample(w*2, h*2, texdata, x*2,y*2, 0);
					tmpdata[c*4+1] = sample(w*2, h*2, texdata, x*2,y*2, 1);
					tmpdata[c*4+2] = sample(w*2, h*2, texdata, x*2,y*2, 2);
					tmpdata[c*4+3] = sample(w*2, h*2, texdata, x*2,y*2, 3);
				}
			texdata = tmpdata;
		}
	}
	
	if(config.debug)
		dbg_msg("gfx", "%d = %dx%d", tex, w, h);
	
	/* upload texture */
	if(config.gfx_texture_compression)
	{
		oglformat = GL_COMPRESSED_RGBA_ARB;
		if(format == IMG_RGB)
			oglformat = GL_COMPRESSED_RGB_ARB;
	}
	else
	{
		oglformat = GL_RGBA;
		if(format == IMG_RGB)
			oglformat = GL_RGB;
	}
		
	glGenTextures(1, &textures[tex].tex);
	glBindTexture(GL_TEXTURE_2D, textures[tex].tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, oglformat, w, h, oglformat, GL_UNSIGNED_BYTE, texdata);
	
	/* calculate memory usage */
	textures[tex].memsize = w*h*4;
	if(mipmap)
	{
		while(w > 2 && h > 2)
		{
			w>>=1;
			h>>=1;
			textures[tex].memsize += w*h*4;
		}
	}
	
	memory_usage += textures[tex].memsize;
	mem_free(tmpdata);
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

/* simple uncompressed RGBA loaders */
int gfx_load_texture(const char *filename)
{
	int l = strlen(filename);
	IMAGE_INFO img;
	if(l < 3)
		return 0;
	if(gfx_load_png(&img, filename))
	{
		int id = gfx_load_texture_raw(img.width, img.height, img.format, img.data);
		mem_free(img.data);
		return id;
	}
	
	return 0;
}

int gfx_load_png(IMAGE_INFO *img, const char *filename)
{
	unsigned char *buffer;
	png_t png;
	
	/* open file for reading */
	png_init(0,0);

	if(png_open_file(&png, filename) != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", filename);
		return 0;
	}
	
	if(png.depth != 8 || (png.color_type != PNG_TRUECOLOR && png.color_type != PNG_TRUECOLOR_ALPHA))
	{
		dbg_msg("game/png", "invalid format. filename='%s'", filename);
		png_close_file(&png);
        return 0;
	}
		
	buffer = (unsigned char *)mem_alloc(png.width * png.height * png.bpp, 1);
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
	glfwCloseWindow();
	glfwTerminate();
}

void gfx_screenshot()
{
	do_screenshot = 1;
}

void gfx_swap()
{
	if(do_screenshot)
	{
		/* fetch image data */
		int y;
		int w = screen_width;
		int h = screen_height;
		unsigned char *pixel_data = (unsigned char *)mem_alloc(w*(h+1)*3, 1);
		unsigned char *temp_row = pixel_data+w*h*3;
		glReadPixels(0,0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);
		
		/* flip the pixel because opengl works from bottom left corner */
		for(y = 0; y < h/2; y++)
		{
			mem_copy(temp_row, pixel_data+y*w*3, w*3);
			mem_copy(pixel_data+y*w*3, pixel_data+(h-y-1)*w*3, w*3);
			mem_copy(pixel_data+(h-y-1)*w*3, temp_row,w*3);
		}
		
		/* find filename */
		{
			char wholepath[1024];
			char filename[128];
			static int index = 1;
			png_t png;

			for(; index < 1000; index++)
			{
				IOHANDLE io;
				sprintf(filename, "screenshots/screenshot%04d.png", index);
				engine_savepath(filename, wholepath, sizeof(wholepath));
				
				io = io_open(wholepath, IOFLAG_READ);
				if(io)
					io_close(io);
				else
					break;
			}
		
			/* save png */
			dbg_msg("client", "saved screenshot to '%s'", wholepath);
			png_open_file_write(&png, wholepath);
			png_set_data(&png, w, h, 8, PNG_TRUECOLOR, (unsigned char *)pixel_data);
			png_close_file(&png);
		}

		/* clean up */
		mem_free(pixel_data);
		do_screenshot = 0;
	}
	
	glfwSwapBuffers();
	glFinish();
	glfwPollEvents();
}

int gfx_screenwidth()
{
	return screen_width;
}

int gfx_screenheight()
{
	return screen_height;
}

void gfx_texture_set(int slot)
{
	dbg_assert(drawing == 0, "called gfx_texture_set within begin");
	if(slot == -1)
		glDisable(GL_TEXTURE_2D);
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[slot].tex);
	}
}

void gfx_clear(float r, float g, float b)
{
	glClearColor(r,g,b,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gfx_mapscreen(float tl_x, float tl_y, float br_x, float br_y)
{
	screen_x0 = tl_x;
	screen_y0 = tl_y;
	screen_x1 = br_x;
	screen_y1 = br_y;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(tl_x, br_x, br_y, tl_y, 1.0f, 10.f);
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
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(x, y, 0);
}


void gfx_quads_begin()
{
	dbg_assert(drawing == 0, "called quads_begin twice");
	drawing = DRAWING_QUADS;
	
	gfx_quads_setsubset(0,0,1,1);
	gfx_quads_setrotation(0);
	gfx_setcolor(1,1,1,1);
}

void gfx_quads_end()
{
	dbg_assert(drawing == DRAWING_QUADS, "called quads_end without begin");
	flush();
	drawing = 0;
}


void gfx_quads_setrotation(float angle)
{
	dbg_assert(drawing == DRAWING_QUADS, "called gfx_quads_setrotation without begin");
	rotation = angle;
}

void gfx_setcolorvertex(int i, float r, float g, float b, float a)
{
	dbg_assert(drawing != 0, "called gfx_quads_setcolorvertex without begin");
	color[i].r = r;
	color[i].g = g;
	color[i].b = b;
	color[i].a = a;
}

void gfx_setcolor(float r, float g, float b, float a)
{
	dbg_assert(drawing != 0, "called gfx_quads_setcolor without begin");
	gfx_setcolorvertex(0, r, g, b, a);
	gfx_setcolorvertex(1, r, g, b, a);
	gfx_setcolorvertex(2, r, g, b, a);
	gfx_setcolorvertex(3, r, g, b, a);
}

void gfx_quads_setsubset(float tl_u, float tl_v, float br_u, float br_v)
{
	dbg_assert(drawing == DRAWING_QUADS, "called gfx_quads_setsubset without begin");

	texture[0].u = tl_u;
	texture[0].v = tl_v;

	texture[1].u = br_u;
	texture[1].v = tl_v;

	texture[2].u = br_u;
	texture[2].v = br_v;

	texture[3].u = tl_u;
	texture[3].v = br_v;
}

static void rotate(VEC3 *center, VEC3 *point)
{
	float x = point->x - center->x;
	float y = point->y - center->y;
	point->x = x * cosf(rotation) - y * sinf(rotation) + center->x;
	point->y = x * sinf(rotation) + y * cosf(rotation) + center->y;
}

void gfx_quads_draw(float x, float y, float w, float h)
{
	gfx_quads_drawTL(x-w/2, y-h/2,w,h);
}

void gfx_quads_drawTL(float x, float y, float width, float height)
{
	VEC3 center;

	dbg_assert(drawing == DRAWING_QUADS, "called quads_draw without begin");

	center.x = x + width/2;
	center.y = y + height/2;
	center.z = 0;
	
	vertices[num_vertices].pos.x = x;
	vertices[num_vertices].pos.y = y;
	vertices[num_vertices].tex = texture[0];
	vertices[num_vertices].color = color[0];
	rotate(&center, &vertices[num_vertices].pos);

	vertices[num_vertices + 1].pos.x = x+width;
	vertices[num_vertices + 1].pos.y = y;
	vertices[num_vertices + 1].tex = texture[1];
	vertices[num_vertices + 1].color = color[1];
	rotate(&center, &vertices[num_vertices + 1].pos);

	vertices[num_vertices + 2].pos.x = x + width;
	vertices[num_vertices + 2].pos.y = y+height;
	vertices[num_vertices + 2].tex = texture[2];
	vertices[num_vertices + 2].color = color[2];
	rotate(&center, &vertices[num_vertices + 2].pos);

	vertices[num_vertices + 3].pos.x = x;
	vertices[num_vertices + 3].pos.y = y+height;
	vertices[num_vertices + 3].tex = texture[3];
	vertices[num_vertices + 3].color = color[3];
	rotate(&center, &vertices[num_vertices + 3].pos);
	
	draw_quad();
}

void gfx_quads_draw_freeform(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3)
{
	dbg_assert(drawing == DRAWING_QUADS, "called quads_draw_freeform without begin");
	
	vertices[num_vertices].pos.x = x0;
	vertices[num_vertices].pos.y = y0;
	vertices[num_vertices].tex = texture[0];
	vertices[num_vertices].color = color[0];

	vertices[num_vertices + 1].pos.x = x1;
	vertices[num_vertices + 1].pos.y = y1;
	vertices[num_vertices + 1].tex = texture[1];
	vertices[num_vertices + 1].color = color[1];

	vertices[num_vertices + 2].pos.x = x3;
	vertices[num_vertices + 2].pos.y = y3;
	vertices[num_vertices + 2].tex = texture[3];
	vertices[num_vertices + 2].color = color[3];

	vertices[num_vertices + 3].pos.x = x2;
	vertices[num_vertices + 3].pos.y = y2;
	vertices[num_vertices + 3].tex = texture[2];
	vertices[num_vertices + 3].color = color[2];
	
	draw_quad();
}

void gfx_quads_text(float x, float y, float size, const char *text)
{
	float startx = x;

	gfx_quads_begin();

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

static int word_length(const char *text)
{
	int s = 1;
	while(1)
	{
		if(*text == 0)
			return s-1;
		if(*text == '\n' || *text == '\t' || *text == ' ')
			return s;
		text++;
		s++;
	}
}

static float pretty_r=1;
static float pretty_g=1;
static float pretty_b=1;
static float pretty_a=1;

FONT *gfx_font_set;

float gfx_text_raw(void *font_set_v, float x, float y, float size, const char *text, int length)
{
    FONT_SET *font_set = font_set_v;
    float fake_to_screen_x = (screen_width/(screen_x1-screen_x0));
    float fake_to_screen_y = (screen_height/(screen_y1-screen_y0));

    FONT *font;

    // to correct coords, convert to screen coords, round, and convert back
    int actual_x = x * fake_to_screen_x;
    int actual_y = y * fake_to_screen_y;
    x = actual_x / fake_to_screen_x;
    y = actual_y / fake_to_screen_y;

    // same with size
    int actual_size = size * fake_to_screen_y;
    size = actual_size / fake_to_screen_y;

    font = font_set_pick(font_set, actual_size);

    int i;
    float draw_x;

    if (length < 0)
        length = strlen(text);

    for (i = 0; i < 2; i++)
    {
        const unsigned char *c = (unsigned char *)text;
        int to_render = length;
        draw_x = x;

        if (i == 0)
            gfx_texture_set(font->outline_texture);
        else
            gfx_texture_set(font->text_texture);

        gfx_quads_begin();
        if (i == 0)
            gfx_setcolor(0.0f, 0.0f, 0.0f, 0.3f);
        else
            gfx_setcolor(pretty_r, pretty_g, pretty_b, pretty_a);
        
        while (to_render--)
        {
            float tex_x0, tex_y0, tex_x1, tex_y1;
            float width, height;
            float x_offset, y_offset, x_advance;

            float advance;

            font_character_info(font, *c, &tex_x0, &tex_y0, &tex_x1, &tex_y1, &width, &height, &x_offset, &y_offset, &x_advance);

            gfx_quads_setsubset(tex_x0, tex_y0, tex_x1, tex_y1);

            gfx_quads_drawTL(draw_x+x_offset*size, y+y_offset*size, width*size, height*size);

            advance = x_advance + font_kerning(font, *c, *(c+1));

            draw_x += advance*size;

            c++;
        }

        gfx_quads_end();
    }

    return draw_x;
}

void gfx_text(void *font_set_v, float x, float y, float size, const char *text, int max_width)
{
	if(max_width == -1)
		gfx_text_raw(font_set_v, x, y, size, text, -1);
	else
	{
		float startx = x;
		while(*text)
		{
			int wlen = word_length(text);
			float w = gfx_text_width(font_set_v, size, text, wlen);
			if(x+w-startx > max_width)
			{
				y += size;
				x = startx;
			}
			
			x = gfx_text_raw(font_set_v, x, y, size, text, wlen);
			
			text += wlen;
		}
	}
    gfx_text_raw(font_set_v, x, y, size, text, -1);
}

float gfx_text_width(void *font_set_v, float size, const char *text, int length)
{
    FONT_SET *font_set = font_set_v;
    FONT *font;
    float fake_to_screen_y = (screen_height/(screen_y1-screen_y0));

    int actual_size = size * fake_to_screen_y;
    size = actual_size / fake_to_screen_y;

    font = font_set_pick(font_set, actual_size);

    return font_text_width(font, text, size, length);
}

void gfx_pretty_text_color(float r, float g, float b, float a)
{
	pretty_r = r;
	pretty_g = g;
	pretty_b = b;
	pretty_a = a;
}

float gfx_pretty_text_raw(float x, float y, float size, const char *text_, int length)
{
	const unsigned char *text = (unsigned char *)text_;
	const float spacing = 0.05f;
	gfx_texture_set(current_font->font_texture);
	gfx_quads_begin();
	gfx_setcolor(pretty_r, pretty_g, pretty_b, pretty_a);
	
	if(length < 0)
		length = strlen(text_);
	
	while(length)
	{
		const int c = *text;
		const float width = current_font->m_CharEndTable[c] - current_font->m_CharStartTable[c];
		double x_nudge = 0;

		text++;

		x -= size * current_font->m_CharStartTable[c];
		
		gfx_quads_setsubset(
			(c%16)/16.0f, /* startx */
			(c/16)/16.0f, /* starty */
			(c%16)/16.0f+1.0f/16.0f, /* endx */
			(c/16)/16.0f+1.0f/16.0f); /* endy */

		gfx_quads_drawTL(x, y, size, size);

		if(length > 1 && text[1])
			x_nudge = extra_kerning[text[0] + text[1] * 256];

		x += (width + current_font->m_CharStartTable[c] + spacing + x_nudge) * size;
		length--;
	}

	gfx_quads_end();
	
	return x;
}

void gfx_pretty_text(float x, float y, float size, const char *text, int max_width)
{
    //gfx_text(gfx_font_set, x, y, 0.8*size, text, max_width);
    //return;
	if(max_width == -1)
		gfx_pretty_text_raw(x, y, size, text, -1);
	else
	{
		float startx = x;
		while(*text)
		{
			int wlen = word_length(text);
			float w = gfx_pretty_text_width(size, text, wlen);
			if(x+w-startx > max_width)
			{
				y += size-2;
				x = startx;
			}
			
			x = gfx_pretty_text_raw(x, y, size, text, wlen);
			
			text += wlen;
		}
	}
}

float gfx_pretty_text_width(float size, const char *text_, int length)
{
    //return gfx_text_width(gfx_font_set, 0.8*size, text_, length);

	const float spacing = 0.05f;
	float w = 0.0f;
	const unsigned char *text = (unsigned char *)text_;
	const unsigned char *stop;

	if (length == -1)
		stop = text + strlen((char*)text);
	else
		stop = text + length;

	while (text < stop)
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



void gfx_lines_begin()
{
	dbg_assert(drawing == 0, "called begin twice");
	drawing = DRAWING_LINES;
	gfx_setcolor(1,1,1,1);
}

void gfx_lines_end()
{
	dbg_assert(drawing == DRAWING_LINES, "called end without begin");
	flush();
	drawing = 0;
}

void gfx_lines_draw(float x0, float y0, float x1, float y1)
{
	dbg_assert(drawing == DRAWING_LINES, "called draw without begin");
	
	vertices[num_vertices].pos.x = x0;
	vertices[num_vertices].pos.y = y0;
	vertices[num_vertices].tex = texture[0];
	vertices[num_vertices].color = color[0];

	vertices[num_vertices + 1].pos.x = x1;
	vertices[num_vertices + 1].pos.y = y1;
	vertices[num_vertices + 1].tex = texture[1];
	vertices[num_vertices + 1].color = color[1];
	
	draw_line();
}

void gfx_clip_enable(int x, int y, int w, int h)
{
	glScissor(x, gfx_screenheight()-(y+h), w, h);
	glEnable(GL_SCISSOR_TEST);
}

void gfx_clip_disable()
{
	glDisable(GL_SCISSOR_TEST);
}
