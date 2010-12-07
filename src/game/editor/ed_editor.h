/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_EDITOR_ED_EDITOR_H
#define GAME_EDITOR_ED_EDITOR_H

#include <base/system.h>
#include <base/math.h>
#include <base/tl/array.h>
#include <base/tl/algorithm.h>
#include <base/tl/sorted_array.h>
#include <base/tl/string.h>

#include <math.h>
#include <game/mapitems.h>
#include <game/client/render.h>

#include <engine/shared/datafile.h>
#include <engine/shared/config.h>
#include <engine/editor.h>
#include <engine/graphics.h>

#include <game/client/ui.h>

typedef void (*INDEX_MODIFY_FUNC)(int *pIndex);

//CRenderTools m_RenderTools;

// CEditor SPECIFIC
enum
{
	MODE_LAYERS=0,
	MODE_IMAGES,
	
	DIALOG_NONE=0,
	DIALOG_FILE,
};

struct CEntity
{
	CPoint m_Position;
	int m_Type;
};

class CEnvelope
{
public:
	int m_Channels;
	array<CEnvPoint> m_lPoints;
	char m_aName[32];
	float m_Bottom, m_Top;
	
	CEnvelope(int Chan)
	{
		m_Channels = Chan;
		m_aName[0] = 0;
		m_Bottom = 0;
		m_Top = 0;
	}
	
	void Resort()
	{
		sort(m_lPoints.all());
		FindTopBottom(0xf);
	}

	void FindTopBottom(int ChannelMask)
	{
		m_Top = -1000000000.0f;
		m_Bottom = 1000000000.0f;
		for(int i = 0; i < m_lPoints.size(); i++)
		{
			for(int c = 0; c < m_Channels; c++)
			{
				if(ChannelMask&(1<<c))
				{
					float v = fx2f(m_lPoints[i].m_aValues[c]);
					if(v > m_Top) m_Top = v;
					if(v < m_Bottom) m_Bottom = v;
				}
			}
		}
	}
	
	int Eval(float Time, float *pResult)
	{
		CRenderTools::RenderEvalEnvelope(m_lPoints.base_ptr(), m_lPoints.size(), m_Channels, Time, pResult);
		return m_Channels;
	}
	
	void AddPoint(int Time, int v0, int v1=0, int v2=0, int v3=0)
	{
		CEnvPoint p;
		p.m_Time = Time;
		p.m_aValues[0] = v0;
		p.m_aValues[1] = v1;
		p.m_aValues[2] = v2;
		p.m_aValues[3] = v3;
		p.m_Curvetype = CURVETYPE_LINEAR;
		m_lPoints.add(p);
		Resort();
	}
	
	float EndTime()
	{
		if(m_lPoints.size())
			return m_lPoints[m_lPoints.size()-1].m_Time*(1.0f/1000.0f);
		return 0;
	}
};


class CLayer;
class CLayerGroup;
class CEditorMap;

class CLayer
{
public:
	class CEditor *m_pEditor;
	class IGraphics *Graphics();
	class ITextRender *TextRender();

	CLayer()
	{
		m_Type = LAYERTYPE_INVALID;
		m_pTypeName = "(invalid)";
		m_Visible = true;
		m_Readonly = false;
		m_SaveToMap = true;
		m_Flags = 0;
		m_pEditor = 0;
	}
	
	virtual ~CLayer()
	{
	}
	
	
	virtual void BrushSelecting(CUIRect Rect) {}
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect) { return 0; }
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect) {}
	virtual void BrushDraw(CLayer *pBrush, float x, float y) {}
	virtual void BrushPlace(CLayer *pBrush, float x, float y) {}
	virtual void BrushFlipX() {}
	virtual void BrushFlipY() {}
	virtual void BrushRotate(float Amount) {}
	
	virtual void Render() {}
	virtual int RenderProperties(CUIRect *pToolbox) { return 0; }
	
	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc) {}
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc) {}
	
	virtual void GetSize(float *w, float *h) { *w = 0; *h = 0;}
	
	const char *m_pTypeName;
	int m_Type;
	int m_Flags;

	bool m_Readonly;
	bool m_Visible;
	bool m_SaveToMap;
};

