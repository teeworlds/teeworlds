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

/* compressed textures */
#define GL_COMPRESSED_RGB_ARB 0x84ED
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#define GL_COMPRESSED_ALPHA_ARB 0x84E9

#define TEXTURE_MAX_ANISOTROPY_EXT 0x84FE


void gfx_font_init();

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
	int num_modes = sizeof(fakemodes)/sizeof(VIDEO_MODE);
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
		num_modes = 0;
	}
	else if(modes == (SDL_Rect**)-1)
	{
		/* all modes */
	}
	else
	{
		int i;
		num_modes = 0;
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
	
	return num_modes;
}


#include "graphics.h"

class CGraphics_OpenGL : public IEngineGraphics
{
protected:
	/* */
	typedef struct { float x, y, z; } CPoint;
	typedef struct { float u, v; } CTexCoord;
	typedef struct { float r, g, b, a; } CColor;

	typedef struct
	{
		CPoint m_Pos;
		CTexCoord m_Tex;
		CColor m_Color;
	} CVertex;
	
	enum
	{
		MAX_VERTICES = 32*1024,
		MAX_TEXTURES = 1024*4,
		
		DRAWING_QUADS=1,
		DRAWING_LINES=2		
	};

	CVertex m_aVertices[MAX_VERTICES];
	int m_NumVertices;

	CColor m_aColor[4];
	CTexCoord m_aTexture[4];

	bool m_RenderEnable;

	float m_Rotation;
	int m_Drawing;
	bool m_DoScreenshot;

	float m_ScreenX0;
	float m_ScreenY0;
	float m_ScreenX1;
	float m_ScreenY1;

	int m_InvalidTexture;

	struct CTexture
	{
		GLuint tex;
		int memsize;
		int flags;
		int next;
	};

	enum
	{
		
	};

	CTexture m_aTextures[MAX_TEXTURES];
	int m_FirstFreeTexture;
	int m_TextureMemoryUsage;


	void Flush()
	{
		if(m_NumVertices == 0)
			return;
			
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glVertexPointer(3, GL_FLOAT,
				sizeof(CVertex),
				(char*)m_aVertices);
		glTexCoordPointer(2, GL_FLOAT,
				sizeof(CVertex),
				(char*)m_aVertices + sizeof(float)*3);
		glColorPointer(4, GL_FLOAT,
				sizeof(CVertex),
				(char*)m_aVertices + sizeof(float)*5);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		if(m_RenderEnable)
		{
			if(m_Drawing == DRAWING_QUADS)
				glDrawArrays(GL_QUADS, 0, m_NumVertices);
			else if(m_Drawing == DRAWING_LINES)
				glDrawArrays(GL_LINES, 0, m_NumVertices);
		}
		
		/* Reset pointer */
		m_NumVertices = 0;
	}

	void AddVertices(int count)
	{
		m_NumVertices += count;
		if((m_NumVertices + count) >= MAX_VERTICES)
			Flush();
	}
	
	void Rotate(CPoint *pCenter, CPoint *pPoint)
	{
		float x = pPoint->x - pCenter->x;
		float y = pPoint->y - pCenter->y;
		pPoint->x = x * cosf(m_Rotation) - y * sinf(m_Rotation) + pCenter->x;
		pPoint->y = x * sinf(m_Rotation) + y * cosf(m_Rotation) + pCenter->y;
	}
	
	


	static unsigned char sample(int w, int h, const unsigned char *data, int u, int v, int offset)
	{
		return (data[(v*w+u)*4+offset]+
		data[(v*w+u+1)*4+offset]+
		data[((v+1)*w+u)*4+offset]+
		data[((v+1)*w+u+1)*4+offset])/4;
	}	
public:
	CGraphics_OpenGL()
	{
		m_NumVertices = 0;
		
		m_ScreenX0 = 0;
		m_ScreenY0 = 0;
		m_ScreenX1 = 0;
		m_ScreenY1 = 0;
		
		m_ScreenWidth = -1;
		m_ScreenHeight = -1;
		
		m_Rotation = 0;
		m_Drawing = 0;
		m_InvalidTexture = 0;
		
		m_TextureMemoryUsage = 0;
		
		m_RenderEnable = true;
		m_DoScreenshot = false;
	}
	

