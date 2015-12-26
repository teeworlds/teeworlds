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
	int m_LineSprite0;
	int m_LineSprite1;
	int m_LineSpriteSizeX;
	int m_LineSpriteSizeY;
	int m_LineAnimationType;
	int m_LineAnimationSpeed;
	int m_LineSpriteSpeed;
	
	CModAPI_ModItem_LineStyle& SetOuter(int Width, const vec4& Color);
	CModAPI_ModItem_LineStyle& SetInner(int Width, const vec4& Color);
	CModAPI_ModItem_LineStyle& SetLineSprite(int SpriteId0, int SpriteId1, int SpriteSizeX, int SpriteSizeY, int Speed);
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
