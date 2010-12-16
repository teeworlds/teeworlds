/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

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

#include <engine/shared/engine.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <math.h>

#include "graphics.h"

// compressed textures
#define GL_COMPRESSED_RGB_ARB 0x84ED
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#define GL_COMPRESSED_ALPHA_ARB 0x84E9

#define TEXTURE_MAX_ANISOTROPY_EXT 0x84FE


static CVideoMode g_aFakeModes[] = {
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

void CGraphics_OpenGL::Flush()
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
	
	// Reset pointer
	m_NumVertices = 0;
}

void CGraphics_OpenGL::AddVertices(int Count)
{
	m_NumVertices += Count;
	if((m_NumVertices + Count) >= MAX_VERTICES)
		Flush();
}

void CGraphics_OpenGL::Rotate4(const CPoint &rCenter, CVertex *pPoints)
{
	float c = cosf(m_Rotation);
	float s = sinf(m_Rotation);
	float x, y;
	int i;

	for(i = 0; i < 4; i++)
	{
		x = pPoints[i].m_Pos.x - rCenter.x;
		y = pPoints[i].m_Pos.y - rCenter.y;
		pPoints[i].m_Pos.x = x * c - y * s + rCenter.x;
		pPoints[i].m_Pos.y = x * s + y * c + rCenter.y;
	}
}

unsigned char CGraphics_OpenGL::Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset)
{
	return (pData[(v*w+u)*4+Offset]+
	pData[(v*w+u+1)*4+Offset]+
	pData[((v+1)*w+u)*4+Offset]+
	pData[((v+1)*w+u+1)*4+Offset])/4;
}	

CGraphics_OpenGL::CGraphics_OpenGL()
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

void CGraphics_OpenGL::ClipEnable(int x, int y, int w, int h)
{
	//if(no_gfx) return;
	glScissor(x, ScreenHeight()-(y+h), w, h);
	glEnable(GL_SCISSOR_TEST);
}

void CGraphics_OpenGL::ClipDisable()
{
	//if(no_gfx) return;
	glDisable(GL_SCISSOR_TEST);
}
	
void CGraphics_OpenGL::BlendNone()
{
	glDisable(GL_BLEND);
}

void CGraphics_OpenGL::BlendNormal()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CGraphics_OpenGL::BlendAdditive()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

int CGraphics_OpenGL::MemoryUsage() const
{ 
	return m_TextureMemoryUsage;
}	
	
void CGraphics_OpenGL::MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY)
{
	m_ScreenX0 = TopLeftX;
	m_ScreenY0 = TopLeftY;
	m_ScreenX1 = BottomRightX;
	m_ScreenY1 = BottomRightY;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(TopLeftX, BottomRightX, BottomRightY, TopLeftY, 1.0f, 10.f);
}

void CGraphics_OpenGL::GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY)
{
	*pTopLeftX = m_ScreenX0;
	*pTopLeftY = m_ScreenY0;
	*pBottomRightX = m_ScreenX1;
	*pBottomRightY = m_ScreenY1;
}

void CGraphics_OpenGL::LinesBegin()
{
	dbg_assert(m_Drawing == 0, "called begin twice");
	m_Drawing = DRAWING_LINES;
	SetColor(1,1,1,1);
}

void CGraphics_OpenGL::LinesEnd()
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called end without begin");
	Flush();
	m_Drawing = 0;
}

void CGraphics_OpenGL::LinesDraw(const CLineItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called draw without begin");
	
	for(int i = 0; i < Num; ++i)
	{
		m_aVertices[m_NumVertices + 2*i].m_Pos.x = pArray[i].m_X0;
		m_aVertices[m_NumVertices + 2*i].m_Pos.y = pArray[i].m_Y0;
		m_aVertices[m_NumVertices + 2*i].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices + 2*i].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.x = pArray[i].m_X1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.y = pArray[i].m_Y1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 2*i + 1].m_Color = m_aColor[1];
	}

	AddVertices(2*Num);
}

int CGraphics_OpenGL::UnloadTexture(int Index)
{
	if(Index == m_InvalidTexture)
		return 0;
		
	if(Index < 0)
		return 0;
		
	glDeleteTextures(1, &m_aTextures[Index].m_Tex);
	m_aTextures[Index].m_Next = m_FirstFreeTexture;
	m_TextureMemoryUsage -= m_aTextures[Index].m_MemSize;
	m_FirstFreeTexture = Index;
	return 0;
}


