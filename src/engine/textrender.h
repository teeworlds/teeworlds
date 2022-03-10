/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_TEXTRENDER_H
#define ENGINE_TEXTRENDER_H
#include "kernel.h"
#include <base/vmath.h>
#include <base/tl/array.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/graphics.h>

// TextRender Features
enum
{
	TEXTFLAG_NO_RENDER=1,

	// Allow using '\\n' to linebreak
	// If unset, '\\n' will be replaced with space
	TEXTFLAG_ALLOW_NEWLINE=2,

	// Display "…" when the text is truncated
	TEXTFLAG_ELLIPSIS=4,

	// If set, newline will try not to break words
	TEXTFLAG_WORD_WRAP=8,

	// Masks
	TEXTALIGN_MASK_HORI=3,
	TEXTALIGN_MASK_VERT=12
};

enum ETextAlignment
{
	TEXTALIGN_LEFT=0,
	TEXTALIGN_CENTER=1,
	TEXTALIGN_RIGHT=2,
	TEXTALIGN_TOP=0,
	TEXTALIGN_MIDDLE=4,
	TEXTALIGN_BOTTOM=8,

	TEXTALIGN_TL = TEXTALIGN_TOP | TEXTALIGN_LEFT,
	TEXTALIGN_TC = TEXTALIGN_TOP | TEXTALIGN_CENTER,
	TEXTALIGN_TR = TEXTALIGN_TOP | TEXTALIGN_RIGHT,
	TEXTALIGN_ML = TEXTALIGN_MIDDLE | TEXTALIGN_LEFT,
	TEXTALIGN_MC = TEXTALIGN_MIDDLE | TEXTALIGN_CENTER,
	TEXTALIGN_MR = TEXTALIGN_MIDDLE | TEXTALIGN_RIGHT,
	TEXTALIGN_BL = TEXTALIGN_BOTTOM | TEXTALIGN_LEFT,
	TEXTALIGN_BC = TEXTALIGN_BOTTOM | TEXTALIGN_CENTER,
	TEXTALIGN_BR = TEXTALIGN_BOTTOM | TEXTALIGN_RIGHT
};

struct CGlyph;

class CScaledGlyph
{
public:
	CGlyph *m_pGlyph;
	float m_Size;
	int m_NumChars;
	int m_Line;
	vec2 m_Advance;
	vec4 m_TextColor;
	vec4 m_SecondaryColor;
};

struct CTextBoundingBox
{
	float x, y, w, h;
	float Right() const { return x + w; }
	float Bottom() const { return y + h; }
};

class CTextCursor
{
	friend class CTextRender;

	// Stats
	float m_Width;
	float m_Height;
	int m_PageCountWhenDrawn;
	bool m_Truncated;
	int m_LineCount;
	int m_CharCount;

	// Deferred: everything is top left aligned
	//           alignments only happen during drawing
	vec2 m_CursorPos;
	vec2 m_Advance;
	bool m_SkipTextRender;
	float m_NextLineAdvanceY;
	array<CScaledGlyph> m_Glyphs;
	int64 m_StringVersion;

