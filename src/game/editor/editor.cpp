/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/system.h>

#include <engine/shared/datafile.h>
#include <engine/shared/config.h>
#include <engine/client.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/gamecore.h>
#include <game/localization.h>
#include <game/client/lineinput.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/generated/client_data.h>

#include "auto_map.h"
#include "editor.h"

int CEditor::ms_CheckerTexture;
int CEditor::ms_BackgroundTexture;
int CEditor::ms_CursorTexture;
int CEditor::ms_EntitiesTexture;
const void* CEditor::ms_pUiGotContext;

enum
{
	BUTTON_CONTEXT=1,
};

CEditorImage::~CEditorImage()
{
	m_pEditor->Graphics()->UnloadTexture(m_TexID);
	if(m_pData)
	{
		mem_free(m_pData);
		m_pData = 0;
	}
}

CLayerGroup::CLayerGroup()
{
	m_aName[0] = 0;
	m_Visible = true;
	m_SaveToMap = true;
	m_Collapse = false;
	m_GameGroup = false;
	m_OffsetX = 0;
	m_OffsetY = 0;
	m_ParallaxX = 100;
	m_ParallaxY = 100;

	m_UseClipping = 0;
	m_ClipX = 0;
	m_ClipY = 0;
	m_ClipW = 0;
	m_ClipH = 0;
}

CLayerGroup::~CLayerGroup()
{
	Clear();
}

void CLayerGroup::Convert(CUIRect *pRect)
{
	pRect->x += m_OffsetX;
	pRect->y += m_OffsetY;
}

void CLayerGroup::Mapping(float *pPoints)
{
	m_pMap->m_pEditor->RenderTools()->MapscreenToWorld(
		m_pMap->m_pEditor->m_WorldOffsetX, m_pMap->m_pEditor->m_WorldOffsetY,
		m_ParallaxX/100.0f, m_ParallaxY/100.0f,
		m_OffsetX, m_OffsetY,
		m_pMap->m_pEditor->Graphics()->ScreenAspect(), m_pMap->m_pEditor->m_WorldZoom, pPoints);

	pPoints[0] += m_pMap->m_pEditor->m_EditorOffsetX;
	pPoints[1] += m_pMap->m_pEditor->m_EditorOffsetY;
	pPoints[2] += m_pMap->m_pEditor->m_EditorOffsetX;
	pPoints[3] += m_pMap->m_pEditor->m_EditorOffsetY;
}

void CLayerGroup::MapScreen()
{
	float aPoints[4];
	Mapping(aPoints);
	m_pMap->m_pEditor->Graphics()->MapScreen(aPoints[0], aPoints[1], aPoints[2], aPoints[3]);
}

void CLayerGroup::Render()
{
	MapScreen();
	IGraphics *pGraphics = m_pMap->m_pEditor->Graphics();

	if(m_UseClipping)
	{
		float aPoints[4];
		m_pMap->m_pGameGroup->Mapping(aPoints);
		float x0 = (m_ClipX - aPoints[0]) / (aPoints[2]-aPoints[0]);
		float y0 = (m_ClipY - aPoints[1]) / (aPoints[3]-aPoints[1]);
		float x1 = ((m_ClipX+m_ClipW) - aPoints[0]) / (aPoints[2]-aPoints[0]);
		float y1 = ((m_ClipY+m_ClipH) - aPoints[1]) / (aPoints[3]-aPoints[1]);

		pGraphics->ClipEnable((int)(x0*pGraphics->ScreenWidth()), (int)(y0*pGraphics->ScreenHeight()),
			(int)((x1-x0)*pGraphics->ScreenWidth()), (int)((y1-y0)*pGraphics->ScreenHeight()));
	}

	for(int i = 0; i < m_lLayers.size(); i++)
	{
		if(m_lLayers[i]->m_Visible && m_lLayers[i] != m_pMap->m_pGameLayer)
		{
			if(m_pMap->m_pEditor->m_ShowDetail || !(m_lLayers[i]->m_Flags&LAYERFLAG_DETAIL))
				m_lLayers[i]->Render();
		}
	}

	pGraphics->ClipDisable();
}

void CLayerGroup::AddLayer(CLayer *l)
{
	m_pMap->m_Modified = true;
	m_lLayers.add(l);
}

void CLayerGroup::DeleteLayer(int Index)
{
	if(Index < 0 || Index >= m_lLayers.size()) return;
	delete m_lLayers[Index];
	m_lLayers.remove_index(Index);
	m_pMap->m_Modified = true;
}

void CLayerGroup::GetSize(float *w, float *h)
{
	*w = 0; *h = 0;
	for(int i = 0; i < m_lLayers.size(); i++)
	{
		float lw, lh;
		m_lLayers[i]->GetSize(&lw, &lh);
		*w = max(*w, lw);
		*h = max(*h, lh);
	}
}


int CLayerGroup::SwapLayers(int Index0, int Index1)
{
	if(Index0 < 0 || Index0 >= m_lLayers.size()) return Index0;
	if(Index1 < 0 || Index1 >= m_lLayers.size()) return Index0;
	if(Index0 == Index1) return Index0;
	m_pMap->m_Modified = true;
	swap(m_lLayers[Index0], m_lLayers[Index1]);
	return Index1;
}

void CEditorImage::AnalyseTileFlags()
{
	if(m_Format == CImageInfo::FORMAT_RGB)
	{
		for(int i = 0; i < 256; ++i)
			m_aTileFlags[i] = TILEFLAG_OPAQUE;
	}
	else
	{
		mem_zero(m_aTileFlags, sizeof(m_aTileFlags));

		int tw = m_Width/16; // tilesizes
		int th = m_Height/16;
		if(tw == th)
		{
			unsigned char *pPixelData = (unsigned char *)m_pData;

			int TileID = 0;
			for(int ty = 0; ty < 16; ty++)
				for(int tx = 0; tx < 16; tx++, TileID++)
				{
					bool Opaque = true;
					for(int x = 0; x < tw; x++)
						for(int y = 0; y < th; y++)
						{
							int p = (ty*tw+y)*m_Width + tx*tw+x;
							if(pPixelData[p*4+3] < 250)
							{
								Opaque = false;
								break;
							}
						}

					if(Opaque)
						m_aTileFlags[TileID] |= TILEFLAG_OPAQUE;
				}
		}
	}
}

void CEditor::EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser)
{
	CEditor *pThis = (CEditor *)pUser;
	if(Env < 0 || Env >= pThis->m_Map.m_lEnvelopes.size())
	{
		pChannels[0] = 0;
		pChannels[1] = 0;
		pChannels[2] = 0;
		pChannels[3] = 0;
		return;
	}

	CEnvelope *e = pThis->m_Map.m_lEnvelopes[Env];
	float t = pThis->m_AnimateTime+TimeOffset;
	t *= pThis->m_AnimateSpeed;
	e->Eval(t, pChannels);
}

/********************************************************
 OTHER
*********************************************************/

// copied from gc_menu.cpp, should be more generalized
//extern int ui_do_edit_box(void *id, const CUIRect *rect, char *str, int str_size, float font_size, bool hidden=false);
int CEditor::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden, int Corners)
{
	int Inside = UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;
	static float s_ScrollStart = 0.0f;

	FontSize *= UI()->Scale();

	if(UI()->LastActiveItem() == pID)
	{
		m_EditBoxActive = 2;
		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;

		if(Inside && UI()->MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = UI()->MouseX();
			int MxRel = (int)(UI()->MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(TextRender()->TextWidth(0, FontSize, pStr, i) - *Offset > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!UI()->MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(UI()->MouseX() < pRect->x && s_ScrollStart-UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
			else if(UI()->MouseX() > pRect->x+pRect->w && UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
		}

		for(int i = 0; i < Input()->NumEvents(); i++)
		{
			Len = str_length(pStr);
			int NumChars = Len;
			ReturnValue |= CLineInput::Manipulate(Input()->GetEvent(i), pStr, StrSize, StrSize, &Len, &s_AtIndex, &NumChars);
		}
	}

	bool JustGotActive = false;

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
		{
			s_AtIndex = min(s_AtIndex, str_length(pStr));
			s_DoScroll = false;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pID)
				JustGotActive = true;
			UI()->SetActiveItem(pID);
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	RenderTools()->DrawUIRect(&Textbox, vec4(1, 1, 1, 0.5f), Corners, 3.0f);
	Textbox.VMargin(2.0f, &Textbox);

	const char *pDisplayStr = pStr;
	char aStars[128];

	if(Hidden)
	{
		unsigned s = str_length(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(UI()->LastActiveItem() == pID && !JustGotActive && (UpdateOffset || Input()->NumEvents()))
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		if(w-*Offset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1);
			do
			{
				*Offset += min(wt-*Offset-Textbox.w, Textbox.w/3);
			}
			while(w-*Offset > Textbox.w);
		}
		else if(w-*Offset < 0.0f)
		{
			// move to the right
			do
			{
				*Offset = max(0.0f, *Offset-Textbox.w/3);
			}
			while(w-*Offset < 0.0f);
		}
	}
	UI()->ClipEnable(pRect);
	Textbox.x -= *Offset;

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, -1);

	// render the cursor
	if(UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox = *pRect;
		Textbox.VSplitLeft(2.0f, 0, &Textbox);
		Textbox.x += (w-*Offset-TextRender()->TextWidth(0, FontSize, "|", -1)/2);

		if((2*time_get()/time_freq()) % 2)	// make it blink
			UI()->DoLabel(&Textbox, "|", FontSize, -1);
	}
	UI()->ClipDisable();

	return ReturnValue;
}

vec4 CEditor::ButtonColorMul(const void *pID)
{
	if(UI()->ActiveItem() == pID)
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == pID)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

float CEditor::UiDoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float s_OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
	float Ret = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = UI()->MouseY()-s_OffsetY;
		Ret = (Cur-Min)/Max;
		if(Ret < 0.0f) Ret = 0.0f;
		if(Ret > 1.0f) Ret = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			s_OffsetY = UI()->MouseY()-Handle.y;
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.w = Rail.x-Slider.x;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
	Slider.x = Rail.x+Rail.w;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);

	return Ret;
}

vec4 CEditor::GetButtonColor(const void *pID, int Checked)
{
	if(Checked < 0)
		return vec4(0,0,0,0.5f);

	if(Checked > 0)
	{
		if(UI()->HotItem() == pID)
			return vec4(1,0,0,0.75f);
		return vec4(1,0,0,0.5f);
	}

	if(UI()->HotItem() == pID)
		return vec4(1,1,1,0.75f);
	return vec4(1,1,1,0.5f);
}

int CEditor::DoButton_Editor_Common(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(UI()->MouseInside(pRect))
	{
		if(Flags&BUTTON_CONTEXT)
			ms_pUiGotContext = pID;
		if(m_pTooltip)
			m_pTooltip = pToolTip;
	}

	if(UI()->HotItem() == pID && pToolTip)
		m_pTooltip = (const char *)pToolTip;

	return UI()->DoButtonLogic(pID, pText, Checked, pRect);

	// Draw here
	//return UI()->DoButton(id, text, checked, r, draw_func, 0);
}


int CEditor::DoButton_Editor(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_ALL, 3.0f);
	CUIRect NewRect = *pRect;
	NewRect.y += NewRect.h/2.0f-7.0f;
	float tw = min(TextRender()->TextWidth(0, 10.0f, pText, -1), NewRect.w);
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, NewRect.x + NewRect.w/2-tw/2, NewRect.y - 1.0f, 10.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = NewRect.w;
	TextRender()->TextEx(&Cursor, pText, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_File(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_ALL, 3.0f);

	CUIRect t = *pRect;
	t.VMargin(5.0f, &t);
	UI()->DoLabel(&t, pText, 10, -1, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	CUIRect r = *pRect;
	RenderTools()->DrawUIRect(&r, vec4(0.5f, 0.5f, 0.5f, 1.0f), CUI::CORNER_T, 3.0f);

	r = *pRect;
	r.VMargin(5.0f, &r);
	UI()->DoLabel(&r, pText, 10, -1, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_MenuItem(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	if(UI()->HotItem() == pID || Checked)
		RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_ALL, 3.0f);

	CUIRect t = *pRect;
	t.VMargin(5.0f, &t);
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, t.x, t.y - 1.0f, 10.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = t.w;
	TextRender()->TextEx(&Cursor, pText, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_Tab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_T, 5.0f);
	CUIRect NewRect = *pRect;
	NewRect.y += NewRect.h/2.0f-7.0f;
	UI()->DoLabel(&NewRect, pText, 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_Ex(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip, int Corners, float FontSize)
{
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), Corners, 3.0f);
	CUIRect NewRect = *pRect;
	NewRect.HMargin(NewRect.h/2.0f-FontSize/2.0f-1.0f, &NewRect);
	UI()->DoLabel(&NewRect, pText, FontSize, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_ButtonInc(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_R, 3.0f);
	UI()->DoLabel(pRect, pText?pText:"+", 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

int CEditor::DoButton_ButtonDec(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip)
{
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, Checked), CUI::CORNER_L, 3.0f);
	UI()->DoLabel(pRect, pText?pText:"-", 10, 0, -1);
	return DoButton_Editor_Common(pID, pText, Checked, pRect, Flags, pToolTip);
}

void CEditor::RenderGrid(CLayerGroup *pGroup)
{
	if(!m_GridActive)
		return;

	float aGroupPoints[4];
	pGroup->Mapping(aGroupPoints);

	float w = UI()->Screen()->w;
	float h = UI()->Screen()->h;

	int LineDistance = GetLineDistance();

	int XOffset = aGroupPoints[0]/LineDistance;
	int YOffset = aGroupPoints[1]/LineDistance;
	int XGridOffset = XOffset % m_GridFactor;
	int YGridOffset = YOffset % m_GridFactor;

	Graphics()->TextureSet(-1);
	Graphics()->LinesBegin();

	for(int i = 0; i < (int)w; i++)
	{
		if((i+YGridOffset) % m_GridFactor == 0)
			Graphics()->SetColor(1.0f, 0.3f, 0.3f, 0.3f);
		else
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.15f);

		IGraphics::CLineItem Line = IGraphics::CLineItem(LineDistance*XOffset, LineDistance*i+LineDistance*YOffset, w+aGroupPoints[2], LineDistance*i+LineDistance*YOffset);
		Graphics()->LinesDraw(&Line, 1);

		if((i+XGridOffset) % m_GridFactor == 0)
			Graphics()->SetColor(1.0f, 0.3f, 0.3f, 0.3f);
		else
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.15f);

		Line = IGraphics::CLineItem(LineDistance*i+LineDistance*XOffset, LineDistance*YOffset, LineDistance*i+LineDistance*XOffset, h+aGroupPoints[3]);
		Graphics()->LinesDraw(&Line, 1);
	}
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->LinesEnd();
}

void CEditor::RenderBackground(CUIRect View, int Texture, float Size, float Brightness)
{
	Graphics()->TextureSet(Texture);
	Graphics()->BlendNormal();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Brightness, Brightness, Brightness, 1.0f);
	Graphics()->QuadsSetSubset(0,0, View.w/Size, View.h/Size);
	IGraphics::CQuadItem QuadItem(View.x, View.y, View.w, View.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

int CEditor::UiDoValueSelector(void *pID, CUIRect *pRect, const char *pLabel, int Current, int Min, int Max, int Step, float Scale, const char *pToolTip)
{
	// logic
	static float s_Value;
	int Inside = UI()->MouseInside(pRect);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0) || Input()->KeyDown(KEY_ESCAPE))
		{
			m_LockMouse = false;
			UI()->SetActiveItem(0);
		}
		else
		{
			if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
				s_Value += m_MouseDeltaX*0.05f;
			else
				s_Value += m_MouseDeltaX;

			if(absolute(s_Value) > Scale)
			{
				int Count = (int)(s_Value/Scale);
				s_Value = fmod(s_Value, Scale);
				Current += Step*Count;
				if(Current < Min)
					Current = Min;
				if(Current > Max)
					Current = Max;
			}
		}
		if(pToolTip)
			m_pTooltip = pToolTip;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			m_LockMouse = true;
			s_Value = 0;
			UI()->SetActiveItem(pID);
		}
		if(pToolTip)
			m_pTooltip = pToolTip;
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf),"%s %d", pLabel, Current);
	RenderTools()->DrawUIRect(pRect, GetButtonColor(pID, 0), CUI::CORNER_ALL, 5.0f);
	pRect->y += pRect->h/2.0f-7.0f;
	UI()->DoLabel(pRect, aBuf, 10, 0, -1);

	return Current;
}

CLayerGroup *CEditor::GetSelectedGroup()
{
	if(m_SelectedGroup >= 0 && m_SelectedGroup < m_Map.m_lGroups.size())
		return m_Map.m_lGroups[m_SelectedGroup];
	return 0x0;
}

CLayer *CEditor::GetSelectedLayer(int Index)
{
	CLayerGroup *pGroup = GetSelectedGroup();
	if(!pGroup)
		return 0x0;

	if(m_SelectedLayer >= 0 && m_SelectedLayer < m_Map.m_lGroups[m_SelectedGroup]->m_lLayers.size())
		return pGroup->m_lLayers[m_SelectedLayer];
	return 0x0;
}

CLayer *CEditor::GetSelectedLayerType(int Index, int Type)
{
	CLayer *p = GetSelectedLayer(Index);
	if(p && p->m_Type == Type)
		return p;
	return 0x0;
}

CQuad *CEditor::GetSelectedQuad()
{
	CLayerQuads *ql = (CLayerQuads *)GetSelectedLayerType(0, LAYERTYPE_QUADS);
	if(!ql)
		return 0;
	if(m_SelectedQuad >= 0 && m_SelectedQuad < ql->m_lQuads.size())
		return &ql->m_lQuads[m_SelectedQuad];
	return 0;
}

