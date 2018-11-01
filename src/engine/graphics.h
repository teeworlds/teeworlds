/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#include <base/vmath.h>

#include "kernel.h"


class CImageInfo
{
public:
	enum
	{
		FORMAT_AUTO=-1,
		FORMAT_RGB=0,
		FORMAT_RGBA=1,
		FORMAT_ALPHA=2,
	};

	/* Variable: width
		Contains the width of the image */
	int m_Width;

	/* Variable: height
		Contains the height of the image */
	int m_Height;

	/* Variable: format
		Contains the format of the image. See <Image Formats> for more information. */
	int m_Format;

	/* Variable: data
		Pointer to the image data. */
	void *m_pData;
};

/*
	Structure: CVideoMode
*/
class CVideoMode
{
public:
	int m_Width, m_Height;
	int m_Red, m_Green, m_Blue;

	bool operator<(const CVideoMode &Other) { return Other.m_Width < m_Width; }
};

class IGraphics : public IInterface
{
	MACRO_INTERFACE("graphics", 0)
protected:
	int m_ScreenWidth;
	int m_ScreenHeight;
	int m_DesktopScreenWidth;
	int m_DesktopScreenHeight;
public:
	/* Constants: Texture Loading Flags
		TEXLOAD_NORESAMPLE - Prevents the texture from any resampling
		TEXLOAD_NOMIPMAPS - Prevents the texture from generating mipmaps
		TEXLOAD_ARRAY_256 - Texture will be loaded as 3D texture with 16*16 subtiles
		TEXLOAD_MULTI_DIMENSION - Texture will be loaded as 2D and 3D texture
	*/
	enum
	{
		TEXLOAD_NORESAMPLE = 1,
		TEXLOAD_NOMIPMAPS = 2,
		TEXLOAD_ARRAY_256 = 4,
		TEXLOAD_MULTI_DIMENSION = 8,
	};

	/* Constants: Wrap Modes */
	enum
	{
		WRAP_REPEAT = 0,
		WRAP_CLAMP,
	};

	class CTextureHandle
	{
		friend class IGraphics;
		int m_Id;
	public:
		CTextureHandle()
		: m_Id(-1)
		{}

		bool IsValid() const { return Id() >= 0; }
		int Id() const { return m_Id; }
		void Invalidate() { m_Id = -1; }
	};

	int ScreenWidth() const { return m_ScreenWidth; }
	int ScreenHeight() const { return m_ScreenHeight; }
	float ScreenAspect() const { return (float)ScreenWidth()/(float)ScreenHeight(); }
	int DesktopWidth() const { return m_DesktopScreenWidth; }
	int DesktopHeight() const { return m_DesktopScreenHeight; }
	float DesktopAspect() const { return m_DesktopScreenWidth/(float)m_DesktopScreenHeight; }

	virtual void Clear(float r, float g, float b) = 0;

	virtual void ClipEnable(int x, int y, int w, int h) = 0;
	virtual void ClipDisable() = 0;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY) = 0;
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY) = 0;

	// TODO: These should perhaps not be virtuals
	virtual void BlendNone() = 0;
	virtual void BlendNormal() = 0;
	virtual void BlendAdditive() = 0;
	virtual void WrapNormal() = 0;
	virtual void WrapClamp() = 0;
	virtual void WrapMode(int WrapU, int WrapV) = 0;
	virtual int MemoryUsage() const = 0;

	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType) = 0;

	virtual int UnloadTexture(CTextureHandle *Index) = 0;
	virtual CTextureHandle LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags) = 0;
	virtual int LoadTextureRawSub(CTextureHandle TextureID, int x, int y, int Width, int Height, int Format, const void *pData) = 0;
	virtual CTextureHandle LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags) = 0;
	virtual void TextureSet(CTextureHandle Texture) = 0;
	void TextureClear() { TextureSet(CTextureHandle()); }

	struct CLineItem
	{
		float m_X0, m_Y0, m_X1, m_Y1;
		CLineItem() {}
		CLineItem(float x0, float y0, float x1, float y1) : m_X0(x0), m_Y0(y0), m_X1(x1), m_Y1(y1) {}
	};
	virtual void LinesBegin() = 0;
	virtual void LinesEnd() = 0;
	virtual void LinesDraw(const CLineItem *pArray, int Num) = 0;

	virtual void QuadsBegin() = 0;
	virtual void QuadsEnd() = 0;
	virtual void QuadsSetRotation(float Angle) = 0;
	virtual void QuadsSetSubset(float TopLeftY, float TopLeftV, float BottomRightU, float BottomRightV, int TextureIndex = -1) = 0;
	virtual void QuadsSetSubsetFree(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, int TextureIndex = -1) = 0;

	struct CQuadItem
	{
		float m_X, m_Y, m_Width, m_Height;
		CQuadItem() {}
		CQuadItem(float x, float y, float w, float h) : m_X(x), m_Y(y), m_Width(w), m_Height(h) {}
	};
	virtual void QuadsDraw(CQuadItem *pArray, int Num) = 0;
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num) = 0;

	struct CFreeformItem
	{
		float m_X0, m_Y0, m_X1, m_Y1, m_X2, m_Y2, m_X3, m_Y3;
		CFreeformItem() {}
		CFreeformItem(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
			: m_X0(x0), m_Y0(y0), m_X1(x1), m_Y1(y1), m_X2(x2), m_Y2(y2), m_X3(x3), m_Y3(y3) {}
	};
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num) = 0;
	virtual void QuadsText(float x, float y, float Size, const char *pText) = 0;

	struct CColorVertex
	{
		int m_Index;
		float m_R, m_G, m_B, m_A;
		CColorVertex() {}
		CColorVertex(int i, float r, float g, float b, float a) : m_Index(i), m_R(r), m_G(g), m_B(b), m_A(a) {}
	};
	virtual void SetColorVertex(const CColorVertex *pArray, int Num) = 0;
	virtual void SetColor(float r, float g, float b, float a) = 0;
	virtual void SetColor4(vec4 TopLeft, vec4 TopRight, vec4 BottomLeft, vec4 BottomRight) = 0;

	virtual void ReadBackbuffer(unsigned char **ppPixels, int x, int y, int w, int h) = 0;
	virtual void TakeScreenshot(const char *pFilename) = 0;
	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen) = 0;

	virtual int GetDesktopScreenWidth() const = 0;
	virtual int GetDesktopScreenHeight() const = 0;

	virtual void Swap() = 0;
	virtual int GetNumScreens() const = 0;


	// syncronization
	virtual void InsertSignal(class semaphore *pSemaphore) = 0;
	virtual bool IsIdle() const = 0;
	virtual void WaitForIdle() = 0;

protected:
	inline CTextureHandle CreateTextureHandle(int Index)
	{
		CTextureHandle Tex;
		Tex.m_Id = Index;
		return Tex;
	}
};

class IEngineGraphics : public IGraphics
{
	MACRO_INTERFACE("enginegraphics", 0)
public:
	virtual int Init() = 0;
	virtual void Shutdown() = 0;

	virtual bool Fullscreen(bool State) = 0;
	virtual void SetWindowBordered(bool State) = 0;
	virtual bool SetWindowScreen(int Index) = 0;
	virtual bool SetVSync(bool State) = 0;
	virtual int GetWindowScreen() = 0;

	virtual void Minimize() = 0;
	virtual void Maximize() = 0;

	virtual int WindowActive() = 0;
	virtual int WindowOpen() = 0;

};

extern IEngineGraphics *CreateEngineGraphics(); // NOTE: not used
extern IEngineGraphics *CreateEngineGraphicsThreaded();

#endif
