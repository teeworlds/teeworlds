#ifndef MODAPI_MOD_H
#define MODAPI_MOD_H

#include <base/tl/array.h>
#include <modapi/graphics.h>

class IStorage;

enum
{
	MODAPI_MODITEMTYPE_IMAGE = 0,
	MODAPI_MODITEMTYPE_SPRITE,
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

class CModAPI_ModCreator
{
private:
	array<void*> m_ImagesData;
	array<CModAPI_ModItem_Image> m_Images;
	array<CModAPI_ModItem_Sprite> m_Sprites;
	
	int AddSprite(int ImageId, int x, int External, int y, int w, int h, int gx, int gy);
	
public:
	CModAPI_ModCreator();
	
	int AddImage(IStorage* pStorage, const char* Filename);
	int AddSpriteInternal(int ImageId, int x, int y, int w, int h, int gx, int gy);
	int AddSpriteExternal(int ImageId, int x, int y, int w, int h, int gx, int gy);
	
	int Save(class IStorage *pStorage, const char *pFileName);
};

#endif
