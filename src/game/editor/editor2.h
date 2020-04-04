// LordSk
#ifndef GAME_EDITOR_EDITOR2_H
#define GAME_EDITOR_EDITOR2_H

#include <base/tl/sorted_array.h>

#include <base/system.h>

#include <engine/editor.h>
#include <engine/config.h>

#include <game/client/ui.h>
#include <generated/client_data.h>
#include <game/client/components/mapimages.h>

#include "ed2_map.h"
#include "ed2_ui.h"
#include "ed2_console.h"

class IStorage;
class IGraphics;
class IInput;
class IClient;
class IConsole;
class ITextRender;
class IStorage;

// TODO: make this not inherit CEditor2Ui at some point
class CEditor2: public IEditor, public CEditor2Ui
{
	enum
	{
		MAX_HISTORY = 1024,
	};

	IClient *m_pClient;
	IConsole *m_pConsole;
	IStorage *m_pStorage;
	IConfigManager *m_pConfigManager;
	CConfig *m_pConfig;

	vec2 m_RenderGrenadePickupSize;
	vec2 m_RenderShotgunPickupSize;
	vec2 m_RenderLaserPickupSize;

	vec2 m_MousePos;
	vec2 m_UiMousePos;
	vec2 m_UiMouseDelta;
	vec2 m_MapUiPosOffset;
	float m_Zoom;

	IGraphics::CTextureHandle m_CheckerTexture;
	IGraphics::CTextureHandle m_CursorTexture;
	IGraphics::CTextureHandle m_EntitiesTexture;
	IGraphics::CTextureHandle m_GameTexture;

	CEditorMap2 m_Map;
	bool m_MapSaved;
	CEditorConsoleUI m_InputConsole;

	bool m_ConfigShowGrid;
	bool m_ConfigShowGridMajor;
	bool m_ConfigShowGameEntities;
	bool m_ConfigShowExtendedTilemaps;

	float m_GfxScreenWidth;
	float m_GfxScreenHeight;
	float m_ZoomWorldViewWidth;
	float m_ZoomWorldViewHeight;

	CUIRect m_UiScreenRect;
	CUIRect m_UiMainViewRect;
	int m_UiSelectedLayerID;
	int m_UiSelectedGroupID;
	int m_UiSelectedImageID;

	struct CGroupUiState
	{
		u8 m_IsOpen;
		u8 m_IsHidden;
		u8 m_IsHovered;
	};

	struct CLayerUiState
	{
		u8 m_IsHidden;
		u8 m_IsHovered;
	};

	CPlainArray<CGroupUiState,CEditorMap2::MAX_GROUPS> m_UiGroupState;
	CPlainArray<CLayerUiState,CEditorMap2::MAX_LAYERS> m_UiLayerState;

	struct CUIBrushPalette
	{
		u8 m_aTileSelected[256];
		int m_ImageID;
		bool m_PopupEnabled;

		CUIBrushPalette()
		{
			memset(m_aTileSelected, 0, sizeof(m_aTileSelected));
			m_ImageID = -1;
			m_PopupEnabled = false;
		}
	};
	CUIBrushPalette m_UiBrushPaletteState;
	CUIRect m_UiPopupBrushPaletteRect;
	CUIRect m_UiPopupBrushPaletteImageRect;

	struct CFileListItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		bool m_IsLink;
		int m_StorageType;
		time_t m_Created;
		time_t m_Modified;

