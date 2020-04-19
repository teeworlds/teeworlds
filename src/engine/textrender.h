/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_TEXTRENDER_H
#define ENGINE_TEXTRENDER_H
#include "kernel.h"
#include <base/vmath.h>
#include <engine/graphics.h>

enum
{
	TEXTFLAG_RENDER=1,
	TEXTFLAG_ALLOW_NEWLINE=2,
	TEXTFLAG_STOP_AT_END=4
};

class CFont;

class CTextCursor
{
public:
	int m_Flags;
	int m_LineCount;
	int m_GlyphCount;
	int m_CharCount;
	int m_MaxLines;

	float m_StartX;
	float m_StartY;
	float m_LineWidth;
	float m_X, m_Y;

	CFont *m_pFont;
	float m_FontSize;
};

class ITextRender : public IInterface
{
	MACRO_INTERFACE("textrender", 0)
public:
	virtual void SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags) = 0;

	virtual int LoadFont(const char *pFilename) = 0;

	//
	virtual void TextEx(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextDeferredRenderEx(CTextCursor *pCursor, const char *pText, int Length,
		struct CQuadChar* aQuadChar, int QuadCharMaxCount, int* out_pQuadCharCount,
		IGraphics::CTextureHandle* pFontTexture) = 0;
	virtual void TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset,
		vec4 ShadowColor, vec4 TextColor_) = 0;

	// old foolish interface
	virtual void TextColor(float r, float g, float b, float a) = 0;
	virtual void TextOutlineColor(float r, float g, float b, float a) = 0;
	inline void TextColor(const vec4 &Color) { TextColor(Color.r, Color.g, Color.b, Color.a); }
	inline void TextOutlineColor(const vec4 &Color) { TextOutlineColor(Color.r, Color.g, Color.b, Color.a); }
	virtual void Text(void *pFontSetV, float x, float y, float Size, const char *pText, float LineWidth, bool MultiLine=true) = 0;
	virtual float TextWidth(void *pFontSetV, float Size, const char *pText, int StrLength, float LineWidth) = 0;
	virtual int TextLineCount(void *pFontSetV, float Size, const char *pText, float LineWidth) = 0;

	virtual float TextGetLineBaseY(const CTextCursor *pCursor) = 0;
};

class IEngineTextRender : public ITextRender
{
	MACRO_INTERFACE("enginetextrender", 0)
public:
	virtual void Init() = 0;
};

extern IEngineTextRender *CreateEngineTextRender();

#endif
