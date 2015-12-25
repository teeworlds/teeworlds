#ifndef MODAPI_GRAPHICS_H
#define MODAPI_GRAPHICS_H

#include <base/tl/array.h>
#include <engine/graphics.h>

class IMod;

enum
{
	MODAPI_INTERNALIMG_GAME = 0,
	MODAPI_NB_INTERNALIMG,
};

class CModAPI_Image : public CImageInfo
{
public:
	IGraphics::CTextureHandle m_Texture;
};

struct CModAPI_Sprite
{
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	int m_External;
	int m_ImageId;
	int m_GridX;
	int m_GridY;
};

class CModAPI_Client_Graphics
{
private:
	array<CModAPI_Image> m_Images;
	array<CModAPI_Sprite> m_Sprites;
	
public:
	CModAPI_Client_Graphics();
	const CModAPI_Sprite* GetSprite(int Id) const;
	const CModAPI_Image* GetImage(int Id) const;
	
	int OnModLoaded(IMod* pMod, IGraphics* pGraphics);
};

#endif