int CGraphics_OpenGL::LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags)
{
	int Mipmap = 1;
	unsigned char *pTexData = (unsigned char *)pData;
	unsigned char *pTmpData = 0;
	int Oglformat = 0;
	int StoreOglformat = 0;
	int Tex = 0;
	
	// don't waste memory on texture if we are stress testing
	if(g_Config.m_DbgStress)
		return 	m_InvalidTexture;
	
	// grab texture
	Tex = m_FirstFreeTexture;
	m_FirstFreeTexture = m_aTextures[Tex].m_Next;
	m_aTextures[Tex].m_Next = -1;
	
	// resample if needed
	if(!(Flags&TEXLOAD_NORESAMPLE) && g_Config.m_GfxTextureQuality==0)
	{
		if(Width > 16 && Height > 16 && Format == CImageInfo::FORMAT_RGBA)
		{
			unsigned char *pTmpData;
			int c = 0;
			int x, y;

			pTmpData = (unsigned char *)mem_alloc(Width*Height*4, 1);

			Width/=2;
			Height/=2;

			for(y = 0; y < Height; y++)
				for(x = 0; x < Width; x++, c++)
				{
					pTmpData[c*4] = Sample(Width*2, Height*2, pTexData, x*2,y*2, 0);
					pTmpData[c*4+1] = Sample(Width*2, Height*2, pTexData, x*2,y*2, 1);
					pTmpData[c*4+2] = Sample(Width*2, Height*2, pTexData, x*2,y*2, 2);
					pTmpData[c*4+3] = Sample(Width*2, Height*2, pTexData, x*2,y*2, 3);
				}
			pTexData = pTmpData;
		}
	}
	
	Oglformat = GL_RGBA;
	if(Format == CImageInfo::FORMAT_RGB)
		Oglformat = GL_RGB;
	else if(Format == CImageInfo::FORMAT_ALPHA)
		Oglformat = GL_ALPHA;
	
	// upload texture
	if(g_Config.m_GfxTextureCompression)
	{
		StoreOglformat = GL_COMPRESSED_RGBA_ARB;
		if(StoreFormat == CImageInfo::FORMAT_RGB)
			StoreOglformat = GL_COMPRESSED_RGB_ARB;
		else if(StoreFormat == CImageInfo::FORMAT_ALPHA)
			StoreOglformat = GL_COMPRESSED_ALPHA_ARB;
	}
	else
	{
		StoreOglformat = GL_RGBA;
		if(StoreFormat == CImageInfo::FORMAT_RGB)
			StoreOglformat = GL_RGB;
		else if(StoreFormat == CImageInfo::FORMAT_ALPHA)
			StoreOglformat = GL_ALPHA;
	}
		
	glGenTextures(1, &m_aTextures[Tex].m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_aTextures[Tex].m_Tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, StoreOglformat, Width, Height, Oglformat, GL_UNSIGNED_BYTE, pTexData);
	
	// calculate memory usage
	{
		int PixelSize = 4;
		if(StoreFormat == CImageInfo::FORMAT_RGB)
			PixelSize = 3;
		else if(StoreFormat == CImageInfo::FORMAT_ALPHA)
			PixelSize = 1;

		m_aTextures[Tex].m_MemSize = Width*Height*PixelSize;
		if(Mipmap)
		{
			while(Width > 2 && Height > 2)
			{
				Width>>=1;
				Height>>=1;
				m_aTextures[Tex].m_MemSize += Width*Height*PixelSize;
			}
		}
	}
	
	m_TextureMemoryUsage += m_aTextures[Tex].m_MemSize;
	mem_free(pTmpData);
	return Tex;
}

// simple uncompressed RGBA loaders
int CGraphics_OpenGL::LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags)
{
	int l = str_length(pFilename);
	int Id;
	CImageInfo Img;
	
	if(l < 3)
		return -1;
	if(LoadPNG(&Img, pFilename, StorageType))
	{
		if (StoreFormat == CImageInfo::FORMAT_AUTO)
			StoreFormat = Img.m_Format;

		Id = LoadTextureRaw(Img.m_Width, Img.m_Height, Img.m_Format, Img.m_pData, StoreFormat, Flags);
		mem_free(Img.m_pData);
		return Id;
	}
	
	return m_InvalidTexture;
}

