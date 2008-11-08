/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */

#include <base/detect.h>

#include "SDL.h"

#ifdef CONF_FAMILY_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#ifdef CONF_PLATFORM_MACOSX
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

#include <engine/e_client_interface.h>
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
#define GL_COMPRESSED_ALPHA_ARB 0x84E9

#define TEXTURE_MAX_ANISOTROPY_EXT 0x84FE

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

static int no_gfx = 0;

static COLOR color[4];
static TEXCOORD texture[4];

static int do_screenshot = 0;
static int render_enable = 1;

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
	MAX_TEXTURES = 1024*4
};

static TEXTURE textures[MAX_TEXTURES];
static int first_free_texture;
static int memory_usage = 0;

static SDL_Surface *screen_surface;

static const unsigned char null_texture_data[] = {
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
	0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
};

static void flush()
{
	if(num_vertices == 0)
		return;
		
	if(no_gfx)
	{
		num_vertices = 0;
		return;
	}
	
		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
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
	
	if(render_enable)
	{
		if(drawing == DRAWING_QUADS)
			glDrawArrays(GL_QUADS, 0, num_vertices);
		else if(drawing == DRAWING_LINES)
			glDrawArrays(GL_LINES, 0, num_vertices);
	}
	
	/* Reset pointer */
	num_vertices = 0;
}

static void add_vertices(int count)
{
	num_vertices += count;
	if((num_vertices + count) >= vertex_buffer_size)
		flush();
}

int gfx_init()
{
	int i;

	screen_width = config.gfx_screen_width;
	screen_height = config.gfx_screen_height;


	if(config.dbg_stress)
	{
		screen_width = 320;
		screen_height = 240;
		no_gfx = 1;
	}

	if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
	{
        dbg_msg("gfx", "Unable to init SDL: %s\n", SDL_GetError());
        return -1;
    }
	
    atexit(SDL_Quit);

	if(!no_gfx)
	{
		const SDL_VideoInfo *info;
		int flags = SDL_OPENGL;
		
		info = SDL_GetVideoInfo();

		/* set flags */
		flags  = SDL_OPENGL;
		flags |= SDL_GL_DOUBLEBUFFER;
		flags |= SDL_HWPALETTE;
		flags |= SDL_RESIZABLE;

		if(info->hw_available)
			flags |= SDL_HWSURFACE;
		else
			flags |= SDL_SWSURFACE;

		if(info->blit_hw)
			flags |= SDL_HWACCEL;

		if(config.gfx_fullscreen)
			flags |= SDL_FULLSCREEN;

		/* set gl attributes */
		if(config.gfx_fsaa_samples)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.gfx_fsaa_samples);
		}

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		/* set caption */
		SDL_WM_SetCaption("Teeworlds", "Teeworlds");
		
		/* create window */
		screen_surface = SDL_SetVideoMode(screen_width, screen_height, 0, flags);
		if(screen_surface == NULL)
		{
	        dbg_msg("gfx", "Unable to set video mode: %s\n", SDL_GetError());
    	    return -1;
		}
	}
	
	/* Init vertices */
	if (vertices)
		mem_free(vertices);
	vertices = (VERTEX*)mem_alloc(sizeof(VERTEX) * vertex_buffer_size, 1);
	num_vertices = 0;


	/*
	dbg_msg("gfx", "OpenGL version %d.%d.%d", context.version_major(),
											  context.version_minor(),
											  context.version_rev());*/

	
	/* Set all z to -5.0f */
	for (i = 0; i < vertex_buffer_size; i++)
		vertices[i].pos.z = -5.0f;

	/* init textures */
	first_free_texture = 0;
	for(i = 0; i < MAX_TEXTURES; i++)
		textures[i].next = i+1;
	textures[MAX_TEXTURES-1].next = -1;
	
	if(!no_gfx)
	{
		SDL_ShowCursor(0);
		gfx_mapscreen(0,0,config.gfx_screen_width, config.gfx_screen_height);

		/* set some default settings */	
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glAlphaFunc(GL_GREATER, 0);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(0);

		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	/* init input */
	inp_init();
	
	/* create null texture, will get id=0 */
	gfx_load_texture_raw(4,4,IMG_RGBA,null_texture_data,IMG_RGBA,TEXLOAD_NORESAMPLE);

	/* perform some tests */
	/* pixeltest_dotesting(); */
	
	/*if(config.dbg_stress)
		gfx_minimize();*/

	/* set vsync as needed */
	gfx_set_vsync(config.gfx_vsync);
	return 1;
}

float gfx_screenaspect()
{
    return gfx_screenwidth()/(float)gfx_screenheight();
}

