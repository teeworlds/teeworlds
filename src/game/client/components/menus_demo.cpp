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

CMenus::CColumn CMenus::ms_aDemoCols[] = {
	{COL_DEMO_ICON,		-1, " ", -1, 0, 0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_DEMO_NAME,		CMenus::SORT_DEMONAME, Localize("Name"), 0, 200.0f, 0, {0}, {0}, CUI::ALIGN_CENTER},
	{COL_DEMO_DATE,		CMenus::SORT_DATE, Localize("Date"), 1, 160.0f, 0, {0}, {0}, CUI::ALIGN_CENTER},
};

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
		RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), CUI::CORNER_T, 10.0f);

	MainView.Margin(5.0f, &MainView);

	CUIRect SeekBar, ButtonBar, NameBar;

	const bool CtrlDown = Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL);
	static bool s_LastCtrlDown = CtrlDown;

	int CurrentTick = pInfo->m_CurrentTick - pInfo->m_FirstTick;
	int TotalTicks = pInfo->m_LastTick - pInfo->m_FirstTick;
	int64 Now = time_get();

	// we can toggle the seekbar using CTRL
	if(!m_MenuActive && !s_LastCtrlDown && CtrlDown)
	{
		if (m_SeekBarActive)
			m_SeekBarActive = false;
		else
		{
			m_SeekBarActive = true;
			m_SeekBarActivatedTime = Now; // stores at which point of time the seekbar was activated, so we can automatically hide it after few seconds
		}
	}
	s_LastCtrlDown = CtrlDown;

	if(m_SeekBarActivatedTime < Now - 5*time_freq())
		m_SeekBarActive = false;

	if(m_MenuActive)
	{
		MainView.HSplitTop(SeekBarHeight, &SeekBar, &ButtonBar);
		ButtonBar.HSplitTop(Margins, 0, &ButtonBar);
		ButtonBar.HSplitBottom(NameBarHeight, &ButtonBar, &NameBar);
		NameBar.HSplitTop(4.0f, 0, &NameBar);
		m_SeekBarActive = true;
		m_SeekBarActivatedTime = Now;
	}
	else
		SeekBar = MainView;

	// do seekbar
	float PositionToSeek = -1.0f;
	if(m_SeekBarActive || m_MenuActive)
	{
		static bool s_PausedBeforeSeeking = false;
		static float s_PrevAmount = -1.0f;
		const float Rounding = 5.0f;

		// draw seek bar
		RenderTools()->DrawUIRect(&SeekBar, vec4(0.0f, 0.0f, 0.0f, 0.5f), CUI::CORNER_ALL, Rounding);

		// draw filled bar
		float Amount = CurrentTick/(float)TotalTicks;
		CUIRect FilledBar = SeekBar;
		FilledBar.w = (FilledBar.w-2*Rounding)*Amount + 2*Rounding;
		RenderTools()->DrawUIRect(&FilledBar, vec4(1.0f, 1.0f , 1.0f, 0.5f), CUI::CORNER_ALL, Rounding);

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
		char aBuffer[64];
		str_format(aBuffer, sizeof(aBuffer), "%d:%02d / %d:%02d",
			CurrentTick/SERVER_TICK_SPEED/60, (CurrentTick/SERVER_TICK_SPEED)%60,
			TotalTicks/SERVER_TICK_SPEED/60, (TotalTicks/SERVER_TICK_SPEED)%60);
		UI()->DoLabel(&SeekBar, aBuffer, SeekBar.h*0.70f, CUI::ALIGN_CENTER);

		// do the logic
		bool Inside = UI()->MouseInside(&SeekBar);

		if(UI()->CheckActiveItem(&s_PrevAmount))
		{
			if(!UI()->MouseButton(0))
			{
				UI()->SetActiveItem(0);
				if(!s_PausedBeforeSeeking)
					DemoPlayer()->Unpause();
				s_PrevAmount = -1.0f; // so that the same position can be seeked again after releasing the mouse
			}
			else
			{
				float Amount = clamp((UI()->MouseX()-SeekBar.x-Rounding)/(float)(SeekBar.w-2*Rounding), 0.0f, 1.0f);
				if(absolute(s_PrevAmount-Amount) >= (0.1f/UI()->Screen()->w))
				{
					s_PrevAmount = Amount;
					PositionToSeek = Amount;
				}
			}
		}
		else if(UI()->HotItem() == &s_PrevAmount)
		{
			if(UI()->MouseButton(0))
			{
				UI()->SetActiveItem(&s_PrevAmount);
				s_PausedBeforeSeeking = pInfo->m_Paused;
				if(!pInfo->m_Paused)
					DemoPlayer()->Pause();
			}
		}

		if(Inside)
			UI()->SetHotItem(&s_PrevAmount);
	}

	// rewind when reaching the end
	if(CurrentTick == TotalTicks)
	{
		m_pClient->OnReset();
		DemoPlayer()->Pause();
		PositionToSeek = 0.0f;
	}

	bool IncreaseDemoSpeed = Input()->KeyPress(KEY_MOUSE_WHEEL_UP) || Input()->KeyPress(KEY_PLUS) || Input()->KeyPress(KEY_KP_PLUS);
	bool DecreaseDemoSpeed = Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN) || Input()->KeyPress(KEY_MINUS) || Input()->KeyPress(KEY_KP_MINUS);

	// add spacebar for toggling Play/Pause
	if(Input()->KeyPress(KEY_SPACE))
	{
		if(!pInfo->m_Paused)
			DemoPlayer()->Pause();
		else
			DemoPlayer()->Unpause();
	}

	// skip forward/backward using left/right arrow keys
	const bool ShiftDown = Input()->KeyIsPressed(KEY_LSHIFT) || Input()->KeyIsPressed(KEY_RSHIFT);
	const bool SkipBackwards = Input()->KeyPress(KEY_LEFT);
	const bool SkipForwards = Input()->KeyPress(KEY_RIGHT);
	if(SkipBackwards || SkipForwards)
	{
		int DesiredTick = 0;
		if(CtrlDown)
		{
			// Go to previous/next marker if ctrl is held.
			// Go to start/end if there is no marker.

			// Threshold to consider all ticks close to a marker to be that markers position.
			// Necessary, as setting the demo players position does not set it to exactly the desired tick.
			const int Threshold = 10;

			if(SkipForwards)
			{
				DesiredTick = TotalTicks-1; // jump to end if no markers, or after last one
				for(int i = 0; i < pInfo->m_NumTimelineMarkers; i++)
				{
					const int MarkerTick = pInfo->m_aTimelineMarkers[i]-pInfo->m_FirstTick;
					if(abs(MarkerTick-CurrentTick) < Threshold)
					{
						if(i+1 < pInfo->m_NumTimelineMarkers)
							DesiredTick = pInfo->m_aTimelineMarkers[i+1]-pInfo->m_FirstTick;
						break;
					}
					else if(CurrentTick < MarkerTick)
					{
						DesiredTick = MarkerTick;
						break;
					}
				}
			}
			else
			{
				DesiredTick = 0; // jump to start if no markers, or before first one
				for(int i = pInfo->m_NumTimelineMarkers-1; i >= 0; i--)
				{
					const int MarkerTick = pInfo->m_aTimelineMarkers[i]-pInfo->m_FirstTick;
					if(abs(MarkerTick-CurrentTick) < Threshold)
					{
						if(i > 0)
							DesiredTick = pInfo->m_aTimelineMarkers[i-1]-pInfo->m_FirstTick;
						break;
					}
					else if(CurrentTick > MarkerTick)
					{
						DesiredTick = MarkerTick;
						break;
					}
				}
			}

			// Skipping to previous marker/the end is inconvenient if demo continues running
			DemoPlayer()->Pause();
		}
		else
		{
			// Skip longer time if shift is held
			const int SkippedTicks = (ShiftDown ? 30 : 5) * SERVER_TICK_SPEED;
			DesiredTick = CurrentTick + (SkipBackwards ? -1 : 1) * SkippedTicks;
		}

		PositionToSeek = clamp(DesiredTick, 0, TotalTicks-1)/(float)TotalTicks;

		// Show the seek bar for a few seconds after skipping
		m_SeekBarActive = true;
		m_SeekBarActivatedTime = Now;
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
			PositionToSeek = 0.0f;
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
		str_format(aBuffer, sizeof(aBuffer), pInfo->m_Speed >= 1.0f ? "x%.0f" : "x%.2f", pInfo->m_Speed);
		UI()->DoLabel(&ButtonBar, aBuffer, Button.h*0.7f, CUI::ALIGN_LEFT);

		// close button
		ButtonBar.VSplitRight(ButtonbarHeight*3, &ButtonBar, &Button);
		static CButtonContainer s_ExitButton;
		if(DoButton_Menu(&s_ExitButton, Localize("Close"), 0, &Button))
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

	if(IncreaseDemoSpeed)
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
	else if(DecreaseDemoSpeed)
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

	if(PositionToSeek >= 0.0f && PositionToSeek <= 1.0f)
	{
		m_pClient->OnReset();
		m_pClient->m_SuppressEvents = true;
		DemoPlayer()->SetPos(PositionToSeek);
		m_pClient->m_SuppressEvents = false;
		m_pClient->m_pMapLayersBackGround->EnvelopeUpdate();
		m_pClient->m_pMapLayersForeGround->EnvelopeUpdate();
	}
}