class CLayerGroup
{
public:
	class CEditorMap *m_pMap;
	
	array<CLayer*> m_lLayers;
	
	int m_OffsetX;
	int m_OffsetY;

	int m_ParallaxX;
	int m_ParallaxY;
	
	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;
	
	const char *m_pName;
	bool m_GameGroup;
	bool m_Visible;
	bool m_SaveToMap;
	
	CLayerGroup();
	~CLayerGroup();
	
	void Convert(CUIRect *pRect);
	void Render();
	void MapScreen();
	void Mapping(float *pPoints);

	void GetSize(float *w, float *h);
	
	void DeleteLayer(int Index);
	int SwapLayers(int Index0, int Index1);
	
	bool IsEmpty() const 
	{
		return m_lLayers.size() == 0;
	}
	
	void Clear() 
	{ 
		m_lLayers.delete_all();
	}
	
	void AddLayer(CLayer *l)
	{
		m_lLayers.add(l);
	}

	void ModifyImageIndex(INDEX_MODIFY_FUNC Func)
	{
		for(int i = 0; i < m_lLayers.size(); i++)
			m_lLayers[i]->ModifyImageIndex(Func);
	}
	
	void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC Func)
	{
		for(int i = 0; i < m_lLayers.size(); i++)
			m_lLayers[i]->ModifyEnvelopeIndex(Func);
	}
};

class CEditorImage : public CImageInfo
{
public:
	CEditor *m_pEditor;
	
	CEditorImage(CEditor *pEditor)
	{
		m_pEditor = pEditor;
		m_TexId = -1;
		m_aName[0] = 0;
		m_External = 0;
		m_Width = 0;
		m_Height = 0;
		m_pData = 0;
		m_Format = 0;
	}
	
	~CEditorImage();
	
	void AnalyseTileFlags();
	
	int m_TexId;
	int m_External;
	char m_aName[128];
	unsigned char m_aTileFlags[256];
};

class CEditorMap
{
	void MakeGameGroup(CLayerGroup *pGroup);
	void MakeGameLayer(CLayer *pLayer);
public:
	CEditor *m_pEditor;

	CEditorMap()
	{
		Clean();
	}

	array<CLayerGroup*> m_lGroups;
	array<CEditorImage*> m_lImages;
	array<CEnvelope*> m_lEnvelopes;
	
	class CLayerGame *m_pGameLayer;
	class CLayerTele *m_pTeleLayer;
	class CLayerSpeedup *m_pSpeedupLayer;
	class CLayerFront *m_pFrontLayer;
	class CLayerSwitch *m_pSwitchLayer;
	CLayerGroup *m_pGameGroup;
	
	CEnvelope *NewEnvelope(int Channels)
	{
		CEnvelope *e = new CEnvelope(Channels);
		m_lEnvelopes.add(e);
		return e;
	}

	void DeleteEnvelope(int Index);
	
	CLayerGroup *NewGroup()
	{
		CLayerGroup *g = new CLayerGroup;
		g->m_pMap = this;
		m_lGroups.add(g);
		return g;
	}
	
	int SwapGroups(int Index0, int Index1)
	{
		if(Index0 < 0 || Index0 >= m_lGroups.size()) return Index0;
		if(Index1 < 0 || Index1 >= m_lGroups.size()) return Index0;
		if(Index0 == Index1) return Index0;
		swap(m_lGroups[Index0], m_lGroups[Index1]);
		return Index1;
	}
	
	void DeleteGroup(int Index)
	{
		if(Index < 0 || Index >= m_lGroups.size()) return;
		delete m_lGroups[Index];
		m_lGroups.remove_index(Index);
	}
	
