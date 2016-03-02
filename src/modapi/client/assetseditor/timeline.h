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
	enum
	{
		TIMELINE_EDITMODE_ANGLE = 0,
		TIMELINE_EDITMODE_OPACITY,
		TIMELINE_EDITMODE_POSX,
		TIMELINE_EDITMODE_POSY,
		TIMELINE_NUM_EDITMODES,
	};
	
	class CNewFrameButton : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		
		virtual void MouseClickAction()
		{
			m_pTimeline->NewFrame();
		}
		
	public:
		CNewFrameButton(CModAPI_AssetsEditorGui_Timeline* pTimeline) :
			CModAPI_ClientGui_TextButton(pTimeline->m_pConfig, "Create frame", MODAPI_ASSETSEDITOR_ICON_INCREASE),
			m_pTimeline(pTimeline)
		{
			SetWidth(150);
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
	
	class CEditModeButton : public CModAPI_ClientGui_TextButton
	{
	protected:
		CModAPI_AssetsEditorGui_Timeline* m_pTimeline;
		
		virtual void MouseClickAction()
		{
			m_pTimeline->SetEditMode((m_pTimeline->GetEditMode() + 1) % TIMELINE_NUM_EDITMODES);
		}
		
	public:
		CEditModeButton(CModAPI_AssetsEditorGui_Timeline* pTimeline) :
			CModAPI_ClientGui_TextButton(pTimeline->m_pConfig, "", MODAPI_ASSETSEDITOR_ICON_ROTATION),
			m_pTimeline(pTimeline)
		{
			OnEditModeChange(m_pTimeline->GetEditMode());
			SetWidth(150);
		}
		
		void OnEditModeChange(int EditMode)
		{
			switch(m_pTimeline->GetEditMode())
			{
				case TIMELINE_EDITMODE_ANGLE:
					SetIcon(MODAPI_ASSETSEDITOR_ICON_ROTATION);
					SetText("Rotation");
					break;
				case TIMELINE_EDITMODE_OPACITY:
					SetIcon(MODAPI_ASSETSEDITOR_ICON_OPACITY);
					SetText("Opacity");
					break;
				case TIMELINE_EDITMODE_POSX:
					SetIcon(MODAPI_ASSETSEDITOR_ICON_TRANSLATE_X);
					SetText("Translation (X)");
					break;
				case TIMELINE_EDITMODE_POSY:
					SetIcon(MODAPI_ASSETSEDITOR_ICON_TRANSLATE_Y);
					SetText("Translation (Y)");
					break;
			}
			
			Update();
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
	CModAPI_AssetsEditor* m_pAssetsEditor;
	int m_Margin;
	int m_SelectedFrame;
	bool m_SelectedFrameMoved;
	int m_VertexSize;
	int m_ToolbarHeight;
	int m_TimelineTop;
	int m_TimelineBottom;
	int m_EditMode;
	
	float m_ValueScroll;
	float m_TimeScroll;
	float m_TimeScale;
	float m_TimeMax;
	
	CModAPI_ClientGui_Rect m_TimelineRect;
	CEditModeButton* m_pEditModeButton;
	CTimeSlider* m_pTimeSlider;
	CValueSlider* m_pValueSlider;
	CModAPI_ClientGui_HListLayout* m_pToolbar;
	
protected:
	void SetPause(bool Paused);
	void NewFrame();
	void TimeScaleCallback(float Pos);
	void ValueScrollCallback(float Pos);
	void TimeScrollCallback(float Pos);
	void SetEditMode(int Mode);
	int GetEditMode();
	
	int TimeToTimeline(float Time);
	int AngleToTimeline(float Angle);
	float TimelineToAngle(int Value);
	float RelTimelineToAngle(int Rel);
	int OpacityToTimeline(float Opacity);
	float RelTimelineToOpacity(int Rel);
	int PositionToTimeline(float Position);
	float RelTimelineToPosition(int Rel);
	
public:
	CModAPI_AssetsEditorGui_Timeline(CModAPI_AssetsEditor* pAssetsEditor);
	virtual ~CModAPI_AssetsEditorGui_Timeline();
	
	void OnEditedAssetChange();
	void OnEditedAssetFrameChange();
	
	virtual void Update();
	virtual void Render();
	
	virtual void OnMouseButtonClick(int X, int Y);
	virtual void OnMouseButtonRelease();	
	virtual void OnMouseMotion(int RelX, int RelY, int KeyState);
	virtual void OnMouseOver(int X, int Y, int KeyState);
};


#endif