int gfx_window_active()
{
	return SDL_GetAppState()&SDL_APPINPUTFOCUS;
}

int gfx_window_open()
{
	return SDL_GetAppState()&SDL_APPACTIVE;
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
	int num_modes = 0;
	SDL_Rect **modes;

	if(config.gfx_display_all_modes)
	{
		int count = sizeof(fakemodes)/sizeof(VIDEO_MODE);
		mem_copy(list, fakemodes, sizeof(fakemodes));
		if(maxcount < count)
			count = maxcount;
		return count;
	}
	
	/* TODO: fix this code on osx or windows */
		
	modes = SDL_ListModes(NULL, SDL_OPENGL|SDL_GL_DOUBLEBUFFER|SDL_FULLSCREEN);
	if(modes == NULL)
	{
		/* no modes */
	}
	else if(modes == (SDL_Rect**)-1)
	{
		/* all modes */
	}
	else
	{
		int i;
		for(i = 0; modes[i]; ++i)
		{
			if(num_modes == maxcount)
				break;
			list[num_modes].width = modes[i]->w;
			list[num_modes].height = modes[i]->h;
			list[num_modes].red = 8;
			list[num_modes].green = 8;
			list[num_modes].blue = 8;
			num_modes++;
		}
	}
	
	return 1; /* TODO: SDL*/
}

void gfx_set_vsync(int val)
{
	/* TODO: SDL*/
}

int gfx_unload_texture(int index)
{
	if(index < 0)
		return 0;
	glDeleteTextures(1, &textures[index].tex);
	textures[index].next = first_free_texture;
	memory_usage -= textures[index].memsize;
	first_free_texture = index;
	return 0;
}

void gfx_blend_none()
{
	if(no_gfx) return;
	glDisable(GL_BLEND);
}