		bool operator<(const CFileListItem &Other) { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0; }
	};

	typedef bool (*FILE_SELECT_CALLBACK)(const char *pFilename, void *pContext);
	struct CUIFileSelect
	{
		bool m_NewFile;
		const char *m_pButtonText;
		int m_Selected;
		char m_aPath[512];
		char m_aCompletePath[512];

		void *m_pContext;

		sorted_array<CFileListItem> m_aFileList;
		CUIListBox::Entry *m_pListBoxEntries;

		char m_aFilter[64];

		char m_OutputPath[512];

		CUIFileSelect()
		{
			m_Selected = -1;
			m_aPath[0] = '\0';
			m_aCompletePath[0] = '\0';
			m_aFilter[0] = '\0';
			m_OutputPath[0] = '\0';
		}

		static int EditorListdirCallback(const CFsFileInfo* info, int IsDir, int StorageType, void *pUser);
		void PopulateFileList(IStorage *pStorage, int StorageType);
		void GenerateListBoxEntries();
	};
	CUIFileSelect m_UiFileSelectState;

	bool m_UiDetailPanelIsOpen;

	i8 m_MapViewZoom;
	vec2 m_MapViewMove;

	struct CBrush
	{
		array2<CTile> m_aTiles;
		int m_Width;
		int m_Height;

		inline bool IsEmpty() const { return m_Width <= 0;}

		CBrush()
		{
			m_Width = 0;
			m_Height = 0;
		}
	};

	CBrush m_Brush;
	int m_BrushAutomapRuleID;

	struct CUISnapshot
	{
		int m_SelectedLayerID;
		int m_SelectedGroupID;
		int m_ToolID;
	};

	struct CHistoryEntry
	{
		CHistoryEntry* m_pPrev;
		CHistoryEntry* m_pNext;
		CEditorMap2::CSnapshot* m_pSnap;
		CUISnapshot* m_pUiSnap;
		char m_aActionStr[64];
		char m_aDescStr[64];

		inline void SetAction(const char* pStr)
		{
			const int Len = min((int)(sizeof(m_aActionStr)-1), str_length(pStr));
			memmove(m_aActionStr, pStr, Len);
			m_aActionStr[Len] = 0;
		}

		inline void SetDescription(const char* pStr)
		{
			const int Len = min((int)(sizeof(m_aDescStr)-1), str_length(pStr));
			memmove(m_aDescStr, pStr, Len);
			m_aDescStr[Len] = 0;
		}
	};

	CHistoryEntry m_aHistoryEntries[MAX_HISTORY];
	u8 m_aHistoryEntryUsed[MAX_HISTORY];
	CHistoryEntry* m_pHistoryEntryCurrent;

	enum
	{
		PAGE_MAP_EDITOR=0,
		PAGE_ASSET_MANAGER,
		PAGE_COUNT_,

		TOOL_SELECT=0,
		TOOL_DIMENSION,
		TOOL_TILE_BRUSH,
		TOOL_COUNT_
	};

	int m_Page;
	int m_Tool;

	// TODO: here functions are member functions, but not for CBrush. Choose one or the other
	struct CTileSelection
	{
		int m_StartTX;
		int m_StartTY;
		int m_EndTX;
		int m_EndTY;

		CTileSelection()
		{
			m_StartTX = -1;
		}

		inline void Select(int StartTX, int StartTY, int EndTX, int EndTY)
		{
			m_StartTX = min(StartTX, EndTX);
			m_StartTY = min(StartTY, EndTY);
			m_EndTX = max(StartTX, EndTX);
			m_EndTY = max(StartTY, EndTY);
		}

		inline void Deselect() { m_StartTX = -1; }

		inline bool IsValid() const
		{
			return m_StartTX >= 0 && m_StartTY >= 0 && m_EndTX >= 0 && m_EndTY >= 0;
		}

		inline bool IsSelected() const
		{
			return IsValid() && m_EndTX - m_StartTX >= 0 && m_EndTY - m_StartTY >= 0;
		}

		void FitLayer(const CEditorMap2::CLayer& TileLayer);
	};

	CTileSelection m_TileSelection;

	struct CUIPopup
	{
		typedef void (CEditor2::*Func_PopupRender)(void* pPopupData);

		void* m_pData;
		Func_PopupRender m_pFuncRender;
	};

	CPlainArray<CUIPopup,32> m_UiPopupStack;
	CPlainArray<u8,32> m_UiPopupStackToRemove;
	int m_UiPopupStackCount;
	int m_UiCurrentPopupID;

	void RenderLayerGameEntities(const CEditorMap2::CLayer& GameLayer);

	vec2 CalcGroupScreenOffset(float WorldWidth, float WorldHeight, float PosX, float PosY, float ParallaxX,
		float ParallaxY);
	vec2 CalcGroupWorldPosFromUiPos(int GroupID, float WorldWidth, float WorldHeight, vec2 UiPos);
	CUIRect CalcUiRectFromGroupWorldRect(int GroupID, float WorldWidth, float WorldHeight, CUIRect WorldRect);

	static void StaticEnvelopeEval(float TimeOffset, int EnvID, float *pChannels, void *pUser);
	void EnvelopeEval(float TimeOffset, int EnvID, float *pChannels);

	static bool LoadFileCallback(const char *pFilepath, void *pContext);
	static bool SaveFileCallback(const char *pFilepath, void *pContext);

	void RenderMap();
	void RenderMapOverlay();
	void RenderMapEditorUI();
	void RenderMenuBar(CUIRect TopPanel);
	void RenderMapEditorUiLayerGroups(CUIRect NavRect);
	void RenderHistory(CUIRect NavRect);
	void RenderMapEditorUiDetailPanel(CUIRect DetailRect);

	static void NewFileSaveUnsavedFileCb(void *pContext);
	static void InvokeNewCb(bool Choice, void *pContext);
	static void LoadFileSaveUnsavedFileCb(void *pContext);
	static void InvokeLoadPopupCb(bool Choice, void *pContext);

	typedef void (*CHAIN_CALLBACK)(void *pContext);
	typedef struct
	{
		FILE_SELECT_CALLBACK m_pfnSelectCallback;
		CHAIN_CALLBACK m_pfnChainCallback;
		void *m_pContext;
	} SFileSelectChainContext;
	static bool FileSelectChainCallback(const char *pFilename, void *pContext);

	void SaveMapUi(CHAIN_CALLBACK pfnCallback = 0);

	void RenderPopupMenuFile(void* pPopupData);
	void RenderPopupBrushPalette(void* pPopupData);
	void InvokePopupFileSelect(const char *pButtonText, const char *pInitialPath, bool NewFile, FILE_SELECT_CALLBACK pfnCallback, void *pContext);
	bool DoFileSelect(CUIRect MainRect, CUIFileSelect *pState);
	void RenderPopupMapLoad(void* pPopupData);
	void RenderPopupYesNo(void *pPopupData);

	struct CUIPopupYesNo
	{
		bool m_Active;
		bool m_Done;
		bool m_Choice;
		const char *m_pTitle;
		const char *m_pQuestionText;

		CUIPopupYesNo()
		{
			Reset();
		}

		void Reset()
		{
			m_Active = false;
			m_Done = false;
			m_Choice = false;
		}
	};

	struct CUIPopupLoadMap
	{
		bool m_DoSaveMapBefore;
		CUIPopupYesNo m_PopupWarningSaveMap;

		CUIPopupLoadMap()
		{
			Reset();
			m_PopupWarningSaveMap.m_pTitle = Localize("Warning");
			m_PopupWarningSaveMap.m_pQuestionText = Localize("The current map has not been saved, would you like to save it?");
		}

		void Reset()
		{
			m_DoSaveMapBefore = false;
			m_PopupWarningSaveMap.Reset();
		}
	};

	CUIPopupLoadMap m_UiPopupLoadMap;

	bool DoPopupYesNo(CUIPopupYesNo* state);

	void RenderBrush(vec2 Pos);

	void RenderAssetManager();

	void Reset();
	void ResetCamera();
	void ChangeZoom(float Zoom);
	void ChangePage(int Page);
	void ChangeTool(int Tool);

	void SelectLayerBelowCurrentOne();
	void SelectGroupBelowCurrentOne();
	int GroupCalcDragMoveOffset(int ParentGroupListIndex, int* pRelativePos);
	int LayerCalcDragMoveOffset(int ParentGroupListIndex, int LayerListIndex, int RelativePos);

	void SetNewBrush(CTile* aTiles, int Width, int Height);
	void BrushClear();
	void BrushFlipX();
	void BrushFlipY();
	void BrushRotate90Clockwise();
	void BrushRotate90CounterClockwise();
	void BrushPaintLayer(int PaintTX, int PaintTY, int LayerID);
	void BrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID);
	void TileLayerRegionToBrush(int LayerID, int StartTX, int StartTY, int EndTX, int EndTY);

	void CenterViewOnQuad(const CQuad& Quad);

	inline bool IsToolSelect() const { return m_Tool == TOOL_SELECT; }
	inline bool IsToolDimension() const { return m_Tool == TOOL_DIMENSION; }
	inline bool IsToolBrush() const { return m_Tool == TOOL_TILE_BRUSH; }

	int Save(const char* pFilename);
	bool LoadMap(const char *pFileName);
	bool SaveMap(const char *pFileName);
	void OnMapLoaded();

	void EditDeleteLayer(int LyID, int ParentGroupID);
	void EditDeleteGroup(u32 GroupID);
	void EditDeleteImage(int ImageID);
	void EditAddImage(const char* pFilename);
	void EditCreateAndAddGroup();
	int EditCreateAndAddTileLayerUnder(int UnderLyID, int GroupID);
	int EditCreateAndAddQuadLayerUnder(int UnderLyID, int GroupID);
	void EditLayerChangeImage(int LayerID, int NewImageID);
	void EditLayerHighDetail(int LayerID, bool NewHighDetail);
	void EditGroupUseClipping(int GroupID, bool NewUseClipping);
	int EditGroupOrderMove(int GroupListIndex, int RelativePos);
	int EditLayerOrderMove(int ParentGroupListIndex, int LayerListIndex, int RelativePos);
	void EditTileSelectionFlipX(int LayerID);
	void EditTileSelectionFlipY(int LayerID);
	void EditBrushPaintLayer(int PaintTX, int PaintTY, int LayerID);
	void EditBrushPaintLayerFillRectRepeat(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID);
	void EditBrushPaintLayerAutomap(int PaintTX, int PaintTY, int LayerID, int RulesetID);
	void EditBrushPaintLayerFillRectAutomap(int PaintTX, int PaintTY, int PaintW, int PaintH, int LayerID, int RulesetID);
	void EditTileLayerResize(int LayerID, int NewWidth, int NewHeight);
	void EditTileLayerAutoMapWhole(int LayerID, int RulesetID);
	void EditTileLayerAutoMapSection(int LayerID, int RulesetID, int StartTx, int StartTy, int SectionWidth, int SectionHeight);

	void EditHistCondLayerChangeName(int LayerID, const char* pNewName, bool HistoryCondition);
	void EditHistCondLayerChangeColor(int LayerID, vec4 NewColor, bool HistoryCondition);
	void EditHistCondGroupChangeName(int GroupID, const char* pNewName, bool HistoryCondition);
	void EditHistCondGroupChangeParallax(int GroupID, int NewParallaxX, int NewParallaxY, bool HistoryCondition);
	void EditHistCondGroupChangeOffset(int GroupID, int NewOffsetX, int NewOffsetY, bool HistoryCondition);
	void EditHistCondGroupChangeClipX(int GroupID, int NewClipX, bool HistoryCondition);
	void EditHistCondGroupChangeClipY(int GroupID, int NewClipY, bool HistoryCondition);
	void EditHistCondGroupChangeClipRight(int GroupID, int NewClipRight, bool HistoryCondition);
	void EditHistCondGroupChangeClipBottom(int GroupID, int NewClipBottom, bool HistoryCondition);

	void HistoryClear();
	CHistoryEntry* HistoryAllocEntry();
	void HistoryDeallocEntry(CHistoryEntry* pEntry);
	void HistoryNewEntry(const char* pActionStr, const char* pDescStr);
	void HistoryRestoreToEntry(CHistoryEntry* pEntry);
	void HistoryUndo();
	void HistoryRedo();
	void HistoryDeleteEntry(CHistoryEntry* pEntry);

	CUISnapshot* SaveUiSnapshot();
	void RestoreUiSnapshot(CUISnapshot* pUiSnap);

	// How to make a popup:
	// - Make a CEditor2 RenderMyPopup(void*) function
	// - Pass it to PushPopup() along with some data if needed
	// - Exit the popup with ExitPopup() *inside* RenderMyPopup
	/* Example code:
	 *
	 * void RenderMyPopup(void* pPopupData)
	 * {
	 *		...
	 *		if(something)
	 *			ExitPopup();
	 * }
	 *
	 * PushPopup(&CEditor2::RenderMyPopup, ...);
	 *
	*/
	void PushPopup(CUIPopup::Func_PopupRender pFuncRender, void* pPopupData);
	void ExitPopup();
	void RenderPopups();

	const char* GetLayerName(int LayerID);
	const char* GetGroupName(int GroupID);

	static void ConLoad(IConsole::IResult *pResult, void *pUserData);
	static void ConSave(IConsole::IResult *pResult, void *pUserData);
	static void ConShowPalette(IConsole::IResult *pResult, void *pUserData);
	static void ConGameView(IConsole::IResult *pResult, void *pUserData);
	static void ConShowGrid(IConsole::IResult *pResult, void *pUserData);
	static void ConUndo(IConsole::IResult *pResult, void *pUserData);
	static void ConRedo(IConsole::IResult *pResult, void *pUserData);
	static void ConDeleteImage(IConsole::IResult *pResult, void *pUserData);

	inline IClient *Client() { return m_pClient; };
	inline IConsole *Console() { return m_pConsole; };
	inline IStorage *Storage() { return m_pStorage; };
	inline CConfig *Config() { return m_pConfig; }

public:

	CEditor2();
	~CEditor2();

	void Init();
	void UpdateAndRender();
	void Update();
	void Render();
	bool HasUnsavedData() const;
	void OnInput(IInput::CEvent Event);
};

#endif
