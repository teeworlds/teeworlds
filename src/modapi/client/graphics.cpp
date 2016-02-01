#include <modapi/client/graphics.h>
#include <modapi/moditem.h>
#include <modapi/shared/mod.h>
#include <generated/client_data.h>
#include <game/client/render.h>
#include <engine/shared/config.h>

#include <game/gamecore.h>

CModAPI_Client_Graphics::CModAPI_Client_Graphics(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
	
	//Images
	m_InternalImages[MODAPI_IMAGE_GAME].m_Texture = g_pData->m_aImages[IMAGE_GAME].m_Id;
	
	//Animations
	vec2 BodyPos = vec2(0.0f, -4.0f);
	vec2 FootPos = vec2(0.0f, 10.0f);
	
		//Idle
	m_InternalAnimations[MODAPI_ANIMATION_IDLE_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(-7.0f,  0.0f),  0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_IDLE_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(7.0f,  0.0f),  0.0f, 1.f);
	
		//InAir
	m_InternalAnimations[MODAPI_ANIMATION_INAIR_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(-3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_INAIR_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	
		//Walk
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.2f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.4f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.6f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.8f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(1.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(  8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.2f, FootPos + vec2( -8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.4f, FootPos + vec2(-10.0f, -4.0f),  0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.6f, FootPos + vec2( -8.0f, -8.0f),  0.3f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.8f, FootPos + vec2(  4.0f, -4.0f), -0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(1.0f, FootPos + vec2(  8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(-10.0f, -4.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.2f, FootPos + vec2( -8.0f, -8.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.4f, FootPos + vec2(  4.0f, -4.0f),  0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.6f, FootPos + vec2(  8.0f,  0.0f),  0.3f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.8f, FootPos + vec2(  8.0f,  0.0f), -0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(1.0f, FootPos + vec2(-10.0f, -4.0f),  0.0f*pi*2.0f, 1.f);
	
	//TeeAnimations
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_BodyAnimation = -1;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_IDLE_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_IDLE_FRONTFOOT);
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_BodyAnimation = -1;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_INAIR_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_INAIR_FRONTFOOT);
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_BodyAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_BODY);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_FRONTFOOT);
}

const CModAPI_Image* CModAPI_Client_Graphics::GetImage(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_IMAGES)
			return 0;
		else
			return &m_InternalImages[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalImages.size())
			return 0;
		else
			return &m_ExternalImages[Id];
	}
}

const CModAPI_Animation* CModAPI_Client_Graphics::GetAnimation(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_ANIMATIONS)
			return 0;
		else
			return &m_InternalAnimations[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalAnimations.size())
			return 0;
		else
			return &m_ExternalAnimations[Id];
	}
}

const CModAPI_TeeAnimation* CModAPI_Client_Graphics::GetTeeAnimation(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_TEEANIMATIONS)
			return 0;
		else
			return &m_InternalTeeAnimations[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalTeeAnimations.size())
			return 0;
		else
			return &m_ExternalTeeAnimations[Id];
	}
}

const CModAPI_Sprite* CModAPI_Client_Graphics::GetSprite(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_SPRITES)
			return 0;
		else
			return &m_InternalSprites[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalSprites.size())
			return 0;
		else
			return &m_ExternalSprites[Id];
	}
}

const CModAPI_LineStyle* CModAPI_Client_Graphics::GetLineStyle(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_LINESTYLES)
			return 0;
		else
			return &m_InternalLineStyles[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalLineStyles.size())
			return 0;
		else
			return &m_ExternalLineStyles[Id];
	}
}

int CModAPI_Client_Graphics::OnModLoaded(IMod* pMod)
{
	//Load images
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_IMAGE, &Start, &Num);
		
		m_ExternalImages.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Image *pItem = (CModAPI_ModItem_Image*) pMod->GetItem(Start+i, 0, 0);
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			CModAPI_Image* pImage = &m_ExternalImages[Id];
			
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
	
	//Load animations
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_ANIMATION, &Start, &Num);
		
		m_ExternalAnimations.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Animation *pItem = (CModAPI_ModItem_Animation *) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			const CModAPI_AnimationKeyFrame* pFrames = static_cast<CModAPI_AnimationKeyFrame*>(pMod->GetData(pItem->m_KeyFrameData));
			
			CModAPI_Animation* Animation = &m_ExternalAnimations[Id];
			for(int f=0; f<pItem->m_NumKeyFrame; f++)
			{
				Animation->AddKeyFrame(pFrames[f].m_Time, pFrames[f].m_Pos, pFrames[f].m_Angle, pFrames[f].m_Opacity);
			}
		}
	}
	
	//Load sprites
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_SPRITE, &Start, &Num);
		
		m_ExternalSprites.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Sprite *pItem = (CModAPI_ModItem_Sprite *) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			CModAPI_Sprite* sprite = &m_ExternalSprites[Id];
			sprite->m_X = pItem->m_X;
			sprite->m_Y = pItem->m_Y;
			sprite->m_W = pItem->m_W;
			sprite->m_H = pItem->m_H;
			sprite->m_ImageId = pItem->m_ImageId;
			sprite->m_GridX = pItem->m_GridX;
			sprite->m_GridY = pItem->m_GridY;
		}
	}
	
	//Load line styles
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_LINESTYLE, &Start, &Num);
		
		m_ExternalLineStyles.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_LineStyle *pItem = (CModAPI_ModItem_LineStyle*) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id > Num) return 0;
			
			CModAPI_LineStyle* pLineStyle = &m_ExternalLineStyles[Id];
			
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
	for(int i = 0; i < m_ExternalImages.size(); i++)
	{
		m_pGraphics->UnloadTexture(m_ExternalImages[i].m_Texture);
		if(m_ExternalImages[i].m_pData)
		{
			mem_free(m_ExternalImages[i].m_pData);
			m_ExternalImages[i].m_pData = 0;
		}
	}
	
	m_ExternalImages.clear();
	
	//Unload sprites
	m_ExternalSprites.clear();
	
	//Unload line styles
	m_ExternalLineStyles.clear();
	
	return 1;
}

