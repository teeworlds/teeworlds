
#include <base/math.h>

#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>

#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "menus.h"

int CMenus::DoButton_DemoPlayer(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, Checked ? 0.10f : 0.5f)*ButtonColorMul(pID), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, 14.0f, 0);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_DemoPlayer_Sprite(const void *pID, int SpriteId, int Checked, const CUIRect *pRect)
{
	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, Checked ? 0.10f : 0.5f)*ButtonColorMul(pID), CUI::CORNER_ALL, 5.0f);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEMOBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
	
	return UI()->DoButtonLogic(pID, "", Checked, pRect);
}

void CMenus::RenderDemoPlayer(CUIRect MainView)
{
	const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
	
	const float SeekBarHeight = 15.0f;
	const float ButtonbarHeight = 20.0f;
	const float NameBarHeight = 20.0f;
	const float Margins = 5.0f;
	float TotalHeight;
	
	if(m_MenuActive)
		TotalHeight = SeekBarHeight+ButtonbarHeight+NameBarHeight+Margins*3;
	else
		TotalHeight = SeekBarHeight+Margins*2;
	
	MainView.HSplitBottom(TotalHeight, 0, &MainView);
	MainView.VSplitLeft(250.0f, 0, &MainView);
	MainView.VSplitRight(250.0f, &MainView, 0);
	
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);
		
	MainView.Margin(5.0f, &MainView);
	
	CUIRect SeekBar, ButtonBar, NameBar;
	
	int CurrentTick = pInfo->m_CurrentTick - pInfo->m_FirstTick;
	int TotalTicks = pInfo->m_LastTick - pInfo->m_FirstTick;
	
	if(m_MenuActive)
	{
		MainView.HSplitTop(SeekBarHeight, &SeekBar, &ButtonBar);
		ButtonBar.HSplitTop(Margins, 0, &ButtonBar);
		ButtonBar.HSplitBottom(NameBarHeight, &ButtonBar, &NameBar);
		NameBar.HSplitTop(4.0f, 0, &NameBar);
	}
	else
		SeekBar = MainView;

	// do seekbar
	{
		static int s_SeekBarId = 0;
		void *id = &s_SeekBarId;
		char aBuffer[128];
		
		RenderTools()->DrawUIRect(&SeekBar, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);
		
		float Amount = CurrentTick/(float)TotalTicks;
		
		CUIRect FilledBar = SeekBar;
		FilledBar.w = 10.0f + (FilledBar.w-10.0f)*Amount;
		
		RenderTools()->DrawUIRect(&FilledBar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
		
		str_format(aBuffer, sizeof(aBuffer), "%d:%02d / %d:%02d",
			CurrentTick/SERVER_TICK_SPEED/60, (CurrentTick/SERVER_TICK_SPEED)%60,
			TotalTicks/SERVER_TICK_SPEED/60, (TotalTicks/SERVER_TICK_SPEED)%60);
		UI()->DoLabel(&SeekBar, aBuffer, SeekBar.h*0.70f, 0);

		// do the logic
	    int Inside = UI()->MouseInside(&SeekBar);
			
		if(UI()->ActiveItem() == id)
		{
			if(!UI()->MouseButton(0))
				UI()->SetActiveItem(0);
			else
			{
				static float PrevAmount = 0.0f;
				float Amount = (UI()->MouseX()-SeekBar.x)/(float)SeekBar.w;
				if(Amount > 0 && Amount < 1.0f && PrevAmount != Amount)
				{
					PrevAmount = Amount;
					m_pClient->OnReset();
					m_pClient->m_SuppressEvents = true;
					DemoPlayer()->SetPos(Amount);
					m_pClient->m_SuppressEvents = false;
				}
			}
		}
		else if(UI()->HotItem() == id)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(id);
		}		
		
		if(Inside)
			UI()->SetHotItem(id);
	}	

	if(CurrentTick == TotalTicks)
	{
	DemoPlayer()->Pause();
	DemoPlayer()->SetPos(0);
	}

	if(m_MenuActive)
	{
		// do buttons
		CUIRect Button;
		
		// combined play and pause button
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_PlayPauseButton = 0;
		if(!pInfo->m_Paused)
		{
			if(DoButton_DemoPlayer_Sprite(&s_PlayPauseButton, SPRITE_DEMOBUTTON_PAUSE, pInfo->m_Paused, &Button))
				DemoPlayer()->Pause();
		}
		else
		{
			if(DoButton_DemoPlayer_Sprite(&s_PlayPauseButton, SPRITE_DEMOBUTTON_PLAY, !pInfo->m_Paused, &Button))
				DemoPlayer()->Unpause();
		}
		
		// stop button
		
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_ResetButton = 0;
		if(DoButton_DemoPlayer_Sprite(&s_ResetButton, SPRITE_DEMOBUTTON_STOP, false, &Button))
		{
			DemoPlayer()->Pause(); 
			DemoPlayer()->SetPos(0);
		}

		// slowdown
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_SlowDownButton = 0;
		if(DoButton_DemoPlayer_Sprite(&s_SlowDownButton, SPRITE_DEMOBUTTON_SLOWER, 0, &Button))
		{
			if(pInfo->m_Speed > 4.0f) DemoPlayer()->SetSpeed(4.0f);
			else if(pInfo->m_Speed > 2.0f) DemoPlayer()->SetSpeed(2.0f);
			else if(pInfo->m_Speed > 1.0f) DemoPlayer()->SetSpeed(1.0f);
			else if(pInfo->m_Speed > 0.5f) DemoPlayer()->SetSpeed(0.5f);
			else DemoPlayer()->SetSpeed(0.05f);
		}
		
		// fastforward
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_FastForwardButton = 0;
		if(DoButton_DemoPlayer_Sprite(&s_FastForwardButton, SPRITE_DEMOBUTTON_FASTER, 0, &Button))
		{
			if(pInfo->m_Speed < 0.5f) DemoPlayer()->SetSpeed(0.5f);
			else if(pInfo->m_Speed < 1.0f) DemoPlayer()->SetSpeed(1.0f);
			else if(pInfo->m_Speed < 2.0f) DemoPlayer()->SetSpeed(2.0f);
			else if(pInfo->m_Speed < 4.0f) DemoPlayer()->SetSpeed(4.0f);
			else DemoPlayer()->SetSpeed(8.0f);
		}

		// speed meter
		ButtonBar.VSplitLeft(Margins*3, 0, &ButtonBar);
		char aBuffer[64];
		if(pInfo->m_Speed >= 1.0f)
			str_format(aBuffer, sizeof(aBuffer), "x%.0f", pInfo->m_Speed);
		else
			str_format(aBuffer, sizeof(aBuffer), "x%.1f", pInfo->m_Speed);
		UI()->DoLabel(&ButtonBar, aBuffer, Button.h*0.7f, -1);

		// close button
		ButtonBar.VSplitRight(ButtonbarHeight*3, &ButtonBar, &Button);
		static int s_ExitButton = 0;
		if(DoButton_DemoPlayer(&s_ExitButton, Localize("Close"), 0, &Button))
			Client()->Disconnect();

		// demo name
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Demofile: %s", DemoPlayer()->GetDemoName());
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, NameBar.x, NameBar.y, Button.h*0.5f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = MainView.w;
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}
}