int CGraphics_OpenGL::LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention
	
	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		return 0;
	}
	
	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return 0;
	}
	
	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return 0;
	}
		
	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention
	
	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return 1;
}

void CGraphics_OpenGL::ScreenshotDirect(const char *pFilename)
{
	// fetch image data
	int y;
	int w = m_ScreenWidth;
	int h = m_ScreenHeight;
	unsigned char *pPixelData = (unsigned char *)mem_alloc(w*(h+1)*3, 1);
	unsigned char *pTempRow = pPixelData+w*h*3;
	GLint Alignment;
	glGetIntegerv(GL_PACK_ALIGNMENT, &Alignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);
	glPixelStorei(GL_PACK_ALIGNMENT, Alignment);
	
	// flip the pixel because opengl works from bottom left corner
	for(y = 0; y < h/2; y++)
	{
		mem_copy(pTempRow, pPixelData+y*w*3, w*3);
		mem_copy(pPixelData+y*w*3, pPixelData+(h-y-1)*w*3, w*3);
		mem_copy(pPixelData+(h-y-1)*w*3, pTempRow,w*3);
	}
	
	// find filename
	{
		char aWholePath[1024];
		png_t Png; // ignore_convention

		IOHANDLE File  = m_pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE, aWholePath, sizeof(aWholePath));
		if(File)
			io_close(File);
	
		// save png
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "saved screenshot to '%s'", aWholePath);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
		png_open_file_write(&Png, aWholePath); // ignore_convention
		png_set_data(&Png, w, h, 8, PNG_TRUECOLOR, (unsigned char *)pPixelData); // ignore_convention
		png_close_file(&Png); // ignore_convention
	}

	// clean up
	mem_free(pPixelData);
}

void CGraphics_OpenGL::TextureSet(int TextureID)
{
	dbg_assert(m_Drawing == 0, "called Graphics()->TextureSet within begin");
	if(TextureID == -1)
	{
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_aTextures[TextureID].m_Tex);
	}
}

void CGraphics_OpenGL::Clear(float r, float g, float b)
{
	glClearColor(r,g,b,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CGraphics_OpenGL::QuadsBegin()
{
	dbg_assert(m_Drawing == 0, "called quads_begin twice");
	m_Drawing = DRAWING_QUADS;
	
	QuadsSetSubset(0,0,1,1);
	QuadsSetRotation(0);
	SetColor(1,1,1,1);
}

void CGraphics_OpenGL::QuadsEnd()
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_end without begin");
	Flush();
	m_Drawing = 0;
}

void CGraphics_OpenGL::QuadsSetRotation(float Angle)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetRotation without begin");
	m_Rotation = Angle;
}

void CGraphics_OpenGL::SetColorVertex(const CColorVertex *pArray, int Num)
{
	dbg_assert(m_Drawing != 0, "called gfx_quads_setcolorvertex without begin");

	for(int i = 0; i < Num; ++i)
	{
		m_aColor[pArray[i].m_Index].r = pArray[i].m_R;
		m_aColor[pArray[i].m_Index].g = pArray[i].m_G;
		m_aColor[pArray[i].m_Index].b = pArray[i].m_B;
		m_aColor[pArray[i].m_Index].a = pArray[i].m_A;
	}
}

void CGraphics_OpenGL::SetColor(float r, float g, float b, float a)
{
	dbg_assert(m_Drawing != 0, "called gfx_quads_setcolor without begin");
	CColorVertex Array[4] = {
		CColorVertex(0, r, g, b, a),
		CColorVertex(1, r, g, b, a),
		CColorVertex(2, r, g, b, a),
		CColorVertex(3, r, g, b, a)};
	SetColorVertex(Array, 4);
}

void CGraphics_OpenGL::QuadsSetSubset(float TlU, float TlV, float BrU, float BrV)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetSubset without begin");

	m_aTexture[0].u = TlU;	m_aTexture[1].u = BrU;
	m_aTexture[0].v = TlV;	m_aTexture[1].v = TlV;

	m_aTexture[3].u = TlU;	m_aTexture[2].u = BrU;
	m_aTexture[3].v = BrV;	m_aTexture[2].v = BrV;
}

