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
#include FT_STROKER_H

enum
{
	MAX_FACES = 16,
	MAX_CHARACTERS = 64,
	TEXTURE_SIZE = 2048,
	NUM_PAGES_PER_DIM = 4, // 16 pages total

	FONT_NAME_SIZE = 128,
};

// TODO: use SDF or MSDF font instead of multiple font sizes
static int s_aFontSizes[] = {8,9,10,11,12,13,14,15,16,17,18,19,20,24,36,64};
#define NUM_FONT_SIZES (sizeof(s_aFontSizes)/sizeof(int))
#define PAGE_SIZE (TEXTURE_SIZE/NUM_PAGES_PER_DIM)

struct CGlyph
{
	int m_FontSizeIndex;
	int m_ID;
	int m_AtlasIndex;
	int m_PageID;
	FT_Face m_Face;

	bool m_Rendered;
	float m_Width;
	float m_Height;
	float m_BearingX;
	float m_BearingY;
	float m_AdvanceX;
	float m_aUvCoords[4];
};

struct CGlyphIndex
{
	int m_FontSizeIndex;
	int m_ID;
	CGlyph *m_pGlyph;

	friend bool operator ==(const CGlyphIndex& l, const CGlyphIndex& r)
	{
		return l.m_ID == r.m_ID && l.m_FontSizeIndex == r.m_FontSizeIndex;
	};
	friend bool operator < (const CGlyphIndex& l, const CGlyphIndex& r)
	{
		if(l.m_FontSizeIndex == r.m_FontSizeIndex)
			return l.m_ID < r.m_ID;
		return l.m_FontSizeIndex < r.m_FontSizeIndex;
	};
	friend bool operator > (const CGlyphIndex& l, const CGlyphIndex& r) { return r < l; }
	friend bool operator <=(const CGlyphIndex& l, const CGlyphIndex& r) { return !(l > r); }
	friend bool operator >=(const CGlyphIndex& l, const CGlyphIndex& r) { return !(l < r); }
};

class CGlyphMap
{
	class CAtlas
	{
	public:
		array<ivec3> m_Sections;

		int m_ID;
		int m_Width;
		int m_Height;

		ivec2 m_Offset;

		int m_LastFrameAccess;
		int m_Access;
		bool m_IsEmpty;

		CAtlas() { m_LastFrameAccess = 0; m_Access = 0; }
		int TrySection(int Index, int Width, int Height);
		void Init(int Index, int X, int Y, int Width, int Height);
		ivec2 Add(int Width, int Height);
	};

	IGraphics *m_pGraphics;
	FT_Stroker m_FtStroker;
	IGraphics::CTextureHandle m_aTextures[2];
	CAtlas m_aAtlasPages[NUM_PAGES_PER_DIM*NUM_PAGES_PER_DIM];
	int m_ActiveAtlasIndex;
	sorted_array<CGlyphIndex> m_Glyphs;

	int m_NumTotalPages;

	FT_Face m_DefaultFace;
	FT_Face m_VariantFace;
	FT_Face m_aFallbackFaces[MAX_FACES];
	int m_NumFallbackFaces;

	FT_Face m_aFtFaces[MAX_FACES];
	int m_NumFtFaces;

	int AdjustOutlineThicknessToFontSize(int OutlineThickness, int FontSize);

	void InitTexture(int Width, int Height);
	int FitGlyph(int Width, int Height, ivec2 *Position);
	void UploadGlyph(int TextureIndex, int PosX, int PosY, int Width, int Height, const unsigned char *pData);
	bool SetFaceByName(FT_Face *pFace, const char *pFamilyName);
	int GetCharGlyph(int Chr, FT_Face *pFace);
public:
	CGlyphMap(IGraphics *pGraphics, FT_Library FtLibrary);
	~CGlyphMap();

	IGraphics::CTextureHandle GetTexture(int Index) const { return m_aTextures[Index]; }
	FT_Face GetDefaultFace() const { return m_DefaultFace; }
	int AddFace(FT_Face Face);
	void SetDefaultFaceByName(const char *pFamilyName);
	void AddFallbackFaceByName(const char *pFamilyName);
	void SetVariantFaceByName(const char *pFamilyName);

	bool RenderGlyph(CGlyph *pGlyph, bool Render);
	CGlyph *GetGlyph(int Chr, int FontSizeIndex, bool Render);
	int GetFontSizeIndex(int PixelSize) const;
	vec2 Kerning(CGlyph *pLeft, CGlyph *pRight, int PixelSize);

	int NumTotalPages() const { return m_NumTotalPages; }
	void TouchPage(int Index);
	void PagesAccessReset();
};

struct CFontLanguageVariant
{
	char m_aLanguageFile[IO_MAX_PATH_LENGTH];
	char m_aFamilyName[FONT_NAME_SIZE];
};

struct CWordWidthHint
{
	float m_EffectiveAdvanceX;
	int m_CharCount;
	int m_GlyphCount;
	bool m_EndsWithNewline;
	bool m_IsBroken;
};

class CTextRender : public IEngineTextRender
{
	IGraphics *m_pGraphics;
	IGraphics *Graphics() { return m_pGraphics; }

	vec4 m_TextColor;
	vec4 m_TextSecondaryColor;

	CGlyphMap *m_pGlyphMap;
	void *m_apFontData[MAX_FACES];

	// support regional variant fonts
	int m_NumVariants;
	int m_CurrentVariant;
	CFontLanguageVariant *m_pVariants;

	FT_Library m_FTLibrary;

	int LoadFontCollection(const char *pFilename, const void *pBuf, unsigned FileSize);

	static bool IsWestern(int Chr)
	{
		return Chr >= 0x0020 && Chr <= 0x218F;
	}

	CWordWidthHint MakeWord(CTextCursor *pCursor, const char *pText, const char *pEnd, int FontSizeIndex, float Size, int PixelSize, vec2 ScreenScale);
	void TextRefreshGlyphs(CTextCursor *pCursor);

	void DrawText(CTextCursor *pCursor, vec2 Offset, int Texture, bool IsSecondary, float Alpha, int StartGlyph, int NumGlyphs);

public:
	CTextRender();

	void Init();
	void Update();
	void Shutdown();

	void LoadFonts(IStorage *pStorage, IConsole *pConsole);
	void SetFontLanguageVariant(const char *pLanguageFile);

	void TextColor(const vec4 &Color) { m_TextColor = Color; }
	void TextSecondaryColor(const vec4 &Color) { m_TextSecondaryColor = Color; }

	vec4 GetColor() const { return m_TextColor; }
	vec4 GetSecondaryColor() const { return m_TextSecondaryColor; }

	float TextWidth(float FontSize, const char *pText, int Length);
	void TextDeferred(CTextCursor *pCursor, const char *pText, int Length);
	void TextNewline(CTextCursor *pCursor);
	void TextAdvance(CTextCursor *pCursor, float AdvanceX);
	void TextPlain(CTextCursor *pCursor, const char *pText, int Length);
	void TextOutlined(CTextCursor *pCursor, const char *pText, int Length);
	void TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset);

	void DrawTextPlain(CTextCursor *pCursor, float Alpha, int StartGlyph, int NumGlyphs);
	void DrawTextOutlined(CTextCursor *pCursor, float Alpha, int StartGlyph, int NumGlyphs);
	void DrawTextShadowed(CTextCursor *pCursor, vec2 ShadowOffset, float Alpha, int StartGlyph, int NumGlyphs);

	int CharToGlyph(CTextCursor *pCursor, int NumChars, float *pLineWidth = 0);
	vec2 CaretPosition(CTextCursor *pCursor, int NumChars);
};

#endif
