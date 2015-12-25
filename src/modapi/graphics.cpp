#include <modapi/graphics.h>
#include <engine/mod.h>
#include <modapi/mod.h>

CModAPI_Client_Graphics::CModAPI_Client_Graphics()
{
	
}

const CModAPI_Image* CModAPI_Client_Graphics::GetImage(int Id) const
{
	if(Id < 0 || Id >= m_Images.size()) return 0;
	
	return &m_Images[Id];
}

const CModAPI_Sprite* CModAPI_Client_Graphics::GetSprite(int Id) const
{
	if(Id < 0 || Id >= m_Sprites.size()) return 0;
	
	return &m_Sprites[Id];
}

int CModAPI_Client_Graphics::OnModLoaded(IMod* pMod, IGraphics* pGraphics)
{
	//Load images
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_IMAGE, &Start, &Num);
		
		m_Images.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Image *pItem = (CModAPI_ModItem_Image*) pMod->GetItem(Start+i, 0, 0);
			
			if(pItem->m_Id > Num) return 0;
			
			CModAPI_Image* pImage = &m_Images[pItem->m_Id];
			
			// copy base info
			pImage->m_Width = pItem->m_Width;
			pImage->m_Height = pItem->m_Height;
			pImage->m_Format = pItem->m_Format;
			int PixelSize = pImage->m_Format == CImageInfo::FORMAT_RGB ? 3 : 4;

			// copy image data
			void *pData = pMod->GetData(pItem->m_ImageData);
			pImage->m_pData = mem_alloc(pImage->m_Width*pImage->m_Height*PixelSize, 1);
			mem_copy(pImage->m_pData, pData, pImage->m_Width*pImage->m_Height*PixelSize);
			pImage->m_Texture = pGraphics->LoadTextureRaw(pImage->m_Width, pImage->m_Height, pImage->m_Format, pImage->m_pData, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

			// unload image
			pMod->UnloadData(pItem->m_ImageData);
		}
	}
	
	//Load sprites
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_SPRITE, &Start, &Num);
		
		m_Sprites.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Sprite *pItem = (CModAPI_ModItem_Sprite *) pMod->GetItem(Start+i, 0, 0);
			
			if(pItem->m_Id > Num) return 0;
			
			CModAPI_Sprite* sprite = &m_Sprites[pItem->m_Id];
			sprite->m_X = pItem->m_X;
			sprite->m_Y = pItem->m_Y;
			sprite->m_W = pItem->m_W;
			sprite->m_H = pItem->m_H;
			sprite->m_External = pItem->m_External;
			sprite->m_ImageId = pItem->m_ImageId;
			sprite->m_GridX = pItem->m_GridX;
			sprite->m_GridY = pItem->m_GridY;
		}
	}
	
	return 1;
}
