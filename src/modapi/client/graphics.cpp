#include <modapi/client/graphics.h>
#include <engine/storage.h>
#include <modapi/shared/assetsfile.h>
#include <generated/client_data.h>
#include <game/client/render.h>
#include <engine/shared/datafile.h>
#include <engine/shared/config.h>

#include <game/gamecore.h>

#include <engine/external/pnglite/pnglite.h>
	
CModAPI_Client_Graphics::CModAPI_Client_Graphics(IGraphics* pGraphics, CModAPI_AssetManager* pAssetManager)
{
	m_pGraphics = pGraphics;
	m_pAssetManager = pAssetManager;
}

bool CModAPI_Client_Graphics::TextureSet(CModAPI_AssetPath AssetPath)
{
	const CModAPI_Asset_Image* pImage = AssetManager()->GetAsset<CModAPI_Asset_Image>(AssetPath);
	if(pImage == 0)
		return false;
	
	m_pGraphics->TextureSet(pImage->m_Texture);
	
	return true;
}

void CModAPI_Client_Graphics::DrawSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int Flags)
{
	DrawSprite(SpritePath, Pos, Size, Angle, Flags, vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void CModAPI_Client_Graphics::DrawSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int Flags, vec4 Color)
{
	DrawSprite(SpritePath, Pos, vec2(Size, Size), Angle, Flags, Color);
}

void CModAPI_Client_Graphics::DrawSprite(CModAPI_AssetPath SpritePath, vec2 Pos, vec2 Size, float Angle, int Flags, vec4 Color)
{
	//Get sprite
	const CModAPI_Asset_Sprite* pSprite = AssetManager()->GetAsset<CModAPI_Asset_Sprite>(SpritePath);
	if(pSprite == 0)
		return;
	
	//Get texture
	const CModAPI_Asset_Image* pImage = AssetManager()->GetAsset<CModAPI_Asset_Image>(pSprite->m_ImagePath);
	if(pImage == 0)
		return;

	m_pGraphics->BlendNormal();
	m_pGraphics->TextureSet(pImage->m_Texture);
	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
	
	//Compute texture subquad
	float texX0 = pSprite->m_X/(float)max(1, pImage->m_GridWidth);
	float texX1 = (pSprite->m_X + pSprite->m_Width - 1.0f/32.0f)/(float)max(1, pImage->m_GridWidth);
	float texY0 = pSprite->m_Y/(float)max(1, pImage->m_GridHeight);
	float texY1 = (pSprite->m_Y + pSprite->m_Height - 1.0f/32.0f)/(float)max(1, pImage->m_GridHeight);
	
	float Temp = 0;
	if(Flags&CModAPI_Asset_Sprite::FLAG_FLIP_Y)
	{
		Temp = texY0;
		texY0 = texY1;
		texY1 = Temp;
	}

	if(Flags&CModAPI_Asset_Sprite::FLAG_FLIP_X)
	{
		Temp = texX0;
		texX0 = texX1;
		texX1 = Temp;
	}

	m_pGraphics->QuadsSetSubset(texX0, texY0, texX1, texY1);
	
	m_pGraphics->QuadsSetRotation(Angle);

	//Compute sprite size
	float ratio = sqrtf(pSprite->m_Width * pSprite->m_Width + pSprite->m_Height * pSprite->m_Height);
	
	if(Flags & CModAPI_Asset_Sprite::FLAG_SIZE_HEIGHT)
		ratio = pSprite->m_Height;
		
	vec2 QuadSize = vec2(pSprite->m_Width/ratio, pSprite->m_Height/ratio) * Size;
	
	//Draw
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, QuadSize.x, QuadSize.y);
	m_pGraphics->QuadsDraw(&QuadItem, 1);
	
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawAnimatedSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int SpriteFlag, CModAPI_AssetPath AnimationPath, float Time, vec2 Offset)
{
	const CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(AnimationPath);
	if(pAnimation == 0) return;
	
	CModAPI_Asset_Animation::CFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
		
	vec2 AnimationPos = Frame.m_Pos + rotate(Offset, Frame.m_Angle);
	
	float FrameAngle = Frame.m_Angle;
	if(SpriteFlag & CModAPI_Asset_Sprite::FLAG_FLIP_ANIM_X)
	{
		AnimationPos.x = -AnimationPos.x;
		FrameAngle = -FrameAngle;
	}
	else if(SpriteFlag & CModAPI_Asset_Sprite::FLAG_FLIP_ANIM_Y)
	{
		AnimationPos.y = -AnimationPos.y;
		FrameAngle = -FrameAngle;
	}
	
	vec2 SpritePos = Pos + rotate(AnimationPos, Angle);
	
	DrawSprite(SpritePath, SpritePos, Size, Angle + FrameAngle, SpriteFlag, Frame.m_Color);
}

