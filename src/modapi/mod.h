#ifndef MODAPI_MOD_H
#define MODAPI_MOD_H

#include <base/tl/array.h>
#include <modapi/graphics.h>

class IStorage;

enum
{
	MODAPI_MODITEMTYPE_SPRITE = 0,
};

struct CModAPI_ModItem_Image
{
	int m_Width;
	int m_Height;
	int m_ImageData;
	int m_Format;
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
	array<CModAPI_ModItem_Sprite> m_Sprites;
	
public:
	CModAPI_ModCreator();
	
	int AddSprite(int ImageId, int x, int y, int w, int h, int gx, int gy);
	
	int Save(class IStorage *pStorage, const char *pFileName);
};

#endif