int CMenus::DemolistFetchCallback(const char *pName, time_t Date, int IsDir, int StorageType, void *pUser)
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
		Item.m_Date = Date;
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
	Storage()->ListDirectoryInfo(m_DemolistStorageType, m_aCurrentDemoFolder, DemolistFetchCallback, this);
	m_lDemos.sort_range_by(CDemoComparator(
		Config()->m_BrDemoSort, Config()->m_BrDemoSortOrder
	));
}

void CMenus::DemolistOnUpdate(bool Reset)
{
	m_DemolistSelectedIndex = Reset ? m_lDemos.size() > 0 ? 0 : -1 :
										m_DemolistSelectedIndex >= m_lDemos.size() ? m_lDemos.size()-1 : m_DemolistSelectedIndex;
	m_DemolistSelectedIsDir = m_DemolistSelectedIndex < 0 ? false : m_lDemos[m_DemolistSelectedIndex].m_IsDir;
}

void CMenus::RenderDemoList(CUIRect MainView)
{
	CUIRect BottomView;
	MainView.HSplitTop(20.0f, 0, &MainView);

	// back button
	RenderBackButton(MainView);

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, Config()->m_ClMenuAlpha/100.0f), CUI::CORNER_ALL, 5.0f);
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
				char aBuffer[IO_MAX_PATH_LENGTH];
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

	// demo list

	CUIRect Headers;
	ListBox.HSplitTop(GetListHeaderHeight(), &Headers, &ListBox);

	RenderTools()->DrawUIRect(&Headers, vec4(0.0f,0,0,0.15f), 0, 0);

	int NumCols = sizeof(ms_aDemoCols)/sizeof(CColumn);

	// do layout
	for(int i = 0; i < NumCols; i++)
	{
		if(ms_aDemoCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(ms_aDemoCols[i].m_Width, &ms_aDemoCols[i].m_Rect, &Headers);

			if(i+1 < NumCols)
			{
				Headers.VSplitLeft(2, &ms_aDemoCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NumCols-1; i >= 0; i--)
	{
		if(ms_aDemoCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(ms_aDemoCols[i].m_Width, &Headers, &ms_aDemoCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &ms_aDemoCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NumCols; i++)
	{
		if(ms_aDemoCols[i].m_Direction == 0)
			ms_aDemoCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NumCols; i++)
	{
		if(i == COL_DEMO_ICON)
			continue;

		if(DoButton_GridHeader(ms_aDemoCols[i].m_Caption, ms_aDemoCols[i].m_Caption, Config()->m_BrDemoSort == ms_aDemoCols[i].m_Sort, ms_aDemoCols[i].m_Align, &ms_aDemoCols[i].m_Rect))
		{
			if(ms_aDemoCols[i].m_Sort != -1)
			{
				if(Config()->m_BrDemoSort == ms_aDemoCols[i].m_Sort)
					Config()->m_BrDemoSortOrder ^= 1;
				else
					Config()->m_BrDemoSortOrder = 0;
				Config()->m_BrDemoSort = ms_aDemoCols[i].m_Sort;
			}

			DemolistPopulate();
			DemolistOnUpdate(false);
		}
	}

	static CListBox s_ListBox;
	s_ListBox.DoHeader(&ListBox, Localize("Recorded"), GetListHeaderHeight());

	s_ListBox.DoStart(20.0f, m_lDemos.size(), 1, m_DemolistSelectedIndex);
	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = s_ListBox.DoNextItem(&r.front(), (&r.front() - m_lDemos.base_ptr()) == m_DemolistSelectedIndex);
		// marker count
		const CDemoItem& DemoItem = r.front();
		const int DemoMarkerCount = DemoItem.GetMarkerCount();

		if(Item.m_Visible)
		{
			Item.m_Rect.VSplitLeft(Item.m_Rect.h, &FileIcon, &Item.m_Rect);
			Item.m_Rect.VSplitLeft(5.0f, 0, &Item.m_Rect);
			FileIcon.Margin(3.0f, &FileIcon);
			FileIcon.x += 3.0f;

			vec4 IconColor = vec4(1, 1, 1, 1);
			if(!DemoItem.m_IsDir)
			{
				if(DemoMarkerCount < 0)
					IconColor = vec4(0.6f, 0.6f, 0.6f, 1.0f); // not loaded
				else if(DemoMarkerCount > 0)
					IconColor = vec4(0.5, 1, 0.5, 1);
			}

			DoIconColor(IMAGE_FILEICONS, DemoItem.m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_DEMO1, &FileIcon, IconColor);

			if(Item.m_Selected)
			{
				TextRender()->TextColor(CUI::ms_HighlightTextColor);
				TextRender()->TextOutlineColor(CUI::ms_HighlightTextOutlineColor);
			}
			Item.m_Rect.y += 2.0f;
			UI()->DoLabel(&Item.m_Rect, DemoItem.m_aName, Item.m_Rect.h*ms_FontmodHeight*0.8f, CUI::ALIGN_LEFT);
			if(!DemoItem.m_IsDir)
			{
				char aDate[64];
				str_timestamp_ex(DemoItem.m_Date, aDate, sizeof(aDate), FORMAT_SPACE);
				if(!Item.m_Selected)
					TextRender()->TextColor(CUI::ms_TransparentTextColor);
				UI()->DoLabel(&Item.m_Rect, aDate, Item.m_Rect.h*ms_FontmodHeight*0.8f, CUI::ALIGN_RIGHT);
			}
			TextRender()->TextColor(CUI::ms_DefaultTextColor);
			if(Item.m_Selected)
				TextRender()->TextOutlineColor(CUI::ms_DefaultTextOutlineColor);
		}
	}
	m_DemolistSelectedIndex = s_ListBox.DoEnd();
	DemolistOnUpdate(false);

	MainView.HSplitTop(HMargin, 0, &MainView);
	static int s_DemoDetailsDropdown = 0;
	if(!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid)
		DoIndependentDropdownMenu(&s_DemoDetailsDropdown, &MainView, aFooterLabel, ButtonHeight, &CMenus::RenderDemoDetails, &s_DemoDetailsActive);
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
	RenderBackgroundShadow(&BottomView, true);

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
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), Localize("Are you sure that you want to delete the demo '%s'?"), m_lDemos[m_DemolistSelectedIndex].m_aFilename);
				PopupConfirm(Localize("Delete demo"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDeleteDemo);
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
	if(DoButton_Menu(&s_PlayButton, m_DemolistSelectedIsDir?Localize("Open"):Localize("Play", "DemoBrowser"), 0, &Button) || s_ListBox.WasItemActivated())
	{
		if(m_DemolistSelectedIndex >= 0)
		{
			if(m_DemolistSelectedIsDir)	// folder
			{
				if(str_comp(m_lDemos[m_DemolistSelectedIndex].m_aFilename, "..") == 0)	// parent folder
					fs_parent_dir(m_aCurrentDemoFolder);
				else	// sub folder
				{
					char aTemp[IO_MAX_PATH_LENGTH];
					str_copy(aTemp, m_aCurrentDemoFolder, sizeof(aTemp));
					str_format(m_aCurrentDemoFolder, sizeof(m_aCurrentDemoFolder), "%s/%s", aTemp, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					m_DemolistStorageType = m_lDemos[m_DemolistSelectedIndex].m_StorageType;
				}
				DemolistPopulate();
				DemolistOnUpdate(true);
			}
			else // file
			{
				char aBuf[IO_MAX_PATH_LENGTH];
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

void CMenus::PopupConfirmDeleteDemo()
{
	if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
	{
		char aBuf[IO_MAX_PATH_LENGTH];
		str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
		if(Storage()->RemoveFile(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
		{
			DemolistPopulate();
			DemolistOnUpdate(false);
		}
		else
			PopupMessage(Localize("Error"), Localize("Unable to delete the demo"), Localize("Ok"));
	}
}

float CMenus::RenderDemoDetails(CUIRect View)
{
	// render demo info
	if(!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid)
	{
		CUIRect Button;

		const float ButtonHeight = 20.0f;
		const float Spacing = 2.0f;

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		DoInfoBox(&Button, Localize("Created"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aTimestamp);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		DoInfoBox(&Button, Localize("Type"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aType);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		int Length = ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[0]<<24)&0xFF000000) | ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[1]<<16)&0xFF0000) |
					((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[2]<<8)&0xFF00) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[3]&0xFF);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d:%02d", Length/60, Length%60);
		DoInfoBox(&Button, Localize("Length"), aBuf);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		str_format(aBuf, sizeof(aBuf), "%d", m_lDemos[m_DemolistSelectedIndex].m_Info.m_Version);
		DoInfoBox(&Button, Localize("Version"), aBuf);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		DoInfoBox(&Button, Localize("Netversion"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aNetversion);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		const int MarkerCount = m_lDemos[m_DemolistSelectedIndex].GetMarkerCount();
		str_format(aBuf, sizeof(aBuf), "%d", MarkerCount);
		if(MarkerCount > 0)
			TextRender()->TextColor(0.5, 1, 0.5, 1);
		DoInfoBox(&Button, Localize("Markers"), aBuf);
		TextRender()->TextColor(1, 1, 1, 1);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		DoInfoBox(&Button, Localize("Map"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapName);

		View.HSplitTop(Spacing, 0, &View);
		View.HSplitTop(ButtonHeight, &Button, &View);
		Button.VMargin(ButtonHeight, &Button);

		CUIRect ButtonRight;
		Button.VSplitMid(&Button, &ButtonRight);
		float Size = float((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[1]<<16) |
							(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[3]))/1024.0f;
		str_format(aBuf, sizeof(aBuf), Localize("%.3f KiB"), Size);
		DoInfoBox(&Button, Localize("Size"), aBuf);

		unsigned Crc = (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[1]<<16) |
					(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[3]);
		str_format(aBuf, sizeof(aBuf), "%08x", Crc);
		DoInfoBox(&ButtonRight, Localize("Crc"), aBuf);
	}

	//unused
	return 0.0f;
}
