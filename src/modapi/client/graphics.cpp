#include <modapi/client/graphics.h>
#include <modapi/moditem.h>
#include <modapi/shared/mod.h>

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

const CModAPI_LineStyle* CModAPI_Client_Graphics::GetLineStyle(int Id) const
{
	if(Id < 0 || Id >= m_LineStyles.size()) return 0;
	
	return &m_LineStyles[Id];
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
	
	//Load line styles
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_LINESTYLE, &Start, &Num);
		
		m_LineStyles.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_LineStyle *pItem = (CModAPI_ModItem_LineStyle*) pMod->GetItem(Start+i, 0, 0);
			
			if(pItem->m_Id > Num) return 0;
			
			CModAPI_LineStyle* pLineStyle = &m_LineStyles[pItem->m_Id];
			
			pLineStyle->m_OuterWidth = static_cast<float>(pItem->m_OuterWidth);
			pLineStyle->m_OuterColor = ModAPI_IntToColor(pItem->m_OuterColor);
			pLineStyle->m_InnerWidth = static_cast<float>(pItem->m_InnerWidth);
			pLineStyle->m_InnerColor = ModAPI_IntToColor(pItem->m_InnerColor);
			
			pLineStyle->m_LineSpriteType = pItem->m_LineSpriteType;
			pLineStyle->m_LineSprite1 = pItem->m_LineSprite1;
			pLineStyle->m_LineSprite2 = pItem->m_LineSprite2;
			pLineStyle->m_LineSpriteSizeX = pItem->m_LineSpriteSizeX;
			pLineStyle->m_LineSpriteSizeY = pItem->m_LineSpriteSizeY;
			pLineStyle->m_LineSpriteOverlapping = pItem->m_LineSpriteOverlapping;
			pLineStyle->m_LineSpriteAnimationSpeed = pItem->m_LineSpriteAnimationSpeed;
			
			pLineStyle->m_StartPointSprite1 = pItem->m_StartPointSprite1;
			pLineStyle->m_StartPointSprite2 = pItem->m_StartPointSprite2;
			pLineStyle->m_StartPointSpriteX = pItem->m_StartPointSpriteX;
			pLineStyle->m_StartPointSpriteY = pItem->m_StartPointSpriteY;
			pLineStyle->m_StartPointSpriteSizeX = pItem->m_StartPointSpriteSizeX;
			pLineStyle->m_StartPointSpriteSizeY = pItem->m_StartPointSpriteSizeY;
			pLineStyle->m_StartPointSpriteAnimationSpeed = pItem->m_StartPointSpriteAnimationSpeed;
			
			pLineStyle->m_EndPointSprite1 = pItem->m_EndPointSprite1;
			pLineStyle->m_EndPointSprite2 = pItem->m_EndPointSprite2;
			pLineStyle->m_EndPointSpriteX = pItem->m_EndPointSpriteX;
			pLineStyle->m_EndPointSpriteY = pItem->m_EndPointSpriteY;
			pLineStyle->m_EndPointSpriteSizeX = pItem->m_EndPointSpriteSizeX;
			pLineStyle->m_EndPointSpriteSizeY = pItem->m_EndPointSpriteSizeY;
			pLineStyle->m_EndPointSpriteAnimationSpeed = pItem->m_EndPointSpriteAnimationSpeed;
			
			pLineStyle->m_AnimationType = pItem->m_AnimationType;
			pLineStyle->m_AnimationSpeed = static_cast<float>(pItem->m_AnimationSpeed);
		}
	}
	
	return 1;
}

int CModAPI_Client_Graphics::OnModUnloaded(IGraphics* pGraphics)
{
	//Unload images
	for(int i = 0; i < m_Images.size(); i++)
	{
		pGraphics->UnloadTexture(m_Images[i].m_Texture);
		if(m_Images[i].m_pData)
		{
			mem_free(m_Images[i].m_pData);
			m_Images[i].m_pData = 0;
		}
	}
	
	m_Images.clear();
	
	//Unload sprites
	m_Sprites.clear();
	
	//Unload line styles
	m_LineStyles.clear();
	
	return 1;
}
