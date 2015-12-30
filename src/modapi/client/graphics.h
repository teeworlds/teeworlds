#ifndef MODAPI_CLIENT_GRAPHICS_H
#define MODAPI_CLIENT_GRAPHICS_H

#include <base/tl/array.h>
#include <engine/graphics.h>

#include <modapi/graphics.h>

class IMod;

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

struct CModAPI_LineStyle
{
	//Line with quads, like laser
	int m_OuterWidth;
	vec4 m_OuterColor;
	int m_InnerWidth;
	vec4 m_InnerColor;

	//Draw line with sprites, like hook
	int m_LineSpriteType; //MODAPI_LINESTYLE_SPRITETYPE_XXXXX
	int m_LineSprite1;
	int m_LineSprite2;
	int m_LineSpriteSizeX;
	int m_LineSpriteSizeY;
	int m_LineSpriteOverlapping;
	int m_LineSpriteAnimationSpeed;

	//Start point sprite
	int m_StartPointSprite1;
	int m_StartPointSprite2;
	int m_StartPointSpriteX;
	int m_StartPointSpriteY;
	int m_StartPointSpriteSizeX;
	int m_StartPointSpriteSizeY;
	int m_StartPointSpriteAnimationSpeed;
	
	//End point prite
	int m_EndPointSprite1;
	int m_EndPointSprite2;
	int m_EndPointSpriteX;
	int m_EndPointSpriteY;
	int m_EndPointSpriteSizeX;
	int m_EndPointSpriteSizeY;
	int m_EndPointSpriteAnimationSpeed;
	
	//General information
	int m_AnimationType; //MODAPI_LINESTYLE_ANIMATION_XXXXX
	int m_AnimationSpeed;
};

class CModAPI_Client_Graphics
{
private:
	array<CModAPI_Image> m_Images;
	array<CModAPI_Sprite> m_Sprites;
	array<CModAPI_LineStyle> m_LineStyles;
	
public:
	CModAPI_Client_Graphics();
	const CModAPI_Image* GetImage(int Id) const;
	const CModAPI_Sprite* GetSprite(int Id) const;
	const CModAPI_LineStyle* GetLineStyle(int Id) const;
	
	int OnModLoaded(IMod* pMod, IGraphics* pGraphics);
	int OnModUnloaded(IGraphics* pGraphics);
};

#endif