bool CModAPI_Client_Graphics::TextureSet(int ImageID)
{
	const CModAPI_Image* pImage = GetImage(ImageID);
	if(pImage == 0)
		return false;
	
	m_pGraphics->BlendNormal();
	m_pGraphics->TextureSet(pImage->m_Texture);
	m_pGraphics->QuadsBegin();
	
	return true;
}

void CModAPI_Client_Graphics::DrawSprite(CRenderTools* pRenderTools,int SpriteID, vec2 Pos, float Size, float Angle)
{
	const CModAPI_Sprite* pSprite = GetSprite(SpriteID);
	if(pSprite == 0) return;
	
	if(!TextureSet(pSprite->m_ImageId))
		return;

	pRenderTools->SelectModAPISprite(pSprite);
	
	m_pGraphics->QuadsSetRotation(Angle);

	pRenderTools->DrawSprite(Pos.x, Pos.y, Size);
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawAnimatedSprite(CRenderTools* pRenderTools, int SpriteID, vec2 Pos, float Size, float Angle, int AnimationID, float Time, vec2 Offset)
{
	const CModAPI_Animation* pAnimation = GetAnimation(AnimationID);
	if(pAnimation == 0) return;
	
	CModAPI_AnimationFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	float animX = Frame.m_Pos.x + Offset.x - Offset.x * cos(Frame.m_Angle) + Offset.y * sin(Frame.m_Angle);
	float animY = Frame.m_Pos.y + Offset.y - Offset.x * sin(Frame.m_Angle) + Offset.y * cos(Frame.m_Angle);
	
	float X = Pos.x + (animX * cos(Angle) + animY * sin(Angle));
	float Y = Pos.y + (animX * sin(Angle) + animY * cos(Angle));
	
	DrawSprite(pRenderTools, SpriteID, vec2(X, Y), Size, Angle + Frame.m_Angle);
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
		if(!TextureSet(pSprite->m_ImageId))
			return;
		
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
		if(!TextureSet(pSprite->m_ImageId))
			return;
		
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
		if(!TextureSet(pSprite->m_ImageId))
			return;
		
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

void CModAPI_Client_Graphics::DrawText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment)
{	
	float width = pTextRender->TextWidth(0, Size, pText, -1);
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
	
	pTextRender->TextColor(Color.r,Color.g,Color.b,Color.a);
	pTextRender->Text(0, Pos.x, Pos.y, Size, pText, -1);
	
	//reset color
	pTextRender->TextColor(255,255,255,1);
}

void CModAPI_Client_Graphics::DrawAnimatedText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment, int AnimationID, float Time, vec2 Offset)
{
	const CModAPI_Animation* pAnimation = GetAnimation(AnimationID);
	if(pAnimation == 0) return;
	
	CModAPI_AnimationFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	float animX = Frame.m_Pos.x + Offset.x - Offset.x * cos(Frame.m_Angle) + Offset.y * sin(Frame.m_Angle);
	float animY = Frame.m_Pos.y + Offset.y - Offset.x * sin(Frame.m_Angle) + Offset.y * cos(Frame.m_Angle);
	
	float X = Pos.x + animX;
	float Y = Pos.y + animY;
	
	vec4 NewColor = Color;
	NewColor.a *= Frame.m_Opacity;
	
	DrawText(pTextRender, pText, vec2(X, Y), NewColor, Size, Alignment);
}