	void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc)
	{
		for(int i = 0; i < m_lGroups.size(); i++)
			m_lGroups[i]->ModifyImageIndex(pfnFunc);
	}
	
	void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc)
	{
		for(int i = 0; i < m_lGroups.size(); i++)
			m_lGroups[i]->ModifyEnvelopeIndex(pfnFunc);
	}
	
	void Clean();
	void CreateDefault(int EntitiesTexture);

	// io	
	int Save(class IStorage *pStorage, const char *pFilename);
	int Load(class IStorage *pStorage, const char *pFilename, int StorageType);
	void MakeTeleLayer(CLayer *pLayer);
	void MakeSpeedupLayer(CLayer *pLayer);
	void MakeFrontLayer(CLayer *pLayer);
	void MakeSwitchLayer(CLayer *pLayer);
};


struct CProperty
{
	const char *m_pName;
	int m_Value;
	int m_Type;
	int m_Min;
	int m_Max;
};

enum
{
	PROPTYPE_NULL=0,
	PROPTYPE_BOOL,
	PROPTYPE_INT_STEP,
	PROPTYPE_INT_SCROLL,
	PROPTYPE_COLOR,
	PROPTYPE_IMAGE,
	PROPTYPE_ENVELOPE,
	PROPTYPE_SHIFT,
};

typedef struct
{
	int x, y;
	int w, h;
} RECTi;

class CLayerTiles : public CLayer
{
public:
	CLayerTiles(int w, int h);
	~CLayerTiles();

	virtual void Resize(int NewW, int NewH);
	virtual void Shift(int Direction);

	void MakePalette();
	virtual void Render();

	int ConvertX(float x) const;
	int ConvertY(float y) const;
	void Convert(CUIRect Rect, RECTi *pOut);
	void Snap(CUIRect *pRect);
	void Clamp(RECTi *pRect);

	virtual void BrushSelecting(CUIRect Rect);
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect);
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
	virtual void BrushFlipX();
	virtual void BrushFlipY();
	virtual void BrushRotate(float Amount);
	
	virtual int RenderProperties(CUIRect *pToolbox);

	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc);
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc);
	
	void PrepareForSave();

	void GetSize(float *w, float *h) { *w = m_Width*32.0f;  *h = m_Height*32.0f; }
	
	int m_TexId;
	int m_Game;
	int m_Tele;
	int m_Speedup;
	int m_Front;
	int m_Switch;
	int m_Image;
	int m_Width;
	int m_Height;
	CTile *m_pTiles;
};

class CLayerQuads : public CLayer
{
public:
	CLayerQuads();
	~CLayerQuads();

	virtual void Render();
	CQuad *NewQuad();

	virtual void BrushSelecting(CUIRect Rect);
	virtual int BrushGrab(CLayerGroup *pBrush, CUIRect Rect);
	virtual void BrushPlace(CLayer *pBrush, float wx, float wy);
	virtual void BrushFlipX();
	virtual void BrushFlipY();
	virtual void BrushRotate(float Amount);
	
	virtual int RenderProperties(CUIRect *pToolbox);

	virtual void ModifyImageIndex(INDEX_MODIFY_FUNC pfnFunc);
	virtual void ModifyEnvelopeIndex(INDEX_MODIFY_FUNC pfnFunc);
	
	void GetSize(float *w, float *h);
	
	int m_Image;
	array<CQuad> m_lQuads;
};

class CLayerGame : public CLayerTiles
{
public:
	CLayerGame(int w, int h);
	~CLayerGame();

	virtual int RenderProperties(CUIRect *pToolbox);
};

class CLayerTele : public CLayerTiles
{
public:
	CLayerTele(int w, int h);
	~CLayerTele();
	
	CTeleTile *m_pTeleTile;
	unsigned char m_TeleNum;
	
	virtual void Resize(int NewW, int NewH);
	virtual void Shift(int Direction);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect);
};

