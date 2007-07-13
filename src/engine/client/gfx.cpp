#include <baselib/opengl.h>
#include <baselib/vmath.h>
#include <baselib/stream/file.h>

#include <engine/interface.h>

#include "pnglite/pnglite.h"

#include <string.h>


using namespace baselib;

static opengl::context context;

struct custom_vertex
{
	vec3 pos;
	vec2 tex;
	vec4 color;
};

const int vertexBufferSize = 2048;
//static custom_vertex vertices[4];
static custom_vertex* g_pVertices = 0;
static int g_iVertexStart = 0;
static int g_iVertexEnd = 0;
static vec4 g_QuadColor[4];
static vec2 g_QuadTexture[4];

static opengl::vertex_buffer vertex_buffer;
//static int screen_width = 800;
//static int screen_height = 600;
static int screen_width = 1024;
static int screen_height = 768;
static float rotation = 0;
static int quads_drawing = 0;


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
	if (!_bflush && ((g_iVertexEnd + 4) < vertexBufferSize))
	{
		// Just add
		g_iVertexEnd += 4;
	}
	else if (g_iVertexEnd)
	{
		if (!_bflush)
			g_iVertexEnd += 4;
		if(GLEW_VERSION_2_0)
		{
			// set the data
			vertex_buffer.data(g_pVertices, vertexBufferSize * sizeof(custom_vertex), GL_DYNAMIC_DRAW);
			opengl::stream_vertex(&vertex_buffer, 3, GL_FLOAT, sizeof(custom_vertex), 0);
			opengl::stream_texcoord(&vertex_buffer, 0, 2, GL_FLOAT,
					sizeof(custom_vertex),
					sizeof(vec3));
			opengl::stream_color(&vertex_buffer, 4, GL_FLOAT,
					sizeof(custom_vertex),
					sizeof(vec3)+sizeof(vec2));		
			opengl::draw_arrays(GL_QUADS, 0, g_iVertexEnd);
		}
		else
		{
			glVertexPointer(3, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)g_pVertices);
			glTexCoordPointer(2, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)g_pVertices + sizeof(vec3));
			glColorPointer(4, GL_FLOAT,
					sizeof(custom_vertex),
					(char*)g_pVertices + sizeof(vec3) + sizeof(vec2));
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glDrawArrays(GL_QUADS, 0, g_iVertexEnd);
		}
		// Reset pointer
		g_iVertexEnd = 0;
	}
}
	