	virtual void ClipEnable(int x, int y, int w, int h)
	{
		//if(no_gfx) return;
		glScissor(x, ScreenHeight()-(y+h), w, h);
		glEnable(GL_SCISSOR_TEST);
	}

	virtual void ClipDisable()
	{
		//if(no_gfx) return;
		glDisable(GL_SCISSOR_TEST);
	}
		

	virtual void BlendNone()
	{
		glDisable(GL_BLEND);
	}

	virtual void BlendNormal()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	virtual void BlendAdditive()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}

	//int gfx_memory_usage() { return m_MemoryUsage; }	
		
	virtual void MapScreen(float tl_x, float tl_y, float br_x, float br_y)
	{
		m_ScreenX0 = tl_x;
		m_ScreenY0 = tl_y;
		m_ScreenX1 = br_x;
		m_ScreenY1 = br_y;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(tl_x, br_x, br_y, tl_y, 1.0f, 10.f);
	}

	virtual void GetScreen(float *tl_x, float *tl_y, float *br_x, float *br_y)
	{
		*tl_x = m_ScreenX0;
		*tl_y = m_ScreenY0;
		*br_x = m_ScreenX1;
		*br_y = m_ScreenY1;
	}

	virtual void LinesBegin()
	{
		dbg_assert(m_Drawing == 0, "called begin twice");
		m_Drawing = DRAWING_LINES;
		SetColor(1,1,1,1);
	}

	virtual void LinesEnd()
	{
		dbg_assert(m_Drawing == DRAWING_LINES, "called end without begin");
		Flush();
		m_Drawing = 0;
	}

	virtual void LinesDraw(float x0, float y0, float x1, float y1)
	{
		dbg_assert(m_Drawing == DRAWING_LINES, "called draw without begin");
		
		m_aVertices[m_NumVertices].m_Pos.x = x0;
		m_aVertices[m_NumVertices].m_Pos.y = y0;
		m_aVertices[m_NumVertices].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 1].m_Pos.x = x1;
		m_aVertices[m_NumVertices + 1].m_Pos.y = y1;
		m_aVertices[m_NumVertices + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 1].m_Color = m_aColor[1];
		
		AddVertices(2);
	}
	

	
	virtual int UnloadTexture(int Index)
	{
		if(Index == m_InvalidTexture)
			return 0;
			
		if(Index < 0)
			return 0;
			
		glDeleteTextures(1, &m_aTextures[Index].tex);
		m_aTextures[Index].next = m_FirstFreeTexture;
		m_TextureMemoryUsage -= m_aTextures[Index].memsize;
		m_FirstFreeTexture = Index;
		return 0;
	}


	virtual int LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags)
	{
		int mipmap = 1;
		unsigned char *texdata = (unsigned char *)pData;
		unsigned char *tmpdata = 0;
		int oglformat = 0;
		int store_oglformat = 0;
		int tex = 0;
		
		/* don't waste memory on texture if we are stress testing */
		if(config.dbg_stress)
			return 	m_InvalidTexture;
		
		/* grab texture */
		tex = m_FirstFreeTexture;
		m_FirstFreeTexture = m_aTextures[tex].next;
		m_aTextures[tex].next = -1;
		
		/* resample if needed */
		if(!(Flags&TEXLOAD_NORESAMPLE) && config.gfx_texture_quality==0)
		{
			if(Width > 16 && Height > 16 && Format == IMG_RGBA)
			{
				unsigned char *tmpdata;
				int c = 0;
				int x, y;

				tmpdata = (unsigned char *)mem_alloc(Width*Height*4, 1);

				Width/=2;
				Height/=2;

				for(y = 0; y < Height; y++)
					for(x = 0; x < Width; x++, c++)
					{
						tmpdata[c*4] = sample(Width*2, Height*2, texdata, x*2,y*2, 0);
						tmpdata[c*4+1] = sample(Width*2, Height*2, texdata, x*2,y*2, 1);
						tmpdata[c*4+2] = sample(Width*2, Height*2, texdata, x*2,y*2, 2);
						tmpdata[c*4+3] = sample(Width*2, Height*2, texdata, x*2,y*2, 3);
					}
				texdata = tmpdata;
			}
		}
		
		oglformat = GL_RGBA;
		if(Format == IMG_RGB)
			oglformat = GL_RGB;
		else if(Format == IMG_ALPHA)
			oglformat = GL_ALPHA;
		
		/* upload texture */
		if(config.gfx_texture_compression)
		{
			store_oglformat = GL_COMPRESSED_RGBA_ARB;
			if(StoreFormat == IMG_RGB)
				store_oglformat = GL_COMPRESSED_RGB_ARB;
			else if(StoreFormat == IMG_ALPHA)
				store_oglformat = GL_COMPRESSED_ALPHA_ARB;
		}
		else
		{
			store_oglformat = GL_RGBA;
			if(StoreFormat == IMG_RGB)
				store_oglformat = GL_RGB;
			else if(StoreFormat == IMG_ALPHA)
				store_oglformat = GL_ALPHA;
		}
			
		glGenTextures(1, &m_aTextures[tex].tex);
		glBindTexture(GL_TEXTURE_2D, m_aTextures[tex].tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, store_oglformat, Width, Height, oglformat, GL_UNSIGNED_BYTE, texdata);
		
		/* calculate memory usage */
		{
			int pixel_size = 4;
			if(StoreFormat == IMG_RGB)
				pixel_size = 3;
			else if(StoreFormat == IMG_ALPHA)
				pixel_size = 1;

			m_aTextures[tex].memsize = Width*Height*pixel_size;
			if(mipmap)
			{
				while(Width > 2 && Height > 2)
				{
					Width>>=1;
					Height>>=1;
					m_aTextures[tex].memsize += Width*Height*pixel_size;
				}
			}
		}
		
		m_TextureMemoryUsage += m_aTextures[tex].memsize;
		mem_free(tmpdata);
		return tex;
	}

	/* simple uncompressed RGBA loaders */
	virtual int LoadTexture(const char *pFilename, int StoreFormat, int Flags)
	{
		int l = strlen(pFilename);
		int id;
		IMAGE_INFO img;
		
		if(l < 3)
			return -1;
		if(LoadPNG(&img, pFilename))
		{
			if (StoreFormat == IMG_AUTO)
				StoreFormat = img.format;

			id = LoadTextureRaw(img.width, img.height, img.format, img.data, StoreFormat, Flags);
			mem_free(img.data);
			return id;
		}
		
		return m_InvalidTexture;
	}

	virtual int LoadPNG(IMAGE_INFO *pImg, const char *pFilename)
	{
		char aCompleteFilename[512];
		unsigned char *pBuffer;
		png_t png;
		
		/* open file for reading */
		png_init(0,0);

		engine_getpath(aCompleteFilename, sizeof(aCompleteFilename), pFilename, IOFLAG_READ);
		
		if(png_open_file(&png, aCompleteFilename) != PNG_NO_ERROR)
		{
			dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
			return 0;
		}
		
		if(png.depth != 8 || (png.color_type != PNG_TRUECOLOR && png.color_type != PNG_TRUECOLOR_ALPHA))
		{
			dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
			png_close_file(&png);
			return 0;
		}
			
		pBuffer = (unsigned char *)mem_alloc(png.width * png.height * png.bpp, 1);
		png_get_data(&png, pBuffer);
		png_close_file(&png);
		
		pImg->width = png.width;
		pImg->height = png.height;
		if(png.color_type == PNG_TRUECOLOR)
			pImg->format = IMG_RGB;
		else if(png.color_type == PNG_TRUECOLOR_ALPHA)
			pImg->format = IMG_RGBA;
		pImg->data = pBuffer;
		return 1;
	}

	void ScreenshotDirect(const char *filename)
	{
		/* fetch image data */
		int y;
		int w = m_ScreenWidth;
		int h = m_ScreenHeight;
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

	virtual void TextureSet(int TextureID)
	{
		dbg_assert(m_Drawing == 0, "called Graphics()->TextureSet within begin");
		if(TextureID == -1)
		{
			glDisable(GL_TEXTURE_2D);
		}
		else
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_aTextures[TextureID].tex);
		}
	}

	virtual void Clear(float r, float g, float b)
	{
		glClearColor(r,g,b,0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	virtual void QuadsBegin()
	{
		dbg_assert(m_Drawing == 0, "called quads_begin twice");
		m_Drawing = DRAWING_QUADS;
		
		QuadsSetSubset(0,0,1,1);
		QuadsSetRotation(0);
		SetColor(1,1,1,1);
	}

	virtual void QuadsEnd()
	{
		dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_end without begin");
		Flush();
		m_Drawing = 0;
	}

	virtual void QuadsSetRotation(float Angle)
	{
		dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetRotation without begin");
		m_Rotation = Angle;
	}

	virtual void SetColorVertex(int i, float r, float g, float b, float a)
	{
		dbg_assert(m_Drawing != 0, "called gfx_quads_setcolorvertex without begin");
		m_aColor[i].r = r;
		m_aColor[i].g = g;
		m_aColor[i].b = b;
		m_aColor[i].a = a;
	}

	virtual void SetColor(float r, float g, float b, float a)
	{
		dbg_assert(m_Drawing != 0, "called gfx_quads_setcolor without begin");
		SetColorVertex(0, r, g, b, a);
		SetColorVertex(1, r, g, b, a);
		SetColorVertex(2, r, g, b, a);
		SetColorVertex(3, r, g, b, a);
	}

	virtual void QuadsSetSubset(float tl_u, float tl_v, float br_u, float br_v)
	{
		dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetSubset without begin");

		m_aTexture[0].u = tl_u;	m_aTexture[1].u = br_u;
		m_aTexture[0].v = tl_v;	m_aTexture[1].v = tl_v;

		m_aTexture[3].u = tl_u;	m_aTexture[2].u = br_u;
		m_aTexture[3].v = br_v;	m_aTexture[2].v = br_v;
	}

	virtual void QuadsSetSubsetFree(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3)
	{
		m_aTexture[0].u = x0; m_aTexture[0].v = y0;
		m_aTexture[1].u = x1; m_aTexture[1].v = y1;
		m_aTexture[2].u = x2; m_aTexture[2].v = y2;
		m_aTexture[3].u = x3; m_aTexture[3].v = y3;
	}

	virtual void QuadsDraw(float x, float y, float w, float h)
	{
		QuadsDrawTL(x-w/2, y-h/2,w,h);
	}

	virtual void QuadsDrawTL(float x, float y, float w, float h)
	{
		CPoint Center;

		dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_draw without begin");

		Center.x = x + w/2;
		Center.y = y + h/2;
		Center.z = 0;
		
		m_aVertices[m_NumVertices].m_Pos.x = x;
		m_aVertices[m_NumVertices].m_Pos.y = y;
		m_aVertices[m_NumVertices].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices].m_Color = m_aColor[0];
		Rotate(&Center, &m_aVertices[m_NumVertices].m_Pos);

		m_aVertices[m_NumVertices + 1].m_Pos.x = x+w;
		m_aVertices[m_NumVertices + 1].m_Pos.y = y;
		m_aVertices[m_NumVertices + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 1].m_Color = m_aColor[1];
		Rotate(&Center, &m_aVertices[m_NumVertices + 1].m_Pos);

		m_aVertices[m_NumVertices + 2].m_Pos.x = x + w;
		m_aVertices[m_NumVertices + 2].m_Pos.y = y+h;
		m_aVertices[m_NumVertices + 2].m_Tex = m_aTexture[2];
		m_aVertices[m_NumVertices + 2].m_Color = m_aColor[2];
		Rotate(&Center, &m_aVertices[m_NumVertices + 2].m_Pos);

		m_aVertices[m_NumVertices + 3].m_Pos.x = x;
		m_aVertices[m_NumVertices + 3].m_Pos.y = y+h;
		m_aVertices[m_NumVertices + 3].m_Tex = m_aTexture[3];
		m_aVertices[m_NumVertices + 3].m_Color = m_aColor[3];
		Rotate(&Center, &m_aVertices[m_NumVertices + 3].m_Pos);
		
		AddVertices(4);
	}

	void QuadsDrawFreeform(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3)
	{
		dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_draw_freeform without begin");
		
		m_aVertices[m_NumVertices].m_Pos.x = x0;
		m_aVertices[m_NumVertices].m_Pos.y = y0;
		m_aVertices[m_NumVertices].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 1].m_Pos.x = x1;
		m_aVertices[m_NumVertices + 1].m_Pos.y = y1;
		m_aVertices[m_NumVertices + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 1].m_Color = m_aColor[1];

		m_aVertices[m_NumVertices + 2].m_Pos.x = x3;
		m_aVertices[m_NumVertices + 2].m_Pos.y = y3;
		m_aVertices[m_NumVertices + 2].m_Tex = m_aTexture[3];
		m_aVertices[m_NumVertices + 2].m_Color = m_aColor[3];

		m_aVertices[m_NumVertices + 3].m_Pos.x = x2;
		m_aVertices[m_NumVertices + 3].m_Pos.y = y2;
		m_aVertices[m_NumVertices + 3].m_Tex = m_aTexture[2];
		m_aVertices[m_NumVertices + 3].m_Color = m_aColor[2];
		
		AddVertices(4);
	}

	virtual void QuadsText(float x, float y, float Size, float r, float g, float b, float a, const char *pText)
	{
		float startx = x;

		QuadsBegin();
		SetColor(r,g,b,a);

		while(*pText)
		{
			char c = *pText;
			pText++;
			
			if(c == '\n')
			{
				x = startx;
				y += Size;
			}
			else
			{
				QuadsSetSubset(
					(c%16)/16.0f,
					(c/16)/16.0f,
					(c%16)/16.0f+1.0f/16.0f,
					(c/16)/16.0f+1.0f/16.0f);
				
				QuadsDrawTL(x,y,Size,Size);
				x += Size/2;
			}
		}
		
		QuadsEnd();
	}
	
	virtual bool Init()
	{
		/* Set all z to -5.0f */
		for(int i = 0; i < MAX_VERTICES; i++)
			m_aVertices[i].m_Pos.z = -5.0f;

		/* init textures */
		m_FirstFreeTexture = 0;
		for(int i = 0; i < MAX_TEXTURES; i++)
			m_aTextures[i].next = i+1;
		m_aTextures[MAX_TEXTURES-1].next = -1;

		/* set some default settings */	
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glAlphaFunc(GL_GREATER, 0);
		glEnable(GL_ALPHA_TEST);
		glDepthMask(0);

		/* create null texture, will get id=0 */
		static const unsigned char aNullTextureData[] = {
			0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
			0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
			0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
			0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
		};
		
		m_InvalidTexture = LoadTextureRaw(4,4,IMG_RGBA,aNullTextureData,IMG_RGBA,TEXLOAD_NORESAMPLE);
		dbg_msg("", "invalid texture id: %d %d", m_InvalidTexture, m_aTextures[m_InvalidTexture].tex);
		
		return true;
	}
};

