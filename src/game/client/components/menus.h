/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <engine/graphics.h>
#include <engine/demo.h>
#include <engine/contacts.h>
#include <engine/serverbrowser.h>

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

class IScrollbarScale
{
public:
	virtual float ToRelative(int AbsoluteValue, int Min, int Max) = 0;
	virtual int ToAbsolute(float RelativeValue, int Min, int Max) = 0;
};
static class CLinearScrollbarScale : public IScrollbarScale
{
public:
	float ToRelative(int AbsoluteValue, int Min, int Max)
	{
		return (AbsoluteValue - Min) / (float)(Max - Min);
	}
	int ToAbsolute(float RelativeValue, int Min, int Max)
	{
		return round_to_int(RelativeValue*(Max - Min) + Min + 0.1f);
	}
} LinearScrollbarScale;
static class CLogarithmicScrollbarScale : public IScrollbarScale
{
private:
	int m_MinAdjustment;
public:
	CLogarithmicScrollbarScale(int MinAdjustment)
	{
		m_MinAdjustment = max(MinAdjustment, 1); // must be at least 1 to support Min == 0 with logarithm
	}
	float ToRelative(int AbsoluteValue, int Min, int Max)
	{
		if(Min < m_MinAdjustment)
		{
			AbsoluteValue += m_MinAdjustment;
			Min += m_MinAdjustment;
			Max += m_MinAdjustment;
		}
		return (log(AbsoluteValue) - log(Min)) / (float)(log(Max) - log(Min));
	}
	int ToAbsolute(float RelativeValue, int Min, int Max)
	{
		int ResultAdjustment = 0;
		if(Min < m_MinAdjustment)
		{
			Min += m_MinAdjustment;
			Max += m_MinAdjustment;
			ResultAdjustment = -m_MinAdjustment;
		}
		return round_to_int(exp(RelativeValue*(log(Max) - log(Min)) + log(Min))) + ResultAdjustment;
	}
} LogarithmicScrollbarScale(25);

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

	int DoIcon(int ImageId, int SpriteId, const CUIRect *pRect);
	void DoIconColor(int ImageId, int SpriteId, const CUIRect *pRect, const vec4& Color);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, CUI::EAlignment Align, const CUIRect *pRect);

	bool DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden=false, int Corners=CUI::CORNER_ALL);
	void DoEditBoxOption(void *pID, char *pOption, int OptionLength, const CUIRect *pRect, const char *pStr, float VSplitVal, float *pOffset, bool Hidden=false);
	void DoScrollbarOption(void *pID, int *pOption, const CUIRect *pRect, const char *pStr, int Min, int Max, IScrollbarScale *pScale = &LinearScrollbarScale, bool Infinite=false);
	float DoDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback);
	float DoIndependentDropdownMenu(void *pID, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, bool* pActive);
	void DoInfoBox(const CUIRect *pRect, const char *pLable, const char *pValue);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoJoystickBar(const CUIRect *pRect, float Current, float Tolerance, bool Active);
	void DoButton_KeySelect(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(CButtonContainer *pPC, const CUIRect *pRect, int Key, int Modifier, int* NewModifier);
	void UiDoGetButtons(int Start, int Stop, CUIRect View, float ButtonHeight, float Spacing);


	// Scroll region : found in menus_scrollregion.cpp
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
			m_ClipBgColor = vec4(0.0f, 0.0f, 0.0f, 0.25f);
			m_ScrollbarBgColor = vec4(0.0f, 0.0f, 0.0f, 0.25f);
			m_RailBgColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
			m_SliderColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
			m_SliderColorHover = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_SliderColorGrabbed = vec4(0.9f, 0.9f, 0.9f, 1.0f);
			m_Flags = 0;
		}
	};

	/*
	Usage:
		-- Initialization --
		static CScrollRegion s_ScrollRegion(this);
		vec2 ScrollOffset(0, 0);
		s_ScrollRegion.Begin(&ScrollRegionRect, &ScrollOffset);
		Content = ScrollRegionRect;
		Content.y += ScrollOffset.y;

		-- "Register" your content rects --
		CUIRect Rect;
		Content.HSplitTop(SomeValue, &Rect, &Content);
		s_ScrollRegion.AddRect(Rect);

		-- [Optional] Knowing if a rect is clipped --
		s_ScrollRegion.IsRectClipped(Rect);

		-- [Optional] Scroll to a rect (to the last added rect)--
		...
		s_ScrollRegion.AddRect(Rect);
		s_ScrollRegion.ScrollHere(Option);

		-- End --
		s_ScrollRegion.End();
	*/
	// Instances of CScrollRegion must be static, as member addresses are used as UI item IDs
	class CScrollRegion
	{
	private:
		CRenderTools *m_pRenderTools;
		CUI *m_pUI;
		IInput *m_pInput;

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

	public:
		enum {
			SCROLLHERE_KEEP_IN_VIEW=0,
			SCROLLHERE_TOP,
			SCROLLHERE_BOTTOM,
		};

		CScrollRegion(CMenus *pMenus);
		void Begin(CUIRect* pClipRect, vec2* pOutOffset, const CScrollRegionParams* pParams = 0);
		void End();
		void AddRect(CUIRect Rect);
		void ScrollHere(int Option = CScrollRegion::SCROLLHERE_KEEP_IN_VIEW);
		bool IsRectClipped(const CUIRect& Rect) const;
		bool IsScrollbarShown() const;
	};

	// Listbox : found in menus_listbox.cpp
	struct CListboxItem
	{
		int m_Visible;
		int m_Selected;
		CUIRect m_Rect;
	};

	// Instances of CListBox must be static, as member addresses are used as UI item IDs
	class CListBox
	{
	private:
		CMenus *m_pMenus; // TODO: refactor to remove this
		CRenderTools *m_pRenderTools;
		CUI *m_pUI;
		IInput *m_pInput;

		CUIRect m_ListBoxView;
		float m_ListBoxRowHeight;
		int m_ListBoxItemIndex;
		int m_ListBoxSelectedIndex;
		int m_ListBoxNewSelected;
		int m_ListBoxNewSelOffset;
		int m_ListBoxUpdateScroll;
		int m_ListBoxDoneEvents;
		int m_ListBoxNumItems;
		int m_ListBoxItemsPerRow;
		bool m_ListBoxItemActivated;
		const char *m_pBottomText;
		float m_FooterHeight;
		CScrollRegion m_ScrollRegion;
		vec2 m_ScrollOffset;
		char m_aFilterString[64];
		float m_OffsetFilter;

	protected:
		CListboxItem DoNextRow();

	public:
		CListBox(CMenus *pMenus);

		void DoHeader(const CUIRect *pRect, const char *pTitle, float HeaderHeight = 20.0f, float Spacing = 2.0f);
		bool DoFilter(float FilterHeight = 20.0f, float Spacing = 2.0f);
		void DoFooter(const char *pBottomText, float FooterHeight = 20.0f); // call before DoStart to create a footer
		void DoStart(float RowHeight, int NumItems, int ItemsPerRow, int SelectedIndex,
					const CUIRect *pRect = 0, bool Background = true, bool *pActive = 0);
		CListboxItem DoNextItem(const void *pID, bool Selected = false, bool *pActive = 0);
		int DoEnd(bool *pItemActivated);
		bool FilterMatches(const char *pNeedle);
	};


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
	bool m_UseMouseButtons;
	vec2 m_MousePos;
	vec2 m_PrevMousePos;
	bool m_CursorActive;
	bool m_PrevCursorActive;
	bool m_PopupActive;
	int m_ActiveListBox;
	bool m_SkinModified;
	bool m_KeyReaderWasActive;
	bool m_KeyReaderIsActive;

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
	char m_aFilterString[VOTE_REASON_LENGTH];
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
		static CServerFilterInfo ms_FilterFavorites;
		static CServerFilterInfo ms_FilterAll;

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

	enum
	{
		COL_BROWSER_FLAG=0,
		COL_BROWSER_NAME,
		COL_BROWSER_GAMETYPE,
		COL_BROWSER_MAP,
		COL_BROWSER_PLAYERS,
		COL_BROWSER_PING,
		NUM_BROWSER_COLS,

		ADDR_SELECTION_CHANGE = 1,
		ADDR_SELECTION_RESET_SERVER_IF_NOT_FOUND = 2,
		ADDR_SELECTION_REVEAL = 4,
		ADDR_SELECTION_UPDATE_ADDRESS = 8,
	};
	int m_SidebarTab;
	bool m_SidebarActive;
	bool m_ShowServerDetails;
	int m_LastBrowserType; // -1 if not initialized
	int m_aSelectedFilters[IServerBrowser::NUM_TYPES]; // -1 if none selected, -2 if not initialized
	int m_aSelectedServers[IServerBrowser::NUM_TYPES]; // -1 if none selected
	int m_AddressSelection;
	static CColumn ms_aBrowserCols[NUM_BROWSER_COLS];

	CBrowserFilter* GetSelectedBrowserFilter()
	{
		const int Tab = ServerBrowser()->GetType();
		if(m_aSelectedFilters[Tab] == -1)
			return 0;
		return &m_lFilters[m_aSelectedFilters[Tab]];
	}

	const CServerInfo* GetSelectedServerInfo()
	{
		CBrowserFilter* pSelectedFilter = GetSelectedBrowserFilter();
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
	void RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);
	void RenderBackButton(CUIRect MainView);
	inline float GetListHeaderHeight() const { return ms_ListheaderHeight + (g_Config.m_UiWideview ? 3.0f : 0.0f); }
	inline float GetListHeaderHeightFactor() const { return 1.0f + (g_Config.m_UiWideview ? (3.0f/ms_ListheaderHeight) : 0.0f); }

	// found in menus_demo.cpp
	void RenderDemoPlayer(CUIRect MainView);
	void RenderDemoList(CUIRect MainView);
	static float RenderDemoDetails(CUIRect View, void *pUser);

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
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserSidebar(CUIRect View);
	void RenderServerbrowserFriendTab(CUIRect View);
	void RenderServerbrowserFilterTab(CUIRect View);
	void RenderServerbrowserInfoTab(CUIRect View);
	void RenderServerbrowserFriendList(CUIRect View);
	void RenderDetailInfo(CUIRect View, const CServerInfo *pInfo);
	void RenderDetailScoreboard(CUIRect View, const CServerInfo *pInfo, int RowCount, vec4 TextColor = vec4(1,1,1,1));
	void RenderServerbrowserServerDetail(CUIRect View, const CServerInfo *pInfo);
	void RenderServerbrowserBottomBox(CUIRect View);
	void RenderServerbrowserOverlay();
	void RenderFilterHeader(CUIRect View, int FilterIndex);
	int DoBrowserEntry(const void *pID, CUIRect View, const CServerInfo *pEntry, const CBrowserFilter *pFilter, bool Selected);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserSortingUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainToggleMusic(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	void DoFriendListEntry(CUIRect *pView, CFriendItem *pFriend, const void *pID, const CContactInfo *pFriendInfo, const CServerInfo *pServerInfo, bool Checked, bool Clan = false);
	void SetOverlay(int Type, float x, float y, const void *pData);
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
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsTeeBasic(CUIRect MainView);
	void RenderSettingsTeeCustom(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettings(CUIRect MainView);

	bool DoResolutionList(CUIRect* pRect, CListBox* pListBox,
						  const sorted_array<CVideoMode>& lModes);

	// found in menu_callback.cpp
	static float RenderSettingsControlsMouse(CUIRect View, void *pUser);
	static float RenderSettingsControlsJoystick(CUIRect View, void *pUser);
	static float RenderSettingsControlsMovement(CUIRect View, void *pUser);
	static float RenderSettingsControlsWeapon(CUIRect View, void *pUser);
	static float RenderSettingsControlsVoting(CUIRect View, void *pUser);
	static float RenderSettingsControlsChat(CUIRect View, void *pUser);
	static float RenderSettingsControlsScoreboard(CUIRect View, void *pUser);
	static float RenderSettingsControlsStats(CUIRect View, void *pUser);
	static float RenderSettingsControlsMisc(CUIRect View, void *pUser);

	void DoJoystickAxisPicker(CUIRect View);

	void SetActive(bool Active);

	void InvokePopupMenu(void *pID, int Flags, float X, float Y, float W, float H, int (*pfnFunc)(CMenus *pMenu, CUIRect Rect), void *pExtra=0);
	void DoPopupMenu();

	IGraphics::CTextureHandle m_TextureBlob;

	void ToggleMusic();

	void SetMenuPage(int NewPage);

	bool CheckHotKey(int Key);
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
