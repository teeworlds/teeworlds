#ifndef MODAPI_ASSETSEDITOR_TIMELINE_H
#define MODAPI_ASSETSEDITOR_TIMELINE_H

#include <base/vmath.h>
#include <engine/kernel.h>
#include <game/client/render.h>
#include <modapi/client/gui/layout.h>
#include <modapi/client/gui/button.h>

#include "assetseditor.h"

class CModAPI_AssetsEditorGui_Timeline : public CModAPI_ClientGui_Widget
{
	class CCursorToolButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		int m_CursorTool;
		
	protected:
		virtual void MouseClickAction()
		{
			m_pTimeline->SetCursorTool(m_CursorTool);
		}
		
	public:
		CCursorToolButton(CModAPI_AssetsEditorGui_Timeline* pTimeline, int Icon, int CursorTool) :
			CModAPI_ClientGui_IconButton(pTimeline->m_pConfig, Icon),
			m_pTimeline(pTimeline),
			m_CursorTool(CursorTool)
		{
			
		}
	};
	
	class CFirstFrameButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->EditAssetFirstFrame();
		}
		
	public:
		CFirstFrameButton(CModAPI_AssetsEditor* pAssetsEditor) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_FIRST_FRAME),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};
	
	class CLastFrameButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->EditAssetLastFrame();
		}
		
	public:
		CLastFrameButton(CModAPI_AssetsEditor* pAssetsEditor) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_LAST_FRAME),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};
	
	class CPrevFrameButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->EditAssetPrevFrame();
		}
		
	public:
		CPrevFrameButton(CModAPI_AssetsEditor* pAssetsEditor) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_PREV_FRAME),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};
	
	class CNextFrameButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->EditAssetNextFrame();
		}
		
	public:
		CNextFrameButton(CModAPI_AssetsEditor* pAssetsEditor) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, MODAPI_ASSETSEDITOR_ICON_NEXT_FRAME),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
	};
	
	class CPlayPauseButton : public CModAPI_ClientGui_IconButton
	{
	protected:
		CModAPI_AssetsEditor* m_pAssetsEditor;
		
		virtual void MouseClickAction()
		{
			m_pAssetsEditor->SetPause(!m_pAssetsEditor->IsPaused());
		}
		
	public:
		CPlayPauseButton(CModAPI_AssetsEditor* pAssetsEditor) :
			CModAPI_ClientGui_IconButton(pAssetsEditor->m_pGuiConfig, (pAssetsEditor->IsPaused() ? MODAPI_ASSETSEDITOR_ICON_PLAY : MODAPI_ASSETSEDITOR_ICON_PAUSE)),
			m_pAssetsEditor(pAssetsEditor)
		{
			
		}
		
		virtual void Render()
		{
			if(m_pAssetsEditor->IsPaused())
				SetIcon(MODAPI_ASSETSEDITOR_ICON_PLAY);
			else
				SetIcon(MODAPI_ASSETSEDITOR_ICON_PAUSE);
			
			CModAPI_ClientGui_IconButton::Render();
		}
	};
	
	class CTimeScaleSlider : public CModAPI_ClientGui_HSlider
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		
	protected:
		virtual void OnNewPosition(float Pos)
		{
			m_pTimeline->TimeScaleCallback(Pos);
		}
		
	public:
		CTimeScaleSlider(CModAPI_AssetsEditorGui_Timeline* pTimeline) :
			CModAPI_ClientGui_HSlider(pTimeline->m_pConfig),
			m_pTimeline(pTimeline)
		{
			m_Rect.w = 150;
			SetSliderSize(50);
		}
	};
	
	class CTimeSlider : public CModAPI_ClientGui_HSlider
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		
	protected:
		virtual void OnNewPosition(float Pos)
		{
			m_pTimeline->TimeScrollCallback(Pos);
		}
		
	public:
		CTimeSlider(CModAPI_AssetsEditorGui_Timeline* pTimeline) :
			CModAPI_ClientGui_HSlider(pTimeline->m_pConfig),
			m_pTimeline(pTimeline)
		{
			
		}
	};
	
	class CValueSlider : public CModAPI_ClientGui_VSlider
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		
	protected:
		virtual void OnNewPosition(float Pos)
		{
			m_pTimeline->ValueScrollCallback(Pos);
		}
		
	public:
		CValueSlider(CModAPI_AssetsEditorGui_Timeline* pTimeline) :
			CModAPI_ClientGui_VSlider(pTimeline->m_pConfig),
			m_pTimeline(pTimeline)
		{
			m_Pos = 0.5;
		}
	};
	
protected:
	enum
	{
		CURSORTOOL_FRAME_MOVE=0,
		CURSORTOOL_FRAME_ADD,
		CURSORTOOL_FRAME_DELETE,
		CURSORTOOL_FRAME_COLOR,
		NUM_CURSORTOOLS
	};
	
protected:
	CModAPI_AssetsEditor* m_pAssetsEditor;
	int m_Margin;
	int m_ToolbarHeight;
	int m_TimelineTop;
	int m_TimelineBottom;
	int m_ValueHeight;
	
	float m_ValueScroll;
	float m_TimeScroll;
	float m_TimeScale;
	float m_TimeMax;
	
	
	float m_FrameMargin;
	float m_FrameHeight;
	
	int m_CursorTool;
	int m_CursorX;
	int m_CursorY;
	int m_Drag;
	CModAPI_Asset_SkeletonAnimation::CSubPath m_DragedElement;
	CCursorToolButton* m_CursorToolButtons[NUM_CURSORTOOLS];
	
	CModAPI_ClientGui_Rect m_TimelineRect;
	CModAPI_ClientGui_Rect m_ListRect;
	CTimeSlider* m_pTimeSlider;
	CValueSlider* m_pValueSlider;
	CModAPI_ClientGui_HListLayout* m_pToolbar;
	
protected:
	void SetPause(bool Paused);
	void TimeScaleCallback(float Pos);
	void ValueScrollCallback(float Pos);
	void TimeScrollCallback(float Pos);
	
	int TimeToTimeline(float Time);
	float TimelineToTime(int Time);
	
public:
	CModAPI_AssetsEditorGui_Timeline(CModAPI_AssetsEditor* pAssetsEditor);
	virtual ~CModAPI_AssetsEditorGui_Timeline();
	
	void OnEditedAssetChange();
	void OnEditedAssetFrameChange();
	
	int GetCursorTool() { return m_CursorTool; }
	void SetCursorTool(int CursorTool) { m_CursorTool = CursorTool; }
	
	CModAPI_Asset_SkeletonAnimation::CSubPath KeyFramePicking(int X, int Y);
	CModAPI_Asset_Skeleton::CBonePath BonePicking(int X, int Y);
	CModAPI_Asset_Skeleton::CBonePath LayerPicking(int X, int Y);
	
	virtual void Update();
	virtual void Render();
	
	virtual void OnButtonClick(int X, int Y, int Button);
	virtual void OnButtonRelease(int Button);
	virtual void OnMouseOver(int X, int Y, int RelX, int RelY, int KeyState);
};


#endif
