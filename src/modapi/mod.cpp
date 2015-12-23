#include <modapi/mod.h>

#include <engine/storage.h>
#include <engine/shared/datafile.h>

#include <engine/external/pnglite/pnglite.h>

CModAPI_ModCreator::CModAPI_ModCreator()
{
	
}
	
//~ int CModAPI_ModCreator::AddImage(const char* pFileName)
//~ {
	//~ //Load image
	
	//~ return true;
//~ }

int CModAPI_ModCreator::AddSprite(int ImageId, int x, int y, int w, int h, int gx, int gy)
{
	if(ImageId >= MODAPI_NB_INTERNALIMG || ImageId < 0) return -1;
	
	CModAPI_ModItem_Sprite sprite;
	sprite.m_Id = m_Sprites.size();
	sprite.m_X = x;
	sprite.m_Y = y;
	sprite.m_W = w;
	sprite.m_H = h;
	sprite.m_External = 0;
	sprite.m_ImageId = ImageId;
	sprite.m_GridX = gx;
	sprite.m_GridY = gy;
	
	m_Sprites.add(sprite);
	
	return sprite.m_Id;
}

int CModAPI_ModCreator::Save(class IStorage *pStorage, const char *pFileName)
{
	CDataFileWriter df;
	if(!df.Open(pStorage, pFileName)) return 0;
	
	//Save sprites
	for(int i=0; i<m_Sprites.size(); i++)
	{
		df.AddItem(MODAPI_MODITEMTYPE_SPRITE, i, sizeof(CModAPI_ModItem_Sprite), &m_Sprites[i]);
	}
	
	df.Finish();
	
	return 1;
}
