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
	int DoButton_SpriteID(CButtonContainer *pBC, int ImageID, int SpriteID, const CUIRect *pRect, int Corners=CUI::CORNER_ALL, float r=5.0f, bool Fade=true);
	int DoButton_SpriteClean(int ImageID, int SpriteID, const CUIRect *pRect);
	int DoButton_SpriteCleanID(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, bool Blend=true);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active);
	int DoButton_Menu(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName=0, int Corners=CUI::CORNER_ALL, float r=5.0f, float FontFactor=0.0f, vec4 ColorHot=vec4(1.0f, 1.0f, 1.0f, 0.75f), bool TextFade=true);
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_MenuTabTop(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, int Corners=CUI::CORNER_ALL, float r=5.0f, float FontFactor=0.0f);
	int DoButton_Customize(CButtonContainer *pBC, IGraphics::CTextureHandle Texture, int SpriteID, const CUIRect *pRect, float ImageRatio);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect, bool Checked=false);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	int DoButton_MouseOver(int ImageID, int SpriteID, const CUIRect *pRect);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoIcon(int ImageId, int SpriteId, const CUIRect *pRect);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_GridHeaderIcon(CButtonContainer *pBC, int ImageID, int SpriteID, const CUIRect *pRect, int Corners);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);

	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden=false, int Corners=CUI::CORNER_ALL);
	void DoEditBoxOption(void *pID, char *pOption, int OptionLength, const CUIRect *pRect, const char *pStr, float VSplitVal, float *pOffset, bool Hidden=false);
	void DoScrollbarOption(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, float VSplitVal, int Min, int Max, bool infinite=false);
	float DoDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback);
	void DoInfoBox(const CUIRect *pRect, const char *pLable, const char *pValue);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoButton_KeySelect(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(CButtonContainer *pPC, const CUIRect *pRect, int Key);

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
	CListboxItem UiDoListboxNextItem(CListBoxState* pState, const void *pID, bool Selected = false);
	CListboxItem UiDoListboxNextRow(CListBoxState* pState);
	int UiDoListboxEnd(CListBoxState* pState, bool *pItemActivated);

	//static void demolist_listdir_callback(const char *name, int is_dir, void *user);
	//static void demolist_list_callback(const CUIRect *rect, int index, void *user);

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
		PAGE_FRIENDS,
		PAGE_DEMOS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
		PAGE_START,

		PAGE_BROWSER_BROWSER=0,
		PAGE_BROWSER_FRIENDS,
		NUM_PAGE_BROWSER,

		SETTINGS_GENERAL=0,
		SETTINGS_PLAYER,
		SETTINGS_TEE,
		SETTINGS_CONTROLS,
		SETTINGS_GRAPHICS,
		SETTINGS_SOUND,
	};

	int m_GamePage;
	int m_Popup;
	int m_ActivePage;
	int m_MenuPage;
	int m_BorwserPage;
	bool m_MenuActive;
	bool m_UseMouseButtons;
	vec2 m_MousePos;
	vec2 m_PrevMousePos;
	bool m_InfoMode;
	bool m_PopupActive;

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
	bool m_DeletePressed;

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

		bool operator<(const CDemoItem &Other) { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
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
		class CClanFriendItem
		{
		public:
			CClanFriendItem()
			{
				m_lFriendInfos.clear();
				m_lServerInfos.clear();
			}

			~CClanFriendItem()
			{
				m_lFriendInfos.clear();
				m_lServerInfos.clear();
			}

			array<CFriendInfo> m_lFriendInfos;
			array<const CServerInfo*> m_lServerInfos;
		};

		int m_NumFound;
		const CFriendInfo *m_pFriendInfo;
		const CServerInfo *m_pServerInfo;

		CClanFriendItem m_ClanFriend;

		CFriendItem()
		{
			m_pFriendInfo = 0;
			m_pServerInfo = 0;
		}

		bool IsClanFriend()
		{
			return m_pFriendInfo->m_aClan[0] && !m_pFriendInfo->m_aName[0];
		}

		void Reset()
		{
			m_NumFound = 0;
			m_ClanFriend.m_lFriendInfos.clear();
			m_ClanFriend.m_lServerInfos.clear();
		}
	};

	struct CSelectedFriend
	{
		bool m_ClanFriend;
		bool m_FakeFriend;
		unsigned m_NameHash;
		unsigned m_ClanHash;
	};

	enum
	{
		FRIENDS_SORT_TYPE = 0,
		FRIENDS_SORT_SERVER,
		FRIENDS_SORT_NAME,
		FRIENDS_SORT_CLAN,
	};

	int *m_pFriendIndexes;
	array<CFriendItem> m_lFriends;
	int m_FriendlistSelectedIndex;
	const CFriendInfo *m_pDeleteFriendInfo;
	CSelectedFriend m_SelectedFriend;

	bool SortCompareName(int Index1, int Index2) const;
	bool SortCompareClan(int Index1, int Index2) const;
	bool SortCompareServer(int Index1, int Index2) const;
	bool SortCompareType(int Index1, int Index2) const;

	void FriendlistOnUpdate();
	void SortFriends();

	class CBrowserFilter
	{
		bool m_Extended;
		int m_Custom;
		char m_aName[64];
		int m_Filter;
		class IServerBrowser *m_pServerBrowser;

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

		CBrowserFilter() {}
		CBrowserFilter(int Custom, const char* pName, IServerBrowser *pServerBrowser, int Filter, int Ping, int Country, const char* pGametype, const char* pServerAddress);
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

		void GetFilter(class CServerFilterInfo *pFilterInfo) const;
		void SetFilter(const class CServerFilterInfo *pFilterInfo);
	};

	array<CBrowserFilter> m_lFilters;

	int m_SelectedFilter;

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
		COL_BROWSER_FAVORITE,
		COL_BROWSER_INFO,
		NUM_BROWSER_COLS,

		COL_FRIEND_TYPE = 0,
		COL_FRIEND_SERVER,
		COL_FRIEND_NAME,
		COL_FRIEND_CLAN,
		COL_FRIEND_DELETE,
		NUM_FRIEND_COLS,
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
	};

	static CColumn ms_aBrowserCols[NUM_BROWSER_COLS];
	static CColumn ms_aFriendCols[NUM_FRIEND_COLS];

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
	void RenderServerControlServer(CUIRect MainView);

	// found in menus_browser.cpp
	int m_ScrollOffset;
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserFriendList(CUIRect View);
	void RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo);
	//void RenderServerbrowserFriends(CUIRect View);
	void RenderServerbrowserBottomBox(CUIRect View);
	void RenderServerbrowserOverlay();
	bool RenderFilterHeader(CUIRect View, int FilterIndex);
	int DoBrowserEntry(const void *pID, CUIRect *pRect, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected);
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
	void RenderSettings(CUIRect MainView);

	bool DoResolutionList(CUIRect* pRect, CListBoxState* pListBoxState,
						  const sorted_array<CVideoMode>& lModes);

	// found in menu_callback.cpp
	static float RenderSettingsControlsMovement(CUIRect View, void *pUser);
	static float RenderSettingsControlsWeapon(CUIRect View, void *pUser);
	static float RenderSettingsControlsVoting(CUIRect View, void *pUser);
	static float RenderSettingsControlsChat(CUIRect View, void *pUser);
	static float RenderSettingsControlsMisc(CUIRect View, void *pUser);

	void SetActive(bool Active);

	void InvokePopupMenu(void *pID, int Flags, float X, float Y, float W, float H, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra=0);
	void DoPopupMenu();

	static int PopupFilter(CMenus *pMenus, CUIRect View);

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
