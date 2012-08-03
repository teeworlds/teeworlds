/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <engine/demo.h>
#include <engine/friends.h>

#include <game/voting.h>
#include <game/localization.h>
#include <game/client/component.h>
#include <game/client/ui.h>


// compnent to fetch keypresses, override all other input
class CMenusKeyBinder : public CComponent
{
public:
	bool m_TakeKey;
	bool m_GotKey;
	IInput::CEvent m_Key;
	CMenusKeyBinder();
	virtual bool OnInput(IInput::CEvent Event);
};

enum
{
	NO_SELECTION=0,
	SELECTION_SKIN=1,
	SELECTION_BODY=2,
	SELECTION_TATTOO=4,
	SELECTION_DECORATION=8,
	SELECTION_HANDS=16,
	SELECTION_FEET=32,
	SELECTION_EYES=64
};

class CMenus : public CComponent
{
	static vec4 ms_GuiColor;
	static vec4 ms_ColorTabbarInactiveOutgame;
	static vec4 ms_ColorTabbarActiveOutgame;
	static vec4 ms_ColorTabbarInactiveIngame;
	static vec4 ms_ColorTabbarActiveIngame;
	static vec4 ms_ColorTabbarInactive;
	static vec4 ms_ColorTabbarActive;

	float *ButtonFade(const void *pID, float Seconds, int Checked=0);


	int DoButton_DemoPlayer(const void *pID, const char *pText, const CUIRect *pRect);
	int DoButton_Sprite(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, int Corners);
	int DoButton_SpriteClean(int ImageID, int SpriteID, const CUIRect *pRect);
	int DoButton_SpriteCleanID(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active);
	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, float r=5.0f, float FontFactor=0.0f, int Corners=CUI::CORNER_ALL);
	int DoButton_MenuImage(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, float r=5.0f, float FontFactor=0.0f);
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_MenuTabTop(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_Customize(const void *pID, int Texture, int SpriteID, const CUIRect *pRect, float ImageRatio);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	int DoButton_MouseOver(int ImageID, int SpriteID, const CUIRect *pRect);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_GridHeaderIcon(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, int Corners);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);

	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden=false, int Corners=CUI::CORNER_ALL);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(void *pID, const CUIRect *pRect, int Key);

	//static int ui_do_key_reader(void *id, const CUIRect *rect, int key);
	void UiDoGetButtons(int Start, int Stop, CUIRect View);

	struct CListboxItem
	{
		int m_Visible;
		int m_Selected;
		CUIRect m_Rect;
		CUIRect m_HitRect;
	};

	void UiDoListboxStart(const void *pID, const CUIRect *pRect, float RowHeight, const char *pTitle, const char *pBottomText, int NumItems,
						int ItemsPerRow, int SelectedIndex, float ScrollValue);
	CListboxItem UiDoListboxNextItem(const void *pID, bool Selected = false);
	CListboxItem UiDoListboxNextRow();
	int UiDoListboxEnd(float *pScrollValue, bool *pItemActivated);

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
	bool m_MenuActive;
	bool m_UseMouseButtons;
	vec2 m_MousePos;

	// images
	struct CMenuImage
	{
		char m_aName[64];
		int m_OrgTexture;
		int m_GreyTexture;
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

	void PopupMessage(const char *pTopic, const char *pBody, const char *pButton);

	// TODO: this is a bit ugly but.. well.. yeah
	enum { MAX_INPUTEVENTS = 32 };
	static IInput::CEvent m_aInputEvents[MAX_INPUTEVENTS];
	static int m_NumInputEvents;

	// some settings
	static float ms_ButtonHeight;
	static float ms_ListheaderHeight;
	static float ms_FontmodHeight;

	// for settings
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	int m_TeePartSelection;
	int m_TeePartsColorSelection;
	char m_aSaveSkinName[24];

	void SaveSkinfile();
	void WriteLineSkinfile(IOHANDLE File, const char *pLine);

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

	void DemolistOnUpdate(bool Reset);
	void DemolistPopulate();
	static int DemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);

	// friends
	struct CFriendItem
	{
		const CFriendInfo *m_pFriendInfo;
		int m_NumFound;

		bool operator<(const CFriendItem &Other)
		{
			if(m_NumFound && !Other.m_NumFound)
				return true;
			else if(!m_NumFound && Other.m_NumFound)
				return false;
			else
			{
				int Result = str_comp_nocase(m_pFriendInfo->m_aName, Other.m_pFriendInfo->m_aName);
				if(Result)
					return Result < 0;
				else
					return str_comp_nocase(m_pFriendInfo->m_aClan, Other.m_pFriendInfo->m_aClan) < 0;
			}
		}
	};

	sorted_array<CFriendItem> m_lFriends;
	int m_FriendlistSelectedIndex;

	void FriendlistOnUpdate();

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

		void GetFilter(int *pSortHash, int *pPing, int *pCountry, char* pGametype, char* pServerAddress);
		void SetFilter(int SortHash, int Ping, int Country, const char* pGametype, const char* pServerAddress);
	};

	array<CBrowserFilter> m_lFilters;

	int m_SelectedFilter;

	void RemoveFilter(int FilterIndex);
	void Move(bool Up, int Filter);

	class CInfoOverlay
	{
	public:
		enum
		{
			OVERLAY_SERVERINFO=0,
			OVERLAY_HEADERINFO,
		};

		int m_Type;
		const void *m_pData;
		float m_X;
		float m_Y;
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

		COL_FLAG=0,
		COL_NAME,
		COL_GAMETYPE,
		COL_MAP,
		COL_PLAYERS,
		COL_PING,
		COL_FAVORITE,
		COL_INFO,

		NUM_COLS,
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

	static CColumn ms_aCols[NUM_COLS];

	// found in menus.cpp
	int Render();
	//void render_background();
	//void render_loading(float percent);
	int RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);

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
	void RenderServerControl(CUIRect MainView);
	void RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	void RenderServerControlServer(CUIRect MainView);

	// found in menus_browser.cpp
	int m_ScrollOffset;
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo);
	void RenderServerbrowserFilters(CUIRect View);
	void RenderServerbrowserFriends(CUIRect View);
	void RenderServerbrowserConnect(CUIRect View);
	void RenderServerbrowserOverlay();
	bool RenderFilterHeader(CUIRect View, int FilterIndex);
	int DoBrowserEntry(const void *pID, CUIRect *pRect, const CServerInfo *pEntry);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void SetOverlay(int Type, float x, float y, const void *pData);

	// found in menus_settings.cpp
	void RenderLanguageSelection(CUIRect MainView);
	void RenderHSLPicker(CUIRect Picker);
	void RenderSkinSelection(CUIRect MainView);
	void RenderSkinPartSelection(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettings(CUIRect MainView);

	void SetActive(bool Active);

	void InvokePopupMenu(void *pID, int Flags, float X, float Y, float W, float H, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra=0);
	void DoPopupMenu();

	static int PopupFilter(CMenus *pMenus, CUIRect View);

public:
	void RenderBackground();

	void UseMouseButtons(bool Use) { m_UseMouseButtons = Use; }

	static CMenusKeyBinder m_Binder;

	CMenus();

	void RenderLoading();

	bool IsActive() const { return m_MenuActive; }

	virtual void OnInit();

	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Event);
	virtual bool OnMouseMove(float x, float y);
};
#endif
