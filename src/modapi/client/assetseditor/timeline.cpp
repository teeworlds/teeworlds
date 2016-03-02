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

#include "timeline.h"

CModAPI_AssetsEditorGui_Timeline::CModAPI_AssetsEditorGui_Timeline(CModAPI_AssetsEditor* pAssetsEditor) :
	CModAPI_ClientGui_Widget(pAssetsEditor->m_pGuiConfig),
	m_pAssetsEditor(pAssetsEditor),
	m_TimeScroll(0.0f),
	m_ValueScroll(0.0f),
	m_TimeScale(1.0f),
	m_TimeMax(5.0f),
	m_SelectedFrame(-1),
	m_SelectedFrameMoved(false),
	m_VertexSize(16),
	m_ToolbarHeight(30),
	m_TimelineTop(0),
	m_TimelineBottom(0),
	m_pToolbar(0),
	m_EditMode(TIMELINE_EDITMODE_ANGLE)
{
	m_pTimeSlider = new CTimeSlider(this);
	m_pValueSlider = new CValueSlider(this);
	
	CModAPI_ClientGui_Label* pTimeScaleLabel = new CModAPI_ClientGui_Label(m_pConfig, "Time scale:");
	pTimeScaleLabel->SetRect(CModAPI_ClientGui_Rect(
		0, 0, //Positions will be filled when the toolbar is updated
		90,
		pTimeScaleLabel->m_Rect.h
	));
	
	m_pEditModeButton = new CEditModeButton(this);
	
	m_pToolbar = new CModAPI_ClientGui_HListLayout(m_pConfig);
	m_pToolbar->Add(new CFirstFrameButton(m_pAssetsEditor));
	m_pToolbar->Add(new CPrevFrameButton(m_pAssetsEditor));
	m_pToolbar->Add(new CPlayPauseButton(m_pAssetsEditor));
	m_pToolbar->Add(new CNextFrameButton(m_pAssetsEditor));
	m_pToolbar->Add(new CLastFrameButton(m_pAssetsEditor));
	m_pToolbar->Add(m_pEditModeButton);
	m_pToolbar->Add(new CNewFrameButton(this));
	m_pToolbar->Add(pTimeScaleLabel);
	m_pToolbar->Add(new CTimeScaleSlider(this));
	
	m_pToolbar->Update();
}
	
CModAPI_AssetsEditorGui_Timeline::~CModAPI_AssetsEditorGui_Timeline()
{
	if(m_pToolbar) delete m_pToolbar;
	if(m_pTimeSlider) delete m_pTimeSlider;
	if(m_pValueSlider) delete m_pValueSlider;
}

void CModAPI_AssetsEditorGui_Timeline::NewFrame()
{
	if(m_pAssetsEditor->m_EditedAssetType != MODAPI_ASSETTYPE_ANIMATION)
		return;
	
	CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
	if(!pAnimation)
		return;
	
	CModAPI_Asset_Animation::CKeyFrame Frame;
	pAnimation->GetFrame(m_pAssetsEditor->GetTime(), &Frame);
	Frame.m_Time = m_pAssetsEditor->GetTime();
	pAnimation->AddKeyFrame(Frame);
}

void CModAPI_AssetsEditorGui_Timeline::TimeScaleCallback(float Pos)
{
	float MinScale = static_cast<float>(m_Rect.w) / (m_TimeMax);
	float MaxScale = static_cast<float>(m_Rect.w) * 10.0f;
	float ScaleWidth = MaxScale - MinScale;
	m_TimeScale = MinScale + Pos * Pos * ScaleWidth;
}

void CModAPI_AssetsEditorGui_Timeline::ValueScrollCallback(float Pos)
{
	Pos = Pos*2.0f - 1.0f;
	float VisibleValue = static_cast<float>(m_TimelineRect.h);
	float NeededScroll = max(400 - VisibleValue, 0.0f)/2;
	m_ValueScroll = NeededScroll * Pos;
}