void CGraphics_OpenGL::QuadsSetSubsetFree(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
	m_aTexture[0].u = x0; m_aTexture[0].v = y0;
	m_aTexture[1].u = x1; m_aTexture[1].v = y1;
	m_aTexture[2].u = x2; m_aTexture[2].v = y2;
	m_aTexture[3].u = x3; m_aTexture[3].v = y3;
}

void CGraphics_OpenGL::QuadsDraw(CQuadItem *pArray, int Num)
{
	for(int i = 0; i < Num; ++i)
	{
		pArray[i].m_X -= pArray[i].m_Width/2;
		pArray[i].m_Y -= pArray[i].m_Height/2;
	}

	QuadsDrawTL(pArray, Num);
}

void CGraphics_OpenGL::QuadsDrawTL(const CQuadItem *pArray, int Num)
{
	CPoint Center;
	Center.z = 0;

	dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_draw without begin");

	for(int i = 0; i < Num; ++i)
	{
		m_aVertices[m_NumVertices + 4*i].m_Pos.x = pArray[i].m_X;
		m_aVertices[m_NumVertices + 4*i].m_Pos.y = pArray[i].m_Y;
		m_aVertices[m_NumVertices + 4*i].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices + 4*i].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 4*i + 1].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
		m_aVertices[m_NumVertices + 4*i + 1].m_Pos.y = pArray[i].m_Y;
		m_aVertices[m_NumVertices + 4*i + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 4*i + 1].m_Color = m_aColor[1];

		m_aVertices[m_NumVertices + 4*i + 2].m_Pos.x = pArray[i].m_X + pArray[i].m_Width;
		m_aVertices[m_NumVertices + 4*i + 2].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
		m_aVertices[m_NumVertices + 4*i + 2].m_Tex = m_aTexture[2];
		m_aVertices[m_NumVertices + 4*i + 2].m_Color = m_aColor[2];

		m_aVertices[m_NumVertices + 4*i + 3].m_Pos.x = pArray[i].m_X;
		m_aVertices[m_NumVertices + 4*i + 3].m_Pos.y = pArray[i].m_Y + pArray[i].m_Height;
		m_aVertices[m_NumVertices + 4*i + 3].m_Tex = m_aTexture[3];
		m_aVertices[m_NumVertices + 4*i + 3].m_Color = m_aColor[3];

		if(m_Rotation != 0)
		{
			Center.x = pArray[i].m_X + pArray[i].m_Width/2;
			Center.y = pArray[i].m_Y + pArray[i].m_Height/2;

			Rotate4(Center, &m_aVertices[m_NumVertices + 4*i]);
		}
	}

	AddVertices(4*Num);
}

void CGraphics_OpenGL::QuadsDrawFreeform(const CFreeformItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called quads_draw_freeform without begin");
	
	for(int i = 0; i < Num; ++i)
	{
		m_aVertices[m_NumVertices + 4*i].m_Pos.x = pArray[i].m_X0;
		m_aVertices[m_NumVertices + 4*i].m_Pos.y = pArray[i].m_Y0;
		m_aVertices[m_NumVertices + 4*i].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices + 4*i].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 4*i + 1].m_Pos.x = pArray[i].m_X1;
		m_aVertices[m_NumVertices + 4*i + 1].m_Pos.y = pArray[i].m_Y1;
		m_aVertices[m_NumVertices + 4*i + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 4*i + 1].m_Color = m_aColor[1];

		m_aVertices[m_NumVertices + 4*i + 2].m_Pos.x = pArray[i].m_X3;
		m_aVertices[m_NumVertices + 4*i + 2].m_Pos.y = pArray[i].m_Y3;
		m_aVertices[m_NumVertices + 4*i + 2].m_Tex = m_aTexture[3];
		m_aVertices[m_NumVertices + 4*i + 2].m_Color = m_aColor[3];

		m_aVertices[m_NumVertices + 4*i + 3].m_Pos.x = pArray[i].m_X2;
		m_aVertices[m_NumVertices + 4*i + 3].m_Pos.y = pArray[i].m_Y2;
		m_aVertices[m_NumVertices + 4*i + 3].m_Tex = m_aTexture[2];
		m_aVertices[m_NumVertices + 4*i + 3].m_Color = m_aColor[2];
	}
	
	AddVertices(4*Num);
}

