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

#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "maplayers.h"
#include "menus.h"

int CMenus::DoButton_DemoPlayer(const void *pID, const char *pText, const CUIRect *pRect)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds);

	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, 0.5f+(*pFade/Seconds)*0.25f), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, 14.0f, 0);
	return UI()->DoButtonLogic(pID, pText, false, pRect);
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

	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_T, 10.0f);

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
		static int s_SeekBarID = 0;
		void *id = &s_SeekBarID;
		char aBuffer[128];

		// draw seek bar
		RenderTools()->DrawUIRect(&SeekBar, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);

		// draw filled bar
		float Amount = CurrentTick/(float)TotalTicks;
		CUIRect FilledBar = SeekBar;
		FilledBar.w = 10.0f + (FilledBar.w-10.0f)*Amount;
		RenderTools()->DrawUIRect(&FilledBar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);

		// draw markers
		for(int i = 0; i < pInfo->m_NumTimelineMarkers; i++)
		{
			float Ratio = (pInfo->m_aTimelineMarkers[i]-pInfo->m_FirstTick) / (float)TotalTicks;
			Graphics()->TextureClear();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(SeekBar.x + (SeekBar.w-10.0f)*Ratio, SeekBar.y, UI()->PixelSize(), SeekBar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// draw time
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

	if(m_MenuActive)
	{
		// do buttons
		CUIRect Button;

		// combined play and pause button
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_PlayPauseButton = 0;
		if(!pInfo->m_Paused)
		{
			if(DoButton_SpriteID(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PAUSE, &Button, CUI::CORNER_ALL))
				DemoPlayer()->Pause();
		}
		else
		{
			if(DoButton_SpriteID(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PLAY, &Button, CUI::CORNER_ALL))
				DemoPlayer()->Unpause();
		}

		// stop button

		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_ResetButton = 0;
		if(DoButton_SpriteID(&s_ResetButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_STOP, &Button, CUI::CORNER_ALL))
		{
			m_pClient->OnReset();
			DemoPlayer()->Pause();
			DemoPlayer()->SetPos(0);
		}

		// slowdown
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_SlowDownButton = 0;
		if(DoButton_SpriteID(&s_SlowDownButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_SLOWER, &Button, CUI::CORNER_ALL) || Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
			DecreaseDemoSpeed = true;

		// fastforward
		ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
		ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
		static int s_FastForwardButton = 0;
		if(DoButton_SpriteID(&s_FastForwardButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_FASTER, &Button, CUI::CORNER_ALL))
			IncreaseDemoSpeed = true;

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

	if(IncreaseDemoSpeed || Input()->KeyPresses(KEY_MOUSE_WHEEL_UP))
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
	else if(DecreaseDemoSpeed || Input()->KeyPresses(KEY_MOUSE_WHEEL_DOWN))
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
	int Length = str_length(pName);
	if((pName[0] == '.' && (pName[1] == 0 ||
		(pName[1] == '.' && pName[2] == 0 && !str_comp(pSelf->m_aCurrentDemoFolder, "demos")))) ||
		(!IsDir && (Length < 5 || str_comp(pName+Length-5, ".demo"))))
		return 0;

	CDemoItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	if(IsDir)
	{
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", pName);
		Item.m_Valid = false;
	}
	else
	{
		str_copy(Item.m_aName, pName, min(static_cast<int>(sizeof(Item.m_aName)), Length-4));
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

void CMenus::RenderDemoList(CUIRect MainView)
{
	CUIRect BottomView;
	MainView.HSplitTop(20.0f, 0, &MainView);

	// back button
	RenderBackButton(MainView);

	// cut view
	MainView.HSplitBottom(80.0f, &MainView, &BottomView);
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

	CUIRect ListBox, Button, Label, FileIcon;
	MainView.HSplitTop(230.0f, &ListBox, &MainView);

	static int s_DemoListId = 0;
	static float s_ScrollValue = 0;
	UiDoListboxHeader(&ListBox, Localize("Recorded"), 20.0f, 2.0f);
	UiDoListboxStart(&s_DemoListId, 20.0f, 0, m_lDemos.size(), 1, m_DemolistSelectedIndex, s_ScrollValue);
	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		CListboxItem Item = UiDoListboxNextItem((void*)(&r.front()));
		if(Item.m_Visible)
		{
			Item.m_Rect.VSplitLeft(Item.m_Rect.h, &FileIcon, &Item.m_Rect);
			Item.m_Rect.VSplitLeft(5.0f, 0, &Item.m_Rect);
			FileIcon.Margin(3.0f, &FileIcon);
			FileIcon.x += 3.0f;
			DoIcon(IMAGE_FILEICONS, r.front().m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_DEMO1, &FileIcon);
			if(!str_comp(m_lDemos[m_DemolistSelectedIndex].m_aName, r.front().m_aName))
			{
				TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
				TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
				Item.m_Rect.y += 2.0f;
				UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight*0.8f, 0);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			}
			else
			{
				Item.m_Rect.y += 2.0f;
				UI()->DoLabel(&Item.m_Rect, r.front().m_aName, Item.m_Rect.h*ms_FontmodHeight*0.8f, 0);
			}
		}
	}
	bool Activated = false;
	m_DemolistSelectedIndex = UiDoListboxEnd(&s_ScrollValue, &Activated);
	DemolistOnUpdate(false);

	// render demo info
	int NumOptions = (!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid) ? 8 : 0;
	float ButtonHeight = 20.0f;
	float Spacing = 2.0f;
	float BackgroundHeight = (float)(NumOptions+1)*ButtonHeight+(float)NumOptions*Spacing;

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(BackgroundHeight, &MainView, 0);
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	MainView.HSplitTop(ButtonHeight, &Label, &MainView);
	Label.y += 2.0f;
	UI()->DoLabel(&Label, aFooterLabel, ButtonHeight*ms_FontmodHeight*0.8f, 0);
	if(!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid)
	{
		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		DoInfoBox(&Button, Localize("Created"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aTimestamp);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		DoInfoBox(&Button, Localize("Type"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aType);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		int Length = ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[0]<<24)&0xFF000000) | ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[1]<<16)&0xFF0000) |
					((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[2]<<8)&0xFF00) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[3]&0xFF);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d:%02d", Length/60, Length%60);
		DoInfoBox(&Button, Localize("Length"), aBuf);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		str_format(aBuf, sizeof(aBuf), "%d", m_lDemos[m_DemolistSelectedIndex].m_Info.m_Version);
		DoInfoBox(&Button, Localize("Version"), aBuf);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		DoInfoBox(&Button, Localize("Netversion"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aNetversion);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		DoInfoBox(&Button, Localize("Map"), m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapName);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		float Size = float((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[1]<<16) |
					(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[3]))/1024.0f;
		str_format(aBuf, sizeof(aBuf), Localize("%.3f KiB"), Size);
		DoInfoBox(&Button, Localize("Size"), aBuf);

		MainView.HSplitTop(Spacing, 0, &MainView);
		MainView.HSplitTop(ButtonHeight, &Button, &MainView);
		Button.VSplitLeft(ButtonHeight, 0, &Button);
		unsigned Crc = (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[1]<<16) |
					(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[3]);
		str_format(aBuf, sizeof(aBuf), "%08x", Crc);
		DoInfoBox(&Button, Localize("Crc"), aBuf);
	}

	// demo buttons
	int NumButtons = m_DemolistSelectedIsDir ? 2 : 4;
	Spacing = 3.0f;
	float ButtonWidth = (BottomView.w/6.0f)-(Spacing*5.0)/6.0f;
	float BackgroundWidth = ButtonWidth*(float)NumButtons+(float)(NumButtons-1)*Spacing;

	BottomView.VSplitRight(BackgroundWidth, 0, &BottomView);
	RenderTools()->DrawUIRect4(&BottomView, vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.25f), vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.0f, 0.0f), CUI::CORNER_T, 5.0f);

	BottomView.HSplitTop(25.0f, &BottomView, 0);
	BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
	static int s_RefreshButton = 0;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button))
	{
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	if(!m_DemolistSelectedIsDir)
	{
		BottomView.VSplitLeft(Spacing, 0, &BottomView);
		BottomView.VSplitLeft(ButtonWidth, &Button, &BottomView);
		static int s_DeleteButton = 0;
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
		static int s_RenameButton = 0;
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
	static int s_PlayButton = 0;
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
