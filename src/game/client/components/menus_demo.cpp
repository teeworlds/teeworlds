/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>

#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/client/render.h>
#include <game/client/gameclient.h>

#include <game/client/ui.h>

#include <generated/client_data.h>

#include "maplayers.h"
#include "menus.h"

int CMenus::DoButton_DemoPlayer(CButtonContainer *pBC, const char *pText, const CUIRect *pRect)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds);

	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, 0.5f+(Fade/Seconds)*0.25f), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, 14.0f, CUI::ALIGN_CENTER);
	return UI()->DoButtonLogic(pBC->GetID(), pText, false, pRect);
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
	MainView.VSplitLeft(50.0f, 0, &MainView);
	MainView.VSplitRight(450.0f, &MainView, 0);

	if (m_SeekBarActive || m_MenuActive) // only draw the background if SeekBar or Menu is active
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), CUI::CORNER_T, 10.0f);

	MainView.Margin(5.0f, &MainView);

	CUIRect SeekBar, ButtonBar, NameBar;

	int CurrentTick = pInfo->m_CurrentTick - pInfo->m_FirstTick;
	int TotalTicks = pInfo->m_LastTick - pInfo->m_FirstTick;

	// we can toggle the seekbar using CTRL
	if(!m_MenuActive && (Input()->KeyPress(KEY_LCTRL) || Input()->KeyPress(KEY_RCTRL)))
	{
		if (m_SeekBarActive)
			m_SeekBarActive = false;
		else
		{
			m_SeekBarActive = true;
			m_SeekBarActivatedTime = time_get(); // stores at which point of time the seekbar was activated, so we can automatically hide it after few seconds
		}
	}

	if(m_SeekBarActivatedTime < time_get() - 5*time_freq())
		m_SeekBarActive = false;

	if(m_MenuActive)
	{
		MainView.HSplitTop(SeekBarHeight, &SeekBar, &ButtonBar);
		ButtonBar.HSplitTop(Margins, 0, &ButtonBar);
		ButtonBar.HSplitBottom(NameBarHeight, &ButtonBar, &NameBar);
		NameBar.HSplitTop(4.0f, 0, &NameBar);
		m_SeekBarActive = true;
		m_SeekBarActivatedTime = time_get();
	}
	else
		SeekBar = MainView;

	// do seekbar
	if(m_SeekBarActive || m_MenuActive)
	{
		static int s_SeekBarID = 0;
		void *id = &s_SeekBarID;
		char aBuffer[128];
		const float Rounding = 5.0f;

		// draw seek bar
		RenderTools()->DrawUIRect(&SeekBar, vec4(0,0,0,0.5f), CUI::CORNER_ALL, Rounding);

		// draw filled bar
		float Amount = CurrentTick/(float)TotalTicks;
		CUIRect FilledBar = SeekBar;
		FilledBar.w = max(2*Rounding, FilledBar.w*Amount);
		RenderTools()->DrawUIRect(&FilledBar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, Rounding);

		// draw markers
		for(int i = 0; i < pInfo->m_NumTimelineMarkers; i++)
		{
			float Ratio = (pInfo->m_aTimelineMarkers[i]-pInfo->m_FirstTick) / (float)TotalTicks;
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(SeekBar.x + (SeekBar.w-2*Rounding)*Ratio, SeekBar.y, UI()->PixelSize(), SeekBar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// draw time
		str_format(aBuffer, sizeof(aBuffer), "%d:%02d / %d:%02d",
			CurrentTick/SERVER_TICK_SPEED/60, (CurrentTick/SERVER_TICK_SPEED)%60,
			TotalTicks/SERVER_TICK_SPEED/60, (TotalTicks/SERVER_TICK_SPEED)%60);
		UI()->DoLabel(&SeekBar, aBuffer, SeekBar.h*0.70f, CUI::ALIGN_CENTER);

		// do the logic
		int Inside = UI()->MouseInside(&SeekBar);

		if(UI()->CheckActiveItem(id))
		{
			if(!UI()->MouseButton(0))
				UI()->SetActiveItem(0);
			else
			{
				static float PrevAmount = 0.0f;
				float Amount = (UI()->MouseX()-SeekBar.x)/(float)SeekBar.w;
				if(Amount > 0.0f && Amount < 1.0f && absolute(PrevAmount-Amount) >= (1.0f/UI()->Screen()->w))
				{
					PrevAmount = Amount;
					m_pClient->OnReset();
					m_pClient->m_SuppressEvents = true;
					DemoPlayer()->SetPos(Amount);
					m_pClient->m_SuppressEvents = false;
					m_pClient->m_pMapLayersBackGround->EnvelopeUpdate();
					m_pClient->m_pMapLayersForeGround->EnvelopeUpdate();
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
		m_pClient->OnReset();
		DemoPlayer()->Pause();
		DemoPlayer()->SetPos(0);
	}

	bool IncreaseDemoSpeed = false, DecreaseDemoSpeed = false;

	//add spacebar for toggling Play/Pause
	if(Input()->KeyPress(KEY_SPACE))
	{
		if(!pInfo->m_Paused)
			DemoPlayer()->Pause();
		else
			DemoPlayer()->Unpause();
	}

	if(m_MenuActive)
	{
		// do buttons
		CUIRect Button;

		// combined play and pause button
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static CButtonContainer s_PlayPauseButton;
		if(!pInfo->m_Paused)
		{
			if(DoButton_SpriteID(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PAUSE, false, &Button, CUI::CORNER_ALL))
				DemoPlayer()->Pause();
		}
		else
		{
			if(DoButton_SpriteID(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PLAY, false, &Button, CUI::CORNER_ALL))
				DemoPlayer()->Unpause();
		}

		// stop button

		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static CButtonContainer s_ResetButton;
		if(DoButton_SpriteID(&s_ResetButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_STOP, false, &Button, CUI::CORNER_ALL))
		{
			m_pClient->OnReset();
			DemoPlayer()->Pause();
			DemoPlayer()->SetPos(0);
		}

		// slowdown
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static CButtonContainer s_SlowDownButton;
		if(DoButton_SpriteID(&s_SlowDownButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_SLOWER, false, &Button, CUI::CORNER_ALL))
			DecreaseDemoSpeed = true;

		// fastforward
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static CButtonContainer s_FastForwardButton;
		if(DoButton_SpriteID(&s_FastForwardButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_FASTER, false, &Button, CUI::CORNER_ALL))
			IncreaseDemoSpeed = true;

		// speed meter
		ButtonBar.VSplitLeft(Margins*3, 0, &ButtonBar);
		char aBuffer[64];
		if(pInfo->m_Speed >= 1.0f)
			str_format(aBuffer, sizeof(aBuffer), "x%.0f", pInfo->m_Speed);
		else
			str_format(aBuffer, sizeof(aBuffer), "x%.2f", pInfo->m_Speed);
		UI()->DoLabel(&ButtonBar, aBuffer, Button.h*0.7f, CUI::ALIGN_LEFT);

		// close button
		ButtonBar.VSplitRight(ButtonbarHeight*3, &ButtonBar, &Button);
		static CButtonContainer s_ExitButton;
		if(DoButton_DemoPlayer(&s_ExitButton, Localize("Close"), &Button))
			Client()->Disconnect();

		// demo name
		char aDemoName[64] = {0};
		DemoPlayer()->GetDemoName(aDemoName, sizeof(aDemoName));
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), Localize("Demofile: %s"), aDemoName);
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, NameBar.x, NameBar.y, Button.h*0.5f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = MainView.w;
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	if(IncreaseDemoSpeed || Input()->KeyPress(KEY_MOUSE_WHEEL_UP) || Input()->KeyPress(KEY_PLUS) || Input()->KeyPress(KEY_KP_PLUS))
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
	else if(DecreaseDemoSpeed || Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) || Input()->KeyPress(KEY_MINUS) || Input()->KeyPress(KEY_KP_MINUS))
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
}

int CMenus::DemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CMenus *pSelf = (CMenus *)pUser;
	if(str_comp(pName, ".") == 0
		|| (str_comp(pName, "..") == 0 && str_comp(pSelf->m_aCurrentDemoFolder, "demos") == 0)
		|| (!IsDir && !str_endswith(pName, ".demo")))
	{
		return 0;
	}

	CDemoItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	if(IsDir)
	{
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", pName);
		Item.m_Valid = false;
	}
	else
	{
		str_truncate(Item.m_aName, sizeof(Item.m_aName), pName, str_length(pName) - 5);
		Item.m_InfosLoaded = false;
	}
	Item.m_IsDir = IsDir != 0;
	Item.m_StorageType = StorageType;
	pSelf->m_lDemos.add_unsorted(Item);

	return 0;
}

void CMenus::DemolistPopulate()
{
	m_lDemos.clear();
	if(!str_comp(m_aCurrentDemoFolder, "demos"))
		m_DemolistStorageType = IStorage::TYPE_ALL;
	Storage()->ListDirectory(m_DemolistStorageType, m_aCurrentDemoFolder, DemolistFetchCallback, this);
	m_lDemos.sort_range();
}

void CMenus::DemolistOnUpdate(bool Reset)
{
	m_DemolistSelectedIndex = Reset ? m_lDemos.size() > 0 ? 0 : -1 :
										m_DemolistSelectedIndex >= m_lDemos.size() ? m_lDemos.size()-1 : m_DemolistSelectedIndex;
	m_DemolistSelectedIsDir = m_DemolistSelectedIndex < 0 ? false : m_lDemos[m_DemolistSelectedIndex].m_IsDir;
}

inline int DemoGetMarkerCount(CDemoHeader Demo)
{
	int DemoMarkerCount = ((Demo.m_aNumTimelineMarkers[0]<<24)&0xFF000000) |
			((Demo.m_aNumTimelineMarkers[1]<<16)&0xFF0000) |
			((Demo.m_aNumTimelineMarkers[2]<<8)&0xFF00) |
			(Demo.m_aNumTimelineMarkers[3]&0xFF);
	return DemoMarkerCount;
}

void CMenus::RenderDemoList(CUIRect MainView)
{
	CUIRect BottomView;
	MainView.HSplitTop(20.0f, 0, &MainView);

	// back button
	RenderBackButton(MainView);

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), CUI::CORNER_ALL, 5.0f);
	BottomView.HSplitTop(20.f, 0, &BottomView);

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
				Item->m_Valid = DemoPlayer()->GetDemoInfo(Storage(), aBuffer, Item->m_StorageType, &Item->m_Info);
				Item->m_InfosLoaded = true;
			}
			if(!Item->m_Valid)
				str_copy(aFooterLabel, Localize("Invalid Demo"), sizeof(aFooterLabel));
			else
				str_copy(aFooterLabel, Localize("Demo details"), sizeof(aFooterLabel));
		}
	}

	static bool s_DemoDetailsActive = true;
	const int NumOptions = 8;
	const float ButtonHeight = 20.0f;
	const float ButtonSpacing = 2.0f;
	const float HMargin = 5.0f;
	const float BackgroundHeight = s_DemoDetailsActive ? (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*ButtonSpacing : ButtonHeight;

	CUIRect ListBox, Button, FileIcon;
	MainView.HSplitTop(MainView.h - BackgroundHeight - 2 * HMargin, &ListBox, &MainView);

	static CListBox s_ListBox(this);
	s_ListBox.DoHeader(&ListBox, Localize("Recorded"), GetListHeaderHeight());
	s_ListBox.DoStart(20.0f, m_lDemos.size(), 1, m_DemolistSelectedIndex);
	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = s_ListBox.DoNextItem((void*)(&r.front()));
		// marker count
		const CDemoItem& DemoItem = r.front();
		int DemoMarkerCount = 0;
		if(DemoItem.m_Valid && DemoItem.m_InfosLoaded)
			DemoMarkerCount = DemoGetMarkerCount(DemoItem.m_Info);

		if(Item.m_Visible)
		{
			Item.m_Rect.VSplitLeft(Item.m_Rect.h, &FileIcon, &Item.m_Rect);
			Item.m_Rect.VSplitLeft(5.0f, 0, &Item.m_Rect);
			FileIcon.Margin(3.0f, &FileIcon);
			FileIcon.x += 3.0f;

			vec4 IconColor = vec4(1, 1, 1, 1);
			if(!DemoItem.m_IsDir)
			{
				IconColor = vec4(0.6f, 0.6f, 0.6f, 1.0f); // not loaded
				if(DemoItem.m_Valid && DemoItem.m_InfosLoaded)
					IconColor = DemoMarkerCount > 0 ? vec4(0.5, 1, 0.5, 1) : vec4(1,1,1,1);
			}

			DoIconColor(IMAGE_FILEICONS, r.front().m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_DEMO1, &FileIcon, IconColor);
			if((&r.front() - m_lDemos.base_ptr()) == m_DemolistSelectedIndex) // selected
			{
				TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
				TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
				Item.m_Rect.y += 2.0f;
				UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			}
			else
			{
				Item.m_Rect.y += 2.0f;
				UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
			}
		}
	}
	bool Activated = false;
	m_DemolistSelectedIndex = s_ListBox.DoEnd(&Activated);
	DemolistOnUpdate(false);

	MainView.HSplitTop(HMargin, 0, &MainView);
	static int s_DemoDetailsDropdown = 0;
	if(!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid)
		DoIndependentDropdownMenu(&s_DemoDetailsDropdown, &MainView, aFooterLabel, ButtonHeight, RenderDemoDetails, &s_DemoDetailsActive);
	else
	{
		CUIRect Header;
		MainView.HSplitTop(ButtonHeight, &Header, &MainView);
		RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
		Header.y += 2.0f;
		UI()->DoLabel(&Header, aFooterLabel, ButtonHeight*ms_FontmodHeight*0.8f, CUI::ALIGN_CENTER);
	}

	// demo buttons
	int NumButtons = m_DemolistSelectedIsDir ? 2 : 4;
	float Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;
	float BackgroundWidth = ButtonWidth*(float)NumButtons+(float)(NumButtons-1)*Spacing;

	BottomView.VSplitRight(BackgroundWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, g_Config.m_ClMenuAlpha/100.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static CButtonContainer s_RefreshButton;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button) || (Input()->KeyPress(KEY_R) && (Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))))
	{
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	if(!m_DemolistSelectedIsDir)
	{
		BottomView.VSplitLeft(Spacing, 0, &BottomView);
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_DeleteButton;
		if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &Button) || m_DeletePressed)
		{
			if(m_DemolistSelectedIndex >= 0)
			{
				UI()->SetActiveItem(0);
				m_Popup = POPUP_DELETE_DEMO;
				return;
			}
		}

		BottomView.VSplitLeft(Spacing, 0, &BottomView);
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static CButtonContainer s_RenameButton;
		if(DoButton_Menu(&s_RenameButton, Localize("Rename"), 0, &Button))
		{
			if(m_DemolistSelectedIndex >= 0)
			{
				UI()->SetActiveItem(0);
				m_Popup = POPUP_RENAME_DEMO;
				str_copy(m_aCurrentDemoFile, m_lDemos[m_DemolistSelectedIndex].m_aFilename, sizeof(m_aCurrentDemoFile));
				return;
			}
		}
	}

	BottomView.VSplitLeft(Spacing, 0, &BottomView);
	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static CButtonContainer s_PlayButton;
	if(DoButton_Menu(&s_PlayButton, m_DemolistSelectedIsDir?Localize("Open"):Localize("Play", "DemoBrowser"), 0, &Button) || Activated)
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
					PopupMessage(Localize("Error loading demo"), pError, Localize("Ok"));
				else
				{
					UI()->SetActiveItem(0);
					return;
				}
			}
		}
	}
}

