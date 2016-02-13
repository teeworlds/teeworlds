#ifndef MODAPI_ASSETSEDITOR_VIEW_H
#define MODAPI_ASSETSEDITOR_VIEW_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/button.h>
#include <modapi/client/gui/layout.h>

#include "assetseditor.h"

class CModAPI_AssetsEditorGui_View : public CModAPI_ClientGui_Widget
{
protected:
	enum
	{
		MODAPI_ASSETSEDITOR_GIZMOTYPE_NONE=0,
		MODAPI_ASSETSEDITOR_GIZMOTYPE_VISUALCLUE,
		MODAPI_ASSETSEDITOR_GIZMOTYPE_XYZ,
		MODAPI_ASSETSEDITOR_GIZMOTYPE_SCALING,
	};
	
	enum
	{
		MODAPI_ASSETSEDITOR_DRAGITEM_NONE,
		MODAPI_ASSETSEDITOR_DRAGITEM_AIM,
		MODAPI_ASSETSEDITOR_DRAGITEM_MOTION,
		MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_X,
		MODAPI_ASSETSEDITOR_DRAGITEM_TRANSLATION_Y,
		MODAPI_ASSETSEDITOR_DRAGITEM_ROTATION,
		MODAPI_ASSETSEDITOR_DRAGITEM_SCALING_X,
		MODAPI_ASSETSEDITOR_DRAGITEM_SCALING_Y,
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	vec2 m_AimDir;
	vec2 m_MotionDir;
	int m_DragedElement;
	float m_TeeScale;
	
	int m_GizmoType;
	vec2 m_GizmoPos;
	vec2 m_GizmoAxisX;
	vec2 m_GizmoAxisY;
	vec2 m_GizmoAxisZ;
	
	float m_GizmoAxisXLength;
	float m_GizmoAxisYLength;
	float m_GizmoAxisZLength;
	
	CTeeRenderInfo m_TeeRenderInfo;

protected:
	vec2 GetTeePosition();
	vec2 GetAimPosition();
	vec2 GetMotionPosition();
	void UpdateGizmo(CModAPI_Asset_Animation* pAnimation, vec2 Pos, float Angle, int Alignment);
	void UpdateGizmoFromTeeAnimation(CModAPI_Asset_TeeAnimation* pTeeAnimation, CModAPI_TeeAnimationState* pTeeState, vec2 TeePos);
	
	void RenderGizmo();
	
	void RenderImage();
	void RenderSprite();
	void RenderAnimation();
	void RenderTeeAnimation();
	void RenderAttach();
	
public:
	CModAPI_AssetsEditorGui_View(CModAPI_AssetsEditor* pAssetsEditor);
	virtual void Render();
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();
	virtual void OnMouseOver(int X, int Y, int KeyState);
	virtual void OnMouseMotion(int X, int Y, int KeyState);
};

#endif
