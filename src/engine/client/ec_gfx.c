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


#if 0
typedef struct { unsigned char r, g, b, a; } PIXEL;

static PIXEL *pixeltest_data = 0;
static PIXEL *pixeltest_tex = 0;
static int pixeltest_w = 64;
static int pixeltest_h = 64;

static int pixeltest_analysedata(int fault)
{
	int x,y,c;
	PIXEL *pp = pixeltest_data;
	for(y = 0; y < pixeltest_h; y++)
		for(x = 0; x < pixeltest_w; x++, pp++, c++)
		{
			PIXEL p = *pp;
			if(p.r > fault)
				return p.r;
		}
	return 0;
}

static void pixeltest_filltile(int id)
{
	int x, y;
	for(y = 0; y < 1024; y++)
		for(x = 0; x < 1024; x++)
		{
			int this_id = (x/64) + (y/64)*16;
			if(this_id == id)
			{
				if(x%64 == 0 || x%64 == 63 || y%64 == 0 || y%64 == 63)
				{
					pixeltest_tex[y*1024+x].r = 0;
					pixeltest_tex[y*1024+x].g = 0;
					pixeltest_tex[y*1024+x].b = 255;
					pixeltest_tex[y*1024+x].a = 255;
				}
				else
				{
					pixeltest_tex[y*1024+x].r = 0;
					pixeltest_tex[y*1024+x].g = 255;
					pixeltest_tex[y*1024+x].b = 0;
					pixeltest_tex[y*1024+x].a = 255;
				}
			}
			else
			{
				pixeltest_tex[y*1024+x].r = 255;
				pixeltest_tex[y*1024+x].g = 0;
				pixeltest_tex[y*1024+x].b = 0;
				pixeltest_tex[y*1024+x].a = 255;
			}
		}
	
}

static void pixeltest_dotesting()
{
	int result, texture_id;
	float start_tx=0;
	float start_ty=0;
	float end_tx=1;
	float end_ty=1;
	int tile_id = 8*16+3;
	int t;
	float frac = 0;
	pixeltest_data = (PIXEL *)mem_alloc(pixeltest_w*(pixeltest_h)*4, 1);
	pixeltest_tex = (PIXEL *)mem_alloc(1024*(1024)*4, 1);
	
	/*pixeltest_filltile(0);*/
	/*pixeltest_filltile(0xff);*/
	
	dbg_msg("", "%f", 1.0f/1024.0f);

	gfx_mapscreen(0,0,screen_width, screen_height);
	
	/*
		start = 0.5f/1024.0f;
		end = 1.0f-(0.5f/1024.0f)
	*/

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	for(t = 0; t < 256; t++)
	{
		/*t = 41;*/
		
		pixeltest_filltile(tile_id);
		texture_id = gfx_load_texture_raw(1024,1024,IMG_RGBA,pixeltest_tex,IMG_RGBA,TEXLOAD_NORESAMPLE);

		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		gfx_texture_set(texture_id);
		gfx_quads_begin();
		/*
			gfx_quads_setsubset(start_tx,start_ty,0.001f,0.001f);
			gfx_quads_drawTL(pixeltest_w/2, pixeltest_h/2, pixeltest_w, pixeltest_h);
		*/ /*0.000977*/
		/*
			gfx_quads_setsubset(0.999f,0.999f,end_tx,end_ty);
			gfx_quads_drawTL(0,0,pixeltest_w/2, pixeltest_h/2);
		*/
		
		{
			float tile_pixelsize = 1024/32.0f;
			float scale = pixeltest_w*((t+1)/250.0f);
			float final_tilesize = scale/(screen_width) * gfx_screenwidth();
			float final_tilesize_scale = final_tilesize/tile_pixelsize;
			/*float frac = (1.0f/texsize);// * (1/final_tilesize_scale);*/
			
			float tx0 = 0.5f/1024.0f + (tile_id%16)*64.0f / (1024.0f) + frac;
			float ty0 = 0.5f/1024.0f + (tile_id/16)*64.0f / (1024.0f) + frac;
			float tx1 = 0.5f/1024.0f + (((tile_id%16) + 1)*64.0f-1) / (1024.0f) - frac;
			float ty1 = 0.5f/1024.0f + (((tile_id/16) + 1)*64.0f-1) / (1024.0f) - frac;
			dbg_msg("", "scale: %f %f", scale, final_tilesize_scale);
			gfx_quads_setsubset(tx0,ty0,tx1,ty1);
			/*gfx_quads_setsubset(0,0,1,1);*/
			gfx_quads_drawTL(pixeltest_w/4+(t+1)/250.0f,pixeltest_h/4,pixeltest_w/2,pixeltest_w/2);
		}
		
		gfx_quads_end();

		glReadPixels(0,screen_height-pixeltest_h, pixeltest_w, pixeltest_h, GL_RGBA, GL_UNSIGNED_BYTE, pixeltest_data);
		
		result = pixeltest_analysedata(2);
		if(result)
		{
			frac += 0.000001f;
			dbg_msg("", "fail on %d with %d newfrac = %f", t, result, frac);
			t--;
			
			/*start_tx += 0.000001f; start_ty += 0.000001f;
			dbg_msg("", "found stuff %f %f %d %f %f", start_tx, start_ty, result, 0.5f/1024, 0.5f/1024);

			end_tx -= 0.000001f; end_ty -= 0.000001f;
			dbg_msg("", "found stuff %f %d %f", end_tx, result, 1.0f-0.5f/1024);*/
		}
		/*else
			dbg_msg("", "pass on %d", t);*/

		glfwSwapBuffers();
		glfwPollEvents();

		if(glfwGetKey(KEY_ESC))
			exit(1);
		
		gfx_unload_texture(texture_id);
	}

	dbg_msg("", "frac=%f  %f of a pixel", frac, frac/(1.0f/1024.0f));
	exit(1);
}
#endif

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