void CModAPI_Client_Graphics::DrawLineElement_TilingNone(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time)
{
	vec2 LineDiff = EndPoint - StartPoint;
	vec2 LineDir = normalize(LineDiff);
	float LineAngle = angle(LineDir);
	
	vec2 ElemStartPoint = StartPoint + LineDiff * pElement->m_StartPosition;
	float ElemSize = Size*pElement->m_Size;
	int ElemFlag = CModAPI_Asset_Sprite::FLAG_SIZE_HEIGHT;
	
	if(pElement->m_AnimationPath.IsNull())
		DrawSprite(pElement->m_SpritePath, ElemStartPoint, ElemSize, LineAngle, ElemFlag, vec4(1.0f, 1.0f, 1.0f, 1.0f));
	else
		DrawAnimatedSprite(pElement->m_SpritePath, ElemStartPoint, ElemSize, LineAngle, ElemFlag, pElement->m_AnimationPath, Time, vec2(0.0f, 0.0f));
}

void CModAPI_Client_Graphics::DrawLineElement_TilingStretch(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time)
{	
	vec2 LineDiff = EndPoint - StartPoint;
	vec2 LineDir = normalize(LineDiff);
	float LineAngle = angle(LineDir);
	
	vec2 ElemStartPoint = StartPoint + LineDiff * pElement->m_StartPosition;
	vec2 ElemEndPoint = StartPoint + LineDiff * pElement->m_EndPosition;
	vec2 ElemDir = normalize(ElemEndPoint - ElemStartPoint);
	float ElemLength = distance(ElemStartPoint, ElemEndPoint);
	
	vec2 ElemShift = vec2(0.0f, 0.0f);
	vec2 ElemSize = vec2(ElemLength, Size * pElement->m_Size);
	vec4 ElemColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	const CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pElement->m_AnimationPath);
	if(pAnimation)
	{
		CModAPI_Asset_Animation::CFrame Frame;
		pAnimation->GetFrame(Time, &Frame);
		
		ElemColor = Frame.m_Color;
		ElemShift = Frame.m_Pos * Size;
		ElemSize *= Frame.m_Size;
	}
	
	vec2 Pos = (ElemStartPoint + ElemEndPoint)/2.0f;
	
	const CModAPI_Asset_Sprite* pSprite = AssetManager()->GetAsset<CModAPI_Asset_Sprite>(pElement->m_SpritePath);
	if(!pSprite)
		return;
		
	const CModAPI_Asset_Image* pImage = AssetManager()->GetAsset<CModAPI_Asset_Image>(pSprite->m_ImagePath);
	if(pImage == 0)
		return;
	
	//Compute texture subquad
	float texX0 = pSprite->m_X/(float)pImage->m_GridWidth;
	float texX1 = (pSprite->m_X + pSprite->m_Width - 1.0f/32.0f)/(float)pImage->m_GridWidth;
	float texY0 = pSprite->m_Y/(float)pImage->m_GridHeight;
	float texY1 = (pSprite->m_Y + pSprite->m_Height - 1.0f/32.0f)/(float)pImage->m_GridHeight;

	float ratio = pSprite->m_Height;
	
	m_pGraphics->TextureSet(pImage->m_Texture);
	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(ElemColor.r*ElemColor.a, ElemColor.g*ElemColor.a, ElemColor.b*ElemColor.a, ElemColor.a);
	m_pGraphics->QuadsSetSubset(texX0, texY0, texX1, texY1);
	m_pGraphics->QuadsSetRotation(LineAngle);
	
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, ElemSize.x, ElemSize.y * pSprite->m_Height/ratio);
	m_pGraphics->QuadsDraw(&QuadItem, 1);
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawLineElement_TilingRepeated(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time)
{
	const CModAPI_Asset_Sprite* pSprite = AssetManager()->GetAsset<CModAPI_Asset_Sprite>(pElement->m_SpritePath);
	if(!pSprite)
		return;
		
	const CModAPI_Asset_Image* pImage = AssetManager()->GetAsset<CModAPI_Asset_Image>(pSprite->m_ImagePath);
	if(pImage == 0)
		return;
	
	vec2 LineDiff = EndPoint - StartPoint;
	vec2 LineDir = normalize(LineDiff);
	float LineAngle = angle(LineDir);
	
	vec2 ElemStartPoint = StartPoint + LineDiff * pElement->m_StartPosition;
	vec2 ElemEndPoint = StartPoint + LineDiff * pElement->m_EndPosition;
	vec2 ElemDir = normalize(ElemEndPoint - ElemStartPoint);
	vec2 ElemDirOrtho = vec2(ElemDir.y, -ElemDir.x);
	float ElemLength = distance(ElemStartPoint, ElemEndPoint);
	
	vec2 ElemShift = vec2(0.0f, 0.0f);
	vec2 ElemSize = vec2(1.0f, 1.0f) * Size * pElement->m_Size;
	vec4 ElemColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	const CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pElement->m_AnimationPath);
	if(pAnimation)
	{
		CModAPI_Asset_Animation::CFrame Frame;
		pAnimation->GetFrame(Time, &Frame);
		
		ElemColor = Frame.m_Color;
		ElemShift = Frame.m_Pos * Size;
		ElemSize *= Frame.m_Size;
	}
	
	//Compute texture subquad
	float texX0 = pSprite->m_X/(float)max(1, pImage->m_GridWidth);
	float texX1 = (pSprite->m_X + pSprite->m_Width - 1.0f/32.0f)/(float)max(1, pImage->m_GridWidth);
	float texY0 = pSprite->m_Y/(float)max(1, pImage->m_GridHeight);
	float texY1 = (pSprite->m_Y + pSprite->m_Height - 1.0f/32.0f)/(float)max(1, pImage->m_GridHeight);

	float ratio = pSprite->m_Height;
	
	m_pGraphics->TextureSet(pImage->m_Texture);
	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(ElemColor.r*ElemColor.a, ElemColor.g*ElemColor.a, ElemColor.b*ElemColor.a, ElemColor.a);
	m_pGraphics->QuadsSetSubset(texX0, texY0, texX1, texY1);
	m_pGraphics->QuadsSetRotation(LineAngle);

	int nbTile = 0;
	float LengthIter = 0.0f;
	vec2 QuadSize = vec2(pSprite->m_Width/ratio, pSprite->m_Height/ratio) * ElemSize;
	while(LengthIter <= ElemLength && nbTile < 1024)
	{
		//Compute sprite size
		vec2 Pos = ElemStartPoint + ElemDir * LengthIter + ElemDir * ElemShift.x + ElemDirOrtho * ElemShift.y;
		//Draw
		IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, QuadSize.x, QuadSize.y);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		LengthIter += QuadSize.x;
		nbTile++;
	}
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawLine(CModAPI_AssetPath LineStylePath, vec2 StartPoint, vec2 EndPoint, float Size, float Time)
{	
	const CModAPI_Asset_LineStyle* pLineStyle = AssetManager()->GetAsset<CModAPI_Asset_LineStyle>(LineStylePath);
	if(pLineStyle == 0) return;
	
	//Line elements
	m_pGraphics->BlendNormal();
	
	for(int i=0; i<pLineStyle->m_Elements.size(); i++)
	{
		switch(pLineStyle->m_Elements[i].m_TilingType)
		{
			case CModAPI_Asset_LineStyle::TILINGTYPE_NONE:
				DrawLineElement_TilingNone(&pLineStyle->m_Elements[i], StartPoint, EndPoint, Size, Time);
				break;
			case CModAPI_Asset_LineStyle::TILINGTYPE_STRETCH:
				DrawLineElement_TilingStretch(&pLineStyle->m_Elements[i], StartPoint, EndPoint, Size, Time);
				break;
			case CModAPI_Asset_LineStyle::TILINGTYPE_REPEATED:
				DrawLineElement_TilingRepeated(&pLineStyle->m_Elements[i], StartPoint, EndPoint, Size, Time);
				break;
		}
		/*
		if(pLineStyle->m_Elements[i].m_SpritePath.IsInternal() && pLineStyle->m_Elements[i].m_SpritePath.GetId() == MODAPI_SPRITE_SPECIAL_WHITE_SQUARE)
		{
			vec2 Pos = (StartPoint + EndPoint)/2.0f;
			vec2 QuadSize = vec2(
				Length,
				Size * pLineStyle->m_Elements[i].m_Size
				);
			
			vec4 Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			const CModAPI_Asset_Animation* pAnimation = GetAsset<CModAPI_Asset_Animation>(pLineStyle->m_Elements[i].m_AnimationPath);
			if(pAnimation)
			{
				CModAPI_Asset_Animation::CFrame Frame;
				pAnimation->GetFrame(Time, &Frame);
				
				Color = Frame.m_Color;
			}
			
			m_pGraphics->BlendNormal();
			m_pGraphics->TextureClear();
			m_pGraphics->QuadsBegin();
			m_pGraphics->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
			m_pGraphics->QuadsSetRotation(Angle);
			IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, QuadSize.x, QuadSize.y);
			m_pGraphics->QuadsDraw(&QuadItem, 1);
			m_pGraphics->QuadsEnd();
		}
		else
		{
			const CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.GetAsset(pLineStyle->m_Elements[i].m_SpritePath);
			if(!pSprite)
				continue;
				
			const CModAPI_Asset_Image* pImage = m_ImagesCatalog.GetAsset(pSprite->m_ImagePath);
			if(pImage == 0)
				continue;
			
			vec4 Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			const CModAPI_Asset_Animation* pAnimation = GetAsset<CModAPI_Asset_Animation>(pLineStyle->m_Elements[i].m_AnimationPath);
			if(pAnimation)
			{
				CModAPI_Asset_Animation::CFrame Frame;
				pAnimation->GetFrame(Time, &Frame);
				
				Color = Frame.m_Color;
			}
			
			//Compute texture subquad
			float texX0 = pSprite->m_X/(float)pImage->m_GridWidth;
			float texX1 = (pSprite->m_X + pSprite->m_Width - 1.0f/32.0f)/(float)pImage->m_GridWidth;
			float texY0 = pSprite->m_Y/(float)pImage->m_GridHeight;
			float texY1 = (pSprite->m_Y + pSprite->m_Height - 1.0f/32.0f)/(float)pImage->m_GridHeight;

			float ratio = sqrtf(pSprite->m_Width * pSprite->m_Width + pSprite->m_Height * pSprite->m_Height);
			
			m_pGraphics->TextureSet(pImage->m_Texture);
			m_pGraphics->QuadsBegin();
			m_pGraphics->SetColor(Color.r*Color.a, Color.g*Color.a, Color.b*Color.a, Color.a);
			m_pGraphics->QuadsSetSubset(texX0, texY0, texX1, texY1);
			m_pGraphics->QuadsSetRotation(Angle);
		
			float LengthIter = 0.0f;
			while(LengthIter <= Length)
			{
				//Compute sprite size
				vec2 QuadSize = vec2(pSprite->m_Width/ratio, pSprite->m_Height/ratio) * Size * pLineStyle->m_Elements[i].m_Size;
				vec2 Pos = StartPoint + LineDir * LengthIter;
				//Draw
				IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, QuadSize.x, QuadSize.y);
				m_pGraphics->QuadsDraw(&QuadItem, 1);
				
				LengthIter += QuadSize.x;
			}
			m_pGraphics->QuadsEnd();
		}
		*/
	}
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

void CModAPI_Client_Graphics::DrawAnimatedText(
	ITextRender* pTextRender,
	const char *pText,
	vec2 Pos,
	vec4 Color,
	float Size,
	int Alignment,
	CModAPI_AssetPath AnimationPath,
	float Time,
	vec2 Offset
)
{
	const CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(AnimationPath);
	if(pAnimation == 0) return;
	
	CModAPI_Asset_Animation::CFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	vec2 TextPos = Pos + Frame.m_Pos + rotate(Offset, Frame.m_Angle);
	
	vec4 NewColor = Color * Frame.m_Color;
	
	DrawText(pTextRender, pText, TextPos, NewColor, Size, Alignment);
}

void CModAPI_Client_Graphics::GetTeeAlignAxis(int Align, vec2 Dir, vec2 Aim, vec2& PartDirX, vec2& PartDirY)
{
	PartDirX = vec2(1.0f, 0.0f);
	PartDirY = vec2(0.0f, 1.0f);
	switch(Align)
	{
		case MODAPI_TEEALIGN_AIM:
			PartDirX = normalize(Aim);
			if(Aim.x >= 0.0f)
			{
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
		case MODAPI_TEEALIGN_HORIZONTAL:
			if(Aim.x < 0.0f)
			{
				PartDirX = vec2(-1.0f, 0.0f);
			}
			PartDirY = vec2(0.0f, 1.0f);
			break;
		case MODAPI_TEEALIGN_DIRECTION:
			PartDirX = normalize(Dir);
			if(Dir.x >= 0.0f)
			{
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
	}
}

void CModAPI_Client_Graphics::ApplyTeeAlign(CModAPI_Asset_Animation::CFrame& Frame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset)
{
	Flags = 0x0;
	vec2 PartDirX = vec2(1.0f, 0.0f);
	vec2 PartDirY = vec2(0.0f, 1.0f);
	switch(Align)
	{
		case MODAPI_TEEALIGN_AIM:
			PartDirX = normalize(Aim);
			if(Aim.x >= 0.0f)
			{
				Frame.m_Angle = angle(Aim) + Frame.m_Angle;
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				Frame.m_Angle = angle(Aim) - Frame.m_Angle;
				Flags = CModAPI_Asset_Sprite::FLAG_FLIP_Y;
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
		case MODAPI_TEEALIGN_HORIZONTAL:
			if(Aim.x < 0.0f)
			{
				Frame.m_Angle = -Frame.m_Angle;
				Flags = CModAPI_Asset_Sprite::FLAG_FLIP_X;
				PartDirX = vec2(-1.0f, 0.0f);
			}
			PartDirY = vec2(0.0f, 1.0f);
			break;
		case MODAPI_TEEALIGN_DIRECTION:
			PartDirX = normalize(Dir);
			if(Dir.x >= 0.0f)
			{
				Frame.m_Angle = angle(Dir) + Frame.m_Angle;
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				Frame.m_Angle = angle(Dir) - Frame.m_Angle;
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
				Flags = CModAPI_Asset_Sprite::FLAG_FLIP_Y;
			}
			break;
	}
	
	Frame.m_Pos = Offset + PartDirX * Frame.m_Pos.x + PartDirY * Frame.m_Pos.y;	
}

/*
void CModAPI_Client_Graphics::DrawAttach(CRenderTools* pRenderTools, const CModAPI_AttachAnimationState* pState, CModAPI_AssetPath AttachPath, vec2 Pos, float Scaling)
{
	const CModAPI_Asset_Attach* pAttach = AssetManager()->GetAsset<CModAPI_Asset_Attach>(AttachPath);
	if(pAttach == 0) return;
	
	for(int i=0; i<pAttach->m_BackElements.size(); i++)
	{
		DrawSprite(pAttach->m_BackElements[i].m_SpritePath,
			Pos + vec2(pState->m_Frames[i].m_Pos.x, pState->m_Frames[i].m_Pos.y) * Scaling,
			pAttach->m_BackElements[i].m_Size * Scaling,
			pState->m_Frames[i].m_Angle,
			pState->m_Flags[i],
			pState->m_Frames[i].m_Color
		);
	}
}

void CModAPI_Client_Graphics::DrawTee(CRenderTools* pRenderTools, CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pTeeState, vec2 Pos, vec2 Dir, int Emote)
{
	float Scaling = pInfo->m_Size/64.0f;
	
	float BodySize = pInfo->m_Size;
	float HandSize = BodySize*30.0f/64.0f;
	float FootSize = BodySize/2.1f;
	
	IGraphics::CQuadItem QuadItemBody(Pos.x + pTeeState->m_Body.m_Pos.x*Scaling, Pos.y + pTeeState->m_Body.m_Pos.y*Scaling, BodySize, BodySize);
	IGraphics::CQuadItem QuadItemBackFoot(Pos.x + pTeeState->m_BackFoot.m_Pos.x*Scaling, Pos.y + pTeeState->m_BackFoot.m_Pos.y*Scaling, FootSize, FootSize);
	IGraphics::CQuadItem QuadItemFrontFoot(Pos.x + pTeeState->m_FrontFoot.m_Pos.x*Scaling, Pos.y + pTeeState->m_FrontFoot.m_Pos.y*Scaling, FootSize, FootSize);
	IGraphics::CQuadItem QuadItemBackHand(Pos.x + pTeeState->m_BackHand.m_Pos.x*Scaling, Pos.y + pTeeState->m_BackHand.m_Pos.y*Scaling, HandSize, HandSize);
	IGraphics::CQuadItem QuadItemFrontHand(Pos.x + pTeeState->m_FrontHand.m_Pos.x*Scaling, Pos.y + pTeeState->m_FrontHand.m_Pos.y*Scaling, HandSize, HandSize);
	IGraphics::CQuadItem QuadItem;
	
	bool IndicateAirJump = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
		
	if(pTeeState->m_BackHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		vec4 Color = pInfo->m_aColors[3] * pTeeState->m_BackHand.m_Color;
		float Opacity = pTeeState->m_BackHand.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);

		//draw hand, outline
		QuadItem = QuadItemBackHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemBackHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}
	
	if(pTeeState->m_FrontHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		
		vec4 Color = pInfo->m_aColors[3] * pTeeState->m_FrontHand.m_Color;
		float Opacity = pTeeState->m_FrontHand.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);

		//draw hand, outline
		QuadItem = QuadItemFrontHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemFrontHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4] * pTeeState->m_BackFoot.m_Color;
		float Opacity = pTeeState->m_BackFoot.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBackFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration, outline
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[2] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body, outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4] * pTeeState->m_FrontFoot.m_Color;
		float Opacity = pTeeState->m_FrontFoot.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemFrontFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4] * pTeeState->m_BackFoot.m_Color;
		float Opacity = pTeeState->m_BackFoot.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		
		QuadItem = QuadItemBackFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[2] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw body
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw marking
	if(pInfo->m_aTextures[1].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[1]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[1] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_MARKING, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body shadow and upper outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0] * pTeeState->m_Body.m_Color;
		float Opacity = pTeeState->m_Body.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_SHADOW, 0, 0, 0);
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_UPPER_OUTLINE, 0, 0, 0);
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsEnd();
	}
	
	// draw eyes
	if(pInfo->m_aTextures[5].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[5]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(0.0f);
		vec4 Color = pInfo->m_aColors[5];
		float Opacity = Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
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

		float EyeScale = BodySize*0.60f;
		float h = (Emote == EMOTE_BLINK) ? BodySize*0.15f/2.0f : EyeScale/2.0f;
		vec2 Offset = vec2(Dir.x*0.125f, -0.05f+Dir.y*0.10f)*BodySize;
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_Body.m_Pos.x*Scaling + Offset.x, Pos.y + pTeeState->m_Body.m_Pos.y*Scaling + Offset.y, EyeScale, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4] * pTeeState->m_FrontFoot.m_Color;
		float Opacity = pTeeState->m_FrontFoot.m_Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		
		QuadItem = QuadItemFrontFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
}
*/

void CModAPI_Client_Graphics::InitTeeAnimationState(CModAPI_TeeAnimationState* pState, vec2 MotionDir, vec2 AimDir)
{
	pState->m_MotionDir = MotionDir;
	pState->m_AimDir = AimDir;
	
	pState->m_Body.Position(vec2(0.0f, -4.0f));
	pState->m_BackFoot.Position(vec2(0.0f, 10.0f));
	pState->m_FrontFoot.Position(vec2(0.0f, 10.0f));
}

void CModAPI_Client_Graphics::AddTeeAnimationState(CModAPI_TeeAnimationState* pState, CModAPI_AssetPath TeeAnimationPath, float Time)
{
	const CModAPI_Asset_TeeAnimation* pTeeAnim = AssetManager()->GetAsset<CModAPI_Asset_TeeAnimation>(TeeAnimationPath);
	if(pTeeAnim == 0) return;
	
	if(!pTeeAnim->m_BodyAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBodyAnim = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnim->m_BodyAnimationPath);
		if(pBodyAnim)
		{
			pBodyAnim->GetFrame(Time, &pState->m_Body);
		}
	}
	
	if(!pTeeAnim->m_BackFootAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBackFootAnim = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnim->m_BackFootAnimationPath);
		if(pBackFootAnim)
		{
			pBackFootAnim->GetFrame(Time, &pState->m_BackFoot);
		}
	}
	
	if(!pTeeAnim->m_FrontFootAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pFrontFootAnim = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnim->m_FrontFootAnimationPath);
		if(pFrontFootAnim)
		{
			pFrontFootAnim->GetFrame(Time, &pState->m_FrontFoot);
		}
	}
	
	if(!pTeeAnim->m_BackHandAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBackHandAnim = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnim->m_BackHandAnimationPath);
		if(pBackHandAnim)
		{
			pState->m_BackHandEnabled = true;
			
			pBackHandAnim->GetFrame(Time, &pState->m_BackHand);
			
			ApplyTeeAlign(pState->m_BackHand, pState->m_BackHandFlags, pTeeAnim->m_BackHandAlignment, pState->m_MotionDir, pState->m_AimDir, vec2(pTeeAnim->m_BackHandOffsetX, pTeeAnim->m_BackHandOffsetY));
		}
	}
	
	if(!pTeeAnim->m_FrontHandAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pFrontHandAnim = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pTeeAnim->m_FrontHandAnimationPath);
		if(pFrontHandAnim)
		{
			pState->m_FrontHandEnabled = true;
			pFrontHandAnim->GetFrame(Time, &pState->m_FrontHand);
		}
	}
}