float CMenus::RenderDemoDetails(CUIRect View, void *pUser)
{
	CMenus *pSelf = (CMenus*)pUser;

	// render demo info
	if(!pSelf->m_DemolistSelectedIsDir && pSelf->m_DemolistSelectedIndex >= 0 && pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Valid)
	{
		CUIRect Button;
		
		const float ButtonHeight = 20.0f;
		const float Spacing = 2.0f;
		
		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		pSelf->DoInfoBox(&Button, Localize("Created"), pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aTimestamp);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		pSelf->DoInfoBox(&Button, Localize("Type"), pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aType);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		int Length = ((pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aLength[0]<<24)&0xFF000000) | ((pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aLength[1]<<16)&0xFF0000) |
					((pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aLength[2]<<8)&0xFF00) | (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aLength[3]&0xFF);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d:%02d", Length/60, Length%60);
		pSelf->DoInfoBox(&Button, Localize("Length"), aBuf);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		str_format(aBuf, sizeof(aBuf), "%d", pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_Version);
		pSelf->DoInfoBox(&Button, Localize("Version"), aBuf);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		pSelf->DoInfoBox(&Button, Localize("Netversion"), pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aNetversion);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		const int MarkerCount = DemoGetMarkerCount(pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info);
		str_format(aBuf, sizeof(aBuf), "%d", MarkerCount);
		if(MarkerCount > 0)
			pSelf->TextRender()->TextColor(0.5, 1, 0.5, 1);
		pSelf->DoInfoBox(&Button, Localize("Markers"), aBuf);
		pSelf->TextRender()->TextColor(1, 1, 1, 1);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		pSelf->DoInfoBox(&Button, Localize("Map"), pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapName);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		Button.VMargin(ButtonHeight, &Button);

		CUIRect ButtonRight;
		Button.VSplitMid(&Button, &ButtonRight);
		float Size = float((pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapSize[0]<<24) | (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapSize[1]<<16) |
							(pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapSize[2]<<8) | (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapSize[3]))/1024.0f;
		str_format(aBuf, sizeof(aBuf), Localize("%.3f KiB"), Size);
		pSelf->DoInfoBox(&Button, Localize("Size"), aBuf);

		unsigned Crc = (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapCrc[0]<<24) | (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapCrc[1]<<16) |
					(pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapCrc[2]<<8) | (pSelf->m_lDemos[pSelf->m_DemolistSelectedIndex].m_Info.m_aMapCrc[3]);
		str_format(aBuf, sizeof(aBuf), "%08x", Crc);
		pSelf->DoInfoBox(&ButtonRight, Localize("Crc"), aBuf);
	}

	//unused
	return 0.0f;
}