void CModAPI_AssetsEditorGui_Timeline::TimeScrollCallback(float Pos)
{
	float VisibleTime = static_cast<float>(m_TimelineRect.w) / m_TimeScale;
	float NeededScroll = max(m_TimeMax - VisibleTime, 0.0f);
	m_TimeScroll = NeededScroll * Pos;
}

void CModAPI_AssetsEditorGui_Timeline::SetEditMode(int Mode)
{
	m_EditMode = Mode;
	
	m_pEditModeButton->OnEditModeChange(m_EditMode);
	m_pToolbar->Update();
}

int CModAPI_AssetsEditorGui_Timeline::GetEditMode()
{
	return m_EditMode;
}

void CModAPI_AssetsEditorGui_Timeline::Update()
{
	m_TimelineRect.x = m_Rect.x + 2*m_pConfig->m_LayoutMargin + m_pValueSlider->m_Rect.w;
	m_TimelineRect.y = m_Rect.y + m_pConfig->m_LayoutMargin;
	m_TimelineRect.w = m_Rect.w - 3*m_pConfig->m_LayoutMargin - m_pValueSlider->m_Rect.w;
	m_TimelineRect.h = m_Rect.h - 4*m_pConfig->m_LayoutMargin - m_ToolbarHeight - m_pTimeSlider->m_Rect.h;
	
	m_TimelineTop = m_TimelineRect.y + 16;
	m_TimelineBottom = m_TimelineTop + m_TimelineRect.h - 16;
	
	m_TimeScale = static_cast<float>(m_Rect.w) / (m_TimeMax/2.0f);
	
	m_pToolbar->SetRect(CModAPI_ClientGui_Rect(
		m_Rect.x,
		m_Rect.y + m_Rect.h - m_ToolbarHeight,
		m_Rect.w,
		m_ToolbarHeight
	));
	
	m_pTimeSlider->SetRect(CModAPI_ClientGui_Rect(
		m_TimelineRect.x,
		m_TimelineRect.y + m_TimelineRect.h + m_pConfig->m_LayoutMargin,
		m_TimelineRect.w,
		m_pTimeSlider->m_Rect.h
	));
	m_pTimeSlider->Update();
	
	m_pValueSlider->SetRect(CModAPI_ClientGui_Rect(
		m_TimelineRect.x - m_pValueSlider->m_Rect.w - m_pConfig->m_LayoutMargin,
		m_TimelineRect.y,
		m_pValueSlider->m_Rect.w,
		m_TimelineRect.h
	));
	m_pValueSlider->Update();
	
	m_pToolbar->Update();
	
	CModAPI_ClientGui_Widget::Update();
}

int CModAPI_AssetsEditorGui_Timeline::TimeToTimeline(float Time)
{		
	return m_TimelineRect.x + m_TimeScale * (Time - m_TimeScroll);
}

int CModAPI_AssetsEditorGui_Timeline::AngleToTimeline(float Angle)
{
	return m_TimelineBottom + ((Angle+2.0f*pi)/(4.0f*pi)) * (m_TimelineTop - m_TimelineBottom);
}

float CModAPI_AssetsEditorGui_Timeline::TimelineToAngle(int Value)
{
	return 4.0f*pi*static_cast<float>(Value - m_TimelineBottom) / static_cast<float>(m_TimelineTop - m_TimelineBottom) - 2.0*pi;
}

float CModAPI_AssetsEditorGui_Timeline::RelTimelineToAngle(int Rel)
{
	return 4.0f*pi*static_cast<float>(Rel) / static_cast<float>(m_TimelineTop - m_TimelineBottom);
}

int CModAPI_AssetsEditorGui_Timeline::OpacityToTimeline(float Opacity)
{
	return m_TimelineBottom + Opacity * (m_TimelineTop - m_TimelineBottom);
}