class CGraphics_SDL : public CGraphics_OpenGL
{
	SDL_Surface *m_pScreenSurface;	
	
	int TryInit()
	{
		const SDL_VideoInfo *pInfo;
		int Flags = SDL_OPENGL;
		
		m_ScreenWidth = config.gfx_screen_width;
		m_ScreenHeight = config.gfx_screen_height;

		pInfo = SDL_GetVideoInfo();

		/* set flags */
		Flags  = SDL_OPENGL;
		Flags |= SDL_GL_DOUBLEBUFFER;
		Flags |= SDL_HWPALETTE;
		if(config.dbg_resizable)
			Flags |= SDL_RESIZABLE;

		if(pInfo->hw_available)
			Flags |= SDL_HWSURFACE;
		else
			Flags |= SDL_SWSURFACE;

		if(pInfo->blit_hw)
			Flags |= SDL_HWACCEL;

		if(config.gfx_fullscreen)
			Flags |= SDL_FULLSCREEN;

		/* set gl attributes */
		if(config.gfx_fsaa_samples)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.gfx_fsaa_samples);
		}
		else
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		}

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, config.gfx_vsync);

		/* set caption */
		SDL_WM_SetCaption("Teeworlds", "Teeworlds");
		
		/* create window */
		m_pScreenSurface = SDL_SetVideoMode(m_ScreenWidth, m_ScreenHeight, 0, Flags);
		if(m_pScreenSurface == NULL)
		{
			dbg_msg("gfx", "unable to set video mode: %s", SDL_GetError());
			return -1;
		}
		
		return 0;
	}


	int InitWindow()
	{
		if(TryInit() == 0)
			return 0;
		
		/* try disabling fsaa */
		while(config.gfx_fsaa_samples)
		{
			config.gfx_fsaa_samples--;
			
			if(config.gfx_fsaa_samples)
				dbg_msg("gfx", "lowering FSAA to %d and trying again", config.gfx_fsaa_samples);
			else
				dbg_msg("gfx", "disabling FSAA and trying again");

			if(TryInit() == 0)
				return 0;
		}

		/* try lowering the resolution */
		if(config.gfx_screen_width != 640 || config.gfx_screen_height != 480)
		{
			dbg_msg("gfx", "setting resolution to 640x480 and trying again");
			config.gfx_screen_width = 640;
			config.gfx_screen_height = 480;

			if(TryInit() == 0)
				return 0;
		}

		dbg_msg("gfx", "out of ideas. failed to init graphics");
						
		return -1;		
	}

	
