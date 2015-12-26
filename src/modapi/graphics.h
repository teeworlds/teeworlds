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

enum
{
	MODAPI_LINESTYLEANIM_NONE = 0,
	MODAPI_LINESTYLEANIM_SCALEDOWN,
};

struct CModAPI_LineStyle
{
	float m_OuterWidth;
	vec4 m_OuterColor;
	float m_InnerWidth;
	vec4 m_InnerColor;
	int m_LineSprite0;
	int m_LineSprite1;
	float m_LineSpriteSizeX;
	float m_LineSpriteSizeY;
	int m_AnimationType;
	float m_AnimationSpeed;
	float m_LineSpriteSpeed;
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
};

int ModAPI_ColorToInt(const vec4& Color);
vec4 ModAPI_IntToColor(int Value);

#endif
