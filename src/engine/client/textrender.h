/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_TEXTRENDER_H
#define ENGINE_CLIENT_TEXTRENDER_H

#include <base/tl/array.h>
#include <base/vmath.h>
#include <engine/textrender.h>

// ft2 texture
#include <ft2build.h>
#include FT_FREETYPE_H

enum
{
	MAX_FACES = 16,
	MAX_CHARACTERS = 64,
};

static int aFontSizes[] = {8,9,10,11,12,13,14,15,16,17,18,19,20,36,64};
#define NUM_FONT_SIZES (sizeof(aFontSizes)/sizeof(int))

struct CGlyph
{
	int m_ID;

	// these values are scaled to the pFont size
	// width * font_size == real_size
	float m_Width;
	float m_Height;
	float m_OffsetX;
	float m_OffsetY;
	float m_AdvanceX;

	float m_aUvs[4];
	int64 m_TouchTime;
};

struct CFontSizeData
{
	int m_FontSize;

	IGraphics::CTextureHandle m_aTextures[2];
	int m_TextureWidth;
	int m_TextureHeight;

	int m_NumXChars;
	int m_NumYChars;

	int m_CharMaxWidth;
	int m_CharMaxHeight;

	CGlyph m_aCharacters[MAX_CHARACTERS*MAX_CHARACTERS];

	int m_CurrentCharacter;
	int m_LanguageVariant;
};

class CGlyphMap
{
    FT_Face m_DefaultFace;
	FT_Face m_VariantFace;
	FT_Face m_aFallbackFaces[MAX_FACES];
	int m_NumFallbackFaces;

	FT_Face m_aFtFaces[MAX_FACES];
	int m_NumFtFaces;
    FT_Face GetFace(int Index) { return m_aFtFaces[Index]; }

public:
    CFontSizeData m_aSizes[NUM_FONT_SIZES];

    CGlyphMap();
    FT_Face GetDefaultFace() { return m_DefaultFace; };
    FT_Face GetCharFace(int Chr);
    int AddFace(FT_Face Face);
    void AddFallbackFaceByName(const char *pFamilyName);
    bool SetVariantFaceByName(const char *pFamilyName);
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
		int s = 1;
		while(1)
		{
			if(*pText == 0)
				return s-1;
			if(*pText == '\n' || *pText == '\t' || *pText == ' ')
				return s;
			pText++;
			s++;
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

	CGlyphMap *m_pDefaultFont;

	// support regional variant fonts
	int m_NumVariants;
	int m_CurrentVariant;
	CFontLanguageVariant *m_paVariants;

	FT_Library m_FTLibrary;

	int GetFontSizeIndex(int Pixelsize);

	void Grow(unsigned char *pIn, unsigned char *pOut, int w, int h);

	void InitTexture(CFontSizeData *pSizeData, int CharWidth, int CharHeight, int Xchars, int Ychars, int LangVariant);

	int AdjustOutlineThicknessToFontSize(int OutlineThickness, int FontSize);

	void IncreaseTextureSize(CFontSizeData *pSizeData);

	// TODO: Refactor: move this into a pFont class
	void InitIndex(CGlyphMap *pFont, int Index);

	CFontSizeData *GetSize(CGlyphMap *pFont, int Pixelsize);


	void UploadGlyph(CFontSizeData *pSizeData, int Texnum, int SlotID, int Chr, const void *pData);

	// 32k of data used for rendering glyphs
	unsigned char ms_aGlyphData[(1024/8) * (1024/8)];
	unsigned char ms_aGlyphDataOutlined[(1024/8) * (1024/8)];

	int GetSlot(CFontSizeData *pSizeData);

	int RenderGlyph(CGlyphMap *pFont, CFontSizeData *pSizeData, int Chr, int ReplacingSlot = -1);

	CGlyph *GetChar(CGlyphMap *pFont, CFontSizeData *pSizeData, int Chr);

	// must only be called from the rendering function as the pFont must be set to the correct size
	void RenderSetup(CGlyphMap *pFont, int size);

	vec2 Kerning(CGlyphMap *pFont, int Left, int Right);

	int LoadFontCollection(const char *pFilename);

public:
	CTextRender();

	virtual void Init();
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