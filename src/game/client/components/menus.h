/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <base/tl/string.h>

#include <engine/graphics.h>
#include <engine/demo.h>
#include <engine/contacts.h>
#include <engine/serverbrowser.h>
#include <engine/config.h>

#include <game/voting.h>
#include <game/client/component.h>
#include <game/client/localization.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>

#include "skins.h"


// component to fetch keypresses, override all other input
class CMenusKeyBinder : public CComponent
{
public:
	bool m_TakeKey;
	bool m_GotKey;
	int m_Modifier;
	IInput::CEvent m_Key;
	CMenusKeyBinder();
	virtual bool OnInput(IInput::CEvent Event);
};

class CMenus : public CComponent
{
private:
	typedef float (CMenus::*FDropdownCallback)(CUIRect View);

	bool DoButton_SpriteID(CButtonContainer *pBC, int ImageID, int SpriteID, bool Checked, const CUIRect *pRect, int Corners = CUIRect::CORNER_ALL, float Rounding = 5.0f, bool Fade = true);
	bool DoButton_Toggle(const void *pID, bool Checked, const CUIRect *pRect, bool Active);
	bool DoButton_Menu(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, const char *pImageName = 0, int Corners = CUIRect::CORNER_ALL, float Rounding = 5.0f, float FontFactor = 0.0f, vec4 ColorHot = vec4(1.0f, 1.0f, 1.0f, 0.75f), bool TextFade = true);
	bool DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, bool Checked, const CUIRect *pRect, float Alpha = 1.0f, float FontAlpha = 1.0f, int Corners = CUIRect::CORNER_ALL, float Rounding = 5.0f, float FontFactor = 0.0f);

	bool DoButton_CheckBox(const void *pID, const char *pText, bool Checked, const CUIRect *pRect, bool Locked = false);

	void DoIcon(int ImageId, int SpriteId, const CUIRect *pRect, const vec4 *pColor = 0);
	bool DoButton_GridHeader(const void *pID, const char *pText, bool Checked, int Align, const CUIRect *pRect, int Corners = CUIRect::CORNER_ALL);

	float DoIndependentDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, bool *pActive);
	void DoInfoBox(const CUIRect *pRect, const char *pLable, const char *pValue);

	void DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active);
	void DoButton_KeySelect(CButtonContainer *pBC, const char *pText, const CUIRect *pRect);
	int DoKeyReader(CButtonContainer *pPC, const CUIRect *pRect, int Key, int Modifier, int *pNewModifier);

	enum
	{
		POPUP_NONE=0,
		POPUP_MESSAGE, // generic message popup (one button)
		POPUP_CONFIRM, // generic confirmation popup (two buttons)
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_LOADING_DEMO,
		POPUP_LANGUAGE,
		POPUP_COUNTRY,
		POPUP_RENAME_DEMO,
		POPUP_SAVE_SKIN,
		POPUP_PASSWORD,
		POPUP_QUIT,
	};

	enum
	{
		PAGE_NEWS=0,
		PAGE_GAME,
		PAGE_PLAYERS,
		PAGE_SERVER_INFO,
		PAGE_CALLVOTE,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_DEMOS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
		PAGE_START,

		SETTINGS_GENERAL=0,
		SETTINGS_PLAYER,
		SETTINGS_TBD, // TODO: replace this removed tee page
		SETTINGS_CONTROLS,
		SETTINGS_GRAPHICS,
		SETTINGS_SOUND,

		ACTLB_NONE=0,
		ACTLB_LANG,
		ACTLB_THEME,
	};

	int m_GamePage;
	int m_Popup;
	int m_ActivePage;
	int m_MenuPage;
	int m_MenuPageOld;
	bool m_MenuActive;
	vec2 m_MousePos;
	vec2 m_PrevMousePos;
	int m_ActiveListBox;
	int m_PopupCountrySelection;
	bool m_SkinModified;
	bool m_KeyReaderWasActive;
	bool m_KeyReaderIsActive;

	// generic popups
	typedef void (CMenus::*FPopupButtonCallback)();
	void DefaultButtonCallback() { /* do nothing */ };
	enum
	{
		BUTTON_CONFIRM = 0, // confirm / yes / close / ok
		BUTTON_CANCEL, // cancel / no
		NUM_BUTTONS
	};
	char m_aPopupTitle[128];
	char m_aPopupMessage[256];
	struct
	{
		char m_aLabel[64];
		int m_NextPopup;
		FPopupButtonCallback m_pfnCallback;
	} m_aPopupButtons[NUM_BUTTONS];

	void PopupMessage(const char *pTitle, const char *pMessage,
		const char *pButtonLabel, int NextPopup = POPUP_NONE, FPopupButtonCallback pfnButtonCallback = &CMenus::DefaultButtonCallback);
	void PopupConfirm(const char *pTitle, const char *pMessage,
		const char *pConfirmButtonLabel, const char *pCancelButtonLabel,
		FPopupButtonCallback pfnConfirmButtonCallback = &CMenus::DefaultButtonCallback, int ConfirmNextPopup = POPUP_NONE,
		FPopupButtonCallback pfnCancelButtonCallback = &CMenus::DefaultButtonCallback, int CancelNextPopup = POPUP_NONE);
	void PopupCountry(int Selection, FPopupButtonCallback pfnOkButtonCallback = &CMenus::DefaultButtonCallback);

	// images
	struct CMenuImage
	{
		char m_aName[64];
		IGraphics::CTextureHandle m_OrgTexture;
		IGraphics::CTextureHandle m_GreyTexture;
	};
	array<CMenuImage> m_lMenuImages;

	static int MenuImageScan(const char *pName, int IsDir, int DirType, void *pUser);

	const CMenuImage *FindMenuImage(const char* pName);

	// themes
	class CTheme
	{
	public:
		CTheme() {}
		CTheme(const char *n, bool HasDay, bool HasNight) : m_Name(n), m_HasDay(HasDay), m_HasNight(HasNight) {}

		string m_Name;
		bool m_HasDay;
		bool m_HasNight;
		IGraphics::CTextureHandle m_IconTexture;
		bool operator<(const CTheme &Other) const { return m_Name < Other.m_Name; }
	};
	sorted_array<CTheme> m_lThemes;

	static int ThemeScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int ThemeIconScan(const char *pName, int IsDir, int DirType, void *pUser);

	// gametype icons
	class CGameIcon
	{
	public:
		enum
		{
			GAMEICON_SIZE=64,
			GAMEICON_OLDHEIGHT=192,
		};
		CGameIcon() {}
		CGameIcon(const char *pName) : m_Name(pName) {}

		string m_Name;
		IGraphics::CTextureHandle m_IconTexture;
	};
	array<CGameIcon> m_lGameIcons;
	IGraphics::CTextureHandle m_GameIconDefault;
	void DoGameIcon(const char *pName, const CUIRect *pRect);
	static int GameIconScan(const char *pName, int IsDir, int DirType, void *pUser);

	int64 m_LastInput;

	// for settings
	bool m_NeedRestartPlayer;
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	int m_TeePartSelected;
	char m_aSaveSkinName[MAX_SKIN_ARRAY_SIZE];
	CLineInput m_SkinNameInput;

	bool m_RefreshSkinSelector;
	const CSkins::CSkin *m_pSelectedSkin;

	// for map download popup
	int64 m_DownloadLastCheckTime;
	int m_DownloadLastCheckSize;
	float m_DownloadSpeed;

	// for demo loading popup
	char m_aDemoLoadingFile[IO_MAX_PATH_LENGTH];
	int m_DemoLoadingStorageType;
	bool m_DemoLoadingPopupRendered;

	// for password popup
	char m_aPasswordPopupServerAddress[256];

	// for call vote
	int m_CallvoteSelectedOption;
	int m_CallvoteSelectedPlayer;
	char m_aFilterString[VOTE_REASON_LENGTH];
	char m_aCallvoteReason[VOTE_REASON_LENGTH];

	// demo
	enum
	{
		SORT_DEMONAME=0,
		SORT_LENGTH,
		SORT_DATE,
	};

	struct CDemoItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		int m_StorageType;
		time_t m_Date;

		bool m_InfosLoaded;
		bool m_Valid;
		CDemoHeader m_Info;

		CDemoItem() : m_InfosLoaded(false), m_Valid(false) {}

		int GetMarkerCount() const
		{
			if(!m_Valid || !m_InfosLoaded)
				return -1;
			return bytes_be_to_int(m_Info.m_aNumTimelineMarkers);
		}

		int Length() const
		{
			return bytes_be_to_int(m_Info.m_aLength);
		}

		bool operator<(const CDemoItem &Other) const
		{
			return !str_comp(m_aFilename, "..") ? true
				: !str_comp(Other.m_aFilename, "..") ? false
				: m_IsDir && !Other.m_IsDir ? true
				: !m_IsDir && Other.m_IsDir ? false
				: str_comp_filenames(m_aFilename, Other.m_aFilename) < 0;
		}
	};

	class CDemoComparator
	{
		int m_Type;
		int m_Order;

	public:
		CDemoComparator(int Type, int Order)
		{
			m_Type = Type;
			m_Order = Order;
		}

		bool operator()(const CDemoItem &Self, const CDemoItem &Other)
		{
			if(!str_comp(Self.m_aFilename, ".."))
				return true;
			if(!str_comp(Other.m_aFilename, ".."))
				return false;
			if(Self.m_IsDir && !Other.m_IsDir)
				return true;
			if(!Self.m_IsDir && Other.m_IsDir)
				return false;

			const CDemoItem &Left = m_Order ? Other : Self;
			const CDemoItem &Right = m_Order ? Self : Other;

			if(m_Type == SORT_DEMONAME)
				return str_comp_nocase(Left.m_aFilename, Right.m_aFilename) < 0;
			else if(m_Type == SORT_LENGTH)
				return Left.Length() < Right.Length();
			else if(m_Type == SORT_DATE)
				return Left.m_Date < Right.m_Date;
			return false;
		}
	};

	sorted_array<CDemoItem> m_lDemos;
	char m_aCurrentDemoFolder[IO_MAX_PATH_LENGTH];
	char m_aCurrentDemoFile[IO_MAX_PATH_LENGTH];
	CLineInput m_DemoNameInput;
	int m_DemolistSelectedIndex;
	bool m_DemolistSelectedIsDir;
	int m_DemolistStorageType;
	char m_aDemolistPreviousSelection[IO_MAX_PATH_LENGTH];
	int64 m_SeekBarActivatedTime;
	bool m_SeekBarActive;

	void DemolistOnUpdate(bool Reset);
	void DemolistPopulate();
	static int DemolistFetchCallback(const CFsFileInfo* pFileInfo, int IsDir, int StorageType, void *pUser);

	// friends
	class CFriendItem
	{
	public:
		const CServerInfo *m_pServerInfo;
		char m_aName[MAX_NAME_ARRAY_SIZE];
		char m_aClan[MAX_CLAN_ARRAY_SIZE];
		int m_FriendState;
		bool m_IsPlayer;

		CFriendItem()
		{
			m_pServerInfo = 0;
		}

		bool operator<(const CFriendItem &Other) const
		{
			if(m_aName[0] && !Other.m_aName[0])
				return true;
			if(!m_aName[0] && Other.m_aName[0])
				return false;
			int Result = str_comp_nocase(m_aName, Other.m_aName);
			return Result < 0 || (Result == 0 && str_comp_nocase(m_aClan, Other.m_aClan) < 0);
		}
	};

	enum
	{
		FRIEND_PLAYER_ON = 0,
		FRIEND_CLAN_ON,
		FRIEND_OFF,
		NUM_FRIEND_TYPES
	};
	sorted_array<CFriendItem> m_lFriendList[NUM_FRIEND_TYPES];
	const CFriendItem *m_pDeleteFriend;

	void FriendlistOnUpdate();


	// server browser
	class CBrowserFilter
	{
		bool m_Extended;
		int m_Custom;
		char m_aName[64];
		int m_Filter;
		IServerBrowser *m_pServerBrowser;

		static CServerFilterInfo ms_FilterStandard;
		static CServerFilterInfo ms_FilterRace;
		static CServerFilterInfo ms_FilterFavorites;
		static CServerFilterInfo ms_FilterAll;

	public:
		enum
		{
			FILTER_CUSTOM = 0,
			FILTER_ALL,
			FILTER_STANDARD,
			FILTER_FAVORITES,
			FILTER_RACE,
			NUM_FILTERS,
		};

		CButtonContainer m_DeleteButtonContainer;
		CButtonContainer m_UpButtonContainer;
		CButtonContainer m_DownButtonContainer;

		CBrowserFilter() {}
		CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser);
		void Switch();
		bool Extended() const;
		int Custom() const;
		int Filter() const;
		const char* Name() const;

		void SetFilterNum(int Num);

		int NumSortedServers() const;
		int NumPlayers() const;
		const CServerInfo* SortedGet(int Index) const;
		const void* ID(int Index) const;

		void Reset();
		void GetFilter(CServerFilterInfo *pFilterInfo) const;
		void SetFilter(const CServerFilterInfo *pFilterInfo);
	};

	array<CBrowserFilter> m_lFilters;

	int m_RemoveFilterIndex;

	void LoadFilters();
	void SaveFilters();
	void RemoveFilter(int FilterIndex);
	void MoveFilter(bool Up, int Filter);
	void InitDefaultFilters();

	struct CColumn
	{
		int m_ID;
		int m_Sort;
		CLocConstString m_Caption;
		int m_Direction;
		float m_Width;
		int m_Flags;
		CUIRect m_Rect;
		CUIRect m_Spacer;
		int m_Align;
	};

	enum
	{
		COL_BROWSER_FLAG = 0,
		COL_BROWSER_NAME,
		COL_BROWSER_GAMETYPE,
		COL_BROWSER_MAP,
		COL_BROWSER_PLAYERS,
		COL_BROWSER_PING,
		NUM_BROWSER_COLS,

		COL_DEMO_NAME = 0,
		COL_DEMO_LENGTH,
		COL_DEMO_DATE,
		NUM_DEMO_COLS,

		SIDEBAR_TAB_INFO = 0,
		SIDEBAR_TAB_FILTER,
		SIDEBAR_TAB_FRIEND,
		NUM_SIDEBAR_TABS,

		ADDR_SELECTION_CHANGE = 1, // select the server based on server address input
		ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND = 2, // clear selection if ADDR_SELECTION_CHANGE did not match any server
		ADDR_SELECTION_REVEAL = 4, // scroll to the selected server
		ADDR_SELECTION_UPDATE_ADDRESS = 8, // update address input to the selected server's address
	};
	int m_SidebarTab;
	bool m_SidebarActive;
	bool m_ShowServerDetails;
	int m_LastBrowserType; // -1 if not initialized
	int m_aSelectedFilters[IServerBrowser::NUM_TYPES]; // -1 if none selected, -2 if not initialized
	int m_aSelectedServers[IServerBrowser::NUM_TYPES]; // -1 if none selected
	int m_AddressSelection;
	static CColumn ms_aBrowserCols[NUM_BROWSER_COLS];
	static CColumn ms_aDemoCols[NUM_DEMO_COLS];

	CBrowserFilter *GetSelectedBrowserFilter()
	{
		const int Tab = ServerBrowser()->GetType();
		if(m_aSelectedFilters[Tab] < 0 || m_aSelectedFilters[Tab] >= m_lFilters.size())
			return 0;
		return &m_lFilters[m_aSelectedFilters[Tab]];
	}

	const CServerInfo *GetSelectedServerInfo()
	{
		CBrowserFilter *pSelectedFilter = GetSelectedBrowserFilter();
		if(!pSelectedFilter)
			return 0;
		const int Tab = ServerBrowser()->GetType();
		if(m_aSelectedServers[Tab] < 0 || m_aSelectedServers[Tab] >= pSelectedFilter->NumSortedServers())
			return 0;
		return pSelectedFilter->SortedGet(m_aSelectedServers[Tab]);
	}

	void UpdateServerBrowserAddress();
	const char *GetServerBrowserAddress();
	void SetServerBrowserAddress(const char *pAddress);
	void ServerBrowserFilterOnUpdate();
	void ServerBrowserSortingOnUpdate();


	// video settings
	bool m_CheckVideoSettings;
	enum
	{
		MAX_RESOLUTIONS=256,
	};
	CVideoMode m_aModes[MAX_RESOLUTIONS];
	int m_NumModes;
	struct CVideoFormat
	{
		int m_WidthValue;
		int m_HeightValue;
	};
	sorted_array<CVideoMode> m_lRecommendedVideoModes;
	sorted_array<CVideoMode> m_lOtherVideoModes;
	void UpdatedFilteredVideoModes();
	void UpdateVideoModeSettings();

	// found in menus.cpp
	void RenderMenu(CUIRect Screen);
	void RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);
	void RenderBackButton(CUIRect MainView);
	static void ConchainUpdateMusicState(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void UpdateMusicState();

	// found in menus_demo.cpp
	bool FetchHeader(CDemoItem *pItem);
	void RenderDemoPlayer(CUIRect Screen);
	void RenderDemoList(CUIRect MainView);
	float RenderDemoDetails(CUIRect View);
	void PopupConfirmDeleteDemo();

	// found in menus_start.cpp
	void RenderStartMenu(CUIRect MainView);

	// found in menus_ingame.cpp
	void RenderGame(CUIRect MainView);
	void RenderPlayers(CUIRect MainView);
	void RenderServerInfo(CUIRect MainView);
	void HandleCallvote(int Page, bool Force);
	void RenderServerControl(CUIRect MainView);
	void RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	bool RenderServerControlServer(CUIRect MainView);

	// found in menus_browser.cpp
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserSidebar(CUIRect View);
	void RenderServerbrowserFriendTab(CUIRect View);
	void PopupConfirmRemoveFriend();
	void RenderServerbrowserFilterTab(CUIRect View);
	void RenderServerbrowserInfoTab(CUIRect View);
	void RenderServerbrowserFriendList(CUIRect View);
	void RenderDetailInfo(CUIRect View, const CServerInfo *pInfo, const vec4 &TextColor, const vec4 &TextOutlineColor);
	void RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int RowCount, const vec4 &TextColor, const vec4 &TextOutlineColor);
	void RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo);
	void RenderServerbrowserBottomBox(CUIRect View);
	void RenderFilterHeader(CUIRect View, int FilterIndex);
	void PopupConfirmRemoveFilter();
	void PopupConfirmCountryFilter();
	int DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected, bool ShowServerInfo, CScrollRegion *pScroll = 0);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainConnect(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserSortingUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void DoFriendListEntry(CUIRect *pView, CFriendItem *pFriend, const void *pID, const CContactInfo *pFriendInfo, const CServerInfo *pServerInfo, bool Checked, bool Clan = false);
	void UpdateFriendCounter(const CServerInfo *pEntry);
	void UpdateFriends();

	// found in menus_settings.cpp
	void RenderLanguageSelection(CUIRect MainView, bool Header=true);
	void RenderThemeSelection(CUIRect MainView, bool Header=true);
	void RenderHSLPicker(CUIRect Picker);
	void RenderSkinSelection(CUIRect MainView);
	void RenderSkinPartSelection(CUIRect MainView);
	void RenderSkinPartPalette(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	// void RenderSettingsTBD(CUIRect MainView); // TODO: change removed tee page to something else
	void RenderSettingsTeeBasic(CUIRect MainView);
	void RenderSettingsTeeCustom(CUIRect MainView);
	void PopupConfirmDeleteSkin();
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettings(CUIRect MainView);
	void ResetSettingsGeneral();
	void ResetSettingsControls();
	void ResetSettingsGraphics();
	void ResetSettingsSound();
	void PopupConfirmPlayerCountry();

	bool DoResolutionList(CUIRect* pRect, CListBox* pListBox,
						  const sorted_array<CVideoMode>& lModes);

	// found in menus_callback.cpp
	float RenderSettingsControlsMouse(CUIRect View);
	float RenderSettingsControlsJoystick(CUIRect View);
	float RenderSettingsControlsMovement(CUIRect View);
	float RenderSettingsControlsWeapon(CUIRect View);
	float RenderSettingsControlsVoting(CUIRect View);
	float RenderSettingsControlsChat(CUIRect View);
	float RenderSettingsControlsScoreboard(CUIRect View);
	float RenderSettingsControlsStats(CUIRect View);
	float RenderSettingsControlsMisc(CUIRect View);
	void DoSettingsControlsButtons(int Start, int Stop, CUIRect View, float ButtonHeight, float Spacing);

	void DoJoystickAxisPicker(CUIRect View);

	void SetActive(bool Active);

	// loading
	int m_LoadCurrent;
	int m_LoadTotal;

	void SetMenuPage(int NewPage);

	bool CheckHotKey(int Key) const;

	void RenderBackground(float Time);
	void RenderBackgroundShadow(const CUIRect *pRect, bool TopToBottom, float Rounding = 5.0f);
public:
	void InitLoading(int TotalWorkAmount);
	void RenderLoading(int WorkedAmount = 0);
	bool IsBackgroundNeeded() const;

	struct CSwitchTeamInfo
	{
		char m_aNotification[128];
		bool m_AllowSpec;
		int m_TimeLeft;
	};
	void GetSwitchTeamInfo(CSwitchTeamInfo *pInfo);

	CMenusKeyBinder m_Binder;

	CMenus();

	bool IsActive() const { return m_MenuActive; }

	virtual int GetInitAmount() const;
	virtual void OnInit();

	virtual void OnConsoleInit();
	virtual void OnShutdown();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Event);
	virtual bool OnCursorMove(float x, float y, int CursorType);

	static void Con_Play(IConsole::IResult *pResult, void *pUserData);
};
#endif