static CUIRect gs_ListBoxOriginalView;
static CUIRect gs_ListBoxView;
static float gs_ListBoxRowHeight;
static int gs_ListBoxItemIndex;
static int gs_ListBoxSelectedIndex;
static int gs_ListBoxNewSelected;
static int gs_ListBoxDoneEvents;
static int gs_ListBoxNumItems;
static int gs_ListBoxItemsPerRow;
static float gs_ListBoxScrollValue;
static bool gs_ListBoxItemActivated;

void CMenus::UiDoListboxStart(void *pId, const CUIRect *pRect, float RowHeight, const char *pTitle, const char *pBottomText, int NumItems,
								int ItemsPerRow, int SelectedIndex, float ScrollValue)
{
	CUIRect Scroll, Row;
	CUIRect View = *pRect;
	CUIRect Header, Footer;
	
	// draw header
	View.HSplitTop(ms_ListheaderHeight, &Header, &View);
	RenderTools()->DrawUIRect(&Header, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f); 
	UI()->DoLabel(&Header, pTitle, Header.h*ms_FontmodHeight, 0);

	// draw footers
	View.HSplitBottom(ms_ListheaderHeight, &View, &Footer);
	RenderTools()->DrawUIRect(&Footer, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f); 
	Footer.VSplitLeft(10.0f, 0, &Footer);
	UI()->DoLabel(&Footer, pBottomText, Header.h*ms_FontmodHeight, 0);

	// background
	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	// prepare the scroll
	View.VSplitRight(15, &View, &Scroll);

	// setup the variables	
	gs_ListBoxOriginalView = View;
	gs_ListBoxSelectedIndex = SelectedIndex;
	gs_ListBoxNewSelected = SelectedIndex;
	gs_ListBoxItemIndex = 0;
	gs_ListBoxRowHeight = RowHeight;
	gs_ListBoxNumItems = NumItems;
	gs_ListBoxItemsPerRow = ItemsPerRow;
	gs_ListBoxDoneEvents = 0;
	gs_ListBoxScrollValue = ScrollValue;
	gs_ListBoxItemActivated = false;

	// do the scrollbar
	View.HSplitTop(gs_ListBoxRowHeight, &Row, 0);
	
	int NumViewable = (int)(gs_ListBoxOriginalView.h/Row.h) + 1;
	int Num = (NumItems+gs_ListBoxItemsPerRow-1)/gs_ListBoxItemsPerRow-NumViewable+1;
	if(Num < 0)
		Num = 0;
	if(Num > 0)
	{
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
			gs_ListBoxScrollValue -= 1.0f/Num;
		if(Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			gs_ListBoxScrollValue += 1.0f/Num;
		
		if(gs_ListBoxScrollValue < 0.0f) gs_ListBoxScrollValue = 0.0f;
		if(gs_ListBoxScrollValue > 1.0f) gs_ListBoxScrollValue = 1.0f;
	}
		
	Scroll.HMargin(5.0f, &Scroll);
	gs_ListBoxScrollValue = DoScrollbarV(pId, &Scroll, gs_ListBoxScrollValue);
	
	// the list
	gs_ListBoxView = gs_ListBoxOriginalView;
	gs_ListBoxView.VMargin(5.0f, &gs_ListBoxView);
	UI()->ClipEnable(&gs_ListBoxView);
	gs_ListBoxView.y -= gs_ListBoxScrollValue*Num*Row.h;
}

CMenus::CListboxItem CMenus::UiDoListboxNextRow()
{
	static CUIRect s_RowView;
	CListboxItem Item = {0};
	if(gs_ListBoxItemIndex%gs_ListBoxItemsPerRow == 0)
		gs_ListBoxView.HSplitTop(gs_ListBoxRowHeight /*-2.0f*/, &s_RowView, &gs_ListBoxView);

	s_RowView.VSplitLeft(s_RowView.w/(gs_ListBoxItemsPerRow-gs_ListBoxItemIndex%gs_ListBoxItemsPerRow), &Item.m_Rect, &s_RowView);

	Item.m_Visible = 1;
	//item.rect = row;
	
	Item.m_HitRect = Item.m_Rect;
	
	//CUIRect select_hit_box = item.rect;

	if(gs_ListBoxSelectedIndex == gs_ListBoxItemIndex)
		Item.m_Selected = 1;
	
	// make sure that only those in view can be selected
	if(Item.m_Rect.y+Item.m_Rect.h > gs_ListBoxOriginalView.y)
	{
		
		if(Item.m_HitRect.y < Item.m_HitRect.y) // clip the selection
		{
			Item.m_HitRect.h -= gs_ListBoxOriginalView.y-Item.m_HitRect.y;
			Item.m_HitRect.y = gs_ListBoxOriginalView.y;
		}
		
	}
	else
		Item.m_Visible = 0;

	// check if we need to do more
	if(Item.m_Rect.y > gs_ListBoxOriginalView.y+gs_ListBoxOriginalView.h)
		Item.m_Visible = 0;
		
	gs_ListBoxItemIndex++;
	return Item;
}

CMenus::CListboxItem CMenus::UiDoListboxNextItem(void *pId, bool Selected)
{
	int ThisItemIndex = gs_ListBoxItemIndex;
	if(Selected)
	{
		if(gs_ListBoxSelectedIndex == gs_ListBoxNewSelected)
			gs_ListBoxNewSelected = ThisItemIndex;
		gs_ListBoxSelectedIndex = ThisItemIndex;
	}
	
	CListboxItem Item = UiDoListboxNextRow();

	if(Item.m_Visible && UI()->DoButtonLogic(pId, "", gs_ListBoxSelectedIndex == gs_ListBoxItemIndex, &Item.m_HitRect))
		gs_ListBoxNewSelected = ThisItemIndex;
	
	// process input, regard selected index
	if(gs_ListBoxSelectedIndex == ThisItemIndex)
	{
		if(!gs_ListBoxDoneEvents)
		{
			gs_ListBoxDoneEvents = 1;

			if(m_EnterPressed || (Input()->MouseDoubleClick() && UI()->ActiveItem() == pId))
			{
				gs_ListBoxItemActivated = true;
				UI()->SetActiveItem(0);
			}
			else
			{			
				for(int i = 0; i < m_NumInputEvents; i++)
				{
					int NewIndex = -1;
					if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
					{
						if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = gs_ListBoxNewSelected + 1;
						if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = gs_ListBoxNewSelected - 1;
					}
					if(NewIndex > -1 && NewIndex < gs_ListBoxNumItems)
					{
						// scroll
						int NumViewable = (int)(gs_ListBoxOriginalView.h/gs_ListBoxRowHeight) + 1;
						int ScrollNum = (gs_ListBoxNumItems+gs_ListBoxItemsPerRow-1)/gs_ListBoxItemsPerRow-NumViewable+1;
						if(ScrollNum > 0 && NewIndex/gs_ListBoxItemsPerRow-gs_ListBoxSelectedIndex/gs_ListBoxItemsPerRow)
						{
							// TODO: make the scrolling better
							if(NewIndex - gs_ListBoxSelectedIndex > 0)
								gs_ListBoxScrollValue += 1.0f/ScrollNum;
							else
								gs_ListBoxScrollValue -= 1.0f/ScrollNum;
							if(gs_ListBoxScrollValue < 0.0f) gs_ListBoxScrollValue = 0.0f;
							if(gs_ListBoxScrollValue > 1.0f) gs_ListBoxScrollValue = 1.0f;
						}
						
						gs_ListBoxNewSelected = NewIndex;
					}
				}
			}
		}
		
		//selected_index = i;
		CUIRect r = Item.m_Rect;
		r.Margin(1.5f, &r);
		RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
	}	

	return Item;
}

int CMenus::UiDoListboxEnd(float *pScrollValue, bool *pItemActivated)
{
	UI()->ClipDisable();
	if(pScrollValue)
		*pScrollValue = gs_ListBoxScrollValue;
	if(pItemActivated)
		*pItemActivated = gs_ListBoxItemActivated;
	return gs_ListBoxNewSelected;
}

struct FETCH_CALLBACKINFO
{
	CMenus *m_pSelf;
	const char *m_pPrefix;
};

void CMenus::DemolistFetchCallback(const char *pName, int IsDir, void *pUser)
{
	if(pName[0] == '.')
		return;
	
	FETCH_CALLBACKINFO *pInfo = (FETCH_CALLBACKINFO *)pUser;
	
	CDemoItem Item;
	str_format(Item.m_aFilename, sizeof(Item.m_aFilename), "%s/%s", pInfo->m_pPrefix, pName);
	str_copy(Item.m_aName, pName, sizeof(Item.m_aName));
	pInfo->m_pSelf->m_lDemos.add(Item);
}

void CMenus::DemolistPopulate()
{
	m_lDemos.clear();
	
	
	if(str_comp(m_aCurrentDemoFolder, "demos") != 0) //add parent folder
	{
		CDemoItem Item;
		str_copy(Item.m_aName, "..", sizeof(Item.m_aName));
		str_copy(Item.m_aFilename, "..", sizeof(Item.m_aFilename));
		m_lDemos.add(Item);
	}
	
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%s/%s", Client()->UserDirectory(), m_aCurrentDemoFolder);
	
	FETCH_CALLBACKINFO Info = {this, aBuf};
	fs_listdir(aBuf, DemolistFetchCallback, &Info);
	Info.m_pPrefix = m_aCurrentDemoFolder;
	fs_listdir(m_aCurrentDemoFolder, DemolistFetchCallback, &Info);
}


void CMenus::RenderDemoList(CUIRect MainView)
{
	static int s_SelectedItem = -1;
	static int s_Inited = 0;
	if(!s_Inited)
	{
		DemolistPopulate();
		s_Inited = 1;
		if(m_lDemos.size() > 0)
			s_SelectedItem = 0;
	}

	bool IsDir = false;
	if(s_SelectedItem >= 0 && s_SelectedItem < m_lDemos.size())
	{
		if(str_comp(m_lDemos[s_SelectedItem].m_aName, "..") == 0 || fs_is_dir(m_lDemos[s_SelectedItem].m_aFilename))
			IsDir = true;
	}

	// delete demo
	if(m_DemolistDelEntry)
	{
		if(s_SelectedItem >= 0 && s_SelectedItem < m_lDemos.size() && !IsDir)
		{
			Storage()->RemoveFile(m_lDemos[s_SelectedItem].m_aFilename);
			DemolistPopulate();
			s_SelectedItem = s_SelectedItem-1 < 0 ? m_lDemos.size() > 0 ? 0 : -1 : s_SelectedItem-1;
		}
		m_DemolistDelEntry = false;
	}
	
	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);
	
	CUIRect ButtonBar;
	MainView.HSplitBottom(ms_ButtonHeight+5.0f, &MainView, &ButtonBar);
	ButtonBar.HSplitTop(5.0f, 0, &ButtonBar);
	
	static int s_DemoListId = 0;
	static float s_ScrollValue = 0;
	
	UiDoListboxStart(&s_DemoListId, &MainView, 17.0f, Localize("Demos"), "", m_lDemos.size(), 1, s_SelectedItem, s_ScrollValue);
	//for(int i = 0; i < num_demos; i++)
	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem((void*)(&r.front()));
		if(Item.m_Visible)
			UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight, -1);
	}
	bool Activated = false;
	s_SelectedItem = UiDoListboxEnd(&s_ScrollValue, &Activated);
	
	CUIRect RefreshRect, PlayRect, DeleteRect;
	ButtonBar.VSplitRight(130.0f, &ButtonBar, &PlayRect);
	ButtonBar.VSplitLeft(130.0f, &RefreshRect, &ButtonBar);
	ButtonBar.VSplitLeft(10.0f, &DeleteRect, &ButtonBar);
	ButtonBar.VSplitLeft(120.0f, &DeleteRect, &ButtonBar);
	
	static int s_RefreshButton = 0;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &RefreshRect))
	{
		DemolistPopulate();
	}
	
	static int s_PlayButton = 0;
	char aTitleButton[10];
	if(IsDir)
		str_copy(aTitleButton, Localize("Open"), sizeof(aTitleButton));
	else
		str_copy(aTitleButton, Localize("Play"), sizeof(aTitleButton));
	
	if(DoButton_Menu(&s_PlayButton, aTitleButton, 0, &PlayRect) || Activated)
	{
		if(s_SelectedItem >= 0 && s_SelectedItem < m_lDemos.size())
		{
			if(str_comp(m_lDemos[s_SelectedItem].m_aName, "..") == 0) //parent folder
			{
				DemoSetParentDirectory();
				DemolistPopulate();
				s_SelectedItem = m_lDemos.size() > 0 ? 0 : -1;
			}
			else if(IsDir) //folder
			{
				char aTemp[256];
				str_copy(aTemp, m_aCurrentDemoFolder, sizeof(aTemp));
				str_format(m_aCurrentDemoFolder, sizeof(m_aCurrentDemoFolder), "%s/%s", aTemp, m_lDemos[s_SelectedItem].m_aName);
				DemolistPopulate();
				s_SelectedItem = m_lDemos.size() > 0 ? 0 : -1;
			}
			else //file
			{
				const char *pError = Client()->DemoPlayer_Play(m_lDemos[s_SelectedItem].m_aFilename);
				if(pError)
					PopupMessage(Localize("Error"), str_comp(pError, "error loading demo") ? pError : Localize("error loading demo"), Localize("Ok"));
			}
		}
	}
	
	if(!IsDir)
	{
		static int s_DeleteButton = 0;
		if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &DeleteRect) || m_DeletePressed)
		{
			if(s_SelectedItem >= 0 && s_SelectedItem < m_lDemos.size())
				m_Popup = POPUP_DELETE_DEMO;
		}
	}
}

void CMenus::DemoSetParentDirectory()
{
	int Stop = 0;
	for(int i = 0; i < 256; i++)
	{
		if(m_aCurrentDemoFolder[i] == '/')
			Stop = i;
	}
	
	//keeps chars which are before the last '/' and remove chars which are after
	for(int i = 0; i < 256; i++)
	{
		if(i >= Stop)
			m_aCurrentDemoFolder[i] = '\0';
	}
}
