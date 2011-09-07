/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_GRAPHICS_H
#define ENGINE_CLIENT_GRAPHICS_H

#include <base/tl/ringbuffer.h>

#include <engine/loader.h>

class CGraphics_OpenGL : public IEngineGraphics
{
protected:
	class IResources *m_pResources;
	class IStorage *m_pStorage;
	class IConsole *m_pConsole;

	//
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
	char m_aScreenshotName[128];

	float m_ScreenX0;
	float m_ScreenY0;
	float m_ScreenX1;
	float m_ScreenY1;

	IResource *m_pInvalidTexture;

	int m_TextureMemoryUsage;

	class CResource_Texture : public IResource
	{
		friend class CGraphics_OpenGL;
		void SetLoaded() { m_State = IResource::STATE_LOADED; }
	public:
		CResource_Texture()
		{
			m_TexId = 0;
			mem_zero(&m_ImageInfo, sizeof(m_ImageInfo));
		}

		GLuint m_TexId;
		int m_MemSize;
		int m_Flags;

		// used for loading the texture
		// TODO: should perhaps just be stored at load time
		CImageInfo m_ImageInfo;
	};

	class CTextureHandler : public IResources::IHandler
	{
	public:
		CGraphics_OpenGL *m_pGL;
		static unsigned int PngReadFunc(void *pOutput, unsigned long size, unsigned long numel, void *pUserPtr);
		virtual IResource *Create(IResources::CResourceId Id);
		virtual bool Load(IResource *pResource, void *pData, unsigned DataSize);
		virtual bool Insert(IResource *pResource);
		virtual bool Destroy(IResource *pResource);
	};

	CTextureHandler m_TextureHandler;

	void Flush();
	void AddVertices(int Count);
	void Rotate4(const CPoint &rCenter, CVertex *pPoints);

	static unsigned char Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp);
	static unsigned char *Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData);

	//int GetTextureSlot();
	IResource *LoadTextureRawToResource(IResource *pResource, int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags);
public:

	CGraphics_OpenGL();

	virtual void ClipEnable(int x, int y, int w, int h);
	virtual void ClipDisable();

	virtual void BlendNone();
	virtual void BlendNormal();
	virtual void BlendAdditive();

	virtual int MemoryUsage() const;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY);
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY);

	virtual void LinesBegin();
	virtual void LinesEnd();
	virtual void LinesDraw(const CLineItem *pArray, int Num);

	virtual int UnloadTexture(IResource *pTexture);
	virtual IResource *LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags);

	// simple uncompressed RGBA loaders
	virtual IResource *LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags);
	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType);

	void ScreenshotDirect(const char *pFilename);

	//virtual void TextureSet(int TextureID);
	virtual void TextureSet(IResource *pResource);

	virtual void Clear(float r, float g, float b);

	virtual void QuadsBegin();
	virtual void QuadsEnd();
	virtual void QuadsSetRotation(float Angle);

	virtual void SetColorVertex(const CColorVertex *pArray, int Num);
	virtual void SetColor(float r, float g, float b, float a);

	virtual void QuadsSetSubset(float TlU, float TlV, float BrU, float BrV);
	virtual void QuadsSetSubsetFree(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3);

	virtual void QuadsDraw(CQuadItem *pArray, int Num);
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num);
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num);
	virtual void QuadsText(float x, float y, float Size, float r, float g, float b, float a, const char *pText);

	virtual bool Init();
};

class CGraphics_SDL : public CGraphics_OpenGL
{
	SDL_Surface *m_pScreenSurface;

	int TryInit();
	int InitWindow();
public:
	CGraphics_SDL();

	virtual bool Init();
	virtual void Shutdown();

	virtual void Minimize();
	virtual void Maximize();

	virtual int WindowActive();
	virtual int WindowOpen();

	virtual void TakeScreenshot(const char *pFilename);
	virtual void Swap();

	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes);

};

#endif