float CModAPI_AssetsEditorGui_Timeline::RelTimelineToOpacity(int Rel)
{
	return static_cast<float>(Rel) / static_cast<float>(m_TimelineTop - m_TimelineBottom);
}

int CModAPI_AssetsEditorGui_Timeline::PositionToTimeline(float Position)
{
	return m_TimelineBottom + (Position+16.0f)/(32.0f) * (m_TimelineTop - m_TimelineBottom);
}

float CModAPI_AssetsEditorGui_Timeline::RelTimelineToPosition(int Rel)
{
	return 32.0f*static_cast<float>(Rel) / static_cast<float>(m_TimelineTop - m_TimelineBottom);
}

void CModAPI_AssetsEditorGui_Timeline::Render()
{
	CUIRect rect;
	rect.x = m_Rect.x;
	rect.y = m_Rect.y;
	rect.w = m_Rect.w;
	rect.h = m_Rect.h - m_ToolbarHeight - 5;
	RenderTools()->DrawRoundRect(&rect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), s_LayoutCornerRadius);
	
	Graphics()->ClipEnable(m_TimelineRect.x, m_TimelineRect.y, m_TimelineRect.w, m_TimelineRect.h);
	{
		float TimeIter = 0.0f;
		int RectIter = 0;
		
		while(TimeIter < m_TimeMax)
		{
			if(RectIter%2 == 0)
			{
				CUIRect timerect;
				timerect.x = TimeToTimeline(TimeIter);
				timerect.y = m_TimelineRect.y;
				timerect.w = TimeToTimeline(TimeIter + 0.1f) - timerect.x;
				timerect.h = m_TimelineRect.h;
			
				RenderTools()->DrawUIRect(&timerect, vec4(0.0f, 0.0f, 0.0f, s_LayoutOpacity), 0, s_LayoutCornerRadius);
			}
			RectIter++;
			TimeIter += 0.1f;
		}
	}
	
	{
		int TimePos = TimeToTimeline(m_pAssetsEditor->GetTime());		
			
		IGraphics::CLineItem Line(TimePos, m_TimelineTop-6, TimePos, m_TimelineBottom);
		Graphics()->TextureClear();
		Graphics()->LinesBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->LinesDraw(&Line, 1);
		Graphics()->LinesEnd();
		
		int IconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_TRIANGLE;
		int SubX = IconId%16;
		int SubY = IconId/16;
		
		Graphics()->TextureSet(m_pAssetsEditor->m_ModEditorTexture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset((SubX+1)/16.0f, (SubY+1)/16.0f, SubX/16.0f, SubY/16.0f);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem(TimePos - m_VertexSize/2, m_TimelineTop - m_VertexSize, m_VertexSize, m_VertexSize);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	
	if(m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
		if(pAnimation)
		{
			vec4 ValueColor;
			switch(m_EditMode)
			{
				case TIMELINE_EDITMODE_ANGLE:
					ValueColor = vec4(1.0f, 1.0f, 0.5f, 1.0f);
					break;
				case TIMELINE_EDITMODE_OPACITY:
					ValueColor = vec4(1.0f, 0.5f, 1.0f, 1.0f);
					break;
				case TIMELINE_EDITMODE_POSX:
				case TIMELINE_EDITMODE_POSY:
					ValueColor = vec4(0.5f, 1.0f, 0.5f, 1.0f);
					break;
			}
				
			int LastTimePos = 0.0f;
			int LastValuePos = 0.0f;
	
			for(int i=0; i<pAnimation->m_lKeyFrames.size(); i++)
			{
				int TimePos = TimeToTimeline(pAnimation->m_lKeyFrames[i].m_Time);	
				int ValuePos = 0;
				
				switch(m_EditMode)
				{
					case TIMELINE_EDITMODE_ANGLE:
						ValuePos = AngleToTimeline(pAnimation->m_lKeyFrames[i].m_Angle);
						break;
					case TIMELINE_EDITMODE_OPACITY:
						ValuePos = OpacityToTimeline(pAnimation->m_lKeyFrames[i].m_Opacity);
						break;
					case TIMELINE_EDITMODE_POSX:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.x) - m_ValueScroll;
						break;
					case TIMELINE_EDITMODE_POSY:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.y) - m_ValueScroll;
						break;
				}
				
				if(i>0)
				{
					IGraphics::CLineItem Line(LastTimePos, LastValuePos, TimePos, ValuePos);
					
					Graphics()->TextureClear();
					Graphics()->LinesBegin();
					Graphics()->SetColor(ValueColor.x, ValueColor.y, ValueColor.z, ValueColor.w);
					Graphics()->LinesDraw(&Line, 1);
					Graphics()->LinesEnd();
				}
				
				IGraphics::CLineItem Line(TimePos, m_TimelineTop-6, TimePos, m_TimelineBottom);
				Graphics()->TextureClear();
				Graphics()->LinesBegin();
				Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
				Graphics()->LinesDraw(&Line, 1);
				Graphics()->LinesEnd();
				
				LastTimePos = TimePos;
				LastValuePos = ValuePos;
			}
				
			for(int i=0; i<pAnimation->m_lKeyFrames.size(); i++)
			{
				int TimePos = TimeToTimeline(pAnimation->m_lKeyFrames[i].m_Time);
				int ValuePos = 0;
				
				int ValueIconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_CIRCLE;
				float ValueIconAngle = 0.0f;
					
				switch(m_EditMode)
				{
					case TIMELINE_EDITMODE_ANGLE:
						ValueIconAngle = pAnimation->m_lKeyFrames[i].m_Angle;
						ValuePos = AngleToTimeline(pAnimation->m_lKeyFrames[i].m_Angle);
						ValueIconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_ROTATION;
						break;
					case TIMELINE_EDITMODE_OPACITY:
						ValuePos = OpacityToTimeline(pAnimation->m_lKeyFrames[i].m_Opacity);
						break;
					case TIMELINE_EDITMODE_POSX:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.x) - m_ValueScroll;
						break;
					case TIMELINE_EDITMODE_POSY:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.y) - m_ValueScroll;
						break;
				}
				
				{
					int SubX = ValueIconId%16;
					int SubY = ValueIconId/16;
					
					Graphics()->TextureSet(m_pAssetsEditor->m_ModEditorTexture);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetRotation(ValueIconAngle);
					Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
					Graphics()->SetColor(ValueColor.x, ValueColor.y, ValueColor.z, ValueColor.w);
					IGraphics::CQuadItem QuadItem(TimePos - m_VertexSize/2, ValuePos - m_VertexSize/2, m_VertexSize, m_VertexSize);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
				
				{
					int IconId = MODAPI_ASSETSEDITOR_ICON_MAGNET_TRIANGLE;
					int SubX = IconId%16;
					int SubY = IconId/16;
					
					Graphics()->TextureSet(m_pAssetsEditor->m_ModEditorTexture);
					Graphics()->QuadsBegin();
					Graphics()->QuadsSetSubset(SubX/16.0f, SubY/16.0f, (SubX+1)/16.0f, (SubY+1)/16.0f);
					Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
					IGraphics::CQuadItem QuadItem(TimePos - m_VertexSize/2, m_TimelineTop - m_VertexSize, m_VertexSize, m_VertexSize);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}
		}
	}
	Graphics()->ClipDisable();
		
	m_pTimeSlider->Render();
	m_pValueSlider->Render();
	m_pToolbar->Render();
}