void CModAPI_Client_Graphics::DrawTee(CRenderTools* pRenderTools, CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pTeeState, vec2 Pos, vec2 Dir, int Emote, float Time)
{
	float Size = pInfo->m_Size;
	bool IndicateAirJump = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
	
	IGraphics::CQuadItem BodyItem(Pos.x + pTeeState->m_Body.m_Pos.x, Pos.y + pTeeState->m_Body.m_Pos.y, Size, Size);
	IGraphics::CQuadItem Item;

	// draw back feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, pTeeState->m_BackFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_BackFoot.m_Pos.x, Pos.y + pTeeState->m_BackFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration, outline
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[2].r, pInfo->m_aColors[2].g, pInfo->m_aColors[2].b, pInfo->m_aColors[2].a);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION_OUTLINE, 0, 0, 0);
		
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body, outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_OUTLINE, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, pTeeState->m_FrontFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_FrontFoot.m_Pos.x, Pos.y + pTeeState->m_FrontFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;
		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[4].r*cs, pInfo->m_aColors[4].g*cs, pInfo->m_aColors[4].b*cs, pInfo->m_aColors[4].a * pTeeState->m_BackFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_BackFoot.m_Pos.x, Pos.y + pTeeState->m_BackFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[2].r, pInfo->m_aColors[2].g, pInfo->m_aColors[2].b, pInfo->m_aColors[2].a);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw body
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[0].r, pInfo->m_aColors[0].g, pInfo->m_aColors[0].b, pInfo->m_aColors[0].a);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw marking
	if(pInfo->m_aTextures[1].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[1]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[1].r, pInfo->m_aColors[1].g, pInfo->m_aColors[1].b, pInfo->m_aColors[1].a);
		pRenderTools->SelectSprite(SPRITE_TEE_MARKING, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body shadow and upper outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_SHADOW, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_UPPER_OUTLINE, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw eyes
	if(pInfo->m_aTextures[5].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[5]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->SetColor(pInfo->m_aColors[5].r, pInfo->m_aColors[5].g, pInfo->m_aColors[5].b, pInfo->m_aColors[5].a);
		switch (Emote)
		{
			case EMOTE_PAIN:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_PAIN, 0, 0, 0);
				break;
			case EMOTE_HAPPY:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_HAPPY, 0, 0, 0);
				break;
			case EMOTE_SURPRISE:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_SURPRISE, 0, 0, 0);
				break;
			case EMOTE_ANGRY:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_ANGRY, 0, 0, 0);
				break;
			default:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_NORMAL, 0, 0, 0);
				break;
		}

		float EyeScale = Size*0.60f;
		float h = (Emote == EMOTE_BLINK) ? Size*0.15f/2.0f : EyeScale/2.0f;
		vec2 Offset = vec2(Dir.x*0.125f, -0.05f+Dir.y*0.10f)*Size;
		IGraphics::CQuadItem QuadItem(Pos.x+Offset.x, Pos.y+Offset.y, EyeScale, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;
		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[4].r*cs, pInfo->m_aColors[4].g*cs, pInfo->m_aColors[4].b*cs, pInfo->m_aColors[4].a * pTeeState->m_FrontFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_FrontFoot.m_Pos.x, Pos.y + pTeeState->m_FrontFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
}

void CModAPI_Client_Graphics::InitTeeAnimationState(CModAPI_TeeAnimationState* pState)
{
	pState->m_Body.m_Pos.x = 0.0f;
	pState->m_Body.m_Pos.y = -4.0f;
	pState->m_Body.m_Angle = 0.0f;
	pState->m_Body.m_Opacity = 1.0f;
	
	pState->m_BackFoot.m_Pos.x = 0.0f;
	pState->m_BackFoot.m_Pos.y = 10.0f;
	pState->m_BackFoot.m_Angle = 0.0f;
	pState->m_BackFoot.m_Opacity = 1.0f;
	
	pState->m_FrontFoot.m_Pos.x = 0.0f;
	pState->m_FrontFoot.m_Pos.y = 10.0f;
	pState->m_FrontFoot.m_Angle = 0.0f;
	pState->m_FrontFoot.m_Opacity = 1.0f;
}

void CModAPI_Client_Graphics::AddTeeAnimationState(CModAPI_TeeAnimationState* pState, int TeeAnimationID, float Time)
{
	const CModAPI_TeeAnimation* pTeeAnim = GetTeeAnimation(TeeAnimationID);
	if(pTeeAnim == 0) return;
	
	if(pTeeAnim->m_BodyAnimation >= 0)
	{
		const CModAPI_Animation* pBodyAnim = GetAnimation(pTeeAnim->m_BodyAnimation);
		if(pBodyAnim)
		{
			pBodyAnim->GetFrame(Time, &pState->m_Body);
		}
	}
	
	if(pTeeAnim->m_BackFootAnimation >= 0)
	{
		const CModAPI_Animation* pBackFootAnim = GetAnimation(pTeeAnim->m_BackFootAnimation);
		if(pBackFootAnim)
		{
			pBackFootAnim->GetFrame(Time, &pState->m_BackFoot);
		}
	}
	
	
	if(pTeeAnim->m_FrontFootAnimation >= 0)
	{
		const CModAPI_Animation* pFrontFootAnim = GetAnimation(pTeeAnim->m_FrontFootAnimation);
		if(pFrontFootAnim)
		{
			pFrontFootAnim->GetFrame(Time, &pState->m_FrontFoot);
		}
	}
}