bool gfx_init(bool fullscreen)
{
	if(!context.create(screen_width, screen_height, 24, 8, 16, 0, fullscreen?opengl::context::FLAG_FULLSCREEN:0))
	{
		dbg_msg("game", "failed to create gl context");
		return false;
	}
	// Init vertices
	if (g_pVertices)
		mem_free(g_pVertices);
	g_pVertices = (custom_vertex*)mem_alloc(sizeof(custom_vertex) * vertexBufferSize, 1);
	g_iVertexStart = 0;
	g_iVertexEnd = 0;

	context.set_title("---");

	/*
	dbg_msg("gfx", "OpenGL version %d.%d.%d", context.version_major(),
											  context.version_minor(),
											  context.version_rev());*/

	gfx_mapscreen(0,0,screen_width, screen_height);
	
	// TODO: make wrappers for this
	glEnable(GL_BLEND);
	
	// model
	mat4 mat = mat4::identity;
	opengl::matrix_modelview(&mat);
	
	// Set all z to -5.0f
	for (int i = 0; i < vertexBufferSize; i++)
		g_pVertices[i].pos.z = -5.0f;

	if(GLEW_VERSION_2_0)
	{
		// set the streams
		vertex_buffer.data(g_pVertices, sizeof(vertexBufferSize), GL_DYNAMIC_DRAW);
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
	
	// create null texture, will get id=0
	gfx_load_texture_raw(4,4,IMG_RGBA,null_texture_data);
	
	return true;
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
		textures[tex].tex.data2d(w, h, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else if(format == IMG_BGR)
		textures[tex].tex.data2d(w, h, GL_RGB, GL_BGR, GL_UNSIGNED_BYTE, data);
	else if(format == IMG_BGRA)
		textures[tex].tex.data2d(w, h, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, data);
	return tex;
}

// simple uncompressed RGBA loaders
int gfx_load_texture(const char *filename)
{
	int l = strlen(filename);
	if(l < 3)
		return 0;

	if(	(filename[l-3] == 't' || filename[l-3] == 'T') &&
		(filename[l-2] == 'g' || filename[l-2] == 'G') &&
		(filename[l-1] == 'a' || filename[l-1] == 'A'))
	{
		image_info img;
		if(gfx_load_tga(&img, filename))
		{
			int id = gfx_load_texture_raw(img.width, img.height, img.format, img.data);
			mem_free(img.data);
			return id;
		}
		return 0;
	}

	if(	(filename[l-3] == 'p' || filename[l-3] == 'P') &&
		(filename[l-2] == 'n' || filename[l-2] == 'N') &&
		(filename[l-1] == 'g' || filename[l-1] == 'G'))
	{
		image_info img;
		if(gfx_load_png(&img, filename))
		{
			int id = gfx_load_texture_raw(img.width, img.height, img.format, img.data);
			mem_free(img.data);
			return id;
		}
		
		return 0;
	}
	
	return 0;
}

int gfx_load_tga(image_info *img, const char *filename)
{
	// open file for reading
	file_stream file;
	if(!file.open_r(filename))
	{
		dbg_msg("game/tga", "failed to open file. filename='%s'", filename);
		return 0;
	}
	
	// read header
	unsigned char headers[18];
	file.read(headers, sizeof(headers));
	img->width = headers[12]+(headers[13]<<8);
	img->height = headers[14]+(headers[15]<<8);

	bool flipx = (headers[17] >> 4) & 1;
	bool flipy = !((headers[17] >> 5) & 1);
	
	(void)flipx; // TODO: make use of this flag

	if(headers[2] != 2) // needs to be uncompressed RGB
	{
		dbg_msg("game/tga", "tga not uncompressed rgb. filename='%s'", filename);
		return 0;
	}
		
	if(headers[16] != 32) // needs to be RGBA
	{
		dbg_msg("game/tga", "tga is 32bit. filename='%s'", filename);
		return 0;
	}
	
	// read data
	int data_size = img->width*img->height*4;
	img->data = mem_alloc(data_size, 1);
	img->format = IMG_BGRA;

	if (flipy)
	{
		for (int i = 0; i < img->height; i++)
		{
			file.read((char *)img->data + (img->height-i-1)*img->width*4, img->width*4);
		}
	}
	else
		file.read(img->data, data_size);
	file.close();

	return 1;
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
	if (g_pVertices)
		mem_free(g_pVertices);
	context.destroy();
}

void gfx_swap()
{
	context.swap();
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
	mat4 mat;
	mat.ortho(tl_x, br_x, br_y, tl_y, 1.0f, 10.f);
	opengl::matrix_projection(&mat);
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
	g_QuadColor[i].r = r;
	g_QuadColor[i].g = g;
	g_QuadColor[i].b = b;
	g_QuadColor[i].a = a;
}

void gfx_quads_setcolor(float r, float g, float b, float a)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setcolor without quads_begin");
	g_QuadColor[0] = vec4(r,g,b,a);
	g_QuadColor[1] = vec4(r,g,b,a);
	g_QuadColor[2] = vec4(r,g,b,a);
	g_QuadColor[3] = vec4(r,g,b,a);
	/*gfx_quads_setcolorvertex(0,r,g,b,a);
	gfx_quads_setcolorvertex(1,r,g,b,a);
	gfx_quads_setcolorvertex(2,r,g,b,a);
	gfx_quads_setcolorvertex(3,r,g,b,a);*/
}

void gfx_quads_setsubset(float tl_u, float tl_v, float br_u, float br_v)
{
	dbg_assert(quads_drawing == 1, "called gfx_quads_setsubset without quads_begin");

	g_QuadTexture[0].x = tl_u;
	g_QuadTexture[0].y = tl_v;
	//g_pVertices[g_iVertexEnd].tex.u = tl_u;
	//g_pVertices[g_iVertexEnd].tex.v = tl_v;

	g_QuadTexture[1].x = br_u;
	g_QuadTexture[1].y = tl_v;
	//g_pVertices[g_iVertexEnd + 2].tex.u = br_u;
	//g_pVertices[g_iVertexEnd + 2].tex.v = tl_v;

	g_QuadTexture[2].x = br_u;
	g_QuadTexture[2].y = br_v;
	//g_pVertices[g_iVertexEnd + 1].tex.u = tl_u;
	//g_pVertices[g_iVertexEnd + 1].tex.v = br_v;

	g_QuadTexture[3].x = tl_u;
	g_QuadTexture[3].y = br_v;
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
	
	g_pVertices[g_iVertexEnd].pos.x = x;
	g_pVertices[g_iVertexEnd].pos.y = y;
	g_pVertices[g_iVertexEnd].tex.u = g_QuadTexture[0].x;
	g_pVertices[g_iVertexEnd].tex.v = g_QuadTexture[0].y;
	g_pVertices[g_iVertexEnd].color = g_QuadColor[0];
	rotate(center, g_pVertices[g_iVertexEnd].pos);

	g_pVertices[g_iVertexEnd + 1].pos.x = x+width;
	g_pVertices[g_iVertexEnd + 1].pos.y = y;
	g_pVertices[g_iVertexEnd + 1].tex.u = g_QuadTexture[1].x;
	g_pVertices[g_iVertexEnd + 1].tex.v = g_QuadTexture[1].y;
	g_pVertices[g_iVertexEnd + 1].color = g_QuadColor[1];
	rotate(center, g_pVertices[g_iVertexEnd + 1].pos);

	g_pVertices[g_iVertexEnd + 2].pos.x = x + width;
	g_pVertices[g_iVertexEnd + 2].pos.y = y+height;
	g_pVertices[g_iVertexEnd + 2].tex.u = g_QuadTexture[2].x;
	g_pVertices[g_iVertexEnd + 2].tex.v = g_QuadTexture[2].y;
	g_pVertices[g_iVertexEnd + 2].color = g_QuadColor[2];
	rotate(center, g_pVertices[g_iVertexEnd + 2].pos);

	g_pVertices[g_iVertexEnd + 3].pos.x = x;
	g_pVertices[g_iVertexEnd + 3].pos.y = y+height;
	g_pVertices[g_iVertexEnd + 3].tex.u = g_QuadTexture[3].x;
	g_pVertices[g_iVertexEnd + 3].tex.v = g_QuadTexture[3].y;
	g_pVertices[g_iVertexEnd + 3].color = g_QuadColor[3];
	rotate(center, g_pVertices[g_iVertexEnd + 3].pos);
	
	draw_quad();
}

void gfx_quads_draw_freeform(
	float x0, float y0,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3)
{
	dbg_assert(quads_drawing == 1, "called quads_draw_freeform without quads_begin");
	
	g_pVertices[g_iVertexEnd].pos.x = x0;
	g_pVertices[g_iVertexEnd].pos.y = y0;
	g_pVertices[g_iVertexEnd].tex.u = g_QuadTexture[0].x;
	g_pVertices[g_iVertexEnd].tex.v = g_QuadTexture[0].y;
	g_pVertices[g_iVertexEnd].color = g_QuadColor[0];

	g_pVertices[g_iVertexEnd + 1].pos.x = x1;
	g_pVertices[g_iVertexEnd + 1].pos.y = y1;
	g_pVertices[g_iVertexEnd + 1].tex.u = g_QuadTexture[1].x;
	g_pVertices[g_iVertexEnd + 1].tex.v = g_QuadTexture[1].y;
	g_pVertices[g_iVertexEnd + 1].color = g_QuadColor[1];

	g_pVertices[g_iVertexEnd + 2].pos.x = x3;
	g_pVertices[g_iVertexEnd + 2].pos.y = y3;
	g_pVertices[g_iVertexEnd + 2].tex.u = g_QuadTexture[2].x;
	g_pVertices[g_iVertexEnd + 2].tex.v = g_QuadTexture[2].y;
	g_pVertices[g_iVertexEnd + 2].color = g_QuadColor[2];

	g_pVertices[g_iVertexEnd + 3].pos.x = x2;
	g_pVertices[g_iVertexEnd + 3].pos.y = y2;
	g_pVertices[g_iVertexEnd + 3].tex.u = g_QuadTexture[3].x;
	g_pVertices[g_iVertexEnd + 3].tex.v = g_QuadTexture[3].y;
	g_pVertices[g_iVertexEnd + 3].color = g_QuadColor[3];
	
	draw_quad();
}

void gfx_quads_text(float x, float y, float size, const char *text)
{
	gfx_quads_begin();
	while(*text)
	{
		char c = *text;
		text++;
		
		gfx_quads_setsubset(
			(c%16)/16.0f,
			(c/16)/16.0f,
			(c%16)/16.0f+1.0f/16.0f,
			(c/16)/16.0f+1.0f/16.0f);
		
		gfx_quads_drawTL(x,y,size,size);
		x += size/2;
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
	
	gfx_quads_begin();
	
	while (*text)
	{
		const int c = *text;
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