void CModAPI_AssetsEditorGui_Timeline::OnMouseButtonClick(int X, int Y)
{
	m_SelectedFrame = -1;
		
	if(m_Rect.IsInside(X, Y) && m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
		if(pAnimation)
		{
			for(int i=0; i<pAnimation->m_lKeyFrames.size(); i++)
			{
				int TimePos = TimeToTimeline(pAnimation->m_lKeyFrames[i].m_Time);
				int ValuePos = 0;
				
				switch(m_EditMode)
				{
					case TIMELINE_EDITMODE_ANGLE:
						ValuePos = AngleToTimeline(pAnimation->m_lKeyFrames[i].m_Angle);
						break;
					case TIMELINE_EDITMODE_OPACITY:
						ValuePos = OpacityToTimeline(pAnimation->m_lKeyFrames[i].m_Opacity);
						break;
					case TIMELINE_EDITMODE_POSX:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.x) - m_ValueScroll;
						break;
					case TIMELINE_EDITMODE_POSY:
						ValuePos = PositionToTimeline(pAnimation->m_lKeyFrames[i].m_Pos.y) - m_ValueScroll;
						break;
				}
				
				CModAPI_ClientGui_Rect rectValue(TimePos - m_VertexSize/2, ValuePos - m_VertexSize/2, m_VertexSize, m_VertexSize);
				CModAPI_ClientGui_Rect rectTime(TimePos - m_VertexSize/2, m_TimelineTop - m_VertexSize, m_VertexSize, m_VertexSize);
				
				if(rectValue.IsInside(X, Y))
				{
					m_SelectedFrame = i;
					m_SelectedFrameMoved = false;
					m_pAssetsEditor->HideCursor();
				}
				else if(rectTime.IsInside(X, Y))
				{
					m_SelectedFrame = i;
					m_SelectedFrameMoved = true;
					m_pAssetsEditor->HideCursor();
					
					m_pAssetsEditor->EditAssetFrame(i);
				}
			}
		}
	}
		
	if(m_SelectedFrame == -1 && m_TimelineRect.IsInside(X, Y))
	{
		m_pAssetsEditor->SetTime((static_cast<float>(X - m_TimelineRect.x)/m_TimeScale) + m_TimeScroll);
		m_SelectedFrameMoved = true;
		m_pAssetsEditor->HideCursor();
	}
	
	m_pTimeSlider->OnMouseButtonClick(X, Y);
	m_pValueSlider->OnMouseButtonClick(X, Y);
	m_pToolbar->OnMouseButtonClick(X, Y);
}

