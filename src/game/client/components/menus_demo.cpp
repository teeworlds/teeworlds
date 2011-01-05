/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

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
				if(Amount > 0.0f && Amount < 1.0f && absolute(PrevAmount-Amount) >= 0.01f)
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
			else if(pInfo->m_Speed > 0.75f) DemoPlayer()->SetSpeed(0.75f);
			else if(pInfo->m_Speed > 0.5f) DemoPlayer()->SetSpeed(0.5f);
			else if(pInfo->m_Speed > 0.25f) DemoPlayer()->SetSpeed(0.25f);
			else if(pInfo->m_Speed > 0.1f) DemoPlayer()->SetSpeed(0.1f);
			else DemoPlayer()->SetSpeed(0.05f);
		}
		
		// fastforward
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_FastForwardButton = 0;
		if(DoButton_DemoPlayer_Sprite(&s_FastForwardButton, SPRITE_DEMOBUTTON_FASTER, 0, &Button))
		{
			if(pInfo->m_Speed < 0.1f) DemoPlayer()->SetSpeed(0.1f);
			else if(pInfo->m_Speed < 0.25f) DemoPlayer()->SetSpeed(0.25f);
			else if(pInfo->m_Speed < 0.5f) DemoPlayer()->SetSpeed(0.5f);
			else if(pInfo->m_Speed < 0.75f) DemoPlayer()->SetSpeed(0.75f);
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
			str_format(aBuffer, sizeof(aBuffer), "x%.2f", pInfo->m_Speed);
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

	s_RowView.VSplitLeft(s_RowView.w/(gs_ListBoxItemsPerRow-gs_ListBoxItemIndex%gs_ListBoxItemsPerRow)/(UI()->Scale()), &Item.m_Rect, &s_RowView);

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
						float Offset = (NewIndex/gs_ListBoxItemsPerRow-gs_ListBoxNewSelected/gs_ListBoxItemsPerRow)*gs_ListBoxRowHeight;
						int Scroll = gs_ListBoxOriginalView.y > Item.m_Rect.y+Offset ? -1 :
										gs_ListBoxOriginalView.y+gs_ListBoxOriginalView.h < Item.m_Rect.y+Item.m_Rect.h+Offset ? 1 : 0;
						if(Scroll)
						{
							int NumViewable = (int)(gs_ListBoxOriginalView.h/gs_ListBoxRowHeight) + 1;
							int ScrollNum = (gs_ListBoxNumItems+gs_ListBoxItemsPerRow-1)/gs_ListBoxItemsPerRow-NumViewable+1;
							if(Scroll < 0)
							{
								int Num = (gs_ListBoxOriginalView.y-Item.m_Rect.y-Offset+gs_ListBoxRowHeight-1.0f)/gs_ListBoxRowHeight;
								gs_ListBoxScrollValue -= (1.0f/ScrollNum)*Num;
							}
							else
							{
								int Num = (Item.m_Rect.y+Item.m_Rect.h+Offset-(gs_ListBoxOriginalView.y+gs_ListBoxOriginalView.h)+gs_ListBoxRowHeight-1.0f)/
									gs_ListBoxRowHeight;
								gs_ListBoxScrollValue += (1.0f/ScrollNum)*Num;
							}
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

void CMenus::DemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	int Length = str_length(pName);
	if((pName[0] == '.' && (pName[1] == 0 ||
		(pName[1] == '.' && pName[2] == 0 && !str_comp(pSelf->m_aCurrentDemoFolder, "demos")))) ||
		(!IsDir && (Length < 5 || str_comp(pName+Length-5, ".demo"))))
		return;
	
	CDemoItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	if(IsDir)
	{
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", pName);
		Item.m_aMap[0] = 0;
		Item.m_Valid = false;
	}
	else
	{
		str_copy(Item.m_aName, pName, min(static_cast<int>(sizeof(Item.m_aName)), Length-4));
		Item.m_InfosLoaded = false;
	}
	Item.m_IsDir = IsDir != 0;
	Item.m_StorageType = StorageType;
	pSelf->m_lDemos.add(Item);
}

void CMenus::DemolistPopulate()
{
	m_lDemos.clear();
	if(!str_comp(m_aCurrentDemoFolder, "demos"))
		m_DemolistStorageType = IStorage::TYPE_ALL;
	Storage()->ListDirectory(m_DemolistStorageType, m_aCurrentDemoFolder, DemolistFetchCallback, this);
}

void CMenus::DemolistOnUpdate(bool Reset)
{
	m_DemolistSelectedIndex = Reset ? m_lDemos.size() > 0 ? 0 : -1 :
										m_DemolistSelectedIndex >= m_lDemos.size() ? m_lDemos.size()-1 : m_DemolistSelectedIndex;
	m_DemolistSelectedIsDir = m_DemolistSelectedIndex < 0 ? false : m_lDemos[m_DemolistSelectedIndex].m_IsDir;
}

void CMenus::RenderDemoList(CUIRect MainView)
{
	static int s_Inited = 0;
	if(!s_Inited)
	{
		DemolistPopulate();
		DemolistOnUpdate(true);
		s_Inited = 1;
	}

	char aFooterLabel[128] = {0};
	if(m_DemolistSelectedIndex >= 0)
	{
		CDemoItem *Item = &m_lDemos[m_DemolistSelectedIndex];
		if(str_comp(Item->m_aFilename, "..") == 0)
			str_copy(aFooterLabel, Localize("Parent Folder"), sizeof(aFooterLabel));
		else if(m_DemolistSelectedIsDir)
			str_copy(aFooterLabel, Localize("Folder"), sizeof(aFooterLabel));
		else
		{
			if(!Item->m_InfosLoaded)
			{
				char aBuffer[512];
				str_format(aBuffer, sizeof(aBuffer), "%s/%s", m_aCurrentDemoFolder, Item->m_aFilename);
				Item->m_Valid = DemoPlayer()->GetDemoInfo(Storage(), aBuffer, Item->m_StorageType, Item->m_aMap, sizeof(Item->m_aMap));
				Item->m_InfosLoaded = true;
			}
			if(!Item->m_Valid)
				str_copy(aFooterLabel, Localize("Invalid Demo"), sizeof(aFooterLabel));
			else
				str_format(aFooterLabel, sizeof(aFooterLabel), "%s: %s", Localize("Map"), Item->m_aMap);
		}
	}
	
	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);
	
	CUIRect ButtonBar, RefreshRect, PlayRect, DeleteRect, FileIcon;
	MainView.HSplitBottom(ms_ButtonHeight+5.0f, &MainView, &ButtonBar);
	ButtonBar.HSplitTop(5.0f, 0, &ButtonBar);
	ButtonBar.VSplitRight(130.0f, &ButtonBar, &PlayRect);
	ButtonBar.VSplitLeft(130.0f, &RefreshRect, &ButtonBar);
	ButtonBar.VSplitLeft(10.0f, &DeleteRect, &ButtonBar);
	ButtonBar.VSplitLeft(120.0f, &DeleteRect, &ButtonBar);
	
	static int s_DemoListId = 0;
	static float s_ScrollValue = 0;
	UiDoListboxStart(&s_DemoListId, &MainView, 17.0f, Localize("Demos"), aFooterLabel, m_lDemos.size(), 1, m_DemolistSelectedIndex, s_ScrollValue);
	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem((void*)(&r.front()));
		if(Item.m_Visible)
		{
			Item.m_Rect.VSplitLeft(Item.m_Rect.h, &FileIcon, &Item.m_Rect);
			Item.m_Rect.VSplitLeft(5.0f, 0, &Item.m_Rect);
			DoButton_Icon(IMAGE_FILEICONS, r.front().m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_DEMO1, &FileIcon);
			UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight, -1);
		}
	}
	bool Activated = false;
	m_DemolistSelectedIndex = UiDoListboxEnd(&s_ScrollValue, &Activated);
	DemolistOnUpdate(false);
	
	static int s_RefreshButton = 0;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &RefreshRect))
	{
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	static int s_PlayButton = 0;
	if(DoButton_Menu(&s_PlayButton, m_DemolistSelectedIsDir?Localize("Open"):Localize("Play"), 0, &PlayRect) || Activated)
	{
		if(m_DemolistSelectedIndex >= 0)
		{
			if(m_DemolistSelectedIsDir)	// folder
			{
				if(str_comp(m_lDemos[m_DemolistSelectedIndex].m_aFilename, "..") == 0)	// parent folder
					fs_parent_dir(m_aCurrentDemoFolder);
				else	// sub folder
				{
					char aTemp[256];
					str_copy(aTemp, m_aCurrentDemoFolder, sizeof(aTemp));
					str_format(m_aCurrentDemoFolder, sizeof(m_aCurrentDemoFolder), "%s/%s", aTemp, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					m_DemolistStorageType = m_lDemos[m_DemolistSelectedIndex].m_StorageType;
				}
				DemolistPopulate();
				DemolistOnUpdate(true);
			}
			else // file
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
				const char *pError = Client()->DemoPlayer_Play(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType);
				if(pError)
					PopupMessage(Localize("Error"), str_comp(pError, "error loading demo") ? pError : Localize("Error loading demo"), Localize("Ok"));
				else
				{
					UI()->SetActiveItem(0);
					return;
				}
			}
		}
	}
	
	if(!m_DemolistSelectedIsDir)
	{
		static int s_DeleteButton = 0;
		if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &DeleteRect) || m_DeletePressed)
		{
			if(m_DemolistSelectedIndex >= 0)
			{
				UI()->SetActiveItem(0);
				m_Popup = POPUP_DELETE_DEMO;
			}
		}
	}
}