void CEditor::CallbackOpenMap(const char *pFileName, int StorageType, void *pUser)
{
	CEditor *pEditor = (CEditor*)pUser;
	if(pEditor->Load(pFileName, StorageType))
	{
		str_copy(pEditor->m_aFileName, pFileName, 512);
		pEditor->m_ValidSaveFilename = StorageType == IStorage::TYPE_SAVE && pEditor->m_pFileDialogPath == pEditor->m_aFileDialogCurrentFolder;
		pEditor->SortImages();
		pEditor->m_Dialog = DIALOG_NONE;
		pEditor->m_Map.m_Modified = false;
	}
	else
	{
		pEditor->Reset();
		pEditor->m_aFileName[0] = 0;
	}
}
void CEditor::CallbackAppendMap(const char *pFileName, int StorageType, void *pUser)
{
	CEditor *pEditor = (CEditor*)pUser;
	if(pEditor->Append(pFileName, StorageType))
		pEditor->m_aFileName[0] = 0;
	else
		pEditor->SortImages();

	pEditor->m_Dialog = DIALOG_NONE;
}
void CEditor::CallbackSaveMap(const char *pFileName, int StorageType, void *pUser)
{
	CEditor *pEditor = static_cast<CEditor*>(pUser);
	char aBuf[1024];
	const int Length = str_length(pFileName);
	// add map extension
	if(Length <= 4 || pFileName[Length-4] != '.' || str_comp_nocase(pFileName+Length-3, "map"))
	{
		str_format(aBuf, sizeof(aBuf), "%s.map", pFileName);
		pFileName = aBuf;
	}

	if(pEditor->Save(pFileName))
	{
		str_copy(pEditor->m_aFileName, pFileName, sizeof(pEditor->m_aFileName));
		pEditor->m_ValidSaveFilename = StorageType == IStorage::TYPE_SAVE && pEditor->m_pFileDialogPath == pEditor->m_aFileDialogCurrentFolder;
		pEditor->m_Map.m_Modified = false;
	}

	pEditor->m_Dialog = DIALOG_NONE;
}

void CEditor::DoToolbar(CUIRect ToolBar)
{
	CUIRect TB_Top, TB_Bottom;
	CUIRect Button;

	ToolBar.HSplitTop(ToolBar.h/2.0f, &TB_Top, &TB_Bottom);

	TB_Top.HSplitBottom(2.5f, &TB_Top, 0);
	TB_Bottom.HSplitTop(2.5f, 0, &TB_Bottom);

	// ctrl+o to open
	if(Input()->KeyDown('o') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL)) && m_Dialog == DIALOG_NONE)
	{
		if(HasUnsavedData())
		{
			if(!m_PopupEventWasActivated)
			{
				m_PopupEventType = POPEVENT_LOAD;
				m_PopupEventActivated = true;
			}
		}
		else
			InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_MAP, "Load map", "Load", "maps", "", CallbackOpenMap, this);
	}

	// ctrl+s to save
	if(Input()->KeyDown('s') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL)) && m_Dialog == DIALOG_NONE)
	{
		if(m_aFileName[0] && m_ValidSaveFilename)
		{
			if(!m_PopupEventWasActivated)
			{
				str_copy(m_aFileSaveName, m_aFileName, sizeof(m_aFileSaveName));
				m_PopupEventType = POPEVENT_SAVE;
				m_PopupEventActivated = true;
			}
		}
		else
			InvokeFileDialog(IStorage::TYPE_SAVE, FILETYPE_MAP, "Save map", "Save", "maps", "", CallbackSaveMap, this);
	}

	// detail button
	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_HqButton = 0;
	if(DoButton_Editor(&s_HqButton, "HD", m_ShowDetail, &Button, 0, "[ctrl+h] Toggle High Detail") ||
		(Input()->KeyDown('h') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))))
	{
		m_ShowDetail = !m_ShowDetail;
	}

	TB_Top.VSplitLeft(5.0f, 0, &TB_Top);

	// animation button
	TB_Top.VSplitLeft(40.0f, &Button, &TB_Top);
	static int s_AnimateButton = 0;
	if(DoButton_Editor(&s_AnimateButton, "Anim", m_Animate, &Button, 0, "[ctrl+m] Toggle animation") ||
		(Input()->KeyDown('m') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))))
	{
		m_AnimateStart = time_get();
		m_Animate = !m_Animate;
	}

	TB_Top.VSplitLeft(5.0f, 0, &TB_Top);

	// proof button
	TB_Top.VSplitLeft(40.0f, &Button, &TB_Top);
	static int s_ProofButton = 0;
	if(DoButton_Editor(&s_ProofButton, "Proof", m_ProofBorders, &Button, 0, "[ctrl+p] Toggles proof borders. These borders represent what a player maximum can see.") ||
		(Input()->KeyDown('p') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))))
	{
		m_ProofBorders = !m_ProofBorders;
	}

	TB_Top.VSplitLeft(5.0f, 0, &TB_Top);

	// tile info button
	TB_Top.VSplitLeft(40.0f, &Button, &TB_Top);
	static int s_TileInfoButton = 0;
	if(DoButton_Editor(&s_TileInfoButton, "Info", m_ShowTileInfo, &Button, 0, "[ctrl+i] Show tile informations") ||
		(Input()->KeyDown('i') && (Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))))
	{
		m_ShowTileInfo = !m_ShowTileInfo;
		m_ShowEnvelopePreview = 0;
	}

	TB_Top.VSplitLeft(15.0f, 0, &TB_Top);

	// zoom group
	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_ZoomOutButton = 0;
	if(DoButton_Ex(&s_ZoomOutButton, "ZO", 0, &Button, 0, "[NumPad-] Zoom out", CUI::CORNER_L))
		m_ZoomLevel += 50;

	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_ZoomNormalButton = 0;
	if(DoButton_Ex(&s_ZoomNormalButton, "1:1", 0, &Button, 0, "[NumPad*] Zoom to normal and remove editor offset", 0))
	{
		m_EditorOffsetX = 0;
		m_EditorOffsetY = 0;
		m_ZoomLevel = 100;
	}

	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_ZoomInButton = 0;
	if(DoButton_Ex(&s_ZoomInButton, "ZI", 0, &Button, 0, "[NumPad+] Zoom in", CUI::CORNER_R))
		m_ZoomLevel -= 50;

	TB_Top.VSplitLeft(10.0f, 0, &TB_Top);

	// animation speed
	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_AnimFasterButton = 0;
	if(DoButton_Ex(&s_AnimFasterButton, "A+", 0, &Button, 0, "Increase animation speed", CUI::CORNER_L))
		m_AnimateSpeed += 0.5f;

	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_AnimNormalButton = 0;
	if(DoButton_Ex(&s_AnimNormalButton, "1", 0, &Button, 0, "Normal animation speed", 0))
		m_AnimateSpeed = 1.0f;

	TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
	static int s_AnimSlowerButton = 0;
	if(DoButton_Ex(&s_AnimSlowerButton, "A-", 0, &Button, 0, "Decrease animation speed", CUI::CORNER_R))
	{
		if(m_AnimateSpeed > 0.5f)
			m_AnimateSpeed -= 0.5f;
	}

	TB_Top.VSplitLeft(10.0f, &Button, &TB_Top);


	// brush manipulation
	{
		int Enabled = m_Brush.IsEmpty()?-1:0;

		// flip buttons
		TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
		static int s_FlipXButton = 0;
		if(DoButton_Ex(&s_FlipXButton, "X/X", Enabled, &Button, 0, "[N] Flip brush horizontal", CUI::CORNER_L) || Input()->KeyDown('n'))
		{
			for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
				m_Brush.m_lLayers[i]->BrushFlipX();
		}

		TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
		static int s_FlipyButton = 0;
		if(DoButton_Ex(&s_FlipyButton, "Y/Y", Enabled, &Button, 0, "[M] Flip brush vertical", CUI::CORNER_R) || Input()->KeyDown('m'))
		{
			for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
				m_Brush.m_lLayers[i]->BrushFlipY();
		}

		// rotate buttons
		TB_Top.VSplitLeft(15.0f, &Button, &TB_Top);

		TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
		static int s_RotationAmount = 90;
		bool TileLayer = false;
		// check for tile layers in brush selection
		for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
			if(m_Brush.m_lLayers[i]->m_Type == LAYERTYPE_TILES)
			{
				TileLayer = true;
				s_RotationAmount = max(90, (s_RotationAmount/90)*90);
				break;
			}
		s_RotationAmount = UiDoValueSelector(&s_RotationAmount, &Button, "", s_RotationAmount, TileLayer?90:1, 359, TileLayer?90:1, TileLayer?10.0f:2.0f, "Rotation of the brush in degrees. Use left mouse button to drag and change the value. Hold shift to be more precise.");

		TB_Top.VSplitLeft(5.0f, &Button, &TB_Top);
		TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
		static int s_CcwButton = 0;
		if(DoButton_Ex(&s_CcwButton, "CCW", Enabled, &Button, 0, "[R] Rotates the brush counter clockwise", CUI::CORNER_L) || Input()->KeyDown('r'))
		{
			for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
				m_Brush.m_lLayers[i]->BrushRotate(-s_RotationAmount/360.0f*pi*2);
		}

		TB_Top.VSplitLeft(30.0f, &Button, &TB_Top);
		static int s_CwButton = 0;
		if(DoButton_Ex(&s_CwButton, "CW", Enabled, &Button, 0, "[T] Rotates the brush clockwise", CUI::CORNER_R) || Input()->KeyDown('t'))
		{
			for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
				m_Brush.m_lLayers[i]->BrushRotate(s_RotationAmount/360.0f*pi*2);
		}
	}

	// quad manipulation
	{
		// do add button
		TB_Top.VSplitLeft(10.0f, &Button, &TB_Top);
		TB_Top.VSplitLeft(60.0f, &Button, &TB_Top);
		static int s_NewButton = 0;

		CLayerQuads *pQLayer = (CLayerQuads *)GetSelectedLayerType(0, LAYERTYPE_QUADS);
		//CLayerTiles *tlayer = (CLayerTiles *)get_selected_layer_type(0, LAYERTYPE_TILES);
		if(DoButton_Editor(&s_NewButton, "Add Quad", pQLayer?0:-1, &Button, 0, "Adds a new quad"))
		{
			if(pQLayer)
			{
				float Mapping[4];
				CLayerGroup *g = GetSelectedGroup();
				g->Mapping(Mapping);
				int AddX = f2fx(Mapping[0] + (Mapping[2]-Mapping[0])/2);
				int AddY = f2fx(Mapping[1] + (Mapping[3]-Mapping[1])/2);

				CQuad *q = pQLayer->NewQuad();
				for(int i = 0; i < 5; i++)
				{
					q->m_aPoints[i].x += AddX;
					q->m_aPoints[i].y += AddY;
				}
			}
		}
	}

	// tile manipulation
	{
		TB_Bottom.VSplitLeft(40.0f, &Button, &TB_Bottom);
		static int s_BorderBut = 0;
		CLayerTiles *pT = (CLayerTiles *)GetSelectedLayerType(0, LAYERTYPE_TILES);

		if(DoButton_Editor(&s_BorderBut, "Border", pT?0:-1, &Button, 0, "Adds border tiles"))
		{
			if(pT)
				DoMapBorder();
		}
	}

	TB_Bottom.VSplitLeft(5.0f, 0, &TB_Bottom);

	// refocus button
	TB_Bottom.VSplitLeft(50.0f, &Button, &TB_Bottom);
	static int s_RefocusButton = 0;
	if(DoButton_Editor(&s_RefocusButton, "Refocus", m_WorldOffsetX&&m_WorldOffsetY?0:-1, &Button, 0, "[HOME] Restore map focus") || (m_EditBoxActive == 0 && Input()->KeyDown(KEY_HOME)))
	{
		m_WorldOffsetX = 0;
		m_WorldOffsetY = 0;
	}

	TB_Bottom.VSplitLeft(5.0f, 0, &TB_Bottom);

	// grid button
	TB_Bottom.VSplitLeft(50.0f, &Button, &TB_Bottom);
	static int s_GridButton = 0;
	if(DoButton_Editor(&s_GridButton, "Grid", m_GridActive, &Button, 0, "Toggle Grid"))
	{
		m_GridActive = !m_GridActive;
	}

	TB_Bottom.VSplitLeft(30.0f, 0, &TB_Bottom);

	// grid zoom
	TB_Bottom.VSplitLeft(30.0f, &Button, &TB_Bottom);
	static int s_GridIncreaseButton = 0;
	if(DoButton_Ex(&s_GridIncreaseButton, "G-", 0, &Button, 0, "Decrease grid", CUI::CORNER_L))
	{
		if(m_GridFactor > 1)
			m_GridFactor--;
	}

	TB_Bottom.VSplitLeft(30.0f, &Button, &TB_Bottom);
	static int s_GridNormalButton = 0;
	if(DoButton_Ex(&s_GridNormalButton, "1", 0, &Button, 0, "Normal grid", 0))
		m_GridFactor = 1;

	TB_Bottom.VSplitLeft(30.0f, &Button, &TB_Bottom);

	static int s_GridDecreaseButton = 0;
	if(DoButton_Ex(&s_GridDecreaseButton, "G+", 0, &Button, 0, "Increase grid", CUI::CORNER_R))
	{
		if(m_GridFactor < 15)
			m_GridFactor++;
	}
}

static void Rotate(const CPoint *pCenter, CPoint *pPoint, float Rotation)
{
	int x = pPoint->x - pCenter->x;
	int y = pPoint->y - pCenter->y;
	pPoint->x = (int)(x * cosf(Rotation) - y * sinf(Rotation) + pCenter->x);
	pPoint->y = (int)(x * sinf(Rotation) + y * cosf(Rotation) + pCenter->y);
}