void CModAPI_AssetsEditorGui_Timeline::OnMouseButtonRelease()
{
	m_SelectedFrame = -1;
	m_SelectedFrameMoved = false;
	
	m_pTimeSlider->OnMouseButtonRelease();
	m_pValueSlider->OnMouseButtonRelease();
	m_pToolbar->OnMouseButtonRelease();
	
	m_pAssetsEditor->ShowCursor();
}

void CModAPI_AssetsEditorGui_Timeline::OnMouseMotion(int RelX, int RelY, int KeyState)
{
	m_pTimeSlider->OnMouseMotion(RelX, RelY, KeyState);
	m_pValueSlider->OnMouseMotion(RelX, RelY, KeyState);
	m_pToolbar->OnMouseMotion(RelX, RelY, KeyState);

	if(KeyState & MODAPI_INPUT_SHIFT)
		return;
		
	float MotionFactor = 1.0f;
	if(KeyState & MODAPI_INPUT_CTRL)
		MotionFactor = 0.1f;
	
	if(m_SelectedFrame < 0)
	{
		if(m_SelectedFrameMoved)
		{
			float Motion = static_cast<float>(RelX) * MotionFactor;
			float DeltaTime = Motion / m_TimeScale;
			m_pAssetsEditor->SetTime(clamp(m_pAssetsEditor->GetTime() + DeltaTime, 0.0f, m_TimeMax));
		}
	}
	else if(m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
		if(pAnimation && m_SelectedFrame < pAnimation->m_lKeyFrames.size())
		{
			if(m_SelectedFrameMoved)
			{
				float Motion = static_cast<float>(RelX) * MotionFactor;
				
				float MinTime = 0.0f;
				if(m_SelectedFrame > 0)
					MinTime = pAnimation->m_lKeyFrames[m_SelectedFrame-1].m_Time;
					
				if(KeyState & MODAPI_INPUT_ALT)
				{
					float DeltaTime = Motion / m_TimeScale;
					
					for(int i=m_SelectedFrame; i<pAnimation->m_lKeyFrames.size(); i++)
					{
						float Time = max(pAnimation->m_lKeyFrames[i].m_Time + DeltaTime, MinTime);
						pAnimation->m_lKeyFrames[i].m_Time = Time;
					}
				}
				else
				{
					float MaxTime = m_TimeMax;
					if(m_SelectedFrame+1 < pAnimation->m_lKeyFrames.size())
						MaxTime = pAnimation->m_lKeyFrames[m_SelectedFrame+1].m_Time;
						
					float DeltaTime = Motion / m_TimeScale;
					float Time = clamp(pAnimation->m_lKeyFrames[m_SelectedFrame].m_Time + DeltaTime, MinTime, MaxTime);
					pAnimation->m_lKeyFrames[m_SelectedFrame].m_Time = Time;
				}
			}
			else
			{
				float Motion = static_cast<float>(RelY) * MotionFactor;
				
				switch(m_EditMode)
				{
					case TIMELINE_EDITMODE_ANGLE:
					{
						float Angle = clamp(pAnimation->m_lKeyFrames[m_SelectedFrame].m_Angle + RelTimelineToAngle(Motion), -2.0f*pi, 2.0f*pi);
						pAnimation->m_lKeyFrames[m_SelectedFrame].m_Angle = Angle;
						break;
					}
					case TIMELINE_EDITMODE_OPACITY:
						pAnimation->m_lKeyFrames[m_SelectedFrame].m_Opacity = clamp(pAnimation->m_lKeyFrames[m_SelectedFrame].m_Opacity + RelTimelineToOpacity(Motion), 0.0f, 1.0f);
						break;
					case TIMELINE_EDITMODE_POSX:
						pAnimation->m_lKeyFrames[m_SelectedFrame].m_Pos.x = pAnimation->m_lKeyFrames[m_SelectedFrame].m_Pos.x + RelTimelineToPosition(Motion);
						break;
					case TIMELINE_EDITMODE_POSY:
						pAnimation->m_lKeyFrames[m_SelectedFrame].m_Pos.y = pAnimation->m_lKeyFrames[m_SelectedFrame].m_Pos.y + RelTimelineToPosition(Motion);
						break;
				}
			}
		}
	}
}