	CTextBoundingBox AlignedBoundingBox() const
	{
		CTextBoundingBox Box;
		if((m_Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_RIGHT)
			Box.x = -m_Width;
		else if((m_Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_CENTER)
			Box.x = -m_Width / 2.0f;
		else
			Box.x = 0.0f;

		if((m_Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_BOTTOM)
			Box.y = -m_Height;
		else if((m_Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_MIDDLE)
			Box.y = -m_Height / 2.0f;
		else
			Box.y = 0.0f;

		Box.w = m_Width;
		Box.h = m_Height;
		return Box;
	}

	void Set(float FontSize, float x, float y, int Flags)
	{
		m_FontSize = FontSize;
		m_MaxLines = 1;
		m_MaxWidth = -1.0f;
		m_CursorPos = vec2(x, y);
		m_LineSpacing = 0.0f;
		m_Align = 0; // Top Left
		m_Flags = Flags;
	}

public:
	float m_FontSize;
	int m_MaxLines;
	float m_MaxWidth;
	int m_Align;
	int m_Flags;
	float m_LineSpacing;
	bool m_StartOfLine;

	void Reset(int64 StringVersion = -1)
	{
		if(StringVersion < 0 || m_StringVersion != StringVersion)
		{
			m_Width = 0;
			m_Height = 0;
			m_NextLineAdvanceY = 0;
			m_Advance = vec2(0, 0);
			m_LineCount = 1;
			m_CharCount = 0;
			m_PageCountWhenDrawn = -1;
			m_Truncated = false;
			m_StartOfLine = true;
			m_Glyphs.set_size(0);
			m_StringVersion = StringVersion;
			m_SkipTextRender = false;
		}
		else
		{
			m_SkipTextRender = true;
		}
	}

	void MoveTo(float x, float y) { m_CursorPos = vec2(x, y); }
	void MoveTo(vec2 Position) { m_CursorPos = Position; }
	float Width() const { return m_Width; }
	float Height() const { return m_Height; }
	float BaseLineY() const { return m_NextLineAdvanceY; }
	vec2 CursorPosition() const { return m_CursorPos; }
	vec2 AdvancePosition() const { return m_CursorPos + m_Advance; }
	bool IsTruncated() const { return m_Truncated; }
	int LineCount() const { return m_LineCount; }
	int GlyphCount() const { return m_Glyphs.size(); }
	int CharCount() const { return m_CharCount; }

	// Default Cursor: Top left single line no width limit
	CTextCursor() { Set(10.0f, 0, 0, 0); Reset(); }
	CTextCursor(float FontSize, int Flags = 0) { Set(FontSize, 0, 0, Flags); Reset(); }
	CTextCursor(float FontSize, float x, float y, int Flags = 0) { Set(FontSize, x, y, Flags); Reset(); }

	// Exposed Bounding Box, converted to screen coord.
	CTextBoundingBox BoundingBox() const
	{
		CTextBoundingBox Box = AlignedBoundingBox();
		Box.x += m_CursorPos.x;
		Box.y += m_CursorPos.y;
		return Box;
	}

	bool Rendered() const { return m_Glyphs.size() > 0; }
};

class ITextRender : public IInterface
{
	MACRO_INTERFACE("textrender", 0)
public:
	virtual void LoadFonts(IStorage *pStorage, IConsole *pConsole) = 0;
	virtual void SetFontLanguageVariant(const char *pLanguageFile) = 0;

	inline void TextColor(float r, float g, float b, float a) { TextColor(vec4(r, g, b, a)); }
	inline void TextSecondaryColor(float r, float g, float b, float a) { TextSecondaryColor(vec4(r, g, b, a)); }

	virtual float TextWidth(float FontSize, const char *pText, int Length) = 0;
	virtual void TextDeferred(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextNewline(CTextCursor *pCursor) = 0;
	virtual void TextAdvance(CTextCursor *pCursor, float AdvanceX) = 0;
	virtual void TextPlain(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextOutlined(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset) = 0;

	virtual void TextColor(const vec4 &Color) = 0;
	virtual void TextSecondaryColor(const vec4 &Color) = 0;

	virtual vec4 GetColor() const = 0;
	virtual vec4 GetSecondaryColor() const = 0;

	// These should be only called after TextDeferred, TextOutlined or TextShadowed
	// TODO: allow changing quad colors
	virtual void DrawTextPlain(CTextCursor *pCursor, float Alpha = 1.0f, int StartGlyph = 0, int NumGlyphs = -1) = 0;
	virtual void DrawTextOutlined(CTextCursor *pCursor, float Alpha = 1.0f, int StartGlyph = 0, int NumGlyphs = -1) = 0;
	virtual void DrawTextShadowed(CTextCursor *pCursor, vec2 ShadowOffset, float Alpha = 1.0f, int StartGlyph = 0, int NumGlyphs = -1) = 0;

	// QoL APIs
	virtual int CharToGlyph(CTextCursor *pCursor, int NumChars, float *pLineWidth = 0) = 0;
	virtual vec2 CaretPosition(CTextCursor *pCursor, int NumChars) = 0;
};

class IEngineTextRender : public ITextRender
{
	MACRO_INTERFACE("enginetextrender", 0)
public:
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Shutdown() = 0;
};

extern IEngineTextRender *CreateEngineTextRender();

#endif