void CEditor::DoQuad(CQuad *q, int Index)
{
	enum
	{
		OP_NONE=0,
		OP_MOVE_ALL,
		OP_MOVE_PIVOT,
		OP_ROTATE,
		OP_CONTEXT_MENU,
	};

	// some basic values
	void *pID = &q->m_aPoints[4]; // use pivot addr as id
	static CPoint s_RotatePoints[4];
	static float s_LastWx;
	static float s_LastWy;
	static int s_Operation = OP_NONE;
	static float s_RotateAngle = 0;
	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();

	// get pivot
	float CenterX = fx2f(q->m_aPoints[4].x);
	float CenterY = fx2f(q->m_aPoints[4].y);

	float dx = (CenterX - wx)/m_WorldZoom;
	float dy = (CenterY - wy)/m_WorldZoom;
	if(dx*dx+dy*dy < 50)
		UI()->SetHotItem(pID);

	bool IgnoreGrid;
	if(Input()->KeyPressed(KEY_LALT) || Input()->KeyPressed(KEY_RALT))
		IgnoreGrid = true;
	else
		IgnoreGrid = false;

	// draw selection background
	if(m_SelectedQuad == Index)
	{
		Graphics()->SetColor(0,0,0,1);
		IGraphics::CQuadItem QuadItem(CenterX, CenterY, 7.0f, 7.0f);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}

	if(UI()->ActiveItem() == pID)
	{
		if(m_MouseDeltaWx*m_MouseDeltaWx+m_MouseDeltaWy*m_MouseDeltaWy > 0.5f)
		{
			// check if we only should move pivot
			if(s_Operation == OP_MOVE_PIVOT)
			{
				if(m_GridActive && !IgnoreGrid)
				{
					int LineDistance = GetLineDistance();

					float x = 0.0f;
					float y = 0.0f;
					if(wx >= 0)
						x = (int)((wx+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					else
						x = (int)((wx-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					if(wy >= 0)
						y = (int)((wy+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					else
						y = (int)((wy-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);

					q->m_aPoints[4].x = f2fx(x);
					q->m_aPoints[4].y = f2fx(y);
				}
				else
				{
					q->m_aPoints[4].x += f2fx(wx-s_LastWx);
					q->m_aPoints[4].y += f2fx(wy-s_LastWy);
				}
			}
			else if(s_Operation == OP_MOVE_ALL)
			{
				// move all points including pivot
				if(m_GridActive && !IgnoreGrid)
				{
					int LineDistance = GetLineDistance();

					float x = 0.0f;
					float y = 0.0f;
					if(wx >= 0)
						x = (int)((wx+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					else
						x = (int)((wx-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					if(wy >= 0)
						y = (int)((wy+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
					else
						y = (int)((wy-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);

					int OldX = q->m_aPoints[4].x;
					int OldY = q->m_aPoints[4].y;
					q->m_aPoints[4].x = f2fx(x);
					q->m_aPoints[4].y = f2fx(y);
					int DiffX = q->m_aPoints[4].x - OldX;
					int DiffY = q->m_aPoints[4].y - OldY;

					for(int v = 0; v < 4; v++)
					{
						q->m_aPoints[v].x += DiffX;
						q->m_aPoints[v].y += DiffY;
					}
				}
				else
				{
					for(int v = 0; v < 5; v++)
					{
							q->m_aPoints[v].x += f2fx(wx-s_LastWx);
							q->m_aPoints[v].y += f2fx(wy-s_LastWy);
					}
				}
			}
			else if(s_Operation == OP_ROTATE)
			{
				for(int v = 0; v < 4; v++)
				{
					q->m_aPoints[v] = s_RotatePoints[v];
					Rotate(&q->m_aPoints[4], &q->m_aPoints[v], s_RotateAngle);
				}
			}
		}

		s_RotateAngle += (m_MouseDeltaX) * 0.002f;
		s_LastWx = wx;
		s_LastWy = wy;

		if(s_Operation == OP_CONTEXT_MENU)
		{
			if(!UI()->MouseButton(1))
			{
				static int s_QuadPopupID = 0;
				UiInvokePopupMenu(&s_QuadPopupID, 0, UI()->MouseX(), UI()->MouseY(), 120, 180, PopupQuad);
				m_LockMouse = false;
				s_Operation = OP_NONE;
				UI()->SetActiveItem(0);
			}
		}
		else
		{
			if(!UI()->MouseButton(0))
			{
				m_LockMouse = false;
				s_Operation = OP_NONE;
				UI()->SetActiveItem(0);
			}
		}

		Graphics()->SetColor(1,1,1,1);
	}
	else if(UI()->HotItem() == pID)
	{
		ms_pUiGotContext = pID;

		Graphics()->SetColor(1,1,1,1);
		m_pTooltip = "Left mouse button to move. Hold shift to move pivot. Hold ctrl to rotate. Hold alt to ignore grid.";

		if(UI()->MouseButton(0))
		{
			if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
				s_Operation = OP_MOVE_PIVOT;
			else if(Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))
			{
				m_LockMouse = true;
				s_Operation = OP_ROTATE;
				s_RotateAngle = 0;
				s_RotatePoints[0] = q->m_aPoints[0];
				s_RotatePoints[1] = q->m_aPoints[1];
				s_RotatePoints[2] = q->m_aPoints[2];
				s_RotatePoints[3] = q->m_aPoints[3];
			}
			else
				s_Operation = OP_MOVE_ALL;

			UI()->SetActiveItem(pID);
			if(m_SelectedQuad != Index)
				m_SelectedPoints = 0;
			m_SelectedQuad = Index;
			s_LastWx = wx;
			s_LastWy = wy;
		}

		if(UI()->MouseButton(1))
		{
			if(m_SelectedQuad != Index)
				m_SelectedPoints = 0;
			m_SelectedQuad = Index;
			s_Operation = OP_CONTEXT_MENU;
			UI()->SetActiveItem(pID);
		}
	}
	else
		Graphics()->SetColor(0,1,0,1);

	IGraphics::CQuadItem QuadItem(CenterX, CenterY, 5.0f*m_WorldZoom, 5.0f*m_WorldZoom);
	Graphics()->QuadsDraw(&QuadItem, 1);
}

void CEditor::DoQuadPoint(CQuad *pQuad, int QuadIndex, int V)
{
	void *pID = &pQuad->m_aPoints[V];

	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();

	float px = fx2f(pQuad->m_aPoints[V].x);
	float py = fx2f(pQuad->m_aPoints[V].y);

	float dx = (px - wx)/m_WorldZoom;
	float dy = (py - wy)/m_WorldZoom;
	if(dx*dx+dy*dy < 50)
		UI()->SetHotItem(pID);

	// draw selection background
	if(m_SelectedQuad == QuadIndex && m_SelectedPoints&(1<<V))
	{
		Graphics()->SetColor(0,0,0,1);
		IGraphics::CQuadItem QuadItem(px, py, 7.0f, 7.0f);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}

	enum
	{
		OP_NONE=0,
		OP_MOVEPOINT,
		OP_MOVEUV,
		OP_CONTEXT_MENU
	};

	static bool s_Moved;
	static int s_Operation = OP_NONE;

	bool IgnoreGrid;
	if(Input()->KeyPressed(KEY_LALT) || Input()->KeyPressed(KEY_RALT))
		IgnoreGrid = true;
	else
		IgnoreGrid = false;

	if(UI()->ActiveItem() == pID)
	{
		float dx = m_MouseDeltaWx;
		float dy = m_MouseDeltaWy;
		if(!s_Moved)
		{
			if(dx*dx+dy*dy > 0.5f)
				s_Moved = true;
		}

		if(s_Moved)
		{
			if(s_Operation == OP_MOVEPOINT)
			{
				if(m_GridActive && !IgnoreGrid)
				{
					for(int m = 0; m < 4; m++)
						if(m_SelectedPoints&(1<<m))
						{
							int LineDistance = GetLineDistance();

							float x = 0.0f;
							float y = 0.0f;
							if(wx >= 0)
								x = (int)((wx+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
							else
								x = (int)((wx-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
							if(wy >= 0)
								y = (int)((wy+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
							else
								y = (int)((wy-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);

							pQuad->m_aPoints[m].x = f2fx(x);
							pQuad->m_aPoints[m].y = f2fx(y);
						}
				}
				else
				{
					for(int m = 0; m < 4; m++)
						if(m_SelectedPoints&(1<<m))
						{
							pQuad->m_aPoints[m].x += f2fx(dx);
							pQuad->m_aPoints[m].y += f2fx(dy);
						}
				}
			}
			else if(s_Operation == OP_MOVEUV)
			{
				for(int m = 0; m < 4; m++)
					if(m_SelectedPoints&(1<<m))
					{
						// 0,2;1,3 - line x
						// 0,1;2,3 - line y

						pQuad->m_aTexcoords[m].x += f2fx(dx*0.001f);
						pQuad->m_aTexcoords[(m+2)%4].x += f2fx(dx*0.001f);

						pQuad->m_aTexcoords[m].y += f2fx(dy*0.001f);
						pQuad->m_aTexcoords[m^1].y += f2fx(dy*0.001f);
					}
			}
		}

		if(s_Operation == OP_CONTEXT_MENU)
		{
			if(!UI()->MouseButton(1))
			{
				static int s_PointPopupID = 0;
				UiInvokePopupMenu(&s_PointPopupID, 0, UI()->MouseX(), UI()->MouseY(), 120, 150, PopupPoint);
				UI()->SetActiveItem(0);
			}
		}
		else
		{
			if(!UI()->MouseButton(0))
			{
				if(!s_Moved)
				{
					if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
						m_SelectedPoints ^= 1<<V;
					else
						m_SelectedPoints = 1<<V;
				}
				m_LockMouse = false;
				UI()->SetActiveItem(0);
			}
		}

		Graphics()->SetColor(1,1,1,1);
	}
	else if(UI()->HotItem() == pID)
	{
		ms_pUiGotContext = pID;

		Graphics()->SetColor(1,1,1,1);
		m_pTooltip = "Left mouse button to move. Hold shift to move the texture. Hold alt to ignore grid.";

		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			s_Moved = false;
			if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
			{
				s_Operation = OP_MOVEUV;
				m_LockMouse = true;
			}
			else
				s_Operation = OP_MOVEPOINT;

			if(!(m_SelectedPoints&(1<<V)))
			{
				if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
					m_SelectedPoints |= 1<<V;
				else
					m_SelectedPoints = 1<<V;
				s_Moved = true;
			}

			m_SelectedQuad = QuadIndex;
		}
		else if(UI()->MouseButton(1))
		{
			s_Operation = OP_CONTEXT_MENU;
			m_SelectedQuad = QuadIndex;
			UI()->SetActiveItem(pID);
			if(!(m_SelectedPoints&(1<<V)))
			{
				if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
					m_SelectedPoints |= 1<<V;
				else
					m_SelectedPoints = 1<<V;
				s_Moved = true;
			}
		}
	}
	else
		Graphics()->SetColor(1,0,0,1);

	IGraphics::CQuadItem QuadItem(px, py, 5.0f*m_WorldZoom, 5.0f*m_WorldZoom);
	Graphics()->QuadsDraw(&QuadItem, 1);
}

void CEditor::DoQuadEnvelopes(const array<CQuad> &lQuads, int TexID)
{
	int Num = lQuads.size();
	CEnvelope **apEnvelope = new CEnvelope*[Num];
	mem_zero(apEnvelope, sizeof(CEnvelope*)*Num);
	for(int i = 0; i < Num; i++)
	{
		if((m_ShowEnvelopePreview == 1 && lQuads[i].m_PosEnv == m_SelectedEnvelope) || m_ShowEnvelopePreview == 2)
			if(lQuads[i].m_PosEnv >= 0 && lQuads[i].m_PosEnv < m_Map.m_lEnvelopes.size())
				apEnvelope[i] = m_Map.m_lEnvelopes[lQuads[i].m_PosEnv];
	}

	//Draw Lines
	Graphics()->TextureSet(-1);
	Graphics()->LinesBegin();
	Graphics()->SetColor(80.0f/255, 150.0f/255, 230.f/255, 0.5f);
	for(int j = 0; j < Num; j++)
	{
		if(!apEnvelope[j])
			continue;

		//QuadParams
		const CPoint *pPoints = lQuads[j].m_aPoints;
		for(int i = 0; i < apEnvelope[j]->m_lPoints.size()-1; i++)
		{
			float OffsetX =  fx2f(apEnvelope[j]->m_lPoints[i].m_aValues[0]);
			float OffsetY = fx2f(apEnvelope[j]->m_lPoints[i].m_aValues[1]);
			vec2 Pos0 = vec2(fx2f(pPoints[4].x)+OffsetX, fx2f(pPoints[4].y)+OffsetY);

			OffsetX = fx2f(apEnvelope[j]->m_lPoints[i+1].m_aValues[0]);
			OffsetY = fx2f(apEnvelope[j]->m_lPoints[i+1].m_aValues[1]);
			vec2 Pos1 = vec2(fx2f(pPoints[4].x)+OffsetX, fx2f(pPoints[4].y)+OffsetY);

			IGraphics::CLineItem Line = IGraphics::CLineItem(Pos0.x, Pos0.y, Pos1.x, Pos1.y);
			Graphics()->LinesDraw(&Line, 1);
		}
	}
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->LinesEnd();

	//Draw Quads
	Graphics()->TextureSet(TexID);
	Graphics()->QuadsBegin();
	
	for(int j = 0; j < Num; j++)
	{
		if(!apEnvelope[j])
			continue;

		//QuadParams
		const CPoint *pPoints = lQuads[j].m_aPoints;

		for(int i = 0; i < apEnvelope[j]->m_lPoints.size(); i++)
		{
			//Calc Env Position
			float OffsetX =  fx2f(apEnvelope[j]->m_lPoints[i].m_aValues[0]);
			float OffsetY = fx2f(apEnvelope[j]->m_lPoints[i].m_aValues[1]);
			float Rot = fx2f(apEnvelope[j]->m_lPoints[i].m_aValues[2])/360.0f*pi*2;

			//Set Colours
			float Alpha = (m_SelectedQuadEnvelope == lQuads[j].m_PosEnv && m_SelectedEnvelopePoint == i) ? 0.65f : 0.35f;
			IGraphics::CColorVertex aArray[4] = {
				IGraphics::CColorVertex(0, lQuads[j].m_aColors[0].r, lQuads[j].m_aColors[0].g, lQuads[j].m_aColors[0].b, Alpha),
				IGraphics::CColorVertex(1, lQuads[j].m_aColors[1].r, lQuads[j].m_aColors[1].g, lQuads[j].m_aColors[1].b, Alpha),
				IGraphics::CColorVertex(2, lQuads[j].m_aColors[2].r, lQuads[j].m_aColors[2].g, lQuads[j].m_aColors[2].b, Alpha),
				IGraphics::CColorVertex(3, lQuads[j].m_aColors[3].r, lQuads[j].m_aColors[3].g, lQuads[j].m_aColors[3].b, Alpha)};
			Graphics()->SetColorVertex(aArray, 4);

			//Rotation
			if(Rot != 0)
			{
				static CPoint aRotated[4];
				aRotated[0] = lQuads[j].m_aPoints[0];
				aRotated[1] = lQuads[j].m_aPoints[1];
				aRotated[2] = lQuads[j].m_aPoints[2];
				aRotated[3] = lQuads[j].m_aPoints[3];
				pPoints = aRotated;

				Rotate(&lQuads[j].m_aPoints[4], &aRotated[0], Rot);
				Rotate(&lQuads[j].m_aPoints[4], &aRotated[1], Rot);
				Rotate(&lQuads[j].m_aPoints[4], &aRotated[2], Rot);
				Rotate(&lQuads[j].m_aPoints[4], &aRotated[3], Rot);
			}

			//Set Texture Coords
			Graphics()->QuadsSetSubsetFree(
				fx2f(lQuads[j].m_aTexcoords[0].x), fx2f(lQuads[j].m_aTexcoords[0].y),
				fx2f(lQuads[j].m_aTexcoords[1].x), fx2f(lQuads[j].m_aTexcoords[1].y),
				fx2f(lQuads[j].m_aTexcoords[2].x), fx2f(lQuads[j].m_aTexcoords[2].y),
				fx2f(lQuads[j].m_aTexcoords[3].x), fx2f(lQuads[j].m_aTexcoords[3].y)
			);

			//Set Quad Coords & Draw
			IGraphics::CFreeformItem Freeform(
				fx2f(pPoints[0].x)+OffsetX, fx2f(pPoints[0].y)+OffsetY,
				fx2f(pPoints[1].x)+OffsetX, fx2f(pPoints[1].y)+OffsetY,
				fx2f(pPoints[2].x)+OffsetX, fx2f(pPoints[2].y)+OffsetY,
				fx2f(pPoints[3].x)+OffsetX, fx2f(pPoints[3].y)+OffsetY);
			Graphics()->QuadsDrawFreeform(&Freeform, 1);
		}
	}
	Graphics()->QuadsEnd();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();

	// Draw QuadPoints
	for(int j = 0; j < Num; j++)
	{
		if(!apEnvelope[j])
			continue;

		//QuadParams
		for(int i = 0; i < apEnvelope[j]->m_lPoints.size()-1; i++)
			DoQuadEnvPoint(&lQuads[j], j, i);
	}
	Graphics()->QuadsEnd();
	delete[] apEnvelope;
}

void CEditor::DoQuadEnvPoint(const CQuad *pQuad, int QIndex, int PIndex)
{
	enum
	{
		OP_NONE=0,
		OP_MOVE,
		OP_ROTATE,
	};

	// some basic values
	static float s_LastWx;
	static float s_LastWy;
	static int s_Operation = OP_NONE;
	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();
	CEnvelope *pEnvelope = m_Map.m_lEnvelopes[pQuad->m_PosEnv];
	void *pID = &pEnvelope->m_lPoints[PIndex];
	static int s_ActQIndex = -1;

	// get pivot
	float CenterX = fx2f(pQuad->m_aPoints[4].x)+fx2f(pEnvelope->m_lPoints[PIndex].m_aValues[0]);
	float CenterY = fx2f(pQuad->m_aPoints[4].y)+fx2f(pEnvelope->m_lPoints[PIndex].m_aValues[1]);

	float dx = (CenterX - wx)/m_WorldZoom;
	float dy = (CenterY - wy)/m_WorldZoom;
	if(dx*dx+dy*dy < 50.0f && UI()->ActiveItem() == 0)
	{
		UI()->SetHotItem(pID);
		s_ActQIndex = QIndex;
	}

	bool IgnoreGrid;
	if(Input()->KeyPressed(KEY_LALT) || Input()->KeyPressed(KEY_RALT))
		IgnoreGrid = true;
	else
		IgnoreGrid = false;

	if(UI()->ActiveItem() == pID && s_ActQIndex == QIndex)
	{
		if(s_Operation == OP_MOVE)
		{
			if(m_GridActive && !IgnoreGrid)
			{
				int LineDistance = GetLineDistance();

				float x = 0.0f;
				float y = 0.0f;
				if(wx >= 0)
					x = (int)((wx+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
				else
					x = (int)((wx-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
				if(wy >= 0)
					y = (int)((wy+(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);
				else
					y = (int)((wy-(LineDistance/2)*m_GridFactor)/(LineDistance*m_GridFactor)) * (LineDistance*m_GridFactor);

				pEnvelope->m_lPoints[PIndex].m_aValues[0] = f2fx(x);
				pEnvelope->m_lPoints[PIndex].m_aValues[1] = f2fx(y);
			}
			else
			{
				pEnvelope->m_lPoints[PIndex].m_aValues[0] += f2fx(wx-s_LastWx);
				pEnvelope->m_lPoints[PIndex].m_aValues[1] += f2fx(wy-s_LastWy);
			}
		}
		else if(s_Operation == OP_ROTATE)
			pEnvelope->m_lPoints[PIndex].m_aValues[2] += 10*m_MouseDeltaX;

		s_LastWx = wx;
		s_LastWy = wy;

		if(!UI()->MouseButton(0))
		{
			m_LockMouse = false;
			s_Operation = OP_NONE;
			UI()->SetActiveItem(0);
		}

		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(UI()->HotItem() == pID && s_ActQIndex == QIndex)
	{
		ms_pUiGotContext = pID;

		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		m_pTooltip = "Left mouse button to move. Hold ctrl to rotate. Hold alt to ignore grid.";

		if(UI()->MouseButton(0))
		{
			if(Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL))
			{
				m_LockMouse = true;
				s_Operation = OP_ROTATE;
			}
			else
				s_Operation = OP_MOVE;

			m_SelectedEnvelopePoint = PIndex;
			m_SelectedQuadEnvelope = pQuad->m_PosEnv;

			UI()->SetActiveItem(pID);
			if(m_SelectedQuad != QIndex)
				m_SelectedPoints = 0;
			m_SelectedQuad = QIndex;
			s_LastWx = wx;
			s_LastWy = wy;
		}
		else
		{
			m_SelectedEnvelopePoint = -1;
			m_SelectedQuadEnvelope = -1;
		}
	}
	else
		Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);

	IGraphics::CQuadItem QuadItem(CenterX, CenterY, 5.0f*m_WorldZoom, 5.0f*m_WorldZoom);
	Graphics()->QuadsDraw(&QuadItem, 1);
}

void CEditor::DoMapEditor(CUIRect View, CUIRect ToolBar)
{
	// render all good stuff
	if(!m_ShowPicker)
	{
		for(int g = 0; g < m_Map.m_lGroups.size(); g++)
		{
			if(m_Map.m_lGroups[g]->m_Visible)
				m_Map.m_lGroups[g]->Render();
			//UI()->ClipEnable(&view);
		}

		// render the game above everything else
		if(m_Map.m_pGameGroup->m_Visible && m_Map.m_pGameLayer->m_Visible)
		{
			m_Map.m_pGameGroup->MapScreen();
			m_Map.m_pGameLayer->Render();
		}

		CLayerTiles *pT = static_cast<CLayerTiles *>(GetSelectedLayerType(0, LAYERTYPE_TILES));
		if(m_ShowTileInfo && pT && pT->m_Visible && m_ZoomLevel <= 300)
		{
			GetSelectedGroup()->MapScreen();
			pT->ShowInfo();
		}
	}
	else
	{
		// fix aspect ratio of the image in the picker
		float Max = min(View.w, View.h);
		View.w = View.h = Max;
	}

	static void *s_pEditorID = (void *)&s_pEditorID;
	int Inside = UI()->MouseInside(&View);

	// fetch mouse position
	float wx = UI()->MouseWorldX();
	float wy = UI()->MouseWorldY();
	float mx = UI()->MouseX();
	float my = UI()->MouseY();

	static float s_StartWx = 0;
	static float s_StartWy = 0;

	enum
	{
		OP_NONE=0,
		OP_BRUSH_GRAB,
		OP_BRUSH_DRAW,
		OP_BRUSH_PAINT,
		OP_PAN_WORLD,
		OP_PAN_EDITOR,
	};

	// remap the screen so it can display the whole tileset
	if(m_ShowPicker)
	{
		CUIRect Screen = *UI()->Screen();
		float Size = 32.0*16.0f;
		float w = Size*(Screen.w/View.w);
		float h = Size*(Screen.h/View.h);
		float x = -(View.x/Screen.w)*w;
		float y = -(View.y/Screen.h)*h;
		wx = x+w*mx/Screen.w;
		wy = y+h*my/Screen.h;
		Graphics()->MapScreen(x, y, x+w, y+h);
		CLayerTiles *t = (CLayerTiles *)GetSelectedLayerType(0, LAYERTYPE_TILES);
		if(t)
		{
			m_TilesetPicker.m_Image = t->m_Image;
			m_TilesetPicker.m_TexID = t->m_TexID;
			m_TilesetPicker.Render();
			if(m_ShowTileInfo)
				m_TilesetPicker.ShowInfo();
		}
	}

	static int s_Operation = OP_NONE;

	// draw layer borders
	CLayer *pEditLayers[16];
	int NumEditLayers = 0;
	NumEditLayers = 0;

	if(m_ShowPicker)
	{
		pEditLayers[0] = &m_TilesetPicker;
		NumEditLayers++;
	}
	else
	{
		pEditLayers[0] = GetSelectedLayer(0);
		if(pEditLayers[0])
			NumEditLayers++;

		CLayerGroup *g = GetSelectedGroup();
		if(g)
		{
			g->MapScreen();

			RenderGrid(g);

			for(int i = 0; i < NumEditLayers; i++)
			{
				if(pEditLayers[i]->m_Type != LAYERTYPE_TILES)
					continue;

				float w, h;
				pEditLayers[i]->GetSize(&w, &h);

				IGraphics::CLineItem Array[4] = {
					IGraphics::CLineItem(0, 0, w, 0),
					IGraphics::CLineItem(w, 0, w, h),
					IGraphics::CLineItem(w, h, 0, h),
					IGraphics::CLineItem(0, h, 0, 0)};
				Graphics()->TextureSet(-1);
				Graphics()->LinesBegin();
				Graphics()->LinesDraw(Array, 4);
				Graphics()->LinesEnd();
			}
		}
	}

	if(Inside)
	{
		UI()->SetHotItem(s_pEditorID);

		// do global operations like pan and zoom
		if(UI()->ActiveItem() == 0 && (UI()->MouseButton(0) || UI()->MouseButton(2)))
		{
			s_StartWx = wx;
			s_StartWy = wy;

			if(Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL) || UI()->MouseButton(2))
			{
				if(Input()->KeyPressed(KEY_LSHIFT))
					s_Operation = OP_PAN_EDITOR;
				else
					s_Operation = OP_PAN_WORLD;
				UI()->SetActiveItem(s_pEditorID);
			}
		}

		// brush editing
		if(UI()->HotItem() == s_pEditorID)
		{
			if(m_Brush.IsEmpty())
				m_pTooltip = "Use left mouse button to drag and create a brush.";
			else
				m_pTooltip = "Use left mouse button to paint with the brush. Right button clears the brush.";

			if(UI()->ActiveItem() == s_pEditorID)
			{
				CUIRect r;
				r.x = s_StartWx;
				r.y = s_StartWy;
				r.w = wx-s_StartWx;
				r.h = wy-s_StartWy;
				if(r.w < 0)
				{
					r.x += r.w;
					r.w = -r.w;
				}

				if(r.h < 0)
				{
					r.y += r.h;
					r.h = -r.h;
				}

				if(s_Operation == OP_BRUSH_DRAW)
				{
					if(!m_Brush.IsEmpty())
					{
						// draw with brush
						for(int k = 0; k < NumEditLayers; k++)
						{
							if(pEditLayers[k]->m_Type == m_Brush.m_lLayers[0]->m_Type)
								pEditLayers[k]->BrushDraw(m_Brush.m_lLayers[0], wx, wy);
						}
					}
				}
				else if(s_Operation == OP_BRUSH_GRAB)
				{
					if(!UI()->MouseButton(0))
					{
						// grab brush
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf),"grabbing %f %f %f %f", r.x, r.y, r.w, r.h);
						Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);

						// TODO: do all layers
						int Grabs = 0;
						for(int k = 0; k < NumEditLayers; k++)
							Grabs += pEditLayers[k]->BrushGrab(&m_Brush, r);
						if(Grabs == 0)
							m_Brush.Clear();
					}
					else
					{
						//editor.map.groups[selected_group]->mapscreen();
						for(int k = 0; k < NumEditLayers; k++)
							pEditLayers[k]->BrushSelecting(r);
						Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
					}
				}
				else if(s_Operation == OP_BRUSH_PAINT)
				{
					if(!UI()->MouseButton(0))
					{
						for(int k = 0; k < NumEditLayers; k++)
							pEditLayers[k]->FillSelection(m_Brush.IsEmpty(), m_Brush.m_lLayers[0], r);
					}
					else
					{
						//editor.map.groups[selected_group]->mapscreen();
						for(int k = 0; k < NumEditLayers; k++)
							pEditLayers[k]->BrushSelecting(r);
						Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
					}
				}
			}
			else
			{
				if(UI()->MouseButton(1))
					m_Brush.Clear();

				if(UI()->MouseButton(0) && s_Operation == OP_NONE)
				{
					UI()->SetActiveItem(s_pEditorID);

					if(m_Brush.IsEmpty())
						s_Operation = OP_BRUSH_GRAB;
					else
					{
						s_Operation = OP_BRUSH_DRAW;
						for(int k = 0; k < NumEditLayers; k++)
						{
							if(pEditLayers[k]->m_Type == m_Brush.m_lLayers[0]->m_Type)
								pEditLayers[k]->BrushPlace(m_Brush.m_lLayers[0], wx, wy);
						}

					}

					CLayerTiles *pLayer = (CLayerTiles*)GetSelectedLayerType(0, LAYERTYPE_TILES);
					if((Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT)) && pLayer)
						s_Operation = OP_BRUSH_PAINT;
				}

				if(!m_Brush.IsEmpty())
				{
					m_Brush.m_OffsetX = -(int)wx;
					m_Brush.m_OffsetY = -(int)wy;
					for(int i = 0; i < m_Brush.m_lLayers.size(); i++)
					{
						if(m_Brush.m_lLayers[i]->m_Type == LAYERTYPE_TILES)
						{
							m_Brush.m_OffsetX = -(int)(wx/32.0f)*32;
							m_Brush.m_OffsetY = -(int)(wy/32.0f)*32;
							break;
						}
					}

					CLayerGroup *g = GetSelectedGroup();
					if(g)
					{
						m_Brush.m_OffsetX += g->m_OffsetX;
						m_Brush.m_OffsetY += g->m_OffsetY;
						m_Brush.m_ParallaxX = g->m_ParallaxX;
						m_Brush.m_ParallaxY = g->m_ParallaxY;
						m_Brush.Render();
						float w, h;
						m_Brush.GetSize(&w, &h);

						IGraphics::CLineItem Array[4] = {
							IGraphics::CLineItem(0, 0, w, 0),
							IGraphics::CLineItem(w, 0, w, h),
							IGraphics::CLineItem(w, h, 0, h),
							IGraphics::CLineItem(0, h, 0, 0)};
						Graphics()->TextureSet(-1);
						Graphics()->LinesBegin();
						Graphics()->LinesDraw(Array, 4);
						Graphics()->LinesEnd();
					}
				}
			}
		}

		// quad editing
		{
			if(!m_ShowPicker && m_Brush.IsEmpty())
			{
				// fetch layers
				CLayerGroup *g = GetSelectedGroup();
				if(g)
					g->MapScreen();

				for(int k = 0; k < NumEditLayers; k++)
				{
					if(pEditLayers[k]->m_Type == LAYERTYPE_QUADS)
					{
						CLayerQuads *pLayer = (CLayerQuads *)pEditLayers[k];

						if(!m_ShowEnvelopePreview)
							m_ShowEnvelopePreview = 2;

						Graphics()->TextureSet(-1);
						Graphics()->QuadsBegin();
						for(int i = 0; i < pLayer->m_lQuads.size(); i++)
						{
							for(int v = 0; v < 4; v++)
								DoQuadPoint(&pLayer->m_lQuads[i], i, v);

							DoQuad(&pLayer->m_lQuads[i], i);
						}
						Graphics()->QuadsEnd();
					}
				}

				Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
			}

			// do panning
			if(UI()->ActiveItem() == s_pEditorID)
			{
				if(s_Operation == OP_PAN_WORLD)
				{
					m_WorldOffsetX -= m_MouseDeltaX*m_WorldZoom;
					m_WorldOffsetY -= m_MouseDeltaY*m_WorldZoom;
				}
				else if(s_Operation == OP_PAN_EDITOR)
				{
					m_EditorOffsetX -= m_MouseDeltaX*m_WorldZoom;
					m_EditorOffsetY -= m_MouseDeltaY*m_WorldZoom;
				}

				// release mouse
				if(!UI()->MouseButton(0))
				{
					s_Operation = OP_NONE;
					UI()->SetActiveItem(0);
				}
			}
		}
	}
	else if(UI()->ActiveItem() == s_pEditorID)
	{
		// release mouse
		if(!UI()->MouseButton(0))
		{
			s_Operation = OP_NONE;
			UI()->SetActiveItem(0);
		}
	}

	if(!m_ShowPicker && GetSelectedGroup() && GetSelectedGroup()->m_UseClipping)
	{
		CLayerGroup *g = m_Map.m_pGameGroup;
		g->MapScreen();

		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();

			CUIRect r;
			r.x = GetSelectedGroup()->m_ClipX;
			r.y = GetSelectedGroup()->m_ClipY;
			r.w = GetSelectedGroup()->m_ClipW;
			r.h = GetSelectedGroup()->m_ClipH;

			IGraphics::CLineItem Array[4] = {
				IGraphics::CLineItem(r.x, r.y, r.x+r.w, r.y),
				IGraphics::CLineItem(r.x+r.w, r.y, r.x+r.w, r.y+r.h),
				IGraphics::CLineItem(r.x+r.w, r.y+r.h, r.x, r.y+r.h),
				IGraphics::CLineItem(r.x, r.y+r.h, r.x, r.y)};
			Graphics()->SetColor(1,0,0,1);
			Graphics()->LinesDraw(Array, 4);

		Graphics()->LinesEnd();
	}

	// render screen sizes
	if(m_ProofBorders)
	{
		CLayerGroup *g = m_Map.m_pGameGroup;
		g->MapScreen();

		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();

		float aLastPoints[4];
		float Start = 1.0f; //9.0f/16.0f;
		float End = 16.0f/9.0f;
		const int NumSteps = 20;
		for(int i = 0; i <= NumSteps; i++)
		{
			float aPoints[4];
			float Aspect = Start + (End-Start)*(i/(float)NumSteps);

			RenderTools()->MapscreenToWorld(
				m_WorldOffsetX, m_WorldOffsetY,
				1.0f, 1.0f, 0.0f, 0.0f, Aspect, 1.0f, aPoints);

			if(i == 0)
			{
				IGraphics::CLineItem Array[2] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aPoints[2], aPoints[1]),
					IGraphics::CLineItem(aPoints[0], aPoints[3], aPoints[2], aPoints[3])};
				Graphics()->LinesDraw(Array, 2);
			}

			if(i != 0)
			{
				IGraphics::CLineItem Array[4] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aLastPoints[0], aLastPoints[1]),
					IGraphics::CLineItem(aPoints[2], aPoints[1], aLastPoints[2], aLastPoints[1]),
					IGraphics::CLineItem(aPoints[0], aPoints[3], aLastPoints[0], aLastPoints[3]),
					IGraphics::CLineItem(aPoints[2], aPoints[3], aLastPoints[2], aLastPoints[3])};
				Graphics()->LinesDraw(Array, 4);
			}

			if(i == NumSteps)
			{
				IGraphics::CLineItem Array[2] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aPoints[0], aPoints[3]),
					IGraphics::CLineItem(aPoints[2], aPoints[1], aPoints[2], aPoints[3])};
				Graphics()->LinesDraw(Array, 2);
			}

			mem_copy(aLastPoints, aPoints, sizeof(aPoints));
		}

		if(1)
		{
			Graphics()->SetColor(1,0,0,1);
			for(int i = 0; i < 2; i++)
			{
				float aPoints[4];
				float aAspects[] = {4.0f/3.0f, 16.0f/10.0f, 5.0f/4.0f, 16.0f/9.0f};
				float Aspect = aAspects[i];

				RenderTools()->MapscreenToWorld(
					m_WorldOffsetX, m_WorldOffsetY,
					1.0f, 1.0f, 0.0f, 0.0f, Aspect, 1.0f, aPoints);

				CUIRect r;
				r.x = aPoints[0];
				r.y = aPoints[1];
				r.w = aPoints[2]-aPoints[0];
				r.h = aPoints[3]-aPoints[1];

				IGraphics::CLineItem Array[4] = {
					IGraphics::CLineItem(r.x, r.y, r.x+r.w, r.y),
					IGraphics::CLineItem(r.x+r.w, r.y, r.x+r.w, r.y+r.h),
					IGraphics::CLineItem(r.x+r.w, r.y+r.h, r.x, r.y+r.h),
					IGraphics::CLineItem(r.x, r.y+r.h, r.x, r.y)};
				Graphics()->LinesDraw(Array, 4);
				Graphics()->SetColor(0,1,0,1);
			}
		}

		Graphics()->LinesEnd();
	}

	if (!m_ShowPicker && m_ShowTileInfo && m_ShowEnvelopePreview != 0 && GetSelectedLayer(0) && GetSelectedLayer(0)->m_Type == LAYERTYPE_QUADS)
	{
		GetSelectedGroup()->MapScreen();

		CLayerQuads *pLayer = (CLayerQuads*)GetSelectedLayer(0);
		int TexID = -1;
		if(pLayer->m_Image >= 0 && pLayer->m_Image < m_Map.m_lImages.size())
			TexID = m_Map.m_lImages[pLayer->m_Image]->m_TexID;

		DoQuadEnvelopes(pLayer->m_lQuads, TexID);
		m_ShowEnvelopePreview = 0;
    }

	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
	//UI()->ClipDisable();
}


int CEditor::DoProperties(CUIRect *pToolBox, CProperty *pProps, int *pIDs, int *pNewVal)
{
	int Change = -1;

	for(int i = 0; pProps[i].m_pName; i++)
	{
		CUIRect Slot;
		pToolBox->HSplitTop(13.0f, &Slot, pToolBox);
		CUIRect Label, Shifter;
		Slot.VSplitMid(&Label, &Shifter);
		Shifter.HMargin(1.0f, &Shifter);
		UI()->DoLabel(&Label, pProps[i].m_pName, 10.0f, -1, -1);

		if(pProps[i].m_Type == PROPTYPE_INT_STEP)
		{
			CUIRect Inc, Dec;
			char aBuf[64];

			Shifter.VSplitRight(10.0f, &Shifter, &Inc);
			Shifter.VSplitLeft(10.0f, &Dec, &Shifter);
			str_format(aBuf, sizeof(aBuf),"%d", pProps[i].m_Value);
			RenderTools()->DrawUIRect(&Shifter, vec4(1,1,1,0.5f), 0, 0.0f);
			UI()->DoLabel(&Shifter, aBuf, 10.0f, 0, -1);

			if(DoButton_ButtonDec(&pIDs[i], 0, 0, &Dec, 0, "Decrease"))
			{
				*pNewVal = pProps[i].m_Value-1;
				Change = i;
			}
			if(DoButton_ButtonInc(((char *)&pIDs[i])+1, 0, 0, &Inc, 0, "Increase"))
			{
				*pNewVal = pProps[i].m_Value+1;
				Change = i;
			}
		}
		else if(pProps[i].m_Type == PROPTYPE_BOOL)
		{
			CUIRect No, Yes;
			Shifter.VSplitMid(&No, &Yes);
			if(DoButton_ButtonDec(&pIDs[i], "No", !pProps[i].m_Value, &No, 0, ""))
			{
				*pNewVal = 0;
				Change = i;
			}
			if(DoButton_ButtonInc(((char *)&pIDs[i])+1, "Yes", pProps[i].m_Value, &Yes, 0, ""))
			{
				*pNewVal = 1;
				Change = i;
			}
		}
		else if(pProps[i].m_Type == PROPTYPE_INT_SCROLL)
		{
			int NewValue = UiDoValueSelector(&pIDs[i], &Shifter, "", pProps[i].m_Value, pProps[i].m_Min, pProps[i].m_Max, 1, 1.0f, "Use left mouse button to drag and change the value. Hold shift to be more precise.");
			if(NewValue != pProps[i].m_Value)
			{
				*pNewVal = NewValue;
				Change = i;
			}
		}
		else if(pProps[i].m_Type == PROPTYPE_COLOR)
		{
			static const char *s_paTexts[4] = {"R", "G", "B", "A"};
			static int s_aShift[] = {24, 16, 8, 0};
			int NewColor = 0;

			for(int c = 0; c < 4; c++)
			{
				int v = (pProps[i].m_Value >> s_aShift[c])&0xff;
				NewColor |= UiDoValueSelector(((char *)&pIDs[i])+c, &Shifter, s_paTexts[c], v, 0, 255, 1, 1.0f, "Use left mouse button to drag and change the color value. Hold shift to be more precise.")<<s_aShift[c];

				if(c != 3)
				{
					pToolBox->HSplitTop(13.0f, &Slot, pToolBox);
					Slot.VSplitMid(0, &Shifter);
					Shifter.HMargin(1.0f, &Shifter);
				}
			}

			if(NewColor != pProps[i].m_Value)
			{
				*pNewVal = NewColor;
				Change = i;
			}
		}
		else if(pProps[i].m_Type == PROPTYPE_IMAGE)
		{
			char aBuf[64];
			if(pProps[i].m_Value < 0)
				str_copy(aBuf, "None", sizeof(aBuf));
			else
				str_format(aBuf, sizeof(aBuf),"%s", m_Map.m_lImages[pProps[i].m_Value]->m_aName);

			if(DoButton_Editor(&pIDs[i], aBuf, 0, &Shifter, 0, 0))
				PopupSelectImageInvoke(pProps[i].m_Value, UI()->MouseX(), UI()->MouseY());

			int r = PopupSelectImageResult();
			if(r >= -1)
			{
				*pNewVal = r;
				Change = i;
			}
		}
		else if(pProps[i].m_Type == PROPTYPE_SHIFT)
		{
			CUIRect Left, Right, Up, Down;
			Shifter.VSplitMid(&Left, &Up);
			Left.VSplitRight(1.0f, &Left, 0);
			Up.VSplitLeft(1.0f, 0, &Up);
			Left.VSplitLeft(10.0f, &Left, &Shifter);
			Shifter.VSplitRight(10.0f, &Shifter, &Right);
			RenderTools()->DrawUIRect(&Shifter, vec4(1,1,1,0.5f), 0, 0.0f);
			UI()->DoLabel(&Shifter, "X", 10.0f, 0, -1);
			Up.VSplitLeft(10.0f, &Up, &Shifter);
			Shifter.VSplitRight(10.0f, &Shifter, &Down);
			RenderTools()->DrawUIRect(&Shifter, vec4(1,1,1,0.5f), 0, 0.0f);
			UI()->DoLabel(&Shifter, "Y", 10.0f, 0, -1);
			if(DoButton_ButtonDec(&pIDs[i], "-", 0, &Left, 0, "Left"))
			{
				*pNewVal = 1;
				Change = i;
			}
			if(DoButton_ButtonInc(((char *)&pIDs[i])+3, "+", 0, &Right, 0, "Right"))
			{
				*pNewVal = 2;
				Change = i;
			}
			if(DoButton_ButtonDec(((char *)&pIDs[i])+1, "-", 0, &Up, 0, "Up"))
			{
				*pNewVal = 4;
				Change = i;
			}
			if(DoButton_ButtonInc(((char *)&pIDs[i])+2, "+", 0, &Down, 0, "Down"))
			{
				*pNewVal = 8;
				Change = i;
			}
		}
	}

	return Change;
}

void CEditor::RenderLayers(CUIRect ToolBox, CUIRect ToolBar, CUIRect View)
{
	CUIRect LayersBox = ToolBox;

	if(!m_GuiActive)
		return;

	CUIRect Slot, Button;
	char aBuf[64];

	float LayersHeight = 12.0f;	 // Height of AddGroup button
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;

	for(int g = 0; g < m_Map.m_lGroups.size(); g++)
	{
		// Each group is 19.0f
		// Each layer is 14.0f
		LayersHeight += 19.0f;
		if(!m_Map.m_lGroups[g]->m_Collapse)
			LayersHeight += m_Map.m_lGroups[g]->m_lLayers.size() * 14.0f;
	}

	float ScrollDifference = LayersHeight - LayersBox.h;

	if(LayersHeight > LayersBox.h)	// Do we even need a scrollbar?
	{
		CUIRect Scroll;
		LayersBox.VSplitRight(15.0f, &LayersBox, &Scroll);
		LayersBox.VSplitRight(3.0f, &LayersBox, 0);	// extra spacing
		Scroll.HMargin(5.0f, &Scroll);
		s_ScrollValue = UiDoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

		if(UI()->MouseInside(&Scroll) || UI()->MouseInside(&LayersBox))
		{
			int ScrollNum = (int)((LayersHeight-LayersBox.h)/15.0f)+1;
			if(ScrollNum > 0)
			{
				if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
					s_ScrollValue = clamp(s_ScrollValue - 1.0f/ScrollNum, 0.0f, 1.0f);
				if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
					s_ScrollValue = clamp(s_ScrollValue + 1.0f/ScrollNum, 0.0f, 1.0f);
			}
		}
	}

	float LayerStartAt = ScrollDifference * s_ScrollValue;
	if(LayerStartAt < 0.0f)
		LayerStartAt = 0.0f;

	float LayerStopAt = LayersHeight - ScrollDifference * (1 - s_ScrollValue);
	float LayerCur = 0;

	// render layers
	{
		for(int g = 0; g < m_Map.m_lGroups.size(); g++)
		{
			if(LayerCur > LayerStopAt)
				break;
			else if(LayerCur + m_Map.m_lGroups[g]->m_lLayers.size() * 14.0f + 19.0f < LayerStartAt)
			{
				LayerCur += m_Map.m_lGroups[g]->m_lLayers.size() * 14.0f + 19.0f;
				continue;
			}

			CUIRect VisibleToggle, SaveCheck;
			if(LayerCur >= LayerStartAt)
			{
				LayersBox.HSplitTop(12.0f, &Slot, &LayersBox);
				Slot.VSplitLeft(12, &VisibleToggle, &Slot);
				if(DoButton_Ex(&m_Map.m_lGroups[g]->m_Visible, m_Map.m_lGroups[g]->m_Visible?"V":"H", m_Map.m_lGroups[g]->m_Collapse ? 1 : 0, &VisibleToggle, 0, "Toggle group visibility", CUI::CORNER_L))
					m_Map.m_lGroups[g]->m_Visible = !m_Map.m_lGroups[g]->m_Visible;

				Slot.VSplitRight(12.0f, &Slot, &SaveCheck);
				if(DoButton_Ex(&m_Map.m_lGroups[g]->m_SaveToMap, "S", m_Map.m_lGroups[g]->m_SaveToMap, &SaveCheck, 0, "Enable/disable group for saving", CUI::CORNER_R))
					if(!m_Map.m_lGroups[g]->m_GameGroup)
						m_Map.m_lGroups[g]->m_SaveToMap = !m_Map.m_lGroups[g]->m_SaveToMap;

				str_format(aBuf, sizeof(aBuf),"#%d %s", g, m_Map.m_lGroups[g]->m_aName);
				float FontSize = 10.0f;
				while(TextRender()->TextWidth(0, FontSize, aBuf, -1) > Slot.w)
					FontSize--;
				if(int Result = DoButton_Ex(&m_Map.m_lGroups[g], aBuf, g==m_SelectedGroup, &Slot,
					BUTTON_CONTEXT, m_Map.m_lGroups[g]->m_Collapse ? "Select group. Double click to expand." : "Select group. Double click to collapse.", 0, FontSize))
				{
					m_SelectedGroup = g;
					m_SelectedLayer = 0;

					static int s_GroupPopupId = 0;
					if(Result == 2)
						UiInvokePopupMenu(&s_GroupPopupId, 0, UI()->MouseX(), UI()->MouseY(), 145, 220, PopupGroup);

					if(m_Map.m_lGroups[g]->m_lLayers.size() && Input()->MouseDoubleClick())
						m_Map.m_lGroups[g]->m_Collapse ^= 1;
				}
				LayersBox.HSplitTop(2.0f, &Slot, &LayersBox);
			}
			LayerCur += 14.0f;

			for(int i = 0; i < m_Map.m_lGroups[g]->m_lLayers.size(); i++)
			{
				if(LayerCur > LayerStopAt)
					break;
				else if(LayerCur < LayerStartAt)
				{
					LayerCur += 14.0f;
					continue;
				}

				if(m_Map.m_lGroups[g]->m_Collapse)
					continue;

				//visible
				LayersBox.HSplitTop(12.0f, &Slot, &LayersBox);
				Slot.VSplitLeft(12.0f, 0, &Button);
				Button.VSplitLeft(15, &VisibleToggle, &Button);

				if(DoButton_Ex(&m_Map.m_lGroups[g]->m_lLayers[i]->m_Visible, m_Map.m_lGroups[g]->m_lLayers[i]->m_Visible?"V":"H", 0, &VisibleToggle, 0, "Toggle layer visibility", CUI::CORNER_L))
					m_Map.m_lGroups[g]->m_lLayers[i]->m_Visible = !m_Map.m_lGroups[g]->m_lLayers[i]->m_Visible;

				Button.VSplitRight(12.0f, &Button, &SaveCheck);
				if(DoButton_Ex(&m_Map.m_lGroups[g]->m_lLayers[i]->m_SaveToMap, "S", m_Map.m_lGroups[g]->m_lLayers[i]->m_SaveToMap, &SaveCheck, 0, "Enable/disable layer for saving", CUI::CORNER_R))
					if(m_Map.m_lGroups[g]->m_lLayers[i] != m_Map.m_pGameLayer)
						m_Map.m_lGroups[g]->m_lLayers[i]->m_SaveToMap = !m_Map.m_lGroups[g]->m_lLayers[i]->m_SaveToMap;

				if(m_Map.m_lGroups[g]->m_lLayers[i]->m_aName[0])
					str_format(aBuf, sizeof(aBuf), "%s", m_Map.m_lGroups[g]->m_lLayers[i]->m_aName);
				else if(m_Map.m_lGroups[g]->m_lLayers[i]->m_Type == LAYERTYPE_TILES)
					str_copy(aBuf, "Tiles", sizeof(aBuf));
				else
					str_copy(aBuf, "Quads", sizeof(aBuf));

				float FontSize = 10.0f;
				while(TextRender()->TextWidth(0, FontSize, aBuf, -1) > Button.w)
					FontSize--;
				if(int Result = DoButton_Ex(m_Map.m_lGroups[g]->m_lLayers[i], aBuf, g==m_SelectedGroup&&i==m_SelectedLayer, &Button,
					BUTTON_CONTEXT, "Select layer.", 0, FontSize))
				{
					m_SelectedLayer = i;
					m_SelectedGroup = g;
					static int s_LayerPopupID = 0;
					if(Result == 2)
						UiInvokePopupMenu(&s_LayerPopupID, 0, UI()->MouseX(), UI()->MouseY(), 120, 245, PopupLayer);
				}

				LayerCur += 14.0f;
				LayersBox.HSplitTop(2.0f, &Slot, &LayersBox);
			}
			if(LayerCur > LayerStartAt && LayerCur < LayerStopAt)
				LayersBox.HSplitTop(5.0f, &Slot, &LayersBox);
			LayerCur += 5.0f;
		}
	}

	if(LayerCur <= LayerStopAt)
	{
		LayersBox.HSplitTop(12.0f, &Slot, &LayersBox);

		static int s_NewGroupButton = 0;
		if(DoButton_Editor(&s_NewGroupButton, "Add group", 0, &Slot, 0, "Adds a new group"))
		{
			m_Map.NewGroup();
			m_SelectedGroup = m_Map.m_lGroups.size()-1;
		}
	}
}

void CEditor::ReplaceImage(const char *pFileName, int StorageType, void *pUser)
{
	CEditor *pEditor = (CEditor *)pUser;
	CEditorImage ImgInfo(pEditor);
	if(!pEditor->Graphics()->LoadPNG(&ImgInfo, pFileName, StorageType))
		return;

	CEditorImage *pImg = pEditor->m_Map.m_lImages[pEditor->m_SelectedImage];
	int External = pImg->m_External;
	pEditor->Graphics()->UnloadTexture(pImg->m_TexID);
	if(pImg->m_pData)
	{
		mem_free(pImg->m_pData);
		pImg->m_pData = 0;
	}
	*pImg = ImgInfo;
	pImg->m_External = External;
	pEditor->ExtractName(pFileName, pImg->m_aName, sizeof(pImg->m_aName));
	pImg->m_AutoMapper.Load(pImg->m_aName);
	pImg->m_TexID = pEditor->Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format, ImgInfo.m_pData, CImageInfo::FORMAT_AUTO, 0);
	ImgInfo.m_pData = 0;
	pEditor->SortImages();
	for(int i = 0; i < pEditor->m_Map.m_lImages.size(); ++i)
	{
		if(!str_comp(pEditor->m_Map.m_lImages[i]->m_aName, pImg->m_aName))
			pEditor->m_SelectedImage = i;
	}
	pEditor->m_Dialog = DIALOG_NONE;
}

void CEditor::AddImage(const char *pFileName, int StorageType, void *pUser)
{
	CEditor *pEditor = (CEditor *)pUser;
	CEditorImage ImgInfo(pEditor);
	if(!pEditor->Graphics()->LoadPNG(&ImgInfo, pFileName, StorageType))
		return;

	// check if we have that image already
	char aBuf[128];
	ExtractName(pFileName, aBuf, sizeof(aBuf));
	for(int i = 0; i < pEditor->m_Map.m_lImages.size(); ++i)
	{
		if(!str_comp(pEditor->m_Map.m_lImages[i]->m_aName, aBuf))
			return;
	}

	CEditorImage *pImg = new CEditorImage(pEditor);
	*pImg = ImgInfo;
	pImg->m_TexID = pEditor->Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format, ImgInfo.m_pData, CImageInfo::FORMAT_AUTO, 0);
	ImgInfo.m_pData = 0;
	pImg->m_External = 1;	// external by default
	str_copy(pImg->m_aName, aBuf, sizeof(pImg->m_aName));
	pImg->m_AutoMapper.Load(pImg->m_aName);
	pEditor->m_Map.m_lImages.add(pImg);
	pEditor->SortImages();
	if(pEditor->m_SelectedImage > -1 && pEditor->m_SelectedImage < pEditor->m_Map.m_lImages.size())
	{
		for(int i = 0; i <= pEditor->m_SelectedImage; ++i)
			if(!str_comp(pEditor->m_Map.m_lImages[i]->m_aName, aBuf))
			{
				pEditor->m_SelectedImage++;
				break;
			}
	}
	pEditor->m_Dialog = DIALOG_NONE;
}


static int gs_ModifyIndexDeletedIndex;
static void ModifyIndexDeleted(int *pIndex)
{
	if(*pIndex == gs_ModifyIndexDeletedIndex)
		*pIndex = -1;
	else if(*pIndex > gs_ModifyIndexDeletedIndex)
		*pIndex = *pIndex - 1;
}

int CEditor::PopupImage(CEditor *pEditor, CUIRect View)
{
	static int s_ReplaceButton = 0;
	static int s_RemoveButton = 0;

	CUIRect Slot;
	View.HSplitTop(2.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	CEditorImage *pImg = pEditor->m_Map.m_lImages[pEditor->m_SelectedImage];

	static int s_ExternalButton = 0;
	if(pImg->m_External)
	{
		if(pEditor->DoButton_MenuItem(&s_ExternalButton, "Embed", 0, &Slot, 0, "Embeds the image into the map file."))
		{
			pImg->m_External = 0;
			return 1;
		}
	}
	else
	{
		if(pEditor->DoButton_MenuItem(&s_ExternalButton, "Make external", 0, &Slot, 0, "Removes the image from the map file."))
		{
			pImg->m_External = 1;
			return 1;
		}
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_ReplaceButton, "Replace", 0, &Slot, 0, "Replaces the image with a new one"))
	{
		pEditor->InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_IMG, "Replace Image", "Replace", "mapres", "", ReplaceImage, pEditor);
		return 1;
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_RemoveButton, "Remove", 0, &Slot, 0, "Removes the image from the map"))
	{
		delete pImg;
		pEditor->m_Map.m_lImages.remove_index(pEditor->m_SelectedImage);
		gs_ModifyIndexDeletedIndex = pEditor->m_SelectedImage;
		pEditor->m_Map.ModifyImageIndex(ModifyIndexDeleted);
		return 1;
	}

	return 0;
}

static int CompareImageName(const void *pObject1, const void *pObject2)
{
	CEditorImage *pImage1 = *(CEditorImage**)pObject1;
	CEditorImage *pImage2 = *(CEditorImage**)pObject2;

	return str_comp(pImage1->m_aName, pImage2->m_aName);
}

static int *gs_pSortedIndex = 0;
static void ModifySortedIndex(int *pIndex)
{
	if(*pIndex > -1)
		*pIndex = gs_pSortedIndex[*pIndex];
}

void CEditor::SortImages()
{
	bool Sorted = true;
	for(int i = 1; i < m_Map.m_lImages.size(); i++)
		if( str_comp(m_Map.m_lImages[i]->m_aName, m_Map.m_lImages[i-1]->m_aName) < 0 )
		{
			Sorted = false;
			break;
		}

	if(!Sorted)
	{
		array<CEditorImage*> lTemp = array<CEditorImage*>(m_Map.m_lImages);
		gs_pSortedIndex = new int[lTemp.size()];

		qsort(m_Map.m_lImages.base_ptr(), m_Map.m_lImages.size(), sizeof(CEditorImage*), CompareImageName);

		for(int OldIndex = 0; OldIndex < lTemp.size(); OldIndex++)
			for(int NewIndex = 0; NewIndex < m_Map.m_lImages.size(); NewIndex++)
				if(lTemp[OldIndex] == m_Map.m_lImages[NewIndex])
					gs_pSortedIndex[OldIndex] = NewIndex;

		m_Map.ModifyImageIndex(ModifySortedIndex);

		delete [] gs_pSortedIndex;
		gs_pSortedIndex = 0;
	}
}


void CEditor::RenderImages(CUIRect ToolBox, CUIRect ToolBar, CUIRect View)
{
	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;
	float ImagesHeight = 30.0f + 14.0f * m_Map.m_lImages.size() + 27.0f;
	float ScrollDifference = ImagesHeight - ToolBox.h;

	if(ImagesHeight > ToolBox.h)	// Do we even need a scrollbar?
	{
		CUIRect Scroll;
		ToolBox.VSplitRight(15.0f, &ToolBox, &Scroll);
		ToolBox.VSplitRight(3.0f, &ToolBox, 0);	// extra spacing
		Scroll.HMargin(5.0f, &Scroll);
		s_ScrollValue = UiDoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

		if(UI()->MouseInside(&Scroll) || UI()->MouseInside(&ToolBox))
		{
			int ScrollNum = (int)((ImagesHeight-ToolBox.h)/14.0f)+1;
			if(ScrollNum > 0)
			{
				if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
					s_ScrollValue = clamp(s_ScrollValue - 1.0f/ScrollNum, 0.0f, 1.0f);
				if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
					s_ScrollValue = clamp(s_ScrollValue + 1.0f/ScrollNum, 0.0f, 1.0f);
			}
		}
	}

	float ImageStartAt = ScrollDifference * s_ScrollValue;
	if(ImageStartAt < 0.0f)
		ImageStartAt = 0.0f;

	float ImageStopAt = ImagesHeight - ScrollDifference * (1 - s_ScrollValue);
	float ImageCur = 0.0f;

	for(int e = 0; e < 2; e++) // two passes, first embedded, then external
	{
		CUIRect Slot;

		if(ImageCur > ImageStopAt)
			break;
		else if(ImageCur >= ImageStartAt)
		{

			ToolBox.HSplitTop(15.0f, &Slot, &ToolBox);
			if(e == 0)
				UI()->DoLabel(&Slot, "Embedded", 12.0f, 0);
			else
				UI()->DoLabel(&Slot, "External", 12.0f, 0);
		}
		ImageCur += 15.0f;

		for(int i = 0; i < m_Map.m_lImages.size(); i++)
		{
			if((e && !m_Map.m_lImages[i]->m_External) ||
				(!e && m_Map.m_lImages[i]->m_External))
			{
				continue;
			}

			if(ImageCur > ImageStopAt)
				break;
			else if(ImageCur < ImageStartAt)
			{
				ImageCur += 14.0f;
				continue;
			}
			ImageCur += 14.0f;

			char aBuf[128];
			str_copy(aBuf, m_Map.m_lImages[i]->m_aName, sizeof(aBuf));
			ToolBox.HSplitTop(12.0f, &Slot, &ToolBox);

			if(int Result = DoButton_Editor(&m_Map.m_lImages[i], aBuf, m_SelectedImage == i, &Slot,
				BUTTON_CONTEXT, "Select image"))
			{
				m_SelectedImage = i;

				static int s_PopupImageID = 0;
				if(Result == 2)
					UiInvokePopupMenu(&s_PopupImageID, 0, UI()->MouseX(), UI()->MouseY(), 120, 80, PopupImage);
			}

			ToolBox.HSplitTop(2.0f, 0, &ToolBox);

			// render image
			if(m_SelectedImage == i)
			{
				CUIRect r;
				View.Margin(10.0f, &r);
				if(r.h < r.w)
					r.w = r.h;
				else
					r.h = r.w;
				float Max = (float)(max(m_Map.m_lImages[i]->m_Width, m_Map.m_lImages[i]->m_Height));
				r.w *= m_Map.m_lImages[i]->m_Width/Max;
				r.h *= m_Map.m_lImages[i]->m_Height/Max;
				Graphics()->TextureSet(m_Map.m_lImages[i]->m_TexID);
				Graphics()->BlendNormal();
				Graphics()->WrapClamp();
				Graphics()->QuadsBegin();
				IGraphics::CQuadItem QuadItem(r.x, r.y, r.w, r.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
				Graphics()->WrapNormal();
			}
		}

		// separator
		ToolBox.HSplitTop(5.0f, &Slot, &ToolBox);
		ImageCur += 5.0f;
		IGraphics::CLineItem LineItem(Slot.x, Slot.y+Slot.h/2, Slot.x+Slot.w, Slot.y+Slot.h/2);
		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();
		Graphics()->LinesDraw(&LineItem, 1);
		Graphics()->LinesEnd();
	}

	if(ImageCur + 27.0f > ImageStopAt)
		return;

	CUIRect Slot;
	ToolBox.HSplitTop(5.0f, &Slot, &ToolBox);

	// new image
	static int s_NewImageButton = 0;
	ToolBox.HSplitTop(12.0f, &Slot, &ToolBox);
	if(DoButton_Editor(&s_NewImageButton, "Add", 0, &Slot, 0, "Load a new image to use in the map"))
		InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_IMG, "Add Image", "Add", "mapres", "", AddImage, this);
}


static int EditorListdirCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CEditor *pEditor = (CEditor*)pUser;
	int Length = str_length(pName);
	if((pName[0] == '.' && (pName[1] == 0 ||
		(pName[1] == '.' && pName[2] == 0 && (!str_comp(pEditor->m_pFileDialogPath, "maps") || !str_comp(pEditor->m_pFileDialogPath, "mapres"))))) ||
		(!IsDir && ((pEditor->m_FileDialogFileType == CEditor::FILETYPE_MAP && (Length < 4 || str_comp(pName+Length-4, ".map"))) ||
		(pEditor->m_FileDialogFileType == CEditor::FILETYPE_IMG && (Length < 4 || str_comp(pName+Length-4, ".png"))))))
		return 0;

	CEditor::CFilelistItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	if(IsDir)
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", pName);
	else
		str_copy(Item.m_aName, pName, min(static_cast<int>(sizeof(Item.m_aName)), Length-3));
	Item.m_IsDir = IsDir != 0;
	Item.m_IsLink = false;
	Item.m_StorageType = StorageType;
	pEditor->m_FileList.add(Item);

	return 0;
}

void CEditor::AddFileDialogEntry(int Index, CUIRect *pView)
{
	m_FilesCur++;
	if(m_FilesCur-1 < m_FilesStartAt || m_FilesCur >= m_FilesStopAt)
		return;

	CUIRect Button, FileIcon;
	pView->HSplitTop(15.0f, &Button, pView);
	pView->HSplitTop(2.0f, 0, pView);
	Button.VSplitLeft(Button.h, &FileIcon, &Button);
	Button.VSplitLeft(5.0f, 0, &Button);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_FILEICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(m_FileList[Index].m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_MAP2);
	IGraphics::CQuadItem QuadItem(FileIcon.x, FileIcon.y, FileIcon.w, FileIcon.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	if(DoButton_File(&m_FileList[Index], m_FileList[Index].m_aName, m_FilesSelectedIndex == Index, &Button, 0, 0))
	{
		if(!m_FileList[Index].m_IsDir)
			str_copy(m_aFileDialogFileName, m_FileList[Index].m_aFilename, sizeof(m_aFileDialogFileName));
		else
			m_aFileDialogFileName[0] = 0;
		m_FilesSelectedIndex = Index;

		if(Input()->MouseDoubleClick())
			m_aFileDialogActivate = true;
	}
}

void CEditor::RenderFileDialog()
{
	// GUI coordsys
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
	CUIRect View = *UI()->Screen();
	float Width = View.w, Height = View.h;

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.25f), 0, 0);
	View.VMargin(150.0f, &View);
	View.HMargin(50.0f, &View);
	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.75f), CUI::CORNER_ALL, 5.0f);
	View.Margin(10.0f, &View);

	CUIRect Title, FileBox, FileBoxLabel, ButtonBar, Scroll, PathBox;
	View.HSplitTop(18.0f, &Title, &View);
	View.HSplitTop(5.0f, 0, &View); // some spacing
	View.HSplitBottom(14.0f, &View, &ButtonBar);
	View.HSplitBottom(10.0f, &View, 0); // some spacing
	View.HSplitBottom(14.0f, &View, &PathBox);
	View.HSplitBottom(5.0f, &View, 0); // some spacing
	View.HSplitBottom(14.0f, &View, &FileBox);
	FileBox.VSplitLeft(55.0f, &FileBoxLabel, &FileBox);
	View.HSplitBottom(10.0f, &View, 0); // some spacing
	View.VSplitRight(15.0f, &View, &Scroll);

	// title
	RenderTools()->DrawUIRect(&Title, vec4(1, 1, 1, 0.25f), CUI::CORNER_ALL, 4.0f);
	Title.VMargin(10.0f, &Title);
	UI()->DoLabel(&Title, m_pFileDialogTitle, 12.0f, -1, -1);

	// pathbox
	char aPath[128], aBuf[128];
	if(m_FilesSelectedIndex != -1)
		Storage()->GetCompletePath(m_FileList[m_FilesSelectedIndex].m_StorageType, m_pFileDialogPath, aPath, sizeof(aPath));
	else
		aPath[0] = 0;
	str_format(aBuf, sizeof(aBuf), "Current path: %s", aPath);
	UI()->DoLabel(&PathBox, aBuf, 10.0f, -1, -1);

	// filebox
	if(m_FileDialogStorageType == IStorage::TYPE_SAVE)
	{
		static float s_FileBoxID = 0;
		UI()->DoLabel(&FileBoxLabel, "Filename:", 10.0f, -1, -1);
		if(DoEditBox(&s_FileBoxID, &FileBox, m_aFileDialogFileName, sizeof(m_aFileDialogFileName), 10.0f, &s_FileBoxID))
		{
			// remove '/' and '\'
			for(int i = 0; m_aFileDialogFileName[i]; ++i)
				if(m_aFileDialogFileName[i] == '/' || m_aFileDialogFileName[i] == '\\')
					str_copy(&m_aFileDialogFileName[i], &m_aFileDialogFileName[i+1], (int)(sizeof(m_aFileDialogFileName))-i);
			m_FilesSelectedIndex = -1;
		}
	}

	int Num = (int)(View.h/17.0f)+1;
	static int ScrollBar = 0;
	Scroll.HMargin(5.0f, &Scroll);
	m_FileDialogScrollValue = UiDoScrollbarV(&ScrollBar, &Scroll, m_FileDialogScrollValue);

	int ScrollNum = m_FileList.size()-Num+1;
	if(ScrollNum > 0)
	{
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
			m_FileDialogScrollValue -= 3.0f/ScrollNum;
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			m_FileDialogScrollValue += 3.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	if(m_FilesSelectedIndex > -1)
	{
		for(int i = 0; i < Input()->NumEvents(); i++)
		{
			int NewIndex = -1;
			if(Input()->GetEvent(i).m_Flags&IInput::FLAG_PRESS)
			{
				if(Input()->GetEvent(i).m_Key == KEY_DOWN) NewIndex = m_FilesSelectedIndex + 1;
				if(Input()->GetEvent(i).m_Key == KEY_UP) NewIndex = m_FilesSelectedIndex - 1;
			}
			if(NewIndex > -1 && NewIndex < m_FileList.size())
			{
				//scroll
				float IndexY = View.y - m_FileDialogScrollValue*ScrollNum*17.0f + NewIndex*17.0f;
				int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+17.0f ? 1 : 0;
				if(Scroll)
				{
					if(Scroll < 0)
						m_FileDialogScrollValue = ((float)(NewIndex)+0.5f)/ScrollNum;
					else
						m_FileDialogScrollValue = ((float)(NewIndex-Num)+2.5f)/ScrollNum;
				}

				if(!m_FileList[NewIndex].m_IsDir)
					str_copy(m_aFileDialogFileName, m_FileList[NewIndex].m_aFilename, sizeof(m_aFileDialogFileName));
				else
					m_aFileDialogFileName[0] = 0;
				m_FilesSelectedIndex = NewIndex;
			}
		}
	}

	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		if(Input()->GetEvent(i).m_Flags&IInput::FLAG_PRESS)
		{
			if(Input()->GetEvent(i).m_Key == KEY_RETURN || Input()->GetEvent(i).m_Key == KEY_KP_ENTER)
				m_aFileDialogActivate = true;
		}
	}

	if(m_FileDialogScrollValue < 0) m_FileDialogScrollValue = 0;
	if(m_FileDialogScrollValue > 1) m_FileDialogScrollValue = 1;

	m_FilesStartAt = (int)(ScrollNum*m_FileDialogScrollValue);
	if(m_FilesStartAt < 0)
		m_FilesStartAt = 0;

	m_FilesStopAt = m_FilesStartAt+Num;

	m_FilesCur = 0;

	// set clipping
	UI()->ClipEnable(&View);

	for(int i = 0; i < m_FileList.size(); i++)
		AddFileDialogEntry(i, &View);

	// disable clipping again
	UI()->ClipDisable();

	// the buttons
	static int s_OkButton = 0;
	static int s_CancelButton = 0;
	static int s_NewFolderButton = 0;
	static int s_MapInfoButton = 0;

	CUIRect Button;
	ButtonBar.VSplitRight(50.0f, &ButtonBar, &Button);
	bool IsDir = m_FilesSelectedIndex >= 0 && m_FileList[m_FilesSelectedIndex].m_IsDir;
	if(DoButton_Editor(&s_OkButton, IsDir ? "Open" : m_pFileDialogButtonText, 0, &Button, 0, 0) || m_aFileDialogActivate)
	{
		m_aFileDialogActivate = false;
		if(IsDir)	// folder
		{
			if(str_comp(m_FileList[m_FilesSelectedIndex].m_aFilename, "..") == 0)	// parent folder
			{
				if(fs_parent_dir(m_pFileDialogPath))
					m_pFileDialogPath = m_aFileDialogCurrentFolder;	// leave the link
			}
			else	// sub folder
			{
				if(m_FileList[m_FilesSelectedIndex].m_IsLink)
				{
					m_pFileDialogPath = m_aFileDialogCurrentLink;	// follow the link
					str_copy(m_aFileDialogCurrentLink, m_FileList[m_FilesSelectedIndex].m_aFilename, sizeof(m_aFileDialogCurrentLink));
				}
				else
				{
					char aTemp[MAX_PATH_LENGTH];
					str_copy(aTemp, m_pFileDialogPath, sizeof(aTemp));
					str_format(m_pFileDialogPath, MAX_PATH_LENGTH, "%s/%s", aTemp, m_FileList[m_FilesSelectedIndex].m_aFilename);
				}
			}
			FilelistPopulate(!str_comp(m_pFileDialogPath, "maps") || !str_comp(m_pFileDialogPath, "mapres") ? m_FileDialogStorageType :
				m_FileList[m_FilesSelectedIndex].m_StorageType);
			if(m_FilesSelectedIndex >= 0 && !m_FileList[m_FilesSelectedIndex].m_IsDir)
				str_copy(m_aFileDialogFileName, m_FileList[m_FilesSelectedIndex].m_aFilename, sizeof(m_aFileDialogFileName));
			else
				m_aFileDialogFileName[0] = 0;
		}
		else // file
		{
			str_format(m_aFileSaveName, sizeof(m_aFileSaveName), "%s/%s", m_pFileDialogPath, m_aFileDialogFileName);
			if(!str_comp(m_pFileDialogButtonText, "Save"))
			{
				IOHANDLE File = Storage()->OpenFile(m_aFileSaveName, IOFLAG_READ, IStorage::TYPE_SAVE);
				if(File)
				{
					io_close(File);
					m_PopupEventType = POPEVENT_SAVE;
					m_PopupEventActivated = true;
				}
				else
					if(m_pfnFileDialogFunc)
						m_pfnFileDialogFunc(m_aFileSaveName, m_FilesSelectedIndex >= 0 ? m_FileList[m_FilesSelectedIndex].m_StorageType : m_FileDialogStorageType, m_pFileDialogUser);
			}
			else
				if(m_pfnFileDialogFunc)
					m_pfnFileDialogFunc(m_aFileSaveName, m_FilesSelectedIndex >= 0 ? m_FileList[m_FilesSelectedIndex].m_StorageType : m_FileDialogStorageType, m_pFileDialogUser);
		}
	}

	ButtonBar.VSplitRight(40.0f, &ButtonBar, &Button);
	ButtonBar.VSplitRight(50.0f, &ButtonBar, &Button);
	if(DoButton_Editor(&s_CancelButton, "Cancel", 0, &Button, 0, 0) || Input()->KeyPressed(KEY_ESCAPE))
		m_Dialog = DIALOG_NONE;

	if(m_FileDialogStorageType == IStorage::TYPE_SAVE)
	{
		ButtonBar.VSplitLeft(40.0f, 0, &ButtonBar);
		ButtonBar.VSplitLeft(70.0f, &Button, &ButtonBar);
		if(DoButton_Editor(&s_NewFolderButton, "New folder", 0, &Button, 0, 0))
		{
			m_FileDialogNewFolderName[0] = 0;
			m_FileDialogErrString[0] = 0;
			static int s_NewFolderPopupID = 0;
			UiInvokePopupMenu(&s_NewFolderPopupID, 0, Width/2.0f-200.0f, Height/2.0f-100.0f, 400.0f, 200.0f, PopupNewFolder);
			UI()->SetActiveItem(0);
		}
	}

	if(m_FileDialogStorageType == IStorage::TYPE_SAVE)
	{
		ButtonBar.VSplitLeft(40.0f, 0, &ButtonBar);
		ButtonBar.VSplitLeft(70.0f, &Button, &ButtonBar);
		if(DoButton_Editor(&s_MapInfoButton, "Map details", 0, &Button, 0, 0))
		{
			str_copy(m_Map.m_MapInfo.m_aAuthorTmp, m_Map.m_MapInfo.m_aAuthor, sizeof(m_Map.m_MapInfo.m_aAuthorTmp));
			str_copy(m_Map.m_MapInfo.m_aVersionTmp, m_Map.m_MapInfo.m_aVersion, sizeof(m_Map.m_MapInfo.m_aVersionTmp));
			str_copy(m_Map.m_MapInfo.m_aCreditsTmp, m_Map.m_MapInfo.m_aCredits, sizeof(m_Map.m_MapInfo.m_aCreditsTmp));
			str_copy(m_Map.m_MapInfo.m_aLicenseTmp, m_Map.m_MapInfo.m_aLicense, sizeof(m_Map.m_MapInfo.m_aLicenseTmp));
			static int s_MapInfoPopupID = 0;
			UiInvokePopupMenu(&s_MapInfoPopupID, 0, Width/2.0f-200.0f, Height/2.0f-100.0f, 400.0f, 200.0f, PopupMapInfo);
			UI()->SetActiveItem(0);
		}
	}
}

void CEditor::FilelistPopulate(int StorageType)
{
	m_FileList.clear();
	if(m_FileDialogStorageType != IStorage::TYPE_SAVE && !str_comp(m_pFileDialogPath, "maps"))
	{
		CFilelistItem Item;
		str_copy(Item.m_aFilename, "downloadedmaps", sizeof(Item.m_aFilename));
		str_copy(Item.m_aName, "downloadedmaps/", sizeof(Item.m_aName));
		Item.m_IsDir = true;
		Item.m_IsLink = true;
		Item.m_StorageType = IStorage::TYPE_SAVE;
		m_FileList.add(Item);
	}
	Storage()->ListDirectory(StorageType, m_pFileDialogPath, EditorListdirCallback, this);
	m_FilesSelectedIndex = m_FileList.size() ? 0 : -1;
	m_aFileDialogActivate = false;
}

void CEditor::InvokeFileDialog(int StorageType, int FileType, const char *pTitle, const char *pButtonText,
	const char *pBasePath, const char *pDefaultName,
	void (*pfnFunc)(const char *pFileName, int StorageType, void *pUser), void *pUser)
{
	m_FileDialogStorageType = StorageType;
	m_pFileDialogTitle = pTitle;
	m_pFileDialogButtonText = pButtonText;
	m_pfnFileDialogFunc = pfnFunc;
	m_pFileDialogUser = pUser;
	m_aFileDialogFileName[0] = 0;
	m_aFileDialogCurrentFolder[0] = 0;
	m_aFileDialogCurrentLink[0] = 0;
	m_pFileDialogPath = m_aFileDialogCurrentFolder;
	m_FileDialogFileType = FileType;
	m_FileDialogScrollValue = 0.0f;

	if(pDefaultName)
		str_copy(m_aFileDialogFileName, pDefaultName, sizeof(m_aFileDialogFileName));
	if(pBasePath)
		str_copy(m_aFileDialogCurrentFolder, pBasePath, sizeof(m_aFileDialogCurrentFolder));

	FilelistPopulate(m_FileDialogStorageType);

	m_Dialog = DIALOG_FILE;
}



void CEditor::RenderModebar(CUIRect View)
{
	CUIRect Button;

	// mode buttons
	{
		View.VSplitLeft(65.0f, &Button, &View);
		Button.HSplitTop(30.0f, 0, &Button);
		static int s_Button = 0;
		const char *pButName = m_Mode == MODE_LAYERS ? "Layers" : "Images";
		if(DoButton_Tab(&s_Button, pButName, 0, &Button, 0, "Switch between images and layers managment."))
		{
			if(m_Mode == MODE_LAYERS)
				m_Mode = MODE_IMAGES;
			else
				m_Mode = MODE_LAYERS;
		}
	}

	View.VSplitLeft(5.0f, 0, &View);
}

void CEditor::RenderStatusbar(CUIRect View)
{
	CUIRect Button;
	View.VSplitRight(60.0f, &View, &Button);
	static int s_EnvelopeButton = 0;
	if(DoButton_Editor(&s_EnvelopeButton, "Envelopes", m_ShowEnvelopeEditor, &Button, 0, "Toggles the envelope editor."))
		m_ShowEnvelopeEditor = (m_ShowEnvelopeEditor+1)%4;

	if(m_pTooltip)
	{
		if(ms_pUiGotContext && ms_pUiGotContext == UI()->HotItem())
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "%s Right click for context menu.", m_pTooltip);
			UI()->DoLabel(&View, aBuf, 10.0f, -1, -1);
		}
		else
			UI()->DoLabel(&View, m_pTooltip, 10.0f, -1, -1);
	}
}

void CEditor::RenderEnvelopeEditor(CUIRect View)
{
	if(m_SelectedEnvelope < 0) m_SelectedEnvelope = 0;
	if(m_SelectedEnvelope >= m_Map.m_lEnvelopes.size()) m_SelectedEnvelope = m_Map.m_lEnvelopes.size()-1;

	CEnvelope *pEnvelope = 0;
	if(m_SelectedEnvelope >= 0 && m_SelectedEnvelope < m_Map.m_lEnvelopes.size())
		pEnvelope = m_Map.m_lEnvelopes[m_SelectedEnvelope];

	CUIRect ToolBar, CurveBar, ColorBar;
	View.HSplitTop(15.0f, &ToolBar, &View);
	View.HSplitTop(15.0f, &CurveBar, &View);
	ToolBar.Margin(2.0f, &ToolBar);
	CurveBar.Margin(2.0f, &CurveBar);

	// do the toolbar
	{
		CUIRect Button;
		CEnvelope *pNewEnv = 0;

		ToolBar.VSplitRight(50.0f, &ToolBar, &Button);
		static int s_New4dButton = 0;
		if(DoButton_Editor(&s_New4dButton, "Color+", 0, &Button, 0, "Creates a new color envelope"))
		{
			m_Map.m_Modified = true;
			pNewEnv = m_Map.NewEnvelope(4);
		}

		ToolBar.VSplitRight(5.0f, &ToolBar, &Button);
		ToolBar.VSplitRight(50.0f, &ToolBar, &Button);
		static int s_New2dButton = 0;
		if(DoButton_Editor(&s_New2dButton, "Pos.+", 0, &Button, 0, "Creates a new pos envelope"))
		{
			m_Map.m_Modified = true;
			pNewEnv = m_Map.NewEnvelope(3);
		}

		// Delete button
		if(m_SelectedEnvelope >= 0)
		{
			ToolBar.VSplitRight(10.0f, &ToolBar, &Button);
			ToolBar.VSplitRight(50.0f, &ToolBar, &Button);
			static int s_DelButton = 0;
			if(DoButton_Editor(&s_DelButton, "Delete", 0, &Button, 0, "Delete this envelope"))
			{
				m_Map.m_Modified = true;
				m_Map.DeleteEnvelope(m_SelectedEnvelope);
				if(m_SelectedEnvelope >= m_Map.m_lEnvelopes.size())
					m_SelectedEnvelope = m_Map.m_lEnvelopes.size()-1;
				pEnvelope = m_SelectedEnvelope >= 0 ? m_Map.m_lEnvelopes[m_SelectedEnvelope] : 0;
			}
		}

		if(pNewEnv) // add the default points
		{
			if(pNewEnv->m_Channels == 4)
			{
				pNewEnv->AddPoint(0, 1,1,1,1);
				pNewEnv->AddPoint(1000, 1,1,1,1);
			}
			else
			{
				pNewEnv->AddPoint(0, 0);
				pNewEnv->AddPoint(1000, 0);
			}
		}

		CUIRect Shifter, Inc, Dec;
		ToolBar.VSplitLeft(60.0f, &Shifter, &ToolBar);
		Shifter.VSplitRight(15.0f, &Shifter, &Inc);
		Shifter.VSplitLeft(15.0f, &Dec, &Shifter);
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf),"%d/%d", m_SelectedEnvelope+1, m_Map.m_lEnvelopes.size());
		RenderTools()->DrawUIRect(&Shifter, vec4(1,1,1,0.5f), 0, 0.0f);
		UI()->DoLabel(&Shifter, aBuf, 10.0f, 0, -1);

		static int s_PrevButton = 0;
		if(DoButton_ButtonDec(&s_PrevButton, 0, 0, &Dec, 0, "Previous Envelope"))
			m_SelectedEnvelope--;

		static int s_NextButton = 0;
		if(DoButton_ButtonInc(&s_NextButton, 0, 0, &Inc, 0, "Next Envelope"))
			m_SelectedEnvelope++;

		if(pEnvelope)
		{
			ToolBar.VSplitLeft(15.0f, &Button, &ToolBar);
			ToolBar.VSplitLeft(35.0f, &Button, &ToolBar);
			UI()->DoLabel(&Button, "Name:", 10.0f, -1, -1);

			ToolBar.VSplitLeft(80.0f, &Button, &ToolBar);

			static float s_NameBox = 0;
			if(DoEditBox(&s_NameBox, &Button, pEnvelope->m_aName, sizeof(pEnvelope->m_aName), 10.0f, &s_NameBox))
				m_Map.m_Modified = true;
		}
	}

	bool ShowColorBar = false;
	if(pEnvelope && pEnvelope->m_Channels == 4)
	{
		ShowColorBar = true;
		View.HSplitTop(20.0f, &ColorBar, &View);
		ColorBar.Margin(2.0f, &ColorBar);
		RenderBackground(ColorBar, ms_CheckerTexture, 16.0f, 1.0f);
	}

	RenderBackground(View, ms_CheckerTexture, 32.0f, 0.1f);

	if(pEnvelope)
	{
		static array<int> Selection;
		static int sEnvelopeEditorID = 0;
		static int s_ActiveChannels = 0xf;

		if(pEnvelope)
		{
			CUIRect Button;

			ToolBar.VSplitLeft(15.0f, &Button, &ToolBar);

			static const char *s_paNames[2][4] = {
				{"X", "Y", "R", ""},
				{"R", "G", "B", "A"},
			};

			const char *paDescriptions[2][4] = {
				{"X-axis of the envelope", "Y-axis of the envelope", "Rotation of the envelope", ""},
				{"Red value of the envelope", "Green value of the envelope", "Blue value of the envelope", "Alpha value of the envelope"},
			};

			static int s_aChannelButtons[4] = {0};
			int Bit = 1;
			//ui_draw_button_func draw_func;

			for(int i = 0; i < pEnvelope->m_Channels; i++, Bit<<=1)
			{
				ToolBar.VSplitLeft(15.0f, &Button, &ToolBar);

				/*if(i == 0) draw_func = draw_editor_button_l;
				else if(i == envelope->channels-1) draw_func = draw_editor_button_r;
				else draw_func = draw_editor_button_m;*/

				if(DoButton_Editor(&s_aChannelButtons[i], s_paNames[pEnvelope->m_Channels-3][i], s_ActiveChannels&Bit, &Button, 0, paDescriptions[pEnvelope->m_Channels-3][i]))
					s_ActiveChannels ^= Bit;
			}

			// sync checkbox
			ToolBar.VSplitLeft(15.0f, &Button, &ToolBar);
			ToolBar.VSplitLeft(12.0f, &Button, &ToolBar);
			static int s_SyncButton;
			if(DoButton_Editor(&s_SyncButton, pEnvelope->m_Synchronized?"X":"", 0, &Button, 0, "Enable envelope synchronization between clients"))
				pEnvelope->m_Synchronized = !pEnvelope->m_Synchronized;

			ToolBar.VSplitLeft(4.0f, &Button, &ToolBar);
			ToolBar.VSplitLeft(80.0f, &Button, &ToolBar);
			UI()->DoLabel(&Button, "Synchronized", 10.0f, -1, -1);
		}

		float EndTime = pEnvelope->EndTime();
		if(EndTime < 1)
			EndTime = 1;

		pEnvelope->FindTopBottom(s_ActiveChannels);
		float Top = pEnvelope->m_Top;
		float Bottom = pEnvelope->m_Bottom;

		if(Top < 1)
			Top = 1;
		if(Bottom >= 0)
			Bottom = 0;

		float TimeScale = EndTime/View.w;
		float ValueScale = (Top-Bottom)/View.h;

		if(UI()->MouseInside(&View))
			UI()->SetHotItem(&sEnvelopeEditorID);

		if(UI()->HotItem() == &sEnvelopeEditorID)
		{
			// do stuff
			if(pEnvelope)
			{
				if(UI()->MouseButtonClicked(1))
				{
					// add point
					int Time = (int)(((UI()->MouseX()-View.x)*TimeScale)*1000.0f);
					//float env_y = (UI()->MouseY()-view.y)/TimeScale;
					float aChannels[4];
					pEnvelope->Eval(Time, aChannels);
					pEnvelope->AddPoint(Time,
						f2fx(aChannels[0]), f2fx(aChannels[1]),
						f2fx(aChannels[2]), f2fx(aChannels[3]));
					m_Map.m_Modified = true;
				}

				m_ShowEnvelopePreview = 1;
				m_pTooltip = "Press right mouse button to create a new point";
			}
		}

		vec3 aColors[] = {vec3(1,0.2f,0.2f), vec3(0.2f,1,0.2f), vec3(0.2f,0.2f,1), vec3(1,1,0.2f)};

		// render lines
		{
			UI()->ClipEnable(&View);
			Graphics()->TextureSet(-1);
			Graphics()->LinesBegin();
			for(int c = 0; c < pEnvelope->m_Channels; c++)
			{
				if(s_ActiveChannels&(1<<c))
					Graphics()->SetColor(aColors[c].r,aColors[c].g,aColors[c].b,1);
				else
					Graphics()->SetColor(aColors[c].r*0.5f,aColors[c].g*0.5f,aColors[c].b*0.5f,1);

				float PrevX = 0;
				float aResults[4];
				pEnvelope->Eval(0.000001f, aResults);
				float PrevValue = aResults[c];

				int Steps = (int)((View.w/UI()->Screen()->w) * Graphics()->ScreenWidth());
				for(int i = 1; i <= Steps; i++)
				{
					float a = i/(float)Steps;
					pEnvelope->Eval(a*EndTime, aResults);
					float v = aResults[c];
					v = (v-Bottom)/(Top-Bottom);

					IGraphics::CLineItem LineItem(View.x + PrevX*View.w, View.y+View.h - PrevValue*View.h, View.x + a*View.w, View.y+View.h - v*View.h);
					Graphics()->LinesDraw(&LineItem, 1);
					PrevX = a;
					PrevValue = v;
				}
			}
			Graphics()->LinesEnd();
			UI()->ClipDisable();
		}

		// render curve options
		{
			for(int i = 0; i < pEnvelope->m_lPoints.size()-1; i++)
			{
				float t0 = pEnvelope->m_lPoints[i].m_Time/1000.0f/EndTime;
				float t1 = pEnvelope->m_lPoints[i+1].m_Time/1000.0f/EndTime;

				//dbg_msg("", "%f", end_time);

				CUIRect v;
				v.x = CurveBar.x + (t0+(t1-t0)*0.5f) * CurveBar.w;
				v.y = CurveBar.y;
				v.h = CurveBar.h;
				v.w = CurveBar.h;
				v.x -= v.w/2;
				void *pID = &pEnvelope->m_lPoints[i].m_Curvetype;
				const char *paTypeName[] = {
					"N", "L", "S", "F", "M"
					};

				if(DoButton_Editor(pID, paTypeName[pEnvelope->m_lPoints[i].m_Curvetype], 0, &v, 0, "Switch curve type"))
					pEnvelope->m_lPoints[i].m_Curvetype = (pEnvelope->m_lPoints[i].m_Curvetype+1)%NUM_CURVETYPES;
			}
		}

		// render colorbar
		if(ShowColorBar)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			for(int i = 0; i < pEnvelope->m_lPoints.size()-1; i++)
			{
				float r0 = fx2f(pEnvelope->m_lPoints[i].m_aValues[0]);
				float g0 = fx2f(pEnvelope->m_lPoints[i].m_aValues[1]);
				float b0 = fx2f(pEnvelope->m_lPoints[i].m_aValues[2]);
				float a0 = fx2f(pEnvelope->m_lPoints[i].m_aValues[3]);
				float r1 = fx2f(pEnvelope->m_lPoints[i+1].m_aValues[0]);
				float g1 = fx2f(pEnvelope->m_lPoints[i+1].m_aValues[1]);
				float b1 = fx2f(pEnvelope->m_lPoints[i+1].m_aValues[2]);
				float a1 = fx2f(pEnvelope->m_lPoints[i+1].m_aValues[3]);

				IGraphics::CColorVertex Array[4] = {IGraphics::CColorVertex(0, r0, g0, b0, a0),
													IGraphics::CColorVertex(1, r1, g1, b1, a1),
													IGraphics::CColorVertex(2, r1, g1, b1, a1),
													IGraphics::CColorVertex(3, r0, g0, b0, a0)};
				Graphics()->SetColorVertex(Array, 4);

				float x0 = pEnvelope->m_lPoints[i].m_Time/1000.0f/EndTime;
//				float y0 = (fx2f(envelope->points[i].values[c])-bottom)/(top-bottom);
				float x1 = pEnvelope->m_lPoints[i+1].m_Time/1000.0f/EndTime;
				//float y1 = (fx2f(envelope->points[i+1].values[c])-bottom)/(top-bottom);
				CUIRect v;
				v.x = ColorBar.x + x0*ColorBar.w;
				v.y = ColorBar.y;
				v.w = (x1-x0)*ColorBar.w;
				v.h = ColorBar.h;

				IGraphics::CQuadItem QuadItem(v.x, v.y, v.w, v.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
			Graphics()->QuadsEnd();
		}

		// render handles
		{
			int CurrentValue = 0, CurrentTime = 0;

			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			for(int c = 0; c < pEnvelope->m_Channels; c++)
			{
				if(!(s_ActiveChannels&(1<<c)))
					continue;

				for(int i = 0; i < pEnvelope->m_lPoints.size(); i++)
				{
					float x0 = pEnvelope->m_lPoints[i].m_Time/1000.0f/EndTime;
					float y0 = (fx2f(pEnvelope->m_lPoints[i].m_aValues[c])-Bottom)/(Top-Bottom);
					CUIRect Final;
					Final.x = View.x + x0*View.w;
					Final.y = View.y+View.h - y0*View.h;
					Final.x -= 2.0f;
					Final.y -= 2.0f;
					Final.w = 4.0f;
					Final.h = 4.0f;

					void *pID = &pEnvelope->m_lPoints[i].m_aValues[c];

					if(UI()->MouseInside(&Final))
						UI()->SetHotItem(pID);

					float ColorMod = 1.0f;

					if(UI()->ActiveItem() == pID)
					{
						if(!UI()->MouseButton(0))
						{
							m_SelectedQuadEnvelope = -1;
							m_SelectedEnvelopePoint = -1;

							UI()->SetActiveItem(0);
						}
						else
						{
							if(Input()->KeyPressed(KEY_LSHIFT) || Input()->KeyPressed(KEY_RSHIFT))
							{
								if(i != 0)
								{
									if((Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL)))
										pEnvelope->m_lPoints[i].m_Time += (int)((m_MouseDeltaX));
									else
										pEnvelope->m_lPoints[i].m_Time += (int)((m_MouseDeltaX*TimeScale)*1000.0f);
									if(pEnvelope->m_lPoints[i].m_Time < pEnvelope->m_lPoints[i-1].m_Time)
										pEnvelope->m_lPoints[i].m_Time = pEnvelope->m_lPoints[i-1].m_Time + 1;
									if(i+1 != pEnvelope->m_lPoints.size() && pEnvelope->m_lPoints[i].m_Time > pEnvelope->m_lPoints[i+1].m_Time)
										pEnvelope->m_lPoints[i].m_Time = pEnvelope->m_lPoints[i+1].m_Time - 1;
								}
							}
							else
							{
								if((Input()->KeyPressed(KEY_LCTRL) || Input()->KeyPressed(KEY_RCTRL)))
									pEnvelope->m_lPoints[i].m_aValues[c] -= f2fx(m_MouseDeltaY*0.001f);
								else
									pEnvelope->m_lPoints[i].m_aValues[c] -= f2fx(m_MouseDeltaY*ValueScale);
							}

							m_SelectedQuadEnvelope = m_SelectedEnvelope;
							m_ShowEnvelopePreview = 1;
							m_SelectedEnvelopePoint = i;
							m_Map.m_Modified = true;
						}

						ColorMod = 100.0f;
						Graphics()->SetColor(1,1,1,1);
					}
					else if(UI()->HotItem() == pID)
					{
						if(UI()->MouseButton(0))
						{
							Selection.clear();
							Selection.add(i);
							UI()->SetActiveItem(pID);
						}

						// remove point
						if(UI()->MouseButtonClicked(1))
						{
							pEnvelope->m_lPoints.remove_index(i);
							m_Map.m_Modified = true;
						}

						m_ShowEnvelopePreview = 1;
						ColorMod = 100.0f;
						Graphics()->SetColor(1,0.75f,0.75f,1);
						m_pTooltip = "Left mouse to drag. Hold ctrl to be more precise. Hold shift to alter time point aswell. Right click to delete.";
					}

					if(UI()->ActiveItem() == pID || UI()->HotItem() == pID)
					{
						CurrentTime = pEnvelope->m_lPoints[i].m_Time;
						CurrentValue = pEnvelope->m_lPoints[i].m_aValues[c];
					}

					if (m_SelectedQuadEnvelope == m_SelectedEnvelope && m_SelectedEnvelopePoint == i)
						Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
					else
						Graphics()->SetColor(aColors[c].r*ColorMod, aColors[c].g*ColorMod, aColors[c].b*ColorMod, 1.0f);
					IGraphics::CQuadItem QuadItem(Final.x, Final.y, Final.w, Final.h);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
				}
			}
			Graphics()->QuadsEnd();

			char aBuf[512];
			str_format(aBuf, sizeof(aBuf),"%.3f %.3f", CurrentTime/1000.0f, fx2f(CurrentValue));
			UI()->DoLabel(&ToolBar, aBuf, 10.0f, 0, -1);
		}
	}
}

int CEditor::PopupMenuFile(CEditor *pEditor, CUIRect View)
{
	static int s_NewMapButton = 0;
	static int s_SaveButton = 0;
	static int s_SaveAsButton = 0;
	static int s_OpenButton = 0;
	static int s_AppendButton = 0;
	static int s_ExitButton = 0;

	CUIRect Slot;
	View.HSplitTop(2.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_NewMapButton, "New", 0, &Slot, 0, "Creates a new map"))
	{
		if(pEditor->HasUnsavedData())
		{
			pEditor->m_PopupEventType = POPEVENT_NEW;
			pEditor->m_PopupEventActivated = true;
		}
		else
		{
			pEditor->Reset();
			pEditor->m_aFileName[0] = 0;
		}
		return 1;
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_OpenButton, "Load", 0, &Slot, 0, "Opens a map for editing"))
	{
		if(pEditor->HasUnsavedData())
		{
			pEditor->m_PopupEventType = POPEVENT_LOAD;
			pEditor->m_PopupEventActivated = true;
		}
		else
			pEditor->InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_MAP, "Load map", "Load", "maps", "", pEditor->CallbackOpenMap, pEditor);
		return 1;
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_AppendButton, "Append", 0, &Slot, 0, "Opens a map and adds everything from that map to the current one"))
	{
		pEditor->InvokeFileDialog(IStorage::TYPE_ALL, FILETYPE_MAP, "Append map", "Append", "maps", "", pEditor->CallbackAppendMap, pEditor);
		return 1;
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_SaveButton, "Save", 0, &Slot, 0, "Saves the current map"))
	{
		if(pEditor->m_aFileName[0] && pEditor->m_ValidSaveFilename)
		{
			str_copy(pEditor->m_aFileSaveName, pEditor->m_aFileName, sizeof(pEditor->m_aFileSaveName));
			pEditor->m_PopupEventType = POPEVENT_SAVE;
			pEditor->m_PopupEventActivated = true;
		}
		else
			pEditor->InvokeFileDialog(IStorage::TYPE_SAVE, FILETYPE_MAP, "Save map", "Save", "maps", "", pEditor->CallbackSaveMap, pEditor);
		return 1;
	}

	View.HSplitTop(2.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_SaveAsButton, "Save As", 0, &Slot, 0, "Saves the current map under a new name"))
	{
		pEditor->InvokeFileDialog(IStorage::TYPE_SAVE, FILETYPE_MAP, "Save map", "Save", "maps", "", pEditor->CallbackSaveMap, pEditor);
		return 1;
	}

	View.HSplitTop(10.0f, &Slot, &View);
	View.HSplitTop(12.0f, &Slot, &View);
	if(pEditor->DoButton_MenuItem(&s_ExitButton, "Exit", 0, &Slot, 0, "Exits from the editor"))
	{
		if(pEditor->HasUnsavedData())
		{
			pEditor->m_PopupEventType = POPEVENT_EXIT;
			pEditor->m_PopupEventActivated = true;
		}
		else
			g_Config.m_ClEditor = 0;
		return 1;
	}

	return 0;
}

