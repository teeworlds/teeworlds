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

#define TEXTALIGN_MASK_HORI 3
#define TEXTALIGN_MASK_VERT 12

// TextRender Features
enum
{
	TEXTFLAG_RENDER=1,

	// Allow using '\\n' to linebreak
	// If unset, '\\n' will be replaced with space
	TEXTFLAG_ALLOW_NEWLINE=2,

	// Display "…" when the text is truncated
	TEXTFLAG_ELLIPSIS=4,
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

class CQuadGlyph
{
public:
	int m_Chr;
	int m_FontSizeIndex;
	vec2 m_Offset;
	float m_aUvs[4];
	float m_Width;
	float m_Height;
	int m_NumChars;
	int m_Line;
	vec4 m_TextColor;
	vec4 m_SecondaryColor;
};

class CTextBoundingBox
{
public:
	vec2 m_Min, m_Max;
	float Width() { return m_Max.x - m_Min.x; }
	float Height() { return m_Max.y - m_Min.y; }
};

class CTextCursor
{
	friend class CTextRender;
	vec2 m_CursorPos;

	// Deferred: everything is top left aligned
	//           alignments only happen during drawing
	vec2 m_Advance;
	bool m_StartOfLine;
	bool m_SkipTextRender;
	CTextBoundingBox m_BoundingBox; 
	array<CQuadGlyph> m_Glyphs;
	int m_StringVersion;

	// Stats
	int m_PageCountWhenDrawn;
	int m_LineCount;
	bool m_Truncated;
	int m_GlyphCount;
	int m_CharCount;

	CTextBoundingBox AlignedBoundingBox()
	{
		CTextBoundingBox Box = m_BoundingBox;
		if((m_Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_RIGHT)
		{
			Box.m_Min.x = -Box.m_Max.x;
			Box.m_Max.x = 0;
		}
		else if((m_Align & TEXTALIGN_MASK_HORI) == TEXTALIGN_CENTER)
		{
			Box.m_Max.x = Box.m_Max.x / 2.0f;
			Box.m_Min.x = -Box.m_Max.x;
		}
		
		if((m_Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_BOTTOM)
		{
			Box.m_Min.y = -Box.m_Max.y;
			Box.m_Max.y = 0;
		}
		else if((m_Align & TEXTALIGN_MASK_VERT) == TEXTALIGN_MIDDLE)
		{
			Box.m_Max.y = Box.m_Max.y / 2.0f;
			Box.m_Min.y = -Box.m_Max.y;
		}
		return Box;
	}

public:
	float m_FontSize;
	int m_MaxLines;
	float m_MaxWidth;
	int m_Align;
	int m_Flags;
	float m_LineSpacing;

	void Clear()
	{
		m_BoundingBox.m_Max = vec2(0, 0);
		m_BoundingBox.m_Min = vec2(0, 0);
		m_Advance = vec2(0, 0);
		m_LineCount = 1;
		m_CharCount = 0;
		m_GlyphCount = 0;
		m_PageCountWhenDrawn = -1;
		m_StringVersion = -1;
		m_Truncated = false;
		m_StartOfLine = true;
		m_SkipTextRender = false;
		m_Glyphs.set_size(0);
	}

	// TODO: need better name
	void ClearIfChanged(int StringVersion)
	{
		if (m_StringVersion != StringVersion)
		{
			Clear();
			m_StringVersion = StringVersion;
		}
		else
			m_SkipTextRender = true;
	}

	void MoveTo(float x, float y) { m_CursorPos = vec2(x, y); }

	// Default Params: Top left single line no width/height limit
	CTextCursor()
	{
		m_FontSize = 10.0f;
		m_MaxLines = 1;
		m_MaxWidth = -1.0f;
		m_CursorPos = vec2(0, 0);
		m_LineSpacing = 0.0f;
		m_Align = (ETextAlignment)0; // Top Left
		m_Flags = 0;
		Clear();
	}

	CTextCursor(float FontSize, int Flags = 0) : CTextCursor()
	{
		m_FontSize = FontSize;
		m_Flags = Flags;
	}

	CTextCursor(float FontSize, float x, float y, int Flags = 0) : CTextCursor(FontSize, Flags)
	{
		m_CursorPos = vec2(x, y);
	}

	// Exposed Bounding Box, converted to screen coord.
	CTextBoundingBox BoundingBox()
	{
		CTextBoundingBox Box = AlignedBoundingBox();
		Box.m_Min = m_CursorPos + Box.m_Min;
		Box.m_Max = m_CursorPos + Box.m_Max;
		return Box;
	}
};

class ITextRender : public IInterface
{
	MACRO_INTERFACE("textrender", 0)
public:
	virtual void LoadFonts(IStorage *pStorage, IConsole *pConsole) = 0;
	virtual void SetFontLanguageVariant(const char *pLanguageFile) = 0;

	virtual void TextColor(float r, float g, float b, float a) = 0;
	virtual void TextSecondaryColor(float r, float g, float b, float a) = 0;
	
	virtual float TextWidth(float FontSize, const char *pText, int Length) = 0;
	virtual void TextDeferred(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextNewline(CTextCursor *pCursor) = 0;
	virtual void TextOutlined(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual void TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset) = 0;

	inline void TextColor(const vec4 &Color) { TextColor(Color.r, Color.g, Color.b, Color.a); }
	inline void TextSecondaryColor(const vec4 &Color) { TextSecondaryColor(Color.r, Color.g, Color.b, Color.a); }

	// These should be only called after TextDeferred, TextOutlined or TextShadowed
	// TODO: need better names
	virtual void DrawTextOutlined(CTextCursor *pCursor) = 0;
	virtual void DrawTextShadowed(CTextCursor *pCursor, vec2 ShadowOffset) = 0;
	// TODO: allow changing quad colors
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
