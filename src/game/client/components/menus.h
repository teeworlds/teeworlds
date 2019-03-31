/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <engine/graphics.h>
#include <engine/demo.h>
#include <engine/friends.h>

#include <game/voting.h>
#include <game/client/component.h>
#include <game/client/localization.h>
#include <game/client/ui.h>

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
public:
	class CButtonContainer
	{
	public:
		float m_FadeStartTime;

		const void *GetID() const { return &m_FadeStartTime; }
	};

private:
	typedef float (*FDropdownCallback)(CUIRect View, void *pUser);

	float ButtonFade(CButtonContainer *pBC, float Seconds, int Checked=0);


	int DoButton_DemoPlayer(CButtonContainer *pBC, const char *pText, const CUIRect *pRect);
	int DoButton_SpriteID(CButtonContainer *pBC, int ImageID, int SpriteID, bool Checked, const CUIRect *pRect, int Corners=CUI::CORNER_ALL, float r=5.0f, bool Fade=true);
	int DoButton_SpriteClean(int ImageID, int SpriteID, const CUIRect *pRect);
	int DoButton_SpriteCleanID(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, bool Blend=true);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active);
	int DoButton_Menu(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName=0, int Corners=CUI::CORNER_ALL, float r=5.0f, float FontFactor=0.0f, vec4 ColorHot=vec4(1.0f, 1.0f, 1.0f, 0.75f), bool TextFade=true);
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, float Alpha=1.0f, float FontAlpha=1.0f, int Corners=CUI::CORNER_ALL, float r=5.0f, float FontFactor=0.0f);
	void DoButton_MenuTabTop_Dummy(const char *pText, int Checked, const CUIRect *pRect, float Alpha);
	int DoButton_Customize(CButtonContainer *pBC, IGraphics::CTextureHandle Texture, int SpriteID, const CUIRect *pRect, float ImageRatio);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect, bool Checked=false, bool Locked=false);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect, bool Locked=false);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	int DoButton_MouseOver(int ImageID, int SpriteID, const CUIRect *pRect);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoIcon(int ImageId, int SpriteId, const CUIRect *pRect);
	void DoIconColor(int ImageId, int SpriteId, const CUIRect *pRect, const vec4& Color);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, CUI::EAlignment Align, const CUIRect *pRect);
	// int DoButton_GridHeaderIcon(CButtonContainer *pBC, int ImageID, int SpriteID, const CUIRect *pRect, int Corners);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);

	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden=false, int Corners=CUI::CORNER_ALL);
	void DoEditBoxOption(void *pID, char *pOption, int OptionLength, const CUIRect *pRect, const char *pStr, float VSplitVal, float *pOffset, bool Hidden=false);
	void DoScrollbarOption(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, bool Infinite=false);
	float DoDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback);
	float DoIndependentDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, bool* pActive);
	void DoInfoBox(const CUIRect *pRect, const char *pLable, const char *pValue);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoButton_KeySelect(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(CButtonContainer *pPC, const CUIRect *pRect, int Key, int Modifier, int* NewModifier);

	//static int ui_do_key_reader(void *id, const CUIRect *rect, int key);
	void UiDoGetButtons(int Start, int Stop, CUIRect View, float ButtonHeight, float Spacing);

	struct CListboxItem
	{
		int m_Visible;
		int m_Selected;
		CUIRect m_Rect;
		CUIRect m_HitRect;
	};

	struct CListBoxState
	{
		CUIRect m_ListBoxOriginalView;
		CUIRect m_ListBoxView;
		float m_ListBoxRowHeight;
		int m_ListBoxItemIndex;
		int m_ListBoxSelectedIndex;
		int m_ListBoxNewSelected;
		int m_ListBoxDoneEvents;
		int m_ListBoxNumItems;
		int m_ListBoxItemsPerRow;
		float m_ListBoxScrollValue;
		bool m_ListBoxItemActivated;

		CListBoxState()
		{
			m_ListBoxScrollValue = 0;
		}
	};

	void UiDoListboxHeader(CListBoxState* pState, const CUIRect *pRect, const char *pTitle, float HeaderHeight, float Spacing);
	void UiDoListboxStart(CListBoxState* pState, const void *pID, float RowHeight, const char *pBottomText, int NumItems,
						int ItemsPerRow, int SelectedIndex, const CUIRect *pRect=0, bool Background=true);
	CListboxItem UiDoListboxNextItem(CListBoxState* pState, const void *pID, bool Selected = false, bool* pActive = NULL);
	CListboxItem UiDoListboxNextRow(CListBoxState* pState);
	int UiDoListboxEnd(CListBoxState* pState, bool *pItemActivated);

	//static void demolist_listdir_callback(const char *name, int is_dir, void *user);
	//static void demolist_list_callback(const CUIRect *rect, int index, void *user);

	struct CScrollRegionParams
	{
		float m_ScrollbarWidth;
		float m_ScrollbarMargin;
		float m_SliderMinHeight;
		float m_ScrollSpeed;
		vec4 m_ClipBgColor;
		vec4 m_ScrollbarBgColor;
		vec4 m_RailBgColor;
		vec4 m_SliderColor;
		vec4 m_SliderColorHover;
		vec4 m_SliderColorGrabbed;
		int m_Flags;

		enum {
			FLAG_CONTENT_STATIC_WIDTH = 0x1
		};

		CScrollRegionParams()
		{
			m_ScrollbarWidth = 20;
			m_ScrollbarMargin = 5;
			m_SliderMinHeight = 25;
			m_ScrollSpeed = 5;
			m_ClipBgColor = vec4(0.0f, 0.0f, 0.0f, 0.5f);
			m_ScrollbarBgColor = vec4(0.0f, 0.0f, 0.0f, 0.5f);
			m_RailBgColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
			m_SliderColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
			m_SliderColorHover = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_SliderColorGrabbed = vec4(0.9f, 0.9f, 0.9f, 1.0f);
			m_Flags = 0;
		}
	};

	struct CScrollRegion
	{
		float m_ScrollY;
		float m_ContentH;
		float m_RequestScrollY; // [0, ContentHeight]
		CUIRect m_ClipRect;
		CUIRect m_OldClipRect;
		CUIRect m_RailRect;
		CUIRect m_LastAddedRect; // saved for ScrollHere()
		vec2 m_MouseGrabStart;
		vec2 m_ContentScrollOff;
		bool m_WasClipped;
		CScrollRegionParams m_Params;

		enum {
			SCROLLHERE_KEEP_IN_VIEW=0,
			SCROLLHERE_TOP,
			SCROLLHERE_BOTTOM,
		};

		CScrollRegion()
		{
			m_ScrollY = 0;
			m_ContentH = 0;
			m_RequestScrollY = -1;
			m_ContentScrollOff = vec2(0,0);
			m_WasClipped = false;
			m_Params = CScrollRegionParams();
		}
	};

	// Scroll region
	/*

	Usage:
		-- Initialization --
		static CScrollRegion s_ScrollRegion;
		vec2 ScrollOffset(0, 0);
		BeginScrollRegion(&s_ScrollRegion, &ScrollRegionRect, &ScrollOffset);
		Content = ScrollRegionRect;
		Content.y += ScrollOffset.y;

		-- "Register" your content rects --
		CUIRect Rect;
		Content.HSplitTop(SomeValue, &Rect, &Content);
		ScrollRegionAddRect(&s_ScrollRegion, Rect);

		-- [Optionnal] Knowing if a rect is clipped --
		ScrollRegionIsRectClipped(&s_ScrollRegion, Rect);

		-- [Optionnal] Scroll to a rect (to the last added rect)--
		...
		ScrollRegionAddRect(&s_ScrollRegion, Rect);
		ScrollRegionScrollHere(&s_ScrollRegion, Option);

		-- End --
		EndScrollRegion(&s_ScrollRegion);

	*/

	void BeginScrollRegion(CScrollRegion* pSr, CUIRect* pClipRect, vec2* pOutOffset, const CScrollRegionParams* pParams = 0);
	void EndScrollRegion(CScrollRegion* pSr);
	void ScrollRegionAddRect(CScrollRegion* pSr, CUIRect Rect);
	void ScrollRegionScrollHere(CScrollRegion* pSr, int Option = CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
	bool ScrollRegionIsRectClipped(CScrollRegion* pSr, const CUIRect& Rect);

	enum
	{
		POPUP_NONE=0,
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_MESSAGE,
		POPUP_DISCONNECTED,
		POPUP_PURE,
		POPUP_LANGUAGE,
		POPUP_COUNTRY,
		POPUP_DELETE_DEMO,
		POPUP_RENAME_DEMO,
		POPUP_REMOVE_FRIEND,
		POPUP_REMOVE_FILTER,
		POPUP_SAVE_SKIN,
		POPUP_DELETE_SKIN,
		POPUP_SOUNDERROR,
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
		SETTINGS_TEE,
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
	int m_SidebarTab;
	bool m_SidebarActive;
	bool m_ShowServerDetails;
	bool m_UseMouseButtons;
	vec2 m_MousePos;
	vec2 m_PrevMousePos;
	bool m_PopupActive;
	int m_ActiveListBox;
	bool m_SkinModified;

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
		CGameIcon() {};
		CGameIcon(const char *pName) : m_Name(pName) {}

		string m_Name;
		IGraphics::CTextureHandle m_IconTexture;
	};
	array<CGameIcon> m_lGameIcons;
	IGraphics::CTextureHandle m_GameIconDefault;
	void DoGameIcon(const char *pName, const CUIRect *pRect);
	static int GameIconScan(const char *pName, int IsDir, int DirType, void *pUser);

	int64 m_LastInput;

	// loading
	int m_LoadCurrent;
	int m_LoadTotal;

	//
	char m_aMessageTopic[512];
	char m_aMessageBody[512];
	char m_aMessageButton[512];
	int m_NextPopup;

	void PopupMessage(const char *pTopic, const char *pBody, const char *pButton, int Next=POPUP_NONE);

	// some settings
	static float ms_ButtonHeight;
	static float ms_ListheaderHeight;
	static float ms_FontmodHeight;

	// for settings
	bool m_NeedRestartPlayer;
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	int m_TeePartSelected;
	char m_aSaveSkinName[24];

	void SaveSkinfile();
	bool m_RefreshSkinSelector;
	const CSkins::CSkin *m_pSelectedSkin;

	//
	bool m_EscapePressed;
	bool m_EnterPressed;
	bool m_TabPressed;
	bool m_DeletePressed;
	bool m_UpArrowPressed;
	bool m_DownArrowPressed;

	// for map download popup
	int64 m_DownloadLastCheckTime;
	int m_DownloadLastCheckSize;
	float m_DownloadSpeed;

	// for call vote
	int m_CallvoteSelectedOption;
	int m_CallvoteSelectedPlayer;
	char m_aCallvoteReason[VOTE_REASON_LENGTH];

	// for callbacks
	int *m_pActiveDropdown;

	// demo
	struct CDemoItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		int m_StorageType;

		bool m_InfosLoaded;
		bool m_Valid;
		CDemoHeader m_Info;

		bool operator<(const CDemoItem &Other) const { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0; }
	};

	sorted_array<CDemoItem> m_lDemos;
	char m_aCurrentDemoFolder[256];
	char m_aCurrentDemoFile[64];
	int m_DemolistSelectedIndex;
	bool m_DemolistSelectedIsDir;
	int m_DemolistStorageType;
	int64 m_SeekBarActivatedTime;
	bool m_SeekBarActive;

	void DemolistOnUpdate(bool Reset);
	void DemolistPopulate();
	static int DemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);

	// friends
	class CFriendItem
	{
	public:
		const CServerInfo *m_pServerInfo;
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
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
			if(Result < 0 || (Result == 0 && str_comp_nocase(m_aClan, Other.m_aClan) < 0))
				return true;
			
			return false;
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

	class CBrowserFilter
	{
		bool m_Extended;
		int m_Custom;
		char m_aName[64];
		int m_Filter;
		class IServerBrowser *m_pServerBrowser;

		static class CServerFilterInfo ms_FilterStandard;
		static class CServerFilterInfo ms_FilterFavorites;
		static class CServerFilterInfo ms_FilterAll;

	public:

		enum
		{
			FILTER_CUSTOM=0,
			FILTER_ALL,
			FILTER_STANDARD,
			FILTER_FAVORITES,
		};
		// buttons var
		int m_SwitchButton;
		int m_aButtonID[3];

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
		const CServerInfo *SortedGet(int Index) const;
		const void *ID(int Index) const;

		void Reset();
		void GetFilter(class CServerFilterInfo *pFilterInfo) const;
		void SetFilter(const class CServerFilterInfo *pFilterInfo);
	};

	array<CBrowserFilter> m_lFilters;

	int m_SelectedFilter;
	int m_RemoveFilterIndex;

	void LoadFilters();
	void SaveFilters();
	void RemoveFilter(int FilterIndex);
	void Move(bool Up, int Filter);

	class CInfoOverlay
	{
	public:
		enum
		{
			OVERLAY_SERVERINFO=0,
			OVERLAY_HEADERINFO,
			OVERLAY_PLAYERSINFO,
		};

		int m_Type;
		const void *m_pData;
		float m_X;
		float m_Y;
		bool m_Reset;
	};

	CInfoOverlay m_InfoOverlay;
	bool m_InfoOverlayActive;

	class CServerEntry
	{
	public:
		int m_Filter;
		int m_Index;
	};

	CServerEntry m_SelectedServer;

	enum
	{
		FIXED=1,
		SPACER=2,

		COL_BROWSER_FLAG=0,
		COL_BROWSER_NAME,
		COL_BROWSER_GAMETYPE,
		COL_BROWSER_MAP,
		COL_BROWSER_PLAYERS,
		COL_BROWSER_PING,
		NUM_BROWSER_COLS,
	};

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
		CUI::EAlignment m_Align;
	};

	static CColumn ms_aBrowserCols[NUM_BROWSER_COLS];

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

	CVideoFormat m_aVideoFormats[MAX_RESOLUTIONS];
	sorted_array<CVideoMode> m_lRecommendedVideoModes;
	sorted_array<CVideoMode> m_lOtherVideoModes;
	int m_NumVideoFormats;
	int m_CurrentVideoFormat;
	void UpdateVideoFormats();
	void UpdatedFilteredVideoModes();
	void UpdateVideoModeSettings();

	// found in menus.cpp
	int Render();
	//void render_background();
	//void render_loading(float percent);
	void RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);
	void RenderBackButton(CUIRect MainView);

	// found in menus_demo.cpp
	void RenderDemoPlayer(CUIRect MainView);
	void RenderDemoList(CUIRect MainView);

	// found in menus_start.cpp
	void RenderStartMenu(CUIRect MainView);
	void RenderLogo(CUIRect MainView);

	// found in menus_ingame.cpp
	void RenderGame(CUIRect MainView);
	void RenderPlayers(CUIRect MainView);
	void RenderServerInfo(CUIRect MainView);
	void HandleCallvote(int Page, bool Force);
	void RenderServerControl(CUIRect MainView);
	void RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	bool RenderServerControlServer(CUIRect MainView);

	// found in menus_browser.cpp
	// int m_ScrollOffset;
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserSidebar(CUIRect View);
	void RenderServerbrowserFriendTab(CUIRect View);
	void RenderServerbrowserFilterTab(CUIRect View);
	void RenderServerbrowserInfoTab(CUIRect View);
	void RenderServerbrowserFriendList(CUIRect View);
	void RenderDetailInfo(CUIRect View, const CServerInfo *pInfo);
	void RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int RowCount, vec4 TextColor = vec4(1,1,1,1));
	void RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo);
	//void RenderServerbrowserFriends(CUIRect View);
	void RenderServerbrowserBottomBox(CUIRect View);
	void RenderServerbrowserOverlay();
	void RenderFilterHeader(CUIRect View, int FilterIndex);
	int DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainToggleMusic(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void DoFriendListEntry(CUIRect *pView, CFriendItem *pFriend, const void *pID, const CFriendInfo *pFriendInfo, const CServerInfo *pServerInfo, bool Checked, bool Clan = false);
	void SetOverlay(int Type, float x, float y, const void *pData);
	void UpdateFriendCounter(const CServerInfo *pEntry);
	void UpdateFriends();

	// found in menus_settings.cpp
	void RenderLanguageSelection(CUIRect MainView, bool Header=true);
	void RenderThemeSelection(CUIRect MainView, bool Header=true);
	void RenderHSLPicker(CUIRect Picker);
	void RenderSkinSelection(CUIRect MainView);
	void RenderSkinPartSelection(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsTeeBasic(CUIRect MainView);
	void RenderSettingsTeeCustom(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettingsStats(CUIRect MainView);
	void RenderSettings(CUIRect MainView);

	bool DoResolutionList(CUIRect* pRect, CListBoxState* pListBoxState,
						  const sorted_array<CVideoMode>& lModes);

	// found in menu_callback.cpp
	static float RenderSettingsControlsMovement(CUIRect View, void *pUser);
	static float RenderSettingsControlsWeapon(CUIRect View, void *pUser);
	static float RenderSettingsControlsVoting(CUIRect View, void *pUser);
	static float RenderSettingsControlsChat(CUIRect View, void *pUser);
	static float RenderSettingsControlsScoreboard(CUIRect View, void *pUser);
	static float RenderSettingsControlsStats(CUIRect View, void *pUser);
	static float RenderSettingsControlsMisc(CUIRect View, void *pUser);

	void SetActive(bool Active);

	void InvokePopupMenu(void *pID, int Flags, float X, float Y, float W, float H, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra=0);
	void DoPopupMenu();

	IGraphics::CTextureHandle m_TextureBlob;

	void ToggleMusic();

	void SetMenuPage(int NewPage);
public:
	struct CSwitchTeamInfo
	{
		char m_aNotification[128];
		bool m_AllowSpec;
		int m_TimeLeft;
	};
	void GetSwitchTeamInfo(CSwitchTeamInfo *pInfo);
	void RenderBackground();

	void UseMouseButtons(bool Use) { m_UseMouseButtons = Use; }

	static CMenusKeyBinder m_Binder;

	CMenus();

	void RenderLoading();

	bool IsActive() const { return m_MenuActive; }

	virtual void OnInit();

	virtual void OnConsoleInit();
	virtual void OnShutdown();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Event);
	virtual bool OnMouseMove(float x, float y);
};
#endif