class CLayerSpeedup : public CLayerTiles
{
public:
	CLayerSpeedup(int w, int h);
	~CLayerSpeedup();
	
	CSpeedupTile *m_pSpeedupTile;
	unsigned char m_SpeedupForce;
	unsigned char m_SpeedupMaxSpeed;
	unsigned char m_SpeedupAngle;
	
	virtual void Resize(int NewW, int NewH);
	virtual void Shift(int Direction);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect);
};

class CLayerFront : public CLayerTiles
{
public:
	CLayerFront(int w, int h);

	virtual void Resize(int NewW, int NewH);
	virtual void Shift(int Direction);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
};

class CLayerSwitch : public CLayerTiles
{
public:
	CLayerSwitch(int w, int h);
	~CLayerSwitch();

	CSwitchTile *m_pSwitchTile;
	unsigned char m_SwitchNumber;
	unsigned char m_SwitchDelay;

	virtual void Resize(int NewW, int NewH);
	virtual void Shift(int Direction);
	virtual void BrushDraw(CLayer *pBrush, float wx, float wy);
	virtual void FillSelection(bool Empty, CLayer *pBrush, CUIRect Rect);
};

class CEditor : public IEditor
{
	class IInput *m_pInput;
	class IClient *m_pClient;
	class IConsole *m_pConsole;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IStorage *m_pStorage;
	CRenderTools m_RenderTools;
	CUI m_UI;
public:
	class IInput *Input() { return m_pInput; };
	class IClient *Client() { return m_pClient; };
	class IConsole *Console() { return m_pConsole; };
	class IGraphics *Graphics() { return m_pGraphics; };
	class ITextRender *TextRender() { return m_pTextRender; };
	class IStorage *Storage() { return m_pStorage; };
	CUI *UI() { return &m_UI; }
	CRenderTools *RenderTools() { return &m_RenderTools; }

	CEditor() : m_TilesetPicker(16, 16)
	{
		m_pInput = 0;
		m_pClient = 0;
		m_pGraphics = 0;
		m_pTextRender = 0;

		m_Mode = MODE_LAYERS;
		m_Dialog = 0;
		m_pTooltip = 0;

		m_aFileName[0] = 0;
		m_ValidSaveFilename = false;
		
		m_FileDialogStorageType = 0;
		m_pFileDialogTitle = 0;
		m_pFileDialogButtonText = 0;
		m_pFileDialogUser = 0;
		m_aFileDialogFileName[0] = 0;
		m_aFileDialogCurrentFolder[0] = 0;
		m_aFileDialogCurrentLink[0] = 0;
		m_pFileDialogPath = m_aFileDialogCurrentFolder;
		m_aFileDialogActivate = false;
		m_FileDialogScrollValue = 0.0f;
		m_FilesSelectedIndex = -1;
		m_FilesStartAt = 0;
		m_FilesCur = 0;
		m_FilesStopAt = 999;

		m_WorldOffsetX = 0;
		m_WorldOffsetY = 0;
		m_EditorOffsetX = 0.0f;
		m_EditorOffsetY = 0.0f;
		
		m_WorldZoom = 1.0f;
		m_ZoomLevel = 200;
		m_LockMouse = false;
		m_ShowMousePointer = true;
		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
		m_MouseDeltaWx = 0;
		m_MouseDeltaWy = 0;
		
		m_GuiActive = true;
		m_ProofBorders = false;
		
		m_ShowDetail = true;
		m_Animate = false;
		m_AnimateStart = 0;
		m_AnimateTime = 0;
		m_AnimateSpeed = 1;
		
		m_ShowEnvelopeEditor = 0;
		
		ms_CheckerTexture = 0;
		ms_BackgroundTexture = 0;
		ms_CursorTexture = 0;
		ms_EntitiesTexture = 0;
		ms_FrontTexture = 0;
		ms_TeleTexture = 0;
		ms_SpeedupTexture = 0;
		ms_SwitchTexture = 0;
		
		ms_pUiGotContext = 0;
		
		m_TeleNumber = 1;
		m_SwitchNum = 1;
		m_SwitchDelay = 0;
		
		m_SpeedupForce = 50;
		m_SpeedupMaxSpeed = 0;
		m_SpeedupAngle = 0;
	}
	