void CModAPI_AssetsEditorGui_Timeline::OnMouseOver(int X, int Y, int KeyState)
{
	m_pTimeSlider->OnMouseOver(X, Y, KeyState);
	m_pValueSlider->OnMouseOver(X, Y, KeyState);
	m_pToolbar->OnMouseOver(X, Y, KeyState);

	if(!(KeyState & MODAPI_INPUT_SHIFT))
		return;
	
	if(m_SelectedFrame >= 0 && m_pAssetsEditor->m_EditedAssetType == MODAPI_ASSETTYPE_ANIMATION)
	{
		CModAPI_Asset_Animation* pAnimation = m_pAssetsEditor->ModAPIGraphics()->m_AnimationsCatalog.GetAsset(m_pAssetsEditor->m_EditedAssetPath);
		if(pAnimation && m_SelectedFrame < pAnimation->m_lKeyFrames.size())
		{
			if(!m_SelectedFrameMoved)
			{
				float Value = static_cast<float>(Y);
				
				switch(m_EditMode)
				{
					case TIMELINE_EDITMODE_ANGLE:
					{
						float AngleStep = pi/4.0f;
						float Angle = clamp(TimelineToAngle(Value), -2.0f*pi, 2.0f*pi);
						Angle = AngleStep*static_cast<int>(Angle/AngleStep);
						pAnimation->m_lKeyFrames[m_SelectedFrame].m_Angle = Angle;
						break;
					}
				}
			}
		}
	}
}
