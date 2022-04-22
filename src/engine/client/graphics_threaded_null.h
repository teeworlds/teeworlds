/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_GRAPHICS_THREADED_NULL_H
#define ENGINE_CLIENT_GRAPHICS_THREADED_NULL_H

#include <engine/graphics.h>

class CGraphics_ThreadedNull : public IEngineGraphics
{
public:
	CGraphics_ThreadedNull()
	{
		m_ScreenWidth = 800;
		m_ScreenHeight = 600;
		m_DesktopScreenWidth = 800;
		m_DesktopScreenHeight = 600;
	};

	virtual void ClipEnable(int x, int y, int w, int h) {};
	virtual void ClipDisable() {};

	virtual void BlendNone() {};
	virtual void BlendNormal() {};
	virtual void BlendAdditive() {};

	virtual void WrapNormal() {};
	virtual void WrapClamp() {};
	virtual void WrapMode(int WrapU, int WrapV) {};

	virtual int MemoryUsage() const { return 0; };

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY) {};
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY)
	{
		*pTopLeftX = 0;
		*pTopLeftY = 0;
		*pBottomRightX = 600;
		*pBottomRightY = 600;
	};

	virtual void LinesBegin() {};
	virtual void LinesEnd() {};
	virtual void LinesDraw(const CLineItem *pArray, int Num) {};

	virtual int UnloadTexture(IGraphics::CTextureHandle *Index) { return 0; };
	virtual IGraphics::CTextureHandle LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags) { return CreateTextureHandle(0); };
	virtual int LoadTextureRawSub(IGraphics::CTextureHandle TextureID, int x, int y, int Width, int Height, int Format, const void *pData) { return 0; };

	// simple uncompressed RGBA loaders
	virtual IGraphics::CTextureHandle LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags) { return CreateTextureHandle(0); };
	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType) { return 0; };

	virtual void TextureSet(CTextureHandle TextureID) {};

	virtual void Clear(float r, float g, float b) {};

	virtual void QuadsBegin() {};
	virtual void QuadsEnd() {};
	virtual void QuadsSetRotation(float Angle) {};

	virtual void SetColorVertex(const CColorVertex *pArray, int Num) {};
	virtual void SetColor(float r, float g, float b, float a) {};
	virtual void SetColor4(const vec4 &TopLeft, const vec4 &TopRight, const vec4 &BottomLeft, const vec4 &BottomRight) {};

	virtual void QuadsSetSubset(float TlU, float TlV, float BrU, float BrV, int TextureIndex = -1) {};
	virtual void QuadsSetSubsetFree (
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3, int TextureIndex = -1) {};

	virtual void QuadsDraw(CQuadItem *pArray, int Num) {};
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num) {};
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num) {};
	virtual void QuadsText(float x, float y, float Size, const char *pText) {};

	virtual int GetNumScreens() const { return 0; };
	virtual void Minimize() {};
	virtual void Maximize() {};
	virtual bool Fullscreen(bool State) { return false; };
	virtual void SetWindowBordered(bool State) {};
	virtual bool SetWindowScreen(int Index) { return false; };
	virtual int GetWindowScreen() { return 0; };

	virtual int WindowActive() { return 0; };
	virtual int WindowOpen() { return 0; };

	virtual int Init() { return 0; };
	virtual void Shutdown() {};

	virtual void ReadBackbuffer(unsigned char **ppPixels, int x, int y, int w, int h) {};
	virtual void TakeScreenshot(const char *pFilename) {};
	virtual void Swap() {};
	virtual bool SetVSync(bool State) { return false; };

	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen) { return 0; };

	// syncronization
	virtual void InsertSignal(semaphore *pSemaphore) {};
	virtual bool IsIdle() const { return false; };
	virtual void WaitForIdle() {};
};

#endif // ENGINE_CLIENT_GRAPHICS_THREADED_NULL_H