/*
void CModAPI_Client_Graphics::InitAttachAnimationState(CModAPI_AttachAnimationState* pState, vec2 MotionDir, vec2 AimDir, CModAPI_AssetPath AttachPath, float Time)
{
	
	const CModAPI_Asset_Attach* pAttach = AssetManager()->GetAsset<CModAPI_Asset_Attach>(AttachPath);
	if(pAttach)
	{
		pState->m_Frames.set_size(pAttach->m_BackElements.size());
		pState->m_Flags.set_size(pAttach->m_BackElements.size());
		
		for(int i=0; i<pAttach->m_BackElements.size(); i++)
		{
			pState->m_Frames[i].m_Pos = vec2(0.0f, 0.0f);
			pState->m_Frames[i].m_Angle = 0.0f;
			pState->m_Frames[i].m_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			pState->m_Frames[i].m_ListId = 0;
			pState->m_Flags[i] = 0x0;
			
			const CModAPI_Asset_Animation* pAnimation = AssetManager()->GetAsset<CModAPI_Asset_Animation>(pAttach->m_BackElements[i].m_AnimationPath);
			if(pAnimation)
			{
				pAnimation->GetFrame(Time, &pState->m_Frames[i]);
			}
			
			ApplyTeeAlign(
				pState->m_Frames[i],
				pState->m_Flags[i],
				pAttach->m_BackElements[i].m_Alignment,
				MotionDir,
				AimDir,
				vec2(pAttach->m_BackElements[i].m_OffsetX, pAttach->m_BackElements[i].m_OffsetY)
			);
		}
	}
}
*/