public:
	CGraphics_SDL()
	{
		m_pScreenSurface = 0;
	}

	virtual bool Init()
	{
		{
			int Systems = SDL_INIT_VIDEO;
			
			if(config.snd_enable)
				Systems |= SDL_INIT_AUDIO;

			if(config.cl_eventthread)
				Systems |= SDL_INIT_EVENTTHREAD;
			
			if(SDL_Init(Systems) < 0)
			{
				dbg_msg("gfx", "unable to init SDL: %s", SDL_GetError());
				return -1;
			}
		}
		
		atexit(SDL_Quit);

		#ifdef CONF_FAMILY_WINDOWS
			if(!getenv("SDL_VIDEO_WINDOW_POS") && !getenv("SDL_VIDEO_CENTERED"))
				putenv("SDL_VIDEO_WINDOW_POS=8,27");
		#endif
		
		if(InitWindow() != 0)
			return -1;

		SDL_ShowCursor(0);
			
		CGraphics_OpenGL::Init();
		
		MapScreen(0,0,config.gfx_screen_width, config.gfx_screen_height);

		/* init input */
		inp_init();

		/* font init */
		gfx_font_init();

		return 0;
	}
	
	virtual void Shutdown()
	{
		/* TODO: SDL, is this correct? */
		SDL_Quit();
	}

	virtual void Minimize()
	{
		SDL_WM_IconifyWindow();
	}

	virtual void Maximize()
	{
		/* TODO: SDL */
	}

	virtual int WindowActive()
	{
		return SDL_GetAppState()&SDL_APPINPUTFOCUS;
	}

	virtual int WindowOpen()
	{
		return SDL_GetAppState()&SDL_APPACTIVE;

	}
	
	virtual void TakeScreenshot()
	{
		m_DoScreenshot = true;
	}
	
	virtual void Swap()
	{
		if(m_DoScreenshot)
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

			ScreenshotDirect(filename);
			m_DoScreenshot = false;
		}
		
		{
			static PERFORMACE_INFO pscope = {"glfwSwapBuffers", 0};
			perf_start(&pscope);
			SDL_GL_SwapBuffers();
			perf_end();
		}
		
		if(config.gfx_finish)
			glFinish();		
	}
};

extern IEngineGraphics *CreateEngineGraphics() { return new CGraphics_SDL(); }