int gfx_init()
{
	int i;

	screen_width = config.gfx_screen_width;
	screen_height = config.gfx_screen_height;


	if(config.dbg_stress)
	{
		screen_width = 320;
		screen_height = 240;
	}

	if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
	{
        dbg_msg("gfx", "Unable to init SDL: %s\n", SDL_GetError());
        return -1;
    }
	
    atexit(SDL_Quit);

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
	
	SDL_ShowCursor(0);
	
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
	
	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(0);
	
	gfx_mask_op(MASK_NONE, 0);
	/*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
	
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
	gfx_load_texture_raw(4,4,IMG_RGBA,null_texture_data,IMG_RGBA,TEXLOAD_NORESAMPLE);

	/* perform some tests */
	/* pixeltest_dotesting(); */

	/* set vsync as needed */
	gfx_set_vsync(config.gfx_vsync);
	return 1;
}

float gfx_screenaspect()
{
    return gfx_screenwidth()/(float)gfx_screenheight();
}

void gfx_clear_mask(int fill)
{
	int i;
	glGetIntegerv(GL_DEPTH_WRITEMASK, (GLint*)&i);
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
	glDisable(GL_BLEND);
}


void gfx_blend_normal()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gfx_blend_additive()
{
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
	if(config.dbg_stress)
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

static int64 next_frame = 0;
static int record = 0;

void gfx_swap()
{
	if(record)
	{
		int w = screen_width;
		int h = screen_height;
		unsigned char *pixel_data = (unsigned char *)mem_alloc(w*(h+1)*3, 1);
		/*unsigned char *temp_row = pixel_data+w*h*3;*/
		glReadPixels(0,0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);


		{
			IOHANDLE io;
			char filename[128];
			char header[18] = {0};
			str_format(filename, sizeof(filename), "capture/frame%04d.tga", record);
			record++;
			
			io = engine_openfile(filename, IOFLAG_WRITE);
			if(io)
			{
				header[2] = 2; /* rgb */
				header[12] = w&255; /* width */
				header[13] = w>>8; 
				header[14] = h&255; /* height */
				header[15] = h>>8; 
				header[16] = 24;
				
				io_write(io, header, sizeof(header));
				io_write(io, pixel_data, w*h*3);
				
				io_close(io);
			}
		}
		

		/* clean up */
		mem_free(pixel_data);

		if(next_frame)
			next_frame += time_freq()/30;
		else
			next_frame = time_get() + time_freq()/30;

		while(time_get() < next_frame)
			(void)0;
	}
	
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
	glClearColor(r,g,b,0.0f);
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

void gfx_quads_setsubset_free(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3)
{
	texture[0].u = x0;
	texture[0].v = y0;

	texture[1].u = x1;
	texture[1].v = y1;

	texture[2].u = x2;
	texture[2].v = y2;

	texture[3].u = x3;
	texture[3].v = y3;
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

static float text_r=1;
static float text_g=1;
static float text_b=1;
static float text_a=1;

static FONT_SET *default_font_set = 0;
void gfx_text_set_default_font(void *font)
{
	default_font_set = (FONT_SET *)font;
}


void gfx_text_set_cursor(TEXT_CURSOR *cursor, float x, float y, float font_size, int flags)
{
	mem_zero(cursor, sizeof(*cursor));
	cursor->font_size = font_size;
	cursor->start_x = x;
	cursor->start_y = y;
	cursor->x = x;
	cursor->y = y;
	cursor->line_count = 1;
	cursor->line_width = -1;
	cursor->flags = flags;
	cursor->charcount = 0;
}

void gfx_text_ex(TEXT_CURSOR *cursor, const char *text, int length)
{
	FONT_SET *font_set = cursor->font_set;
	float fake_to_screen_x = (screen_width/(screen_x1-screen_x0));
	float fake_to_screen_y = (screen_height/(screen_y1-screen_y0));

	FONT *font;
	int actual_size;
	int i;
	float draw_x, draw_y;
	const char *end;

	float size = cursor->font_size;

	/* to correct coords, convert to screen coords, round, and convert back */
	int actual_x = cursor->x * fake_to_screen_x;
	int actual_y = cursor->y * fake_to_screen_y;
	cursor->x = actual_x / fake_to_screen_x;
	cursor->y = actual_y / fake_to_screen_y;

	/* same with size */
	actual_size = size * fake_to_screen_y;
	size = actual_size / fake_to_screen_y;

	if(!font_set)
		font_set = default_font_set;

	font = font_set_pick(font_set, actual_size);

	if (length < 0)
		length = strlen(text);
		
	end = text + length;

	/* if we don't want to render, we can just skip the first outline pass */
	i = 1;
	if(cursor->flags&TEXTFLAG_RENDER)
		i = 0;

	for(;i < 2; i++)
	{
		const unsigned char *current = (unsigned char *)text;
		int to_render = length;
		draw_x = cursor->x;
		draw_y = cursor->y;

		if(cursor->flags&TEXTFLAG_RENDER)
		{
			if (i == 0)
				gfx_texture_set(font->outline_texture);
			else
				gfx_texture_set(font->text_texture);

			gfx_quads_begin();
			if (i == 0)
				gfx_setcolor(0.0f, 0.0f, 0.0f, 0.3f*text_a);
			else
				gfx_setcolor(text_r, text_g, text_b, text_a);
		}

		while(to_render > 0)
		{
			int new_line = 0;
			int this_batch = to_render;
			if(cursor->line_width > 0 && !(cursor->flags&TEXTFLAG_STOP_AT_END))
			{
				int wlen = word_length((char *)current);
				TEXT_CURSOR compare = *cursor;
				compare.x = draw_x;
				compare.y = draw_y;
				compare.flags &= ~TEXTFLAG_RENDER;
				compare.line_width = -1;
				gfx_text_ex(&compare, text, wlen);
				
				if(compare.x-draw_x > cursor->line_width)
				{
					/* word can't be fitted in one line, cut it */
					TEXT_CURSOR cutter = *cursor;
					cutter.charcount = 0;
					cutter.x = draw_x;
					cutter.y = draw_y;
					cutter.flags &= ~TEXTFLAG_RENDER;
					cutter.flags |= TEXTFLAG_STOP_AT_END;
					
					gfx_text_ex(&cutter, (const char *)current, wlen);
					wlen = cutter.charcount;
					new_line = 1;
					
					if(wlen <= 3) /* if we can't place 3 chars of the word on this line, take the next */
						wlen = 0;
				}
				else if(compare.x-cursor->start_x > cursor->line_width)
				{
					new_line = 1;
					wlen = 0;
				}
				
				this_batch = wlen;
			}
			
			if((const char *)current+this_batch > end)
				this_batch = (const char *)end-(const char *)current;
			
			to_render -= this_batch;

			while(this_batch-- > 0)
			{
				float tex_x0, tex_y0, tex_x1, tex_y1;
				float width, height;
				float x_offset, y_offset, x_advance;
				float advance;

				if(*current == '\n')
				{
					draw_x = cursor->start_x;
					draw_y += size;
					draw_x = (int)(draw_x * fake_to_screen_x) / fake_to_screen_x; /* realign */
					draw_y = (int)(draw_y * fake_to_screen_y) / fake_to_screen_y;
					current++;
					continue;
				}

				font_character_info(font, *current, &tex_x0, &tex_y0, &tex_x1, &tex_y1, &width, &height, &x_offset, &y_offset, &x_advance);

				if(cursor->flags&TEXTFLAG_RENDER)
				{
					gfx_quads_setsubset(tex_x0, tex_y0, tex_x1, tex_y1);
					gfx_quads_drawTL(draw_x+x_offset*size, draw_y+y_offset*size, width*size, height*size);
				}

				advance = x_advance + font_kerning(font, *current, *(current+1));
								
				if(cursor->flags&TEXTFLAG_STOP_AT_END && draw_x+advance*size-cursor->start_x > cursor->line_width)
				{
					/* we hit the end of the line, no more to render or count */
					to_render = 0;
					break;
				}

				draw_x += advance*size;
				cursor->charcount++;
				current++;
			}
			
			if(new_line)
			{
				draw_x = cursor->start_x;
				draw_y += size;
				draw_x = (int)(draw_x * fake_to_screen_x) / fake_to_screen_x; /* realign */
				draw_y = (int)(draw_y * fake_to_screen_y) / fake_to_screen_y;				
			}
		}

		if(cursor->flags&TEXTFLAG_RENDER)
			gfx_quads_end();
	}

	cursor->x = draw_x;
	cursor->y = draw_y;
}

void gfx_text(void *font_set_v, float x, float y, float size, const char *text, int max_width)
{
	TEXT_CURSOR cursor;
	gfx_text_set_cursor(&cursor, x, y, size, TEXTFLAG_RENDER);
	cursor.line_width = max_width;
	gfx_text_ex(&cursor, text, -1);
}

float gfx_text_width(void *font_set_v, float size, const char *text, int length)
{
	TEXT_CURSOR cursor;
	gfx_text_set_cursor(&cursor, 0, 0, size, 0);
	gfx_text_ex(&cursor, text, length);
	return cursor.x;
}

void gfx_text_color(float r, float g, float b, float a)
{
	text_r = r;
	text_g = g;
	text_b = b;
	text_a = a;
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

void gfx_minimize()
{
	/* TODO: SDL */
}

void gfx_maximize()
{
	/* TODO: SDL */
}