void CEditor::RenderMenubar(CUIRect MenuBar)
{
	static CUIRect s_File /*, view, help*/;

	MenuBar.VSplitLeft(60.0f, &s_File, &MenuBar);
	if(DoButton_Menu(&s_File, "File", 0, &s_File, 0, 0))
		UiInvokePopupMenu(&s_File, 1, s_File.x, s_File.y+s_File.h-1.0f, 120, 150, PopupMenuFile, this);

	/*
	menubar.VSplitLeft(5.0f, 0, &menubar);
	menubar.VSplitLeft(60.0f, &view, &menubar);
	if(do_editor_button(&view, "View", 0, &view, draw_editor_button_menu, 0, 0))
		(void)0;

	menubar.VSplitLeft(5.0f, 0, &menubar);
	menubar.VSplitLeft(60.0f, &help, &menubar);
	if(do_editor_button(&help, "Help", 0, &help, draw_editor_button_menu, 0, 0))
		(void)0;
		*/

	CUIRect Info;
	MenuBar.VSplitLeft(40.0f, 0, &MenuBar);
	MenuBar.VSplitLeft(MenuBar.w*0.75f, &MenuBar, &Info);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "File: %s", m_aFileName);
	UI()->DoLabel(&MenuBar, aBuf, 10.0f, -1, -1);

	str_format(aBuf, sizeof(aBuf), "Z: %i, A: %.1f, G: %i", m_ZoomLevel, m_AnimateSpeed, m_GridFactor);
	UI()->DoLabel(&Info, aBuf, 10.0f, 1, -1);
}

