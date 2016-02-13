#include <base/tl/sorted_array.h>

#include <engine/shared/config.h>
#include <engine/shared/datafile.h>

#include <engine/client.h>
#include <engine/input.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/keys.h>

#include <modapi/client/clientmode.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/label.h>
#include <modapi/client/gui/layout.h>
#include <modapi/client/gui/integer-edit.h>
#include <modapi/client/gui/text-edit.h>

#include <cstddef>

#include "view.h"

CModAPI_AssetsEditorGui_View::CModAPI_AssetsEditorGui_View(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_ClientGui_Widget(pAssetsEditor->m_pGuiConfig),
	m_pAssetsEditor(pAssetsEditor),
	m_AimDir(1.0f, 0.0f),
	m_MotionDir(1.0f, 0.0f),
	m_DragedElement(MODAPI_ASSETSEDITOR_DRAGITEM_NONE),
	m_TeeScale(1.5f),
	m_GizmoType(MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE)
{
	m_GizmoAxisXLength = 64.0f;
	m_GizmoAxisYLength = 64.0f;
	m_GizmoAxisZLength = 96.0f;
	
	for(int p = 0; p < 6; p++)
	{
		m_TeeRenderInfo.m_aTextures[p] = m_pAssetsEditor->m_SkinTexture[p];
		m_TeeRenderInfo.m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	m_TeeRenderInfo.m_aColors[0] = vec4(206.0f/255.0f, 140.0f/255.0f, 90.0f/255.0f, 1.0f);
	m_TeeRenderInfo.m_aColors[1] = vec4(217.0f/255.0f, 183.0f/255.0f, 160.0f/255.0f, 1.0f);
	m_TeeRenderInfo.m_aColors[2] = m_TeeRenderInfo.m_aColors[0];
	m_TeeRenderInfo.m_aColors[3] = m_TeeRenderInfo.m_aColors[0];
	m_TeeRenderInfo.m_aColors[4] = m_TeeRenderInfo.m_aColors[0];
}

void CModAPI_AssetsEditorGui_View::RenderImage()
{
	CModAPI_Asset_Image* pImage = m_pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetAsset(m_pAssetsEditor->m_ViewedAssetPath);
	if(!pImage)
		return;
		
	float ImgRatio = static_cast<float>(pImage->m_Width)/static_cast<float>(pImage->m_Height);
	float WindowRatio = static_cast<float>(m_Rect.w)/static_cast<float>(m_Rect.h);
	float SizeX;
	float SizeY;
	
	if(ImgRatio > WindowRatio)
	{
		SizeX = m_Rect.w;
		SizeY = m_Rect.w/ImgRatio;
	}
	else
	{
		SizeX = m_Rect.h*ImgRatio;
		SizeY = m_Rect.h;
	}
	
	float x0 = m_Rect.x + m_Rect.w/2 - SizeX/2;
	float y0 = m_Rect.y + m_Rect.h/2 - SizeY/2;
	float x1 = x0 + SizeX;
	float y1 = y0 + SizeY;
	float xStep = SizeX / static_cast<float>(pImage->m_GridWidth);
	float yStep = SizeY / static_cast<float>(pImage->m_GridHeight);
	
	//Draw sprites
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	float alpha = 0.75f;
	for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_InternalAssets.size(); i++)
	{
		CModAPI_Asset_Sprite* pSprite = &m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_InternalAssets[i];
		if(pSprite->m_ImagePath == m_pAssetsEditor->m_ViewedAssetPath)
		{
			if(m_pAssetsEditor->IsEditedAsset(MODAPI_ASSETTYPE_SPRITE, CModAPI_AssetPath::Internal(i)))
				Graphics()->SetColor(alpha, alpha*0.5f, alpha*0.5f, alpha);
			else
				Graphics()->SetColor(alpha, alpha, alpha*0.5f, alpha);
				
			IGraphics::CQuadItem QuadItem(x0 + xStep * pSprite->m_X, y0 + yStep * pSprite->m_Y, xStep * pSprite->m_Width, yStep * pSprite->m_Height);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
	}
	for(int i=0; i<m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_ExternalAssets.size(); i++)
	{
		CModAPI_Asset_Sprite* pSprite = &m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.m_ExternalAssets[i];
		if(pSprite->m_ImagePath == m_pAssetsEditor->m_ViewedAssetPath)
		{
			if(m_pAssetsEditor->IsEditedAsset(MODAPI_ASSETTYPE_SPRITE, CModAPI_AssetPath::External(i)))
				Graphics()->SetColor(alpha, alpha*0.5f, alpha*0.5f, alpha);
			else
				Graphics()->SetColor(alpha, alpha, alpha*0.5f, alpha);
				
			IGraphics::CQuadItem QuadItem(x0 + xStep * pSprite->m_X, y0 + yStep * pSprite->m_Y, xStep * pSprite->m_Width, yStep * pSprite->m_Height);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
	}
	Graphics()->QuadsEnd();
	
	//Draw image
	m_pAssetsEditor->ModAPIGraphics()->TextureSet(m_pAssetsEditor->m_ViewedAssetPath);
	
	Graphics()->QuadsBegin();
	IGraphics::CQuadItem QuadItem(x0, y0, SizeX, SizeY);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	
	//Draw grid
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	
	for(int i=0; i<=pImage->m_GridWidth; i++)
	{
		float x = x0 + i * xStep;
		IGraphics::CLineItem Line(x, y0, x, y1);
		Graphics()->LinesDraw(&Line, 1);
	}
	for(int i=0; i<=pImage->m_GridHeight; i++)
	{
		float y = y0 + i * yStep;
		IGraphics::CLineItem Line(x0, y, x1, y);
		Graphics()->LinesDraw(&Line, 1);
	}
	
	Graphics()->LinesEnd();
}

void CModAPI_AssetsEditorGui_View::RenderSprite()
{
	CModAPI_Asset_Sprite* pSprite = m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetAsset(m_pAssetsEditor->m_ViewedAssetPath);
	if(!pSprite)
		return;
	
	CModAPI_Asset_Image* pImage = m_pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetAsset(pSprite->m_ImagePath);
	if(!pImage)
		return;
	
	float SpriteWidthPix = (pImage->m_Width / pImage->m_GridWidth) * pSprite->m_Width;
	float SpriteHeightPix = (pImage->m_Height / pImage->m_GridHeight) * pSprite->m_Height;
	float SpriteScaling = sqrtf(SpriteWidthPix*SpriteWidthPix + SpriteHeightPix*SpriteHeightPix);
	vec2 Pos;
	Pos.x = m_Rect.x + m_Rect.w/2;
	Pos.y = m_Rect.y + m_Rect.h/2;
	
	m_pAssetsEditor->ModAPIGraphics()->DrawSprite(m_pAssetsEditor->m_ViewedAssetPath, Pos, SpriteScaling, 0.0f, 0x0);
}

void CModAPI_AssetsEditorGui_View::RenderGizmo()
{
	if(m_GizmoType == MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE)
		return;
			
	vec2 Pos = m_GizmoPos;
	vec2 AxisXPos = m_GizmoPos + m_GizmoAxisX*m_GizmoAxisXLength;
	vec2 AxisYPos = m_GizmoPos + m_GizmoAxisY*m_GizmoAxisYLength;
	vec2 AxisZPos = m_GizmoPos + m_GizmoAxisZ*m_GizmoAxisZLength;		
	
	IGraphics::CLineItem LineX(Pos.x, Pos.y, AxisXPos.x, AxisXPos.y);
	IGraphics::CLineItem LineY(Pos.x, Pos.y, AxisYPos.x, AxisYPos.y);
	IGraphics::CLineItem LineZ(Pos.x, Pos.y, AxisZPos.x, AxisZPos.y);
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	Graphics()->LinesDraw(&LineX, 1);
	Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	Graphics()->LinesDraw(&LineY, 1);
	Graphics()->SetColor(0.0f, 0.0f, 1.0f, 1.0f);
	Graphics()->LinesDraw(&LineZ, 1);
	Graphics()->LinesEnd();
	
	Graphics()->TextureSet(m_pConfig->m_Texture);
	Graphics()->QuadsBegin();

	if(m_GizmoType == MODAPI_ASSETSEDITOR_GIZMOTYPE_XYZ)
	{
		{
			int IconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_CIRCLE;
			int SubX = IconId % 16;
			int SubY = IconId / 16;
			
			Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
			Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
			IGraphics::CQuadItem QuadItem(AxisXPos.x - 8.0f, AxisXPos.y - 8.0f, 16.0f, 16.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
		{
			int IconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_CIRCLE;
			int SubX = IconId % 16;
			int SubY = IconId / 16;
			
			Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
			Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
			IGraphics::CQuadItem QuadItem(AxisYPos.x - 8.0f, AxisYPos.y - 8.0f, 16.0f, 16.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
		{
			int IconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_ROTATION;
			int SubX = IconId % 16;
			int SubY = IconId / 16;
			
			Graphics()->SetColor(0.0f, 0.0f, 1.0f, 1.0f);
			Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
			Graphics()->QuadsSetRotation(angle(m_GizmoAxisZ));
			IGraphics::CQuadItem QuadItem(AxisZPos.x - 8.0f, AxisZPos.y - 8.0f, 16.0f, 16.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
	}
	Graphics()->QuadsEnd();
}

void CModAPI_AssetsEditorGui_View::RenderAnimation()
{
	CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_ViewedAssetPath);
	if(!pAnimation)
		return;
	
	CModAPI_Asset_Animation::CFrame Frame;
	pAnimation->GetFrame(m_pAssetsEditor->GetTime(), &Frame);
	
	float AnimScale = 1.5f;
	
	vec2 Pos = GetTeePosition() + Frame.m_Pos * AnimScale;
	
	{
		int SubX = 4;
		int SubY = 8;
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+4)/16.0f, (SubY+4)/16.0f);
		Graphics()->QuadsSetRotation(Frame.m_Angle);
		IGraphics::CQuadItem QuadItem(Pos.x - 32.0f, Pos.y - 32.0f, 64.0f, 64.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	m_GizmoAxisX = vec2(1.0f, 0.0f);
	m_GizmoAxisY = vec2(0.0f, 1.0f);
	m_GizmoAxisZ = rotate(vec2(1.0f, 0.0f), Frame.m_Angle);
	m_GizmoPos = Pos;
	
	if(
		m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION &&
		m_pAssetsEditor->m_ViewedAssetPath == m_pAssetsEditor->m_EditedAssetPath &&
		m_pAssetsEditor->m_EditedAssetFrame >= 0 &&
		m_pAssetsEditor->m_EditedAssetFrame < pAnimation->m_lKeyFrames.size()
	)
		m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_XYZ;
	else
		m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_VISUALCLUE;
	
	RenderGizmo();
}

vec2 CModAPI_AssetsEditorGui_View::GetTeePosition()
{
	vec2 TeePos;
	TeePos.x = m_Rect.x + m_Rect.w/2;
	TeePos.y = m_Rect.y + m_Rect.h/2;
	
	return TeePos;
}

vec2 CModAPI_AssetsEditorGui_View::GetAimPosition()
{
	float AimDist = min(m_Rect.w/2.0f - 20.0f, m_Rect.h/2.0f - 20.0f) - 48.0f;
	vec2 AimPos = GetTeePosition() + m_AimDir * AimDist - vec2(16.0f, 16.0f);
	
	return AimPos;
}

vec2 CModAPI_AssetsEditorGui_View::GetMotionPosition()
{
	float MotionDist = min(m_Rect.w/2.0f - 20.0f, m_Rect.h/2.0f - 20.0f);
	vec2 MotionPos = GetTeePosition() + m_MotionDir * MotionDist - vec2(16.0f, 16.0f);
	
	return MotionPos;
}

void CModAPI_AssetsEditorGui_View::UpdateGizmo(CModAPI_Asset_Animation* pAnimation, vec2 Pos, float Angle, int Alignment)
{
	if(!pAnimation)
		return;
	
	if(m_pAssetsEditor->m_EditedAssetFrame >= 0 && m_pAssetsEditor->m_EditedAssetFrame < pAnimation->m_lKeyFrames.size())
		m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_XYZ;
	else
		m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_VISUALCLUE;
	m_GizmoPos = Pos;
	m_pAssetsEditor->ModAPIGraphics()->GetTeeAlignAxis(Alignment, m_MotionDir, m_AimDir, m_GizmoAxisX, m_GizmoAxisY);
	m_GizmoAxisZ = rotate(vec2(1.0f, 0.0f), Angle);
}

void CModAPI_AssetsEditorGui_View::UpdateGizmoFromTeeAnimation(CModAPI_Asset_TeeAnimation* pTeeAnimation, CModAPI_TeeAnimationState* pTeeState, vec2 TeePos)
{
	if(m_pAssetsEditor->m_EditedAssetPath == pTeeAnimation->m_BodyAnimationPath)
	{
		UpdateGizmo(
			m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
			TeePos + pTeeState->m_Body.m_Pos * m_TeeScale,
			pTeeState->m_Body.m_Angle,
			MODAPI_TEEALIGN_NONE
		);
	}
	else if(m_pAssetsEditor->m_EditedAssetPath == pTeeAnimation->m_BackFootAnimationPath)
	{
		UpdateGizmo(
			m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
			TeePos + pTeeState->m_BackFoot.m_Pos * m_TeeScale,
			pTeeState->m_BackFoot.m_Angle,
			MODAPI_TEEALIGN_NONE
		);
	}
	else if(m_pAssetsEditor->m_EditedAssetPath == pTeeAnimation->m_FrontFootAnimationPath)
	{
		UpdateGizmo(
			m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
			TeePos + pTeeState->m_FrontFoot.m_Pos * m_TeeScale,
			pTeeState->m_FrontFoot.m_Angle,
			MODAPI_TEEALIGN_NONE
		);
	}
	else if(m_pAssetsEditor->m_EditedAssetPath == pTeeAnimation->m_BackHandAnimationPath)
	{
		UpdateGizmo(
			m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
			TeePos + pTeeState->m_BackHand.m_Pos * m_TeeScale,
			pTeeState->m_BackHand.m_Angle,
			pTeeAnimation->m_BackHandAlignment
		);
	}
	else if(m_pAssetsEditor->m_EditedAssetPath == pTeeAnimation->m_FrontHandAnimationPath)
	{
		UpdateGizmo(
			m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
			TeePos + pTeeState->m_BackHand.m_Pos * m_TeeScale,
			pTeeState->m_BackHand.m_Angle,
			pTeeAnimation->m_FrontHandAlignment
		);
	}
}



void CModAPI_AssetsEditorGui_View::RenderTeeAnimation()
{
	CModAPI_Asset_TeeAnimation* pTeeAnimation = m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(m_pAssetsEditor->m_ViewedAssetPath);
	if(!pTeeAnimation)
		return;
		
	vec2 TeePos = GetTeePosition();
	
	{
		vec2 AimPos = GetAimPosition();
		
		int SubX = 0;
		int SubY = 8;
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+4)/16.0f, (SubY+4)/16.0f);
		IGraphics::CQuadItem QuadItem(AimPos.x - 32.0f, AimPos.y - 32.0f, 64.0f, 64.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	{
		vec2 MotionPos = GetMotionPosition();
		
		int SubX = 4;
		int SubY = 8;
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+4)/16.0f, (SubY+4)/16.0f);
		Graphics()->QuadsSetRotation(angle(m_MotionDir));
		IGraphics::CQuadItem QuadItem(MotionPos.x - 32.0f, MotionPos.y - 32.0f, 64.0f, 64.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	m_TeeRenderInfo.m_Size = 64.0f * m_TeeScale;
	
	CModAPI_TeeAnimationState TeeAnimState;
	m_pAssetsEditor->ModAPIGraphics()->InitTeeAnimationState(&TeeAnimState, m_MotionDir, m_AimDir);
	m_pAssetsEditor->ModAPIGraphics()->AddTeeAnimationState(&TeeAnimState, m_pAssetsEditor->m_ViewedAssetPath, m_pAssetsEditor->GetTime());
	m_pAssetsEditor->ModAPIGraphics()->DrawTee(RenderTools(), &m_TeeRenderInfo, &TeeAnimState, TeePos, m_AimDir, 0x0);
	
	//Gizmo
	m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE;
	if(m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION && !m_pAssetsEditor->m_EditedAssetPath.IsNull())
	{
		UpdateGizmoFromTeeAnimation(pTeeAnimation, &TeeAnimState, TeePos);
	
		RenderGizmo();
	}
}

void CModAPI_AssetsEditorGui_View::RenderAttach()
{			
	CModAPI_Asset_Attach* pAttach = m_pAssetsEditor->ModAPIGraphics()->m_AttachesCatalog.GetAsset(m_pAssetsEditor->m_ViewedAssetPath);
	if(!pAttach)
		return;
			
	vec2 TeePos = GetTeePosition();
	vec2 AimPos = GetAimPosition();	
	
	if(pAttach->m_CursorPath.IsNull())
	{
		int SubX = 0;
		int SubY = 8;
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+4)/16.0f, (SubY+4)/16.0f);
		IGraphics::CQuadItem QuadItem(AimPos.x - 32.0f, AimPos.y - 32.0f, 64.0f, 64.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	else
	{
		CModAPI_Asset_Sprite* pSprite = m_pAssetsEditor->ModAPIGraphics()->m_SpritesCatalog.GetAsset(pAttach->m_CursorPath);
		if(!pSprite)
			return;
		
		CModAPI_Asset_Image* pImage = m_pAssetsEditor->ModAPIGraphics()->m_ImagesCatalog.GetAsset(pSprite->m_ImagePath);
		if(!pImage)
			return;
		
		float SpriteWidthPix = (pImage->m_Width / pImage->m_GridWidth) * pSprite->m_Width;
		float SpriteHeightPix = (pImage->m_Height / pImage->m_GridHeight) * pSprite->m_Height;
		float SpriteScaling = sqrtf(SpriteWidthPix*SpriteWidthPix + SpriteHeightPix*SpriteHeightPix);
		
		m_pAssetsEditor->ModAPIGraphics()->DrawSprite(pAttach->m_CursorPath, AimPos, 64.0f, 0.0f, 0x0);
	}
	
	{
		vec2 MotionPos = GetMotionPosition();
		
		int SubX = 4;
		int SubY = 8;
		Graphics()->TextureSet(m_pConfig->m_Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+4)/16.0f, (SubY+4)/16.0f);
		Graphics()->QuadsSetRotation(angle(m_MotionDir));
		IGraphics::CQuadItem QuadItem(MotionPos.x - 32.0f, MotionPos.y - 32.0f, 64.0f, 64.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	m_TeeRenderInfo.m_Size = 64.0f * m_TeeScale;
	
	vec2 MoveDir = vec2(1.0f, 0.0f);
	
	CModAPI_TeeAnimationState TeeAnimState;
	m_pAssetsEditor->ModAPIGraphics()->InitTeeAnimationState(&TeeAnimState, m_MotionDir, m_AimDir);
	m_pAssetsEditor->ModAPIGraphics()->AddTeeAnimationState(&TeeAnimState, pAttach->m_TeeAnimationPath, m_pAssetsEditor->GetTime());
			
	CModAPI_AttachAnimationState AttachAnimState;
	m_pAssetsEditor->ModAPIGraphics()->InitAttachAnimationState(&AttachAnimState, m_MotionDir, m_AimDir, m_pAssetsEditor->m_ViewedAssetPath, m_pAssetsEditor->GetTime());
	m_pAssetsEditor->ModAPIGraphics()->DrawAttach(RenderTools(), &AttachAnimState, m_pAssetsEditor->m_ViewedAssetPath, TeePos, m_TeeScale);	
	
	m_pAssetsEditor->ModAPIGraphics()->DrawTee(RenderTools(), &m_TeeRenderInfo, &TeeAnimState, TeePos, m_AimDir, 0x0);
	
	//Gizmo
	m_GizmoType = MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE;
	if(m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION && !m_pAssetsEditor->m_EditedAssetPath.IsNull())
	{
		//Elements
		for(int i=0; i<pAttach->m_BackElements.size(); i++)
		{
			if(m_pAssetsEditor->m_EditedAssetPath == pAttach->m_BackElements[i].m_AnimationPath)
			{
				UpdateGizmo(
					m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath),
					TeePos + AttachAnimState.m_Frames[i].m_Pos * m_TeeScale,
					AttachAnimState.m_Frames[i].m_Angle,
					pAttach->m_BackElements[i].m_Alignment
				);
				break;
			}
		}
		if(m_GizmoType == MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE)
		{
			CModAPI_Asset_TeeAnimation* pTeeAnimation = m_pAssetsEditor->ModAPIGraphics()->m_TeeAnimationsCatalog.GetAsset(pAttach->m_TeeAnimationPath);
			if(pTeeAnimation)
			{
				UpdateGizmoFromTeeAnimation(pTeeAnimation, &TeeAnimState, TeePos);
			}
		}
		
		RenderGizmo();
	}
}

void CModAPI_AssetsEditorGui_View::Render()
{
	CUIRect rect;
	rect.x = m_Rect.x;
	rect.y = m_Rect.y;
	rect.w = m_Rect.w;
	rect.h = m_Rect.h;
	
	RenderTools()->DrawRoundRect(&rect, vec4(1.0f, 1.0f, 1.0f, 0.5f), 5.0f);
	
	Graphics()->ClipEnable(m_Rect.x, m_Rect.y, m_Rect.w, m_Rect.h);
	switch(m_pAssetsEditor->m_ViewedAssetType)
	{
		case MODAPI_ASSETTYPE_IMAGE:
			RenderImage();
			break;
		case MODAPI_ASSETTYPE_SPRITE:
			RenderSprite();
			break;
		case MODAPI_ASSETTYPE_ANIMATION:
			RenderAnimation();
			break;
		case MODAPI_ASSETTYPE_TEEANIMATION:
			RenderTeeAnimation();
			break;
		case MODAPI_ASSETTYPE_ATTACH:
			RenderAttach();
			break;
	}
	Graphics()->ClipDisable();
}

void CModAPI_AssetsEditorGui_View::OnMouseButtonClick(int X, int Y)
{
	if(distance(GetAimPosition(), vec2(X, Y)) < 32.0f)
	{
		m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_AIM;
		return;
	}
	
	if(distance(GetMotionPosition(), vec2(X, Y)) < 32.0f)
	{
		m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_MOTION;
		return;
	}
	
	if(m_GizmoType == MODAPI_ASSETSEDITOR_GIZMOTYPE_XYZ)
	{
		vec2 MagnetXPos = m_GizmoPos + m_GizmoAxisX*m_GizmoAxisXLength;
		if(distance(MagnetXPos, vec2(X, Y)) < 8.0f)
		{
			m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_X;
			return;
		}
		
		vec2 MagnetYPos = m_GizmoPos + m_GizmoAxisY*m_GizmoAxisYLength;
		if(distance(MagnetYPos, vec2(X, Y)) < 8.0f)
		{
			m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_Y;
			return;
		}
		
		vec2 MagnetZPos = m_GizmoPos + m_GizmoAxisZ*m_GizmoAxisZLength;
		
		if(distance(MagnetZPos, vec2(X, Y)) < 8.0f)
		{
			m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_ROTATION;
			return;
		}
	}
}

void CModAPI_AssetsEditorGui_View::OnMouseButtonRelease()
{
	m_DragedElement = MODAPI_ASSETSEDITOR_DRAGITEM_NONE;
}

void CModAPI_AssetsEditorGui_View::OnMouseOver(int X, int Y, int KeyState)
{
	if(m_Rect.IsInside(X, Y))
	{
		vec2 TeePos = GetTeePosition();
		
		switch(m_DragedElement)
		{
			case MODAPI_ASSETSEDITOR_DRAGITEM_AIM:
				m_AimDir = normalize(vec2(X, Y) - TeePos);
				break;
			case MODAPI_ASSETSEDITOR_DRAGITEM_MOTION:
				m_MotionDir = normalize(vec2(X, Y) - TeePos);
				break;
			case MODAPI_ASSETSEDITOR_DRAGITEM_ROTATION:
			{
				CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
				if(pAnimation && m_pAssetsEditor->m_EditedAssetFrame >= 0 && m_pAssetsEditor->m_EditedAssetFrame < pAnimation->m_lKeyFrames.size())
				{
					float angleValue = angle(vec2(X, Y) - m_GizmoPos, m_GizmoAxisX);
					if(m_GizmoAxisX.x * m_GizmoAxisY.y - m_GizmoAxisY.x * m_GizmoAxisX.y > 0.0f)
					{
						pAnimation->m_lKeyFrames[m_pAssetsEditor->m_EditedAssetFrame].m_Angle = -angleValue;
					}
					else
					{
						pAnimation->m_lKeyFrames[m_pAssetsEditor->m_EditedAssetFrame].m_Angle = angleValue;
					}
				}
				
				break;
			}
		}
	}
}

void CModAPI_AssetsEditorGui_View::OnMouseMotion(int X, int Y, int KeyState)
{
	switch(m_DragedElement)
	{
		case MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_X:
		{
			CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
			if(pAnimation && m_pAssetsEditor->m_EditedAssetFrame >= 0 && m_pAssetsEditor->m_EditedAssetFrame < pAnimation->m_lKeyFrames.size())
			{
				pAnimation->m_lKeyFrames[m_pAssetsEditor->m_EditedAssetFrame].m_Pos.x += dot(vec2(X, Y), m_GizmoAxisX) / m_TeeScale;
			}
			
			break;
		}
		case MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_Y:
		{
			CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
			if(pAnimation && m_pAssetsEditor->m_EditedAssetFrame >= 0 && m_pAssetsEditor->m_EditedAssetFrame < pAnimation->m_lKeyFrames.size())
			{
				pAnimation->m_lKeyFrames[m_pAssetsEditor->m_EditedAssetFrame].m_Pos.y += dot(vec2(X, Y), m_GizmoAxisY) / m_TeeScale;
			}
			
			break;
		}
	}
}