void CGraphics_OpenGL::QuadsText(float x, float y, float Size, float r, float g, float b, float a, const char *pText)
{
	float StartX = x;

	QuadsBegin();
	SetColor(r,g,b,a);

	while(*pText)
	{
		char c = *pText;
		pText++;
		
		if(c == '\n')
		{
			x = StartX;
			y += Size;
		}
		else
		{
			QuadsSetSubset(
				(c%16)/16.0f,
				(c/16)/16.0f,
				(c%16)/16.0f+1.0f/16.0f,
				(c/16)/16.0f+1.0f/16.0f);
			
			CQuadItem QuadItem(x, y, Size, Size);
			QuadsDrawTL(&QuadItem, 1);
			x += Size/2;
		}
	}
	
	QuadsEnd();
}

bool CGraphics_OpenGL::Init()
{
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	
	// Set all z to -5.0f
	for(int i = 0; i < MAX_VERTICES; i++)
		m_aVertices[i].m_Pos.z = -5.0f;

	// init textures
	m_FirstFreeTexture = 0;
	for(int i = 0; i < MAX_TEXTURES; i++)
		m_aTextures[i].m_Next = i+1;
	m_aTextures[MAX_TEXTURES-1].m_Next = -1;

	// set some default settings
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(0);

	// create null texture, will get id=0
	static const unsigned char aNullTextureData[] = {
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff, 
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff, 
	};
	
	m_InvalidTexture = LoadTextureRaw(4,4,CImageInfo::FORMAT_RGBA,aNullTextureData,CImageInfo::FORMAT_RGBA,TEXLOAD_NORESAMPLE);
	
	return true;
}

int CGraphics_SDL::TryInit()
{
	const SDL_VideoInfo *pInfo;
	int Flags = SDL_OPENGL;
	
	m_ScreenWidth = g_Config.m_GfxScreenWidth;
	m_ScreenHeight = g_Config.m_GfxScreenHeight;

	pInfo = SDL_GetVideoInfo();
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	// set flags
	Flags  = SDL_OPENGL;
	Flags |= SDL_GL_DOUBLEBUFFER;
	Flags |= SDL_HWPALETTE;
	if(g_Config.m_DbgResizable)
		Flags |= SDL_RESIZABLE;

	if(pInfo->hw_available) // ignore_convention
		Flags |= SDL_HWSURFACE;
	else
		Flags |= SDL_SWSURFACE;

	if(pInfo->blit_hw) // ignore_convention
		Flags |= SDL_HWACCEL;

	if(g_Config.m_GfxFullscreen)
		Flags |= SDL_FULLSCREEN;

	// set gl attributes
	if(g_Config.m_GfxFsaaSamples)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, g_Config.m_GfxFsaaSamples);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, g_Config.m_GfxVsync);

	// set caption
	SDL_WM_SetCaption("Teeworlds", "Teeworlds");
	
	// create window
	m_pScreenSurface = SDL_SetVideoMode(m_ScreenWidth, m_ScreenHeight, 0, Flags);
	if(m_pScreenSurface == NULL)
	{
		dbg_msg("gfx", "unable to set video mode: %s", SDL_GetError());
		return -1;
	}
	
	return 0;
}


int CGraphics_SDL::InitWindow()
{
	if(TryInit() == 0)
		return 0;
	
	// try disabling fsaa
	while(g_Config.m_GfxFsaaSamples)
	{
		g_Config.m_GfxFsaaSamples--;
		
		if(g_Config.m_GfxFsaaSamples)
			dbg_msg("gfx", "lowering FSAA to %d and trying again", g_Config.m_GfxFsaaSamples);
		else
			dbg_msg("gfx", "disabling FSAA and trying again");

		if(TryInit() == 0)
			return 0;
	}

	// try lowering the resolution
	if(g_Config.m_GfxScreenWidth != 640 || g_Config.m_GfxScreenHeight != 480)
	{
		dbg_msg("gfx", "setting resolution to 640x480 and trying again");
		g_Config.m_GfxScreenWidth = 640;
		g_Config.m_GfxScreenHeight = 480;

		if(TryInit() == 0)
			return 0;
	}

	dbg_msg("gfx", "out of ideas. failed to init graphics");
					
	return -1;		
}