	virtual void Init();
	virtual void UpdateAndRender();
	
	void FilelistPopulate(int StorageType);
	void InvokeFileDialog(int StorageType, int FileType, const char *pTitle, const char *pButtonText,
		const char *pBasepath, const char *pDefaultName,
		void (*pfnFunc)(const char *pFilename, int StorageType, void *pUser), void *pUser);
	
	void Reset(bool CreateDefault=true);
	int Save(const char *pFilename);
	int Load(const char *pFilename, int StorageType);
	int Append(const char *pFilename, int StorageType);
	void Render();

	CQuad *GetSelectedQuad();
	CLayer *GetSelectedLayerType(int Index, int Type);
	CLayer *GetSelectedLayer(int Index);
	CLayerGroup *GetSelectedGroup();
	
	int DoProperties(CUIRect *pToolbox, CProperty *pProps, int *pIds, int *pNewVal);
	
	int m_Mode;
	int m_Dialog;
	const char *m_pTooltip;

	char m_aFileName[512];
	bool m_ValidSaveFilename;

	enum
	{
		FILETYPE_MAP,
		FILETYPE_IMG,

		MAX_PATH_LENGTH = 512
	};
	
	int m_FileDialogStorageType;
	const char *m_pFileDialogTitle;
	const char *m_pFileDialogButtonText;
	void (*m_pfnFileDialogFunc)(const char *pFileName, int StorageType, void *pUser);
	void *m_pFileDialogUser;
	char m_aFileDialogFileName[MAX_PATH_LENGTH];
	char m_aFileDialogCurrentFolder[MAX_PATH_LENGTH];
	char m_aFileDialogCurrentLink[MAX_PATH_LENGTH];
	char *m_pFileDialogPath;
	bool m_aFileDialogActivate;
	int m_FileDialogFileType;
	float m_FileDialogScrollValue;
	int m_FilesSelectedIndex;

	struct CFilelistItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		bool m_IsLink;
		int m_StorageType;
		
