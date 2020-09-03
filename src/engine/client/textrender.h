/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_TEXTRENDER_H
#define ENGINE_CLIENT_TEXTRENDER_H

#include <base/tl/sorted_array.h>
#include <base/vmath.h>
#include <engine/textrender.h>

// ft2 texture
#include <ft2build.h>
#include FT_FREETYPE_H

enum
{
	MAX_FACES = 16,
	MAX_CHARACTERS = 64,
	TEXTURE_SIZE = 1024,
	PAGE_COUNT = 2,
};

// TODO: use SDF or MSDF font instead of multiple font sizes
static int aFontSizes[] = {8,9,10,11,12,13,14,15,16,17,18,19,20,36,64};
#define NUM_FONT_SIZES (sizeof(aFontSizes)/sizeof(int))
#define PAGE_SIZE (TEXTURE_SIZE/PAGE_COUNT)

// 32k of data used for rendering glyphs
static unsigned char s_aGlyphData[(1024/8) * (1024/8)];
static unsigned char s_aGlyphDataOutlined[(1024/8) * (1024/8)];

struct CGlyph
{
	int m_FontSizeIndex;
	int m_ID;

	int m_AtlasIndex;
	int m_PageID;
	FT_Face m_Face;

	float m_Width;
	float m_Height;
	float m_OffsetX;
	float m_OffsetY;
	float m_AdvanceX;
	float m_aUvs[4];

	friend bool operator ==(const CGlyph& l, const CGlyph& r)
	{
		return l.m_ID == r.m_ID && l.m_FontSizeIndex == r.m_FontSizeIndex;
	};
	friend bool operator < (const CGlyph& l, const CGlyph& r)
	{
		if (l.m_FontSizeIndex == r.m_FontSizeIndex) return l.m_ID < r.m_ID;
		return l.m_FontSizeIndex < r.m_FontSizeIndex;
	};
	friend bool operator > (const CGlyph& l, const CGlyph& r) { return r < l; };
	friend bool operator <=(const CGlyph& l, const CGlyph& r) { return !(l > r); };
	friend bool operator >=(const CGlyph& l, const CGlyph& r) { return !(l < r); };
};

class CAtlas
{
	array<ivec3> m_Sections;

	int m_ID;
	int m_Width;
	int m_Height;

	ivec2 m_Offset;

	int m_LastFrameAccess;
	int m_Access;

	int TrySection(int Index, int Width, int Height);
public:
	void Init(int Index, int X, int Y, int Width, int Height);

	ivec2 Add(int Width, int Height);

	int GetWidth() { return m_Width; }
	int GetHeight() { return m_Height; }
	int GetOffsetX() { return m_Offset.x; }
	int GetOffsetY() { return m_Offset.y; }

	int GetPageID() { return m_ID; }
	void Touch() { m_Access++; }
	int GetAccess() { return m_LastFrameAccess; }
	void Update() { m_LastFrameAccess = m_Access; m_Access = 0; }
};

class CGlyphMap
{
	IGraphics *m_pGraphics;
	IGraphics::CTextureHandle m_aTextures[2];
	CAtlas m_aAtlasPages[PAGE_COUNT*PAGE_COUNT];
	sorted_array<CGlyph> m_Glyphs;

	int m_NumTotalPages;

	FT_Face m_DefaultFace;
	FT_Face m_VariantFace;
	FT_Face m_aFallbackFaces[MAX_FACES];
	int m_NumFallbackFaces;

	FT_Face m_aFtFaces[MAX_FACES];
	int m_NumFtFaces;

	int AdjustOutlineThicknessToFontSize(int OutlineThickness, int FontSize);

	void Grow(unsigned char *pIn, unsigned char *pOut, int w, int h);
	void InitTexture(int Width, int Height);
	int FitGlyph(int Width, int Height, ivec2 *Position);
	void UploadGlyph(int TextureIndex, int PosX, int PosY, int Width, int Height, const unsigned char *pData);
	bool RenderGlyph(int Chr, int FontSizeIndex, CGlyph *pGlyph);
	bool SetFaceByName(FT_Face *pFace, const char *pFamilyName);
public:
	CGlyphMap(IGraphics *pGraphics);

	IGraphics::CTextureHandle GetTexture(int Index) { return m_aTextures[Index]; }

	FT_Face GetDefaultFace() { return m_DefaultFace; };
	int GetCharGlyph(int Chr, FT_Face *pFace);
	int AddFace(FT_Face Face);
	void SetDefaultFaceByName(const char *pFamilyName);
	void AddFallbackFaceByName(const char *pFamilyName);
	void SetVariantFaceByName(const char *pFamilyName);
	
	CGlyph *GetGlyph(int Chr, int FontSizeIndex);
	int GetFontSizeIndex(int PixelSize);
	vec2 Kerning(CGlyph *pLeft, CGlyph *pRight);

	void PagesAccessReset();
};

struct CFontLanguageVariant
{
	char m_aLanguageFile[128];
	char m_aFamilyName[128];
};

class CTextRender : public IEngineTextRender
{
	IGraphics *m_pGraphics;
	IGraphics *Graphics() { return m_pGraphics; }

	int WordLength(const char *pText)
	{
		int Length = 0;
		while(1)
		{
			const char *pCursor = (pText + Length);
			if(*pCursor == 0)
				return Length;
			if(*pCursor == '\n' || *pCursor == '\t' || *pCursor == ' ')
				return Length+1;
			Length = str_utf8_forward(pText, Length);
		}
	}

	float m_TextR;
	float m_TextG;
	float m_TextB;
	float m_TextA;

	float m_TextOutlineR;
	float m_TextOutlineG;
	float m_TextOutlineB;
	float m_TextOutlineA;

	//int m_FontTextureFormat;

	CGlyphMap *m_pGlyphMap;

	// support regional variant fonts
	int m_NumVariants;
	int m_CurrentVariant;
	CFontLanguageVariant *m_paVariants;

	FT_Library m_FTLibrary;

	int LoadFontCollection(const char *pFilename);

public:
	CTextRender();

	virtual void Init();
	void Update();
	void Shutdown();

	virtual void LoadFonts(IStorage *pStorage, IConsole *pConsole);
	virtual void SetFontLanguageVariant(const char *pLanguageFile);

	virtual void SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags);

	virtual void Text(void *pFontSetV, float x, float y, float Size, const char *pText, float LineWidth, bool MultiLine);
	virtual float TextWidth(void *pFontSetV, float Size, const char *pText, int StrLength, float LineWidth);
	virtual int TextLineCount(void *pFontSetV, float Size, const char *pText, float LineWidth);
	virtual void TextColor(float r, float g, float b, float a);
	virtual void TextOutlineColor(float r, float g, float b, float a);
	
	virtual void TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset,
							  vec4 ShadowColor, vec4 TextColor_);
	virtual void TextDeferredRenderEx(CTextCursor *pCursor, const char *pText, int Length, CQuadChar* aQuadChar,
							int QuadCharMaxCount, int* pQuadCharCount,
							IGraphics::CTextureHandle* pFontTexture);
	virtual void TextEx(CTextCursor *pCursor, const char *pText, int Length);
	
	float TextGetLineBaseY(const CTextCursor *pCursor);
};

#endif
