#include <modapi/client/graphics.h>
#include <modapi/moditem.h>
#include <modapi/shared/mod.h>
#include <generated/client_data.h>
#include <game/client/render.h>

#include <game/gamecore.h>

CModAPI_Client_Graphics::CModAPI_Client_Graphics(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
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

int CModAPI_Client_Graphics::OnModLoaded(IMod* pMod)
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
			pImage->m_Texture = m_pGraphics->LoadTextureRaw(pImage->m_Width, pImage->m_Height, pImage->m_Format, pImage->m_pData, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

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

int CModAPI_Client_Graphics::OnModUnloaded()
{
	//Unload images
	for(int i = 0; i < m_Images.size(); i++)
	{
		m_pGraphics->UnloadTexture(m_Images[i].m_Texture);
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

void CModAPI_Client_Graphics::DrawSprite(CRenderTools* pRenderTools,int SpriteID, vec2 Pos, float Size, float Angle)
{
	const CModAPI_Sprite* pSprite = GetSprite(SpriteID);
	if(pSprite == 0) return;
	
	if(pSprite->m_External)
	{
		const CModAPI_Image* pImage = GetImage(pSprite->m_ImageId);
		if(pImage == 0) return;
		
		m_pGraphics->BlendNormal();
		m_pGraphics->TextureSet(pImage->m_Texture);
		m_pGraphics->QuadsBegin();
	}
	else
	{
		int Texture;
		
		switch(pSprite->m_ImageId)
		{
			case MODAPI_INTERNALIMG_GAME:
				Texture = IMAGE_GAME;
				break;
			default:
				return;
		}
		
		m_pGraphics->BlendNormal();
		m_pGraphics->TextureSet(g_pData->m_aImages[Texture].m_Id);
		m_pGraphics->QuadsBegin();
	}

	pRenderTools->SelectModAPISprite(pSprite);
	
	m_pGraphics->QuadsSetRotation(Angle);

	
	pRenderTools->DrawSprite(Pos.x, Pos.y, Size);
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawLine(CRenderTools* pRenderTools,int LineStyleID, vec2 StartPoint, vec2 EndPoint, float Ms)
{
	const CModAPI_LineStyle* pLineStyle = GetLineStyle(LineStyleID);
	if(pLineStyle == 0) return;
	
	vec2 Dir = normalize(EndPoint-StartPoint);
	vec2 DirOrtho = vec2(-Dir.y, Dir.x);
	float Length = distance(StartPoint, EndPoint);
	float ScaleFactor = 1.0f;
	if(pLineStyle->m_AnimationType == MODAPI_LINESTYLE_ANIMATION_SCALEDOWN)
	{
		float Speed = Ms / pLineStyle->m_AnimationSpeed;
		ScaleFactor = 1.0f - clamp(Speed, 0.0f, 1.0f);
	}

	m_pGraphics->BlendNormal();
	m_pGraphics->TextureClear();
	m_pGraphics->QuadsBegin();

	//Outer line
	if(pLineStyle->m_OuterWidth > 0)
	{
		float Width = pLineStyle->m_OuterWidth;
		const vec4& Color = pLineStyle->m_OuterColor;
		
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPoint.x-Out.x, StartPoint.y-Out.y,
				StartPoint.x+Out.x, StartPoint.y+Out.y,
				EndPoint.x-Out.x, EndPoint.y-Out.y,
				EndPoint.x+Out.x, EndPoint.y+Out.y);
		m_pGraphics->QuadsDrawFreeform(&Freeform, 1);
	}

	//Inner line
	if(pLineStyle->m_InnerWidth > 0)
	{
		float Width = pLineStyle->m_InnerWidth;
		const vec4& Color = pLineStyle->m_InnerColor;
		
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPoint.x-Out.x, StartPoint.y-Out.y,
				StartPoint.x+Out.x, StartPoint.y+Out.y,
				EndPoint.x-Out.x, EndPoint.y-Out.y,
				EndPoint.x+Out.x, EndPoint.y+Out.y);
		m_pGraphics->QuadsDrawFreeform(&Freeform, 1);
	}

	m_pGraphics->QuadsEnd();
	
	//Sprite for line
	if(pLineStyle->m_LineSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_LineSprite1;
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
			
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			m_pGraphics->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			m_pGraphics->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		IGraphics::CQuadItem Array[1024];
		int i = 0;
		float step = pLineStyle->m_LineSpriteSizeX - pLineStyle->m_LineSpriteOverlapping;
		for(float f = pLineStyle->m_LineSpriteSizeX/2.0f; f < Length && i < 1024; f += step, i++)
		{
			vec2 p = StartPoint + Dir*f;
			Array[i] = IGraphics::CQuadItem(p.x, p.y, pLineStyle->m_LineSpriteSizeX, pLineStyle->m_LineSpriteSizeY * ScaleFactor);
		}

		m_pGraphics->QuadsDraw(Array, i);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
	}
	
	//Sprite for start point
	if(pLineStyle->m_StartPointSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_StartPointSprite1;
		
		if(pLineStyle->m_StartPointSprite2 > SpriteId) //Animation
		{
			int NbFrame = 1 + pLineStyle->m_StartPointSprite2 - pLineStyle->m_StartPointSprite1;
			SpriteId = pLineStyle->m_StartPointSprite1 + (static_cast<int>(Ms)/pLineStyle->m_StartPointSpriteAnimationSpeed)%NbFrame;
		}
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			m_pGraphics->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			m_pGraphics->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		vec2 NewPos = StartPoint + (Dir * pLineStyle->m_StartPointSpriteX) + (DirOrtho * pLineStyle->m_StartPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_StartPointSpriteSizeX,
			pLineStyle->m_StartPointSpriteSizeY);
		
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
	}
	
	//Sprite for end point
	if(pLineStyle->m_EndPointSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_EndPointSprite1;
		
		if(pLineStyle->m_EndPointSprite2 > SpriteId) //Animation
		{
			int NbFrame = 1 + pLineStyle->m_EndPointSprite2 - pLineStyle->m_EndPointSprite1;
			SpriteId = pLineStyle->m_EndPointSprite1 + (static_cast<int>(Ms)/pLineStyle->m_EndPointSpriteAnimationSpeed)%NbFrame;
		}
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			m_pGraphics->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			m_pGraphics->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		vec2 NewPos = EndPoint + (Dir * pLineStyle->m_EndPointSpriteX) + (DirOrtho * pLineStyle->m_EndPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_EndPointSpriteSizeX,
			pLineStyle->m_EndPointSpriteSizeY);
		
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
	}
	return;
}

void CModAPI_Client_Graphics::DrawText(ITextRender* pTextRender, const int *pText, vec2 Pos, int RGBA, float Size, int Alignment)
{	
	char aText[64];
	IntsToStr(pText, 16, &aText[0]);
	
	float width = pTextRender-> TextWidth(0, Size, aText, -1);
	float height = pTextRender->TextHeight(Size);
	
	switch(Alignment)
	{
		case MODAPI_TEXTALIGN_CENTER:
			Pos.x -= width/2;
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_RIGHT_BOTTOM:
			break;
		case MODAPI_TEXTALIGN_RIGHT_CENTER:
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_RIGHT_TOP:
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_CENTER_TOP:
			Pos.x -= width/2;
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_LEFT_TOP:
			Pos.x -= width;
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_LEFT_CENTER:
			Pos.x -= width;
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_LEFT_BOTTOM:
			Pos.x -= width;
			break;
		case MODAPI_TEXTALIGN_CENTER_BOTTOM:
			Pos.y -= height/2;
			break;
	}
	
	vec4 Color = ModAPI_IntToColor(RGBA);
	pTextRender->TextColor(Color.r,Color.g,Color.b,Color.a);
	pTextRender->Text(0, Pos.x, Pos.y, Size, aText, -1);
	
	//reset color
	pTextRender->TextColor(255,255,255,1);
}
