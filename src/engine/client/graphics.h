#include "../e_if_gfx.h"

class IGraphics
{
protected:
	int m_ScreenWidth;
	int m_ScreenHeight;
public:
	virtual ~IGraphics() {}
	
	int ScreenWidth() const { return m_ScreenWidth; }
	int ScreenHeight() const { return m_ScreenHeight; }
	float ScreenAspect() const { return (float)ScreenWidth()/(float)ScreenHeight(); }
	
	virtual void Clear(float r, float g, float b) = 0;
	
	virtual void ClipEnable(int x, int y, int w, int h) = 0;
	virtual void ClipDisable() = 0;
	
	virtual void MapScreen(float tl_x, float tl_y, float br_x, float br_y) = 0;
	virtual void GetScreen(float *tl_x, float *tl_y, float *br_x, float *br_y) = 0;
	
	virtual void BlendNone() = 0;
	virtual void BlendNormal() = 0;
	virtual void BlendAdditive() = 0;
	
	virtual int LoadPNG(IMAGE_INFO *pImg, const char *pFilename) =0;
	virtual int UnloadTexture(int Index) = 0;
	virtual int LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags) = 0;
	virtual int LoadTexture(const char *pFilename, int StoreFormat, int Flags) = 0;
	virtual void TextureSet(int TextureID) = 0;
	
	virtual void LinesBegin() = 0;
	virtual void LinesEnd() = 0;
	virtual void LinesDraw(float x0, float y0, float x1, float y1) = 0;
	
	virtual void QuadsBegin() = 0;
	virtual void QuadsEnd() = 0;
	virtual void QuadsSetRotation(float Angle) = 0;
	virtual void QuadsSetSubset(float tl_u, float tl_v, float br_u, float br_v) = 0;
	virtual void QuadsSetSubsetFree(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) = 0;
	
	virtual void QuadsDraw(float x, float y, float w, float h) = 0;
	virtual void QuadsDrawTL(float x, float y, float w, float h) = 0;
	virtual void QuadsDrawFreeform(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) = 0;
	virtual void QuadsText(float x, float y, float Size, float r, float g, float b, float a, const char *pText) = 0;
	
	virtual void SetColorVertex(int i, float r, float g, float b, float a) = 0;
	virtual void SetColor(float r, float g, float b, float a) = 0;
	
	virtual void TakeScreenshot() = 0;
};

class IEngineGraphics : public IGraphics
{
public:
	virtual bool Init() = 0;
	virtual void Shutdown() = 0;
	virtual void Swap() = 0;
	
	virtual void Minimize() = 0;
	virtual void Maximize() = 0;
	
	virtual int WindowActive() = 0;
	virtual int WindowOpen() = 0;
	
};

extern IEngineGraphics *CreateEngineGraphics();
