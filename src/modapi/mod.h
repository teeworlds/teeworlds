#ifndef MODAPI_MOD_H
#define MODAPI_MOD_H

#include <base/tl/array.h>
#include <modapi/graphics.h>

class IStorage;

enum
{
	MODAPI_MODITEMTYPE_IMAGE = 0,
	MODAPI_MODITEMTYPE_SPRITE,
	MODAPI_MODITEMTYPE_LINESTYLE,
};

struct CModAPI_ModItem_Image
{
	int m_Id;
	int m_Width;
	int m_Height;
	int m_Format;
	int m_ImageData;
};

struct CModAPI_ModItem_Sprite
{
	int m_Id;
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	int m_External;
	int m_ImageId;
	int m_GridX;
	int m_GridY;
};

class CModAPI_ModItem_LineStyle
{
public:
	int m_Id;
	
	int m_OuterWidth;
	int m_OuterColor;
	int m_InnerWidth;
	int m_InnerColor;

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
	
	CModAPI_ModItem_LineStyle& SetOuter(int Width, const vec4& Color);
	CModAPI_ModItem_LineStyle& SetInner(int Width, const vec4& Color);
	CModAPI_ModItem_LineStyle& SetLineRepeatedSprite(int SpriteId1, int SpriteSizeX, int SpriteSizeY);
	CModAPI_ModItem_LineStyle& SetStartPointSprite(int SpriteId1, int X, int Y, int W, int H);
	CModAPI_ModItem_LineStyle& SetEndPointSprite(int SpriteId1, int X, int Y, int W, int H);
	CModAPI_ModItem_LineStyle& SetLineAnimation(int Type, int Speed);
};

class CModAPI_ModCreator
{
private:
	array<void*> m_ImagesData;
	array<CModAPI_ModItem_Image> m_Images;
	array<CModAPI_ModItem_Sprite> m_Sprites;
	array<CModAPI_ModItem_LineStyle> m_LineStyles;
	
	int AddSprite(int ImageId, int x, int External, int y, int w, int h, int gx, int gy);
	
public:
	CModAPI_ModCreator();
	
	int AddImage(IStorage* pStorage, const char* Filename);
	int AddSpriteInternal(int ImageId, int x, int y, int w, int h, int gx, int gy);
	int AddSpriteExternal(int ImageId, int x, int y, int w, int h, int gx, int gy);
	CModAPI_ModItem_LineStyle& AddLineStyle();
	
	int Save(class IStorage *pStorage, const char *pFileName);
};

#endif