void gfx_blend_normal()
{
	if(no_gfx) return;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gfx_blend_additive()
{
	if(no_gfx) return;
	glEnable(GL_BLEND);
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

int gfx_load_texture_raw(int w, int h, int format, const void *data, int store_format, int flags)
{
	int mipmap = 1;
	unsigned char *texdata = (unsigned char *)data;
	unsigned char *tmpdata = 0;
	int oglformat = 0;
	int store_oglformat = 0;
	int tex = 0;
	
	/* don't waste memory on texture if we are stress testing */
	if(config.dbg_stress || no_gfx)
		return -1;
	
	/* grab texture */
	tex = first_free_texture;
	first_free_texture = textures[tex].next;
	textures[tex].next = -1;
	
	/* resample if needed */
	if(!(flags&TEXLOAD_NORESAMPLE) && config.gfx_texture_quality==0)
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
	
	oglformat = GL_RGBA;
	if(format == IMG_RGB)
		oglformat = GL_RGB;
	else if(format == IMG_ALPHA)
		oglformat = GL_ALPHA;
	
	/* upload texture */
	if(config.gfx_texture_compression)
	{
		store_oglformat = GL_COMPRESSED_RGBA_ARB;
		if(store_format == IMG_RGB)
			store_oglformat = GL_COMPRESSED_RGB_ARB;
		else if(store_format == IMG_ALPHA)
			store_oglformat = GL_COMPRESSED_ALPHA_ARB;
	}
	else
	{
		store_oglformat = GL_RGBA;
		if(store_format == IMG_RGB)
			store_oglformat = GL_RGB;
		else if(store_format == IMG_ALPHA)
			store_oglformat = GL_ALPHA;
	}
		
	glGenTextures(1, &textures[tex].tex);
	glBindTexture(GL_TEXTURE_2D, textures[tex].tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D, store_oglformat, w, h, oglformat, GL_UNSIGNED_BYTE, texdata);
	
	/* calculate memory usage */
	{
		int pixel_size = 4;
		if(store_format == IMG_RGB)
			pixel_size = 3;
		else if(store_format == IMG_ALPHA)
			pixel_size = 1;

		textures[tex].memsize = w*h*pixel_size;
		if(mipmap)
		{
			while(w > 2 && h > 2)
			{
				w>>=1;
				h>>=1;
				textures[tex].memsize += w*h*pixel_size;
			}
		}
	}
	
	memory_usage += textures[tex].memsize;
	mem_free(tmpdata);
	return tex;
}

/* simple uncompressed RGBA loaders */
int gfx_load_texture(const char *filename, int store_format, int flags)
{
	int l = strlen(filename);
	int id;
	IMAGE_INFO img;
	
	if(l < 3)
		return 0;
	if(gfx_load_png(&img, filename))
	{
		if (store_format == IMG_AUTO)
			store_format = img.format;

		id = gfx_load_texture_raw(img.width, img.height, img.format, img.data, store_format, flags);
		mem_free(img.data);
		return id;
	}
	
	return 0;
}

int gfx_load_png(IMAGE_INFO *img, const char *filename)
{
	char completefilename[512];
	unsigned char *buffer;
	png_t png;
	
	/* open file for reading */
	png_init(0,0);

	engine_getpath(completefilename, sizeof(completefilename), filename, IOFLAG_READ);
	
	if(png_open_file(&png, completefilename) != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", completefilename);
		return 0;
	}
	
	if(png.depth != 8 || (png.color_type != PNG_TRUECOLOR && png.color_type != PNG_TRUECOLOR_ALPHA))
	{
		dbg_msg("game/png", "invalid format. filename='%s'", completefilename);
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

	/* TODO: SDL, is this correct? */
	SDL_Quit();
}

void gfx_screenshot()
{
	do_screenshot = 1;
}

void gfx_swap()
{
	if(do_screenshot)
	{
		/* find filename */
		char filename[128];
		static int index = 1;

		for(; index < 1000; index++)
		{
			IOHANDLE io;
			str_format(filename, sizeof(filename), "screenshots/screenshot%04d.png", index);
			io = engine_openfile(filename, IOFLAG_READ);
			if(io)
				io_close(io);
			else
				break;
		}

		gfx_screenshot_direct(filename);
	
		do_screenshot = 0;	
	}
	
	{
		static PERFORMACE_INFO pscope = {"glfwSwapBuffers", 0};
		perf_start(&pscope);
		SDL_GL_SwapBuffers();
		perf_end();
	}
	
	if(render_enable && config.gfx_finish)
		glFinish();
}

void gfx_screenshot_direct(const char *filename)
{
	/* fetch image data */
	int y;
	int w = screen_width;
	int h = screen_height;
	unsigned char *pixel_data = (unsigned char *)mem_alloc(w*(h+1)*4, 1);
	unsigned char *temp_row = pixel_data+w*h*4;
	glReadPixels(0,0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
	
	/* flip the pixel because opengl works from bottom left corner */
	for(y = 0; y < h/2; y++)
	{
		mem_copy(temp_row, pixel_data+y*w*4, w*4);
		mem_copy(pixel_data+y*w*4, pixel_data+(h-y-1)*w*4, w*4);
		mem_copy(pixel_data+(h-y-1)*w*4, temp_row,w*4);
	}
	
	/* find filename */
	{
		char wholepath[1024];
		png_t png;

		engine_savepath(filename, wholepath, sizeof(wholepath));
	
		/* save png */
		dbg_msg("client", "saved screenshot to '%s'", wholepath);
		png_open_file_write(&png, wholepath);
		png_set_data(&png, w, h, 8, PNG_TRUECOLOR_ALPHA, (unsigned char *)pixel_data);
		png_close_file(&png);
	}

	/* clean up */
	mem_free(pixel_data);
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
	if(no_gfx) return;
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
	if(no_gfx) return;
	glClearColor(r,g,b,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void gfx_mapscreen(float tl_x, float tl_y, float br_x, float br_y)
{
	screen_x0 = tl_x;
	screen_y0 = tl_y;
	screen_x1 = br_x;
	screen_y1 = br_y;
	if(no_gfx) return;
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

	texture[0].u = tl_u;	texture[1].u = br_u;
	texture[0].v = tl_v;	texture[1].v = tl_v;

	texture[3].u = tl_u;	texture[2].u = br_u;
	texture[3].v = br_v;	texture[2].v = br_v;
}

void gfx_quads_setsubset_free(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
	texture[0].u = x0; texture[0].v = y0;
	texture[1].u = x1; texture[1].v = y1;
	texture[2].u = x2; texture[2].v = y2;
	texture[3].u = x3; texture[3].v = y3;
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
	
	add_vertices(4);
}

void gfx_quads_draw_freeform(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
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
	
	add_vertices(4);
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
	
	add_vertices(2);
}

void gfx_clip_enable(int x, int y, int w, int h)
{
	if(no_gfx) return;
	glScissor(x, gfx_screenheight()-(y+h), w, h);
	glEnable(GL_SCISSOR_TEST);
}

void gfx_clip_disable()
{
	if(no_gfx) return;
	glDisable(GL_SCISSOR_TEST);
}

void gfx_minimize()
{
	SDL_WM_IconifyWindow();
}

void gfx_maximize()
{
	/* TODO: SDL */
}