		bool operator<(const CFilelistItem &Other) { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0; }
	};
	sorted_array<CFilelistItem> m_FileList;
	int m_FilesStartAt;
	int m_FilesCur;
	int m_FilesStopAt;

	float m_WorldOffsetX;
	float m_WorldOffsetY;
	float m_EditorOffsetX;
	float m_EditorOffsetY;
	float m_WorldZoom;
	int m_ZoomLevel;
	bool m_LockMouse;
	bool m_ShowMousePointer;
	bool m_GuiActive;
	bool m_ProofBorders;
	float m_MouseDeltaX;
	float m_MouseDeltaY;
	float m_MouseDeltaWx;
	float m_MouseDeltaWy;
	
	bool m_ShowDetail;
	bool m_Animate;
	int64 m_AnimateStart;
	float m_AnimateTime;
	float m_AnimateSpeed;
	
	int m_ShowEnvelopeEditor;
	
	int m_SelectedLayer;
	int m_SelectedGroup;
	int m_SelectedQuad;
	int m_SelectedPoints;
	int m_SelectedEnvelope;
	int m_SelectedImage;
	
	static int ms_CheckerTexture;
	static int ms_BackgroundTexture;
	static int ms_CursorTexture;
	static int ms_EntitiesTexture;
	static int ms_FrontTexture;
	static int ms_TeleTexture;
	static int ms_SpeedupTexture;
	static int ms_SwitchTexture;
	
	CLayerGroup m_Brush;
	CLayerTiles m_TilesetPicker;
	
	static const void *ms_pUiGotContext;
	
	CEditorMap m_Map;
	
    void DoMapBorder();
	int DoButton_Editor_Common(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Editor(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_Tab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Ex(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip, int Corners);
	int DoButton_ButtonDec(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_ButtonInc(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_File(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	
	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_MenuItem(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags=0, const char *pToolTip=0);
	
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, bool Hidden=false);

	void RenderBackground(CUIRect View, int Texture, float Size, float Brightness);

	void UiInvokePopupMenu(void *pId, int Flags, float x, float y, float w, float h, int (*pfnFunc)(CEditor *pEditor, CUIRect Rect), void *pExtra=0);
	void UiDoPopupMenu();
	
	int UiDoValueSelector(void *pId, CUIRect *r, const char *pLabel, int Current, int Min, int Max, int Step, float Scale, const char *pToolTip);

	static int PopupGroup(CEditor *pEditor, CUIRect View);
	static int PopupLayer(CEditor *pEditor, CUIRect View);
	static int PopupQuad(CEditor *pEditor, CUIRect View);
	static int PopupPoint(CEditor *pEditor, CUIRect View);
	static int PopupSelectImage(CEditor *pEditor, CUIRect View);
	static int PopupImage(CEditor *pEditor, CUIRect View);
	static int PopupMenuFile(CEditor *pEditor, CUIRect View);
	static int PopupTele(CEditor *pEditor, CUIRect View);
	static int PopupSpeedup(CEditor *pEditor, CUIRect View);
	static int PopupSwitch(CEditor *pEditor, CUIRect View);


	void PopupSelectImageInvoke(int Current, float x, float y);
	int PopupSelectImageResult();
	
	vec4 ButtonColorMul(const void *pId);

	void DoQuadPoint(CQuad *pQuad, int QuadIndex, int v);
	void DoMapEditor(CUIRect View, CUIRect Toolbar);
	void DoToolbar(CUIRect Toolbar);
	void DoQuad(CQuad *pQuad, int Index);
	float UiDoScrollbarV(const void *id, const CUIRect *pRect, float Current);
	vec4 GetButtonColor(const void *id, int Checked);
	
	static void ReplaceImage(const char *pFilename, int StorageType, void *pUser);
	static void AddImage(const char *pFilename, int StorageType, void *pUser);
	
	void RenderImages(CUIRect Toolbox, CUIRect Toolbar, CUIRect View);
	void RenderLayers(CUIRect Toolbox, CUIRect Toolbar, CUIRect View);
	void RenderModebar(CUIRect View);
	void RenderStatusbar(CUIRect View);
	void RenderEnvelopeEditor(CUIRect View);
	
	void RenderMenubar(CUIRect Menubar);
	void RenderFileDialog();

	void AddFileDialogEntry(int Index, CUIRect *pView);
	void SortImages();
	static void ExtractName(const char *pFileName, char *pName, int BufferSize)
	{
		const char *pExtractedName = pFileName;
		const char *pEnd = 0;
		for(; *pFileName; ++pFileName)
		{
			if(*pFileName == '/' || *pFileName == '\\')
				pExtractedName = pFileName+1;
			else if(*pFileName == '.')
				pEnd = pFileName;
		}

		int Length = pEnd > pExtractedName ? min(BufferSize, (int)(pEnd-pExtractedName+1)) : BufferSize;
		str_copy(pName, pExtractedName, Length);
	}
	
	unsigned char m_TeleNumber;
	
	unsigned char m_SpeedupForce;
	unsigned char m_SpeedupMaxSpeed;
	short m_SpeedupAngle;

	unsigned char m_SwitchNum;
	unsigned char m_SwitchDelay;
};

// make sure to inline this function
inline class IGraphics *CLayer::Graphics() { return m_pEditor->Graphics(); }
inline class ITextRender *CLayer::TextRender() { return m_pEditor->TextRender(); }

#endif