void CEditor::Render()
{
	// basic start
	Graphics()->Clear(1.0f, 0.0f, 1.0f);
	CUIRect View = *UI()->Screen();
	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);

	float Width = View.w;
	float Height = View.h;

	// reset tip
	m_pTooltip = 0;

	if(m_EditBoxActive)
		--m_EditBoxActive;

	// render checker
	RenderBackground(View, ms_CheckerTexture, 32.0f, 1.0f);

	CUIRect MenuBar, CModeBar, ToolBar, StatusBar, EnvelopeEditor, ToolBox;
	m_ShowPicker = Input()->KeyPressed(KEY_SPACE) != 0 && m_Dialog == DIALOG_NONE;

	if(m_GuiActive)
	{

		View.HSplitTop(16.0f, &MenuBar, &View);
		View.HSplitTop(53.0f, &ToolBar, &View);
		View.VSplitLeft(100.0f, &ToolBox, &View);
		View.HSplitBottom(16.0f, &View, &StatusBar);

		if(m_ShowEnvelopeEditor && !m_ShowPicker)
		{
			float size = 125.0f;
			if(m_ShowEnvelopeEditor == 2)
				size *= 2.0f;
			else if(m_ShowEnvelopeEditor == 3)
				size *= 3.0f;
			View.HSplitBottom(size, &View, &EnvelopeEditor);
		}
	}

	//	a little hack for now
	if(m_Mode == MODE_LAYERS)
		DoMapEditor(View, ToolBar);

	// do zooming
	if(Input()->KeyDown(KEY_KP_MINUS))
		m_ZoomLevel += 50;
	if(Input()->KeyDown(KEY_KP_PLUS))
		m_ZoomLevel -= 50;
	if(Input()->KeyDown(KEY_KP_MULTIPLY))
	{
		m_EditorOffsetX = 0;
		m_EditorOffsetY = 0;
		m_ZoomLevel = 100;
	}
	if(m_Dialog == DIALOG_NONE && UI()->MouseInside(&View))
	{
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
			m_ZoomLevel -= 20;

		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			m_ZoomLevel += 20;
	}
	m_ZoomLevel = clamp(m_ZoomLevel, 50, 2000);
	m_WorldZoom = m_ZoomLevel/100.0f;

	if(m_GuiActive)
	{
		float Brightness = 0.25f;
		RenderBackground(MenuBar, ms_BackgroundTexture, 128.0f, Brightness*0);
		MenuBar.Margin(2.0f, &MenuBar);

		RenderBackground(ToolBox, ms_BackgroundTexture, 128.0f, Brightness);
		ToolBox.Margin(2.0f, &ToolBox);

		RenderBackground(ToolBar, ms_BackgroundTexture, 128.0f, Brightness);
		ToolBar.Margin(2.0f, &ToolBar);
		ToolBar.VSplitLeft(100.0f, &CModeBar, &ToolBar);

		RenderBackground(StatusBar, ms_BackgroundTexture, 128.0f, Brightness);
		StatusBar.Margin(2.0f, &StatusBar);

		// do the toolbar
		if(m_Mode == MODE_LAYERS)
			DoToolbar(ToolBar);

		if(m_ShowEnvelopeEditor)
		{
			RenderBackground(EnvelopeEditor, ms_BackgroundTexture, 128.0f, Brightness);
			EnvelopeEditor.Margin(2.0f, &EnvelopeEditor);
		}
	}


	if(m_Mode == MODE_LAYERS)
		RenderLayers(ToolBox, ToolBar, View);
	else if(m_Mode == MODE_IMAGES)
		RenderImages(ToolBox, ToolBar, View);

	Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);

	if(m_GuiActive)
	{
		RenderMenubar(MenuBar);

		RenderModebar(CModeBar);
		if(m_ShowEnvelopeEditor)
			RenderEnvelopeEditor(EnvelopeEditor);
	}

	if(m_Dialog == DIALOG_FILE)
	{
		static int s_NullUiTarget = 0;
		UI()->SetHotItem(&s_NullUiTarget);
		RenderFileDialog();
	}

	if(m_PopupEventActivated)
	{
		static int s_PopupID = 0;
		UiInvokePopupMenu(&s_PopupID, 0, Width/2.0f-200.0f, Height/2.0f-100.0f, 400.0f, 200.0f, PopupEvent);
		m_PopupEventActivated = false;
		m_PopupEventWasActivated = true;
	}


	UiDoPopupMenu();

	if(m_GuiActive)
		RenderStatusbar(StatusBar);

	//
	if(g_Config.m_EdShowkeys)
	{
		Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, View.x+10, View.y+View.h-24-10, 24.0f, TEXTFLAG_RENDER);

		int NKeys = 0;
		for(int i = 0; i < KEY_LAST; i++)
		{
			if(Input()->KeyPressed(i))
			{
				if(NKeys)
					TextRender()->TextEx(&Cursor, " + ", -1);
				TextRender()->TextEx(&Cursor, Input()->KeyName(i), -1);
				NKeys++;
			}
		}
	}

	if(m_ShowMousePointer)
	{
		// render butt ugly mouse cursor
		float mx = UI()->MouseX();
		float my = UI()->MouseY();
		Graphics()->TextureSet(ms_CursorTexture);
		Graphics()->QuadsBegin();
		if(ms_pUiGotContext == UI()->HotItem())
			Graphics()->SetColor(1,0,0,1);
		IGraphics::CQuadItem QuadItem(mx,my, 16.0f, 16.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CEditor::Reset(bool CreateDefault)
{
	m_Map.Clean();

	// create default layers
	if(CreateDefault)
		m_Map.CreateDefault(ms_EntitiesTexture);

	/*
	{
	}*/

	m_SelectedLayer = 0;
	m_SelectedGroup = 0;
	m_SelectedQuad = -1;
	m_SelectedPoints = 0;
	m_SelectedEnvelope = 0;
	m_SelectedImage = 0;

	m_WorldOffsetX = 0;
	m_WorldOffsetY = 0;
	m_EditorOffsetX = 0.0f;
	m_EditorOffsetY = 0.0f;

	m_WorldZoom = 1.0f;
	m_ZoomLevel = 200;

	m_MouseDeltaX = 0;
	m_MouseDeltaY = 0;
	m_MouseDeltaWx = 0;
	m_MouseDeltaWy = 0;

	m_Map.m_Modified = false;

	m_ShowEnvelopePreview = 0;
}

int CEditor::GetLineDistance()
{
	int LineDistance = 512;

	if(m_ZoomLevel <= 100)
		LineDistance = 16;
	else if(m_ZoomLevel <= 250)
		LineDistance = 32;
	else if(m_ZoomLevel <= 450)
		LineDistance = 64;
	else if(m_ZoomLevel <= 850)
		LineDistance = 128;
	else if(m_ZoomLevel <= 1550)
		LineDistance = 256;

	return LineDistance;
}

void CEditorMap::DeleteEnvelope(int Index)
{
	if(Index < 0 || Index >= m_lEnvelopes.size())
		return;

	m_Modified = true;

	// fix links between envelopes and quads
	for(int i = 0; i < m_lGroups.size(); ++i)
		for(int j = 0; j < m_lGroups[i]->m_lLayers.size(); ++j)
			if(m_lGroups[i]->m_lLayers[j]->m_Type == LAYERTYPE_QUADS)
			{
				CLayerQuads *pLayer = static_cast<CLayerQuads *>(m_lGroups[i]->m_lLayers[j]);
				for(int k = 0; k < pLayer->m_lQuads.size(); ++k)
				{
					if(pLayer->m_lQuads[k].m_PosEnv == Index)
						pLayer->m_lQuads[k].m_PosEnv = -1;
					else if(pLayer->m_lQuads[k].m_PosEnv > Index)
						pLayer->m_lQuads[k].m_PosEnv--;
					if(pLayer->m_lQuads[k].m_ColorEnv == Index)
						pLayer->m_lQuads[k].m_ColorEnv = -1;
					else if(pLayer->m_lQuads[k].m_ColorEnv > Index)
						pLayer->m_lQuads[k].m_ColorEnv--;
				}
			}
			else if(m_lGroups[i]->m_lLayers[j]->m_Type == LAYERTYPE_TILES)
			{
				CLayerTiles *pLayer = static_cast<CLayerTiles *>(m_lGroups[i]->m_lLayers[j]);
				if(pLayer->m_ColorEnv == Index)
					pLayer->m_ColorEnv = -1;
				if(pLayer->m_ColorEnv > Index)
					pLayer->m_ColorEnv--;
			}

	m_lEnvelopes.remove_index(Index);
}

void CEditorMap::MakeGameLayer(CLayer *pLayer)
{
	m_pGameLayer = (CLayerGame *)pLayer;
	m_pGameLayer->m_pEditor = m_pEditor;
	m_pGameLayer->m_TexID = m_pEditor->ms_EntitiesTexture;
}

void CEditorMap::MakeGameGroup(CLayerGroup *pGroup)
{
	m_pGameGroup = pGroup;
	m_pGameGroup->m_GameGroup = true;
	str_copy(m_pGameGroup->m_aName, "Game", sizeof(m_pGameGroup->m_aName));
}



void CEditorMap::Clean()
{
	m_lGroups.delete_all();
	m_lEnvelopes.delete_all();
	m_lImages.delete_all();

	m_MapInfo.Reset();

	m_pGameLayer = 0x0;
	m_pGameGroup = 0x0;

	m_Modified = false;
}

void CEditorMap::CreateDefault(int EntitiesTexture)
{
	// add background
	CLayerGroup *pGroup = NewGroup();
	pGroup->m_ParallaxX = 0;
	pGroup->m_ParallaxY = 0;
	CLayerQuads *pLayer = new CLayerQuads;
	pLayer->m_pEditor = m_pEditor;
	CQuad *pQuad = pLayer->NewQuad();
	const int Width = 800000;
	const int Height = 600000;
	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = -Width;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = Width;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = -Height;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = Height;
	pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = 94;
	pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = 132;
	pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = 174;
	pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = 204;
	pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = 232;
	pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = 255;
	pGroup->AddLayer(pLayer);

	// add game layer
	MakeGameGroup(NewGroup());
	MakeGameLayer(new CLayerGame(50, 50));
	m_pGameGroup->AddLayer(m_pGameLayer);
}

void CEditor::Init()
{
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_RenderTools.m_pGraphics = m_pGraphics;
	m_RenderTools.m_pUI = &m_UI;
	m_UI.SetGraphics(m_pGraphics, m_pTextRender);
	m_Map.m_pEditor = this;

	ms_CheckerTexture = Graphics()->LoadTexture("editor/checker.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	ms_BackgroundTexture = Graphics()->LoadTexture("editor/background.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	ms_CursorTexture = Graphics()->LoadTexture("editor/cursor.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	ms_EntitiesTexture = Graphics()->LoadTexture("editor/entities.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

	m_TilesetPicker.m_pEditor = this;
	m_TilesetPicker.MakePalette();
	m_TilesetPicker.m_Readonly = true;

	m_Brush.m_pMap = &m_Map;

	Reset();
	m_Map.m_Modified = false;
}

void CEditor::DoMapBorder()
{
	CLayerTiles *pT = (CLayerTiles *)GetSelectedLayerType(0, LAYERTYPE_TILES);

	for(int i = 0; i < pT->m_Width*2; ++i)
		pT->m_pTiles[i].m_Index = 1;

	for(int i = 0; i < pT->m_Width*pT->m_Height; ++i)
	{
		if(i%pT->m_Width < 2 || i%pT->m_Width > pT->m_Width-3)
			pT->m_pTiles[i].m_Index = 1;
	}

	for(int i = (pT->m_Width*(pT->m_Height-2)); i < pT->m_Width*pT->m_Height; ++i)
		pT->m_pTiles[i].m_Index = 1;
}

void CEditor::UpdateAndRender()
{
	static float s_MouseX = 0.0f;
	static float s_MouseY = 0.0f;

	if(m_Animate)
		m_AnimateTime = (time_get()-m_AnimateStart)/(float)time_freq();
	else
		m_AnimateTime = 0;
	ms_pUiGotContext = 0;

	// handle mouse movement
	float mx, my, Mwx, Mwy;
	float rx, ry;
	{
		Input()->MouseRelative(&rx, &ry);
		UI()->ConvertMouseMove(&rx, &ry);
		m_MouseDeltaX = rx;
		m_MouseDeltaY = ry;

		if(!m_LockMouse)
		{
			s_MouseX += rx;
			s_MouseY += ry;
		}

		s_MouseX = clamp(s_MouseX, 0.0f, UI()->Screen()->w);
		s_MouseY = clamp(s_MouseY, 0.0f, UI()->Screen()->h);

		// update the ui
		mx = s_MouseX;
		my = s_MouseY;
		Mwx = 0;
		Mwy = 0;

		// fix correct world x and y
		CLayerGroup *g = GetSelectedGroup();
		if(g)
		{
			float aPoints[4];
			g->Mapping(aPoints);

			float WorldWidth = aPoints[2]-aPoints[0];
			float WorldHeight = aPoints[3]-aPoints[1];

			Mwx = aPoints[0] + WorldWidth * (s_MouseX/UI()->Screen()->w);
			Mwy = aPoints[1] + WorldHeight * (s_MouseY/UI()->Screen()->h);
			m_MouseDeltaWx = m_MouseDeltaX*(WorldWidth / UI()->Screen()->w);
			m_MouseDeltaWy = m_MouseDeltaY*(WorldHeight / UI()->Screen()->h);
		}

		int Buttons = 0;
		if(Input()->KeyPressed(KEY_MOUSE_1)) Buttons |= 1;
		if(Input()->KeyPressed(KEY_MOUSE_2)) Buttons |= 2;
		if(Input()->KeyPressed(KEY_MOUSE_3)) Buttons |= 4;

		UI()->Update(mx,my,Mwx,Mwy,Buttons);
	}

	// toggle gui
	if(Input()->KeyDown(KEY_TAB))
		m_GuiActive = !m_GuiActive;

	if(Input()->KeyDown(KEY_F10))
		m_ShowMousePointer = false;

	Render();

	if(Input()->KeyDown(KEY_F10))
	{
		Graphics()->TakeScreenshot(0);
		m_ShowMousePointer = true;
	}

	Input()->ClearEvents();
}

IEditor *CreateEditor() { return new CEditor; }