CGraphics_SDL::CGraphics_SDL()
{
	m_pScreenSurface = 0;
}

bool CGraphics_SDL::Init()
{
	{
		int Systems = SDL_INIT_VIDEO;
		
		if(g_Config.m_SndEnable)
			Systems |= SDL_INIT_AUDIO;

		if(g_Config.m_ClEventthread)
			Systems |= SDL_INIT_EVENTTHREAD;
		
		if(SDL_Init(Systems) < 0)
		{
			dbg_msg("gfx", "unable to init SDL: %s", SDL_GetError());
			return true;
		}
	}
	
	atexit(SDL_Quit); // ignore_convention

	#ifdef CONF_FAMILY_WINDOWS
		if(!getenv("SDL_VIDEO_WINDOW_POS") && !getenv("SDL_VIDEO_CENTERED")) // ignore_convention
			putenv("SDL_VIDEO_WINDOW_POS=8,27"); // ignore_convention
	#endif
	
	if(InitWindow() != 0)
		return true;

	SDL_ShowCursor(0);
		
	CGraphics_OpenGL::Init();
	
	MapScreen(0,0,g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
	return false;
}

void CGraphics_SDL::Shutdown()
{
	// TODO: SDL, is this correct?
	SDL_Quit();
}

void CGraphics_SDL::Minimize()
{
	SDL_WM_IconifyWindow();
}

void CGraphics_SDL::Maximize()
{
	// TODO: SDL
}

int CGraphics_SDL::WindowActive()
{
	return SDL_GetAppState()&SDL_APPINPUTFOCUS;
}

int CGraphics_SDL::WindowOpen()
{
	return SDL_GetAppState()&SDL_APPACTIVE;

}

void CGraphics_SDL::TakeScreenshot(const char *pFilename)
{
	char aDate[20];
	str_timestamp(aDate, sizeof(aDate));
	str_format(m_aScreenshotName, sizeof(m_aScreenshotName), "screenshots/%s_%s.png", pFilename?pFilename:"screenshot", aDate);
	m_DoScreenshot = true;
}

void CGraphics_SDL::Swap()
{
	if(m_DoScreenshot)
	{
		ScreenshotDirect(m_aScreenshotName);
		m_DoScreenshot = false;
	}
	
	SDL_GL_SwapBuffers();
	
	if(g_Config.m_GfxFinish)
		glFinish();		
}


int CGraphics_SDL::GetVideoModes(CVideoMode *pModes, int MaxModes)
{
	int NumModes = sizeof(g_aFakeModes)/sizeof(CVideoMode);
	SDL_Rect **ppModes;

	if(g_Config.m_GfxDisplayAllModes)
	{
		int Count = sizeof(g_aFakeModes)/sizeof(CVideoMode);
		mem_copy(pModes, g_aFakeModes, sizeof(g_aFakeModes));
		if(MaxModes < Count)
			Count = MaxModes;
		return Count;
	}
	
	// TODO: fix this code on osx or windows
		
	ppModes = SDL_ListModes(NULL, SDL_OPENGL|SDL_GL_DOUBLEBUFFER|SDL_FULLSCREEN);
	if(ppModes == NULL)
	{
		// no modes
		NumModes = 0;
	}
	else if(ppModes == (SDL_Rect**)-1)
	{
		// all modes
	}
	else
	{
		NumModes = 0;
		for(int i = 0; ppModes[i]; ++i)
		{
			if(NumModes == MaxModes)
				break;
			pModes[NumModes].m_Width = ppModes[i]->w;
			pModes[NumModes].m_Height = ppModes[i]->h;
			pModes[NumModes].m_Red = 8;
			pModes[NumModes].m_Green = 8;
			pModes[NumModes].m_Blue = 8;
			NumModes++;
		}
	}
	
	return NumModes;
}

extern IEngineGraphics *CreateEngineGraphics() { return new CGraphics_SDL(); }
