/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/textrender.h>

#include <engine/shared/jsonparser.h>

#include "textrender.h"

int CGlyphMap::CAtlas::TrySection(int Index, int Width, int Height)
{
	ivec3 Section = m_Sections[Index];
	int CurX = Section.x;
	int CurY = Section.y;

	int FitWidth = Width;

	if(CurX + Width > m_Width - 1)
		return -1;

	for(int i = Index; i < m_Sections.size(); ++i)
	{
		if(FitWidth <= 0)
			break;

		Section = m_Sections[i];
		if(Section.y > CurY)
			CurY = Section.y;
		if(CurY + Height > m_Height - 1)
			return -1;
		FitWidth -= Section.l;
	}

	return CurY;
}

void CGlyphMap::CAtlas::Init(int Index, int X, int Y, int Width, int Height)
{
	m_Offset.x = X;
	m_Offset.y = Y;
	m_Width = Width;
	m_Height = Height;

	m_ID = Index;
	m_Sections.clear();

	ivec3 Section;
	Section.x = 1;
	Section.y = 1;
	Section.l = m_Width - 2;
	m_Sections.add(Section);

	m_IsEmpty = true;
}

ivec2 CGlyphMap::CAtlas::Add(int Width, int Height)
{
	m_IsEmpty = false;
	int BestHeight = m_Height;
	int BestWidth = m_Width;
	int BestSectionIndex = -1;

	ivec2 Position;

	for(int i = 0; i < m_Sections.size(); ++i)
	{
		int y = TrySection(i, Width, Height);
		if(y >= 0)
		{
			ivec3 Section = m_Sections[i];
			int NewHeight = y + Height;
			if((NewHeight < BestHeight) || ((NewHeight == BestHeight) && (Section.l > 0 && Section.l < BestWidth)))
			{
				BestHeight = NewHeight;
				BestWidth = Section.l;
				BestSectionIndex = i;
				Position.x = Section.x;
				Position.y = y;
			}
		}
	}

	if(BestSectionIndex < 0)
	{
		Position.x = -1;
		Position.y = -1;
		return Position;
	}

	ivec3 NewSection;
	NewSection.x = Position.x;
	NewSection.y = Position.y + Height;
	NewSection.l = Width;
	m_Sections.insert(NewSection, m_Sections.all().slice(BestSectionIndex, BestSectionIndex + 1));

	for(int i = BestSectionIndex + 1; i < m_Sections.size(); ++i)
	{
		ivec3 *Section = &m_Sections[i];
		ivec3 *Previous = &m_Sections[i-1];

		if(Section->x >= Previous->x + Previous->l)
			break;

		int Shrink = Previous->x + Previous->l - Section->x;
		Section->x += Shrink;
		Section->l -= Shrink;
		if(Section->l > 0)
			break;

		m_Sections.remove_index(i);
		i -= 1;
	}

	for(int i = 0; i < m_Sections.size()-1; ++i)
	{
		ivec3 *Section = &m_Sections[i];
		ivec3 *Next = &m_Sections[i+1];
		if(Section->y == Next->y)
		{
			Section->l += Next->l;
			m_Sections.remove_index(i+1);
			i -= 1;
		}
	}

	m_Access++;
	return Position + m_Offset;
}

int CGlyphMap::AdjustOutlineThicknessToFontSize(int OutlineThickness, int FontSize)
{
	if(FontSize > 36)
		OutlineThickness *= 4;
	else if(FontSize >= 18)
		OutlineThickness *= 2;
	return OutlineThickness;
}

void CGlyphMap::InitTexture(int Width, int Height)
{
	for(int y = 0; y < NUM_PAGES_PER_DIM; ++y)
	{
		for(int x = 0; x < NUM_PAGES_PER_DIM; ++x)
		{
			m_aAtlasPages[y*NUM_PAGES_PER_DIM+x].Init(m_NumTotalPages++, x * PAGE_SIZE, y * PAGE_SIZE, PAGE_SIZE, PAGE_SIZE);
		}
	}
	m_ActiveAtlasIndex = 0;

	// invalidate all glyphs
	for(int i = 0; i < m_Glyphs.size(); ++i)
		m_Glyphs[i].m_pGlyph->m_Rendered = false;

	int TextureSize = Width*Height;

	void *pMem = mem_alloc(TextureSize);
	mem_zero(pMem, TextureSize);

	for(int i = 0; i < 2; i++)
	{
		if(m_aTextures[i].IsValid())
			m_pGraphics->UnloadTexture(&m_aTextures[i]);

		m_aTextures[i] = m_pGraphics->LoadTextureRaw(Width, Height, CImageInfo::FORMAT_ALPHA, pMem, CImageInfo::FORMAT_ALPHA, IGraphics::TEXLOAD_NOMIPMAPS);
	}
	dbg_msg("textrender", "memory usage: %d", TextureSize);
	mem_free(pMem);
}

int CGlyphMap::FitGlyph(int Width, int Height, ivec2 *pPosition)
{
	*pPosition = m_aAtlasPages[m_ActiveAtlasIndex].Add(Width, Height);
	if(pPosition->x >= 0 && pPosition->y >= 0)
		return m_ActiveAtlasIndex;

	// try next page
	int NextPage = m_ActiveAtlasIndex + 1;
	if(NextPage < NUM_PAGES_PER_DIM*NUM_PAGES_PER_DIM && m_aAtlasPages[NextPage].m_IsEmpty)
	{
		*pPosition = m_aAtlasPages[NextPage].Add(Width, Height);
		if(pPosition->x >= 0 && pPosition->y >= 0)
		{
			m_ActiveAtlasIndex = NextPage;
			return m_ActiveAtlasIndex;
		}
	}

	// out of space, drop a page
	int LeastAccess = INT_MAX;
	int Atlas = 0;
	for(int i = 0; i < NUM_PAGES_PER_DIM*NUM_PAGES_PER_DIM; ++i)
	{
		// Do not drop the active page
		if(m_ActiveAtlasIndex == i)
			continue;

		int PageAccess = m_aAtlasPages[i].m_LastFrameAccess;
		if(PageAccess < LeastAccess)
		{
			LeastAccess = PageAccess;
			Atlas = i;
		}
	}

	int X = m_aAtlasPages[Atlas].m_Offset.x;
	int Y = m_aAtlasPages[Atlas].m_Offset.y;
	int W = m_aAtlasPages[Atlas].m_Width;
	int H = m_aAtlasPages[Atlas].m_Height;

	unsigned char *pMem = (unsigned char *)mem_alloc(W*H);
	mem_zero(pMem, W*H);

	UploadGlyph(0, X, Y, W, H, pMem);

	mem_free(pMem);

	m_aAtlasPages[Atlas].Init(m_NumTotalPages++, X, Y, W, H);
	*pPosition = m_aAtlasPages[Atlas].Add(Width, Height);
	m_ActiveAtlasIndex = Atlas;

	dbg_msg("textrender", "atlas is full, dropping atlas %d, total pages: %u", Atlas, m_NumTotalPages);
	return Atlas;
}

void CGlyphMap::UploadGlyph(int TextureIndex, int PosX, int PosY, int Width, int Height, const unsigned char *pData)
{
	m_pGraphics->LoadTextureRawSub(m_aTextures[TextureIndex], PosX, PosY,
		Width, Height, CImageInfo::FORMAT_ALPHA, pData);
}

bool CGlyphMap::SetFaceByName(FT_Face *pFace, const char *pFamilyName)
{
	FT_Face Face = NULL;
	char aFamilyStyleName[FONT_NAME_SIZE];

	if(pFamilyName != NULL)
	{
		for(int i = 0; i < m_NumFtFaces; ++i)
		{
			str_format(aFamilyStyleName, FONT_NAME_SIZE, "%s %s", m_aFtFaces[i]->family_name, m_aFtFaces[i]->style_name);
			if(str_comp(pFamilyName, aFamilyStyleName) == 0)
			{
				Face = m_aFtFaces[i];
				break;
			}

			if(!Face && str_comp(pFamilyName, m_aFtFaces[i]->family_name) == 0)
			{
				Face = m_aFtFaces[i];
			}
		}
	}

	if(Face)
	{
		*pFace = Face;
		return true;
	}
	return false;
}

CGlyphMap::CGlyphMap(IGraphics *pGraphics, FT_Library FtLibrary)
{
	m_pGraphics = pGraphics;

	FT_Stroker_New(FtLibrary, &m_FtStroker);

	m_DefaultFace = NULL;
	m_VariantFace = NULL;

	mem_zero(m_aFallbackFaces, sizeof(m_aFallbackFaces));
	mem_zero(m_aFtFaces, sizeof(m_aFtFaces));

	m_NumFtFaces = 0;
	m_NumFallbackFaces = 0;
	m_NumTotalPages = 0;

	InitTexture(TEXTURE_SIZE, TEXTURE_SIZE);
}

CGlyphMap::~CGlyphMap()
{
	FT_Stroker_Done(m_FtStroker);

	for(int i = 0; i < m_Glyphs.size(); ++i)
		delete m_Glyphs[i].m_pGlyph;
}

int CGlyphMap::GetCharGlyph(int Chr, FT_Face *pFace)
{
	int GlyphIndex = FT_Get_Char_Index(m_DefaultFace, (FT_ULong)Chr);
	*pFace = m_DefaultFace;

	if(!m_DefaultFace || GlyphIndex)
		return GlyphIndex;

	if(m_VariantFace)
	{
		GlyphIndex = FT_Get_Char_Index(m_VariantFace, (FT_ULong)Chr);
		if(GlyphIndex)
		{
			*pFace = m_VariantFace;
			return GlyphIndex;
		}
	}

	for(int i = 0; i < m_NumFallbackFaces; ++i)
	{
		if(m_aFallbackFaces[i])
		{
			GlyphIndex = FT_Get_Char_Index(m_aFallbackFaces[i], (FT_ULong)Chr);
			if(GlyphIndex)
			{
				*pFace = m_aFallbackFaces[i];
				return GlyphIndex;
			}
		}
	}

	return 0;
}

int CGlyphMap::AddFace(FT_Face Face)
{
	if(m_NumFtFaces == MAX_FACES)
		return -1;

	m_aFtFaces[m_NumFtFaces++] = Face;
	if(!m_DefaultFace)
		m_DefaultFace = Face;

	return 0;
}

void CGlyphMap::SetDefaultFaceByName(const char *pFamilyName)
{
	SetFaceByName(&m_DefaultFace, pFamilyName);
}

void CGlyphMap::AddFallbackFaceByName(const char *pFamilyName)
{
	if(m_NumFallbackFaces == MAX_FACES)
		return;

	FT_Face Face = NULL;
	if(SetFaceByName(&Face, pFamilyName))
	{
		m_aFallbackFaces[m_NumFallbackFaces++] = Face;
	}
}

void CGlyphMap::SetVariantFaceByName(const char *pFamilyName)
{
	FT_Face Face = NULL;
	SetFaceByName(&Face, pFamilyName);
	if(m_VariantFace != Face)
	{
		m_VariantFace = Face;
		InitTexture(TEXTURE_SIZE, TEXTURE_SIZE);
	}
}

bool CGlyphMap::RenderGlyph(CGlyph *pGlyph, bool Render)
{
	if(Render && pGlyph->m_Rendered)
	{
		if(pGlyph->m_AtlasIndex < 0)
			return false;

		if(m_aAtlasPages[pGlyph->m_AtlasIndex].m_ID == pGlyph->m_PageID)
		{
			TouchPage(pGlyph->m_AtlasIndex);
			return false;
		}
	}

	FT_Bitmap *pBitmap;

	FT_Face GlyphFace;
	int GlyphIndex = GetCharGlyph(pGlyph->m_ID, &GlyphFace);
	int FontSize = s_aFontSizes[pGlyph->m_FontSizeIndex];
	FT_Set_Pixel_Sizes(GlyphFace, 0, FontSize);

	if(FT_Load_Glyph(GlyphFace, GlyphIndex, FT_LOAD_NO_BITMAP))
	{
		dbg_msg("textrender", "error loading glyph %d", pGlyph->m_ID);
		return false;
	}

	pBitmap = &GlyphFace->glyph->bitmap;

	// adjust spacing
	int OutlineThickness = AdjustOutlineThicknessToFontSize(1, FontSize);
	int Spacing = 1;
	int Offset = OutlineThickness + Spacing;

	int BitmapWidth = pBitmap->width;
	int BitmapHeight = pBitmap->rows;

	int OutlinedWidth = BitmapWidth + OutlineThickness * 2;
	int OutlinedHeight = BitmapHeight + OutlineThickness * 2;

	int Width = OutlinedWidth + Spacing * 2;
	int Height = OutlinedHeight + Spacing * 2;

	int AtlasIndex = -1;
	int Page = -1;

	if(Render && BitmapWidth > 0 && BitmapHeight > 0)
	{
		// find space in atlas
		ivec2 Position = ivec2(0, 0);
		AtlasIndex = FitGlyph(Width, Height, &Position);
		Page = m_aAtlasPages[AtlasIndex].m_ID;

		FT_BitmapGlyph Glyph;
		FT_Get_Glyph(GlyphFace->glyph, (FT_Glyph *)&Glyph);
		FT_Glyph_To_Bitmap((FT_Glyph *)&Glyph, FT_RENDER_MODE_NORMAL, 0, true);
		pBitmap = &Glyph->bitmap;
		int BitmapLeft = Glyph->left;
		int BitmapTop = Glyph->top;

		UploadGlyph(0, Position.x + Offset, Position.y + Offset, BitmapWidth, BitmapHeight, pBitmap->buffer);
		FT_Done_Glyph((FT_Glyph)Glyph);

		// create outline
		FT_Stroker_Set(m_FtStroker, (OutlineThickness) * 64 + 32, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
		FT_Get_Glyph(GlyphFace->glyph, (FT_Glyph *)&Glyph);
		FT_Glyph_Stroke((FT_Glyph *)&Glyph, m_FtStroker, true);
		FT_Glyph_To_Bitmap((FT_Glyph *)&Glyph, FT_RENDER_MODE_NORMAL, 0, true);
		pBitmap = &Glyph->bitmap;

		int OutlinedPositionX = Position.x + (Glyph->left - BitmapLeft) + Offset;
		int OutlinedPositionY = Position.y + (BitmapTop - Glyph->top) + Offset;
		UploadGlyph(1, OutlinedPositionX, OutlinedPositionY, pBitmap->width, pBitmap->rows, pBitmap->buffer);
		FT_Done_Glyph((FT_Glyph)Glyph);

		TouchPage(AtlasIndex);

		float UVScale = 1.0f / TEXTURE_SIZE;
		pGlyph->m_aUvCoords[0] = (Position.x + Spacing) * UVScale;
		pGlyph->m_aUvCoords[1] = (Position.y + Spacing) * UVScale;
		pGlyph->m_aUvCoords[2] = pGlyph->m_aUvCoords[0] + OutlinedWidth * UVScale;
		pGlyph->m_aUvCoords[3] = pGlyph->m_aUvCoords[1] + OutlinedHeight * UVScale;
	}

	float Scale = 1.0f / FontSize;

	pGlyph->m_AtlasIndex = AtlasIndex;
	pGlyph->m_PageID = Page;
	pGlyph->m_Face = GlyphFace;
	pGlyph->m_Height = OutlinedHeight * Scale;
	pGlyph->m_Width = OutlinedWidth * Scale;
	pGlyph->m_BearingX = (GlyphFace->glyph->bitmap_left-OutlineThickness/2) * Scale;
	pGlyph->m_BearingY = (FontSize - GlyphFace->glyph->bitmap_top-OutlineThickness/2) * Scale;
	pGlyph->m_AdvanceX = (GlyphFace->glyph->advance.x>>6) * Scale;
	pGlyph->m_Rendered = Render;

	return true;
}

CGlyph *CGlyphMap::GetGlyph(int Chr, int FontSizeIndex, bool Render)
{
	CGlyphIndex Index;
	Index.m_FontSizeIndex = FontSizeIndex;
	Index.m_ID = Chr;

	sorted_array<CGlyphIndex>::range r = ::find_binary(m_Glyphs.all(), Index);

	// couldn't find glyph, render a new one
	if(r.empty())
	{
		Index.m_pGlyph = new CGlyph();
		Index.m_pGlyph->m_Rendered = false;
		Index.m_pGlyph->m_ID = Chr;
		Index.m_pGlyph->m_FontSizeIndex = FontSizeIndex;
		if(RenderGlyph(Index.m_pGlyph, Render))
		{
			m_Glyphs.add(Index);
			return Index.m_pGlyph;
		}
		delete Index.m_pGlyph;
		return NULL;
	}

	CGlyph *pMatch = r.front().m_pGlyph;
	if(Render)
		RenderGlyph(pMatch, true);
	return pMatch;
}

vec2 CGlyphMap::Kerning(CGlyph *pLeft, CGlyph *pRight, int PixelSize)
{
	FT_Vector Kerning = {0,0};

	if(pLeft && pRight && pLeft->m_Face == pRight->m_Face)
	{
		FT_Set_Pixel_Sizes(pLeft->m_Face, 0, PixelSize);
		FT_Get_Kerning(pLeft->m_Face, pLeft->m_ID, pRight->m_ID, FT_KERNING_DEFAULT, &Kerning);
	}

	vec2 Vec;
	Vec.x = (float)(Kerning.x>>6);
	Vec.y = (float)(Kerning.y>>6);
	return Vec;
}

int CGlyphMap::GetFontSizeIndex(int PixelSize) const
{
	for(unsigned i = 0; i < NUM_FONT_SIZES; i++)
	{
		if(s_aFontSizes[i] >= PixelSize)
			return i;
	}

	return NUM_FONT_SIZES-1;
}

void CGlyphMap::TouchPage(int Index)
{
	m_aAtlasPages[Index].m_Access++;
}

void CGlyphMap::PagesAccessReset()
{
	for(int i = 0; i < NUM_PAGES_PER_DIM * NUM_PAGES_PER_DIM; ++i)
	{
		m_aAtlasPages[i].m_LastFrameAccess = m_aAtlasPages[i].m_Access;
		m_aAtlasPages[i].m_Access = 0;
	}
}

CWordWidthHint CTextRender::MakeWord(CTextCursor *pCursor, const char *pText, const char *pEnd, int FontSizeIndex, float Size, int PixelSize, vec2 ScreenScale)
{
	bool Render = !(pCursor->m_Flags & TEXTFLAG_NO_RENDER);
	bool BreakWord = !(pCursor->m_Flags & TEXTFLAG_WORD_WRAP);
	bool AllowNewline = pCursor->m_Flags & TEXTFLAG_ALLOW_NEWLINE;
	CWordWidthHint Hint;
	const char *pCur = pText;
	int NextChr = str_utf8_decode(&pCur);
	CGlyph *pNextGlyph = NULL;
	if(NextChr > 0)
	{
		if(NextChr == '\n' || NextChr == '\t')
			pNextGlyph = m_pGlyphMap->GetGlyph(' ', FontSizeIndex, Render);
		else
			pNextGlyph = m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, Render);
	}

	float Scale = 1.0f/PixelSize;
	float MaxWidth = pCursor->m_MaxWidth;
	if(MaxWidth < 0)
		MaxWidth = INFINITY;

	float WordStartAdvanceX = pCursor->m_Advance.x;

	Hint.m_CharCount = 0;
	Hint.m_GlyphCount = 0;
	Hint.m_EffectiveAdvanceX = pCursor->m_Advance.x;
	Hint.m_EndsWithNewline = false;
	Hint.m_IsBroken = false;

	if(*pText == '\0' || pCur > pEnd)
	{
		Hint.m_CharCount = -1;
		return Hint;
	}

	while(true)
	{
		int Chr = NextChr;
		CGlyph *pGlyph = pNextGlyph;
		int CharCount = pCur - pText;
		int NumChars = CharCount - Hint.m_CharCount;
		Hint.m_CharCount = CharCount;

		if(Chr == 0 || pCur > pEnd)
		{
			Hint.m_CharCount--;
			break;
		}

		if(!pGlyph || Chr < 0)
		{
			Hint.m_CharCount = -1;
			return Hint;
		}

		NextChr = str_utf8_decode(&pCur);
		pNextGlyph = NULL;
		if(NextChr > 0)
		{
			if(NextChr == '\n' || NextChr == '\t')
				pNextGlyph = m_pGlyphMap->GetGlyph(' ', FontSizeIndex, Render);
			else
				pNextGlyph = m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, Render);
		}

		vec2 Kerning = m_pGlyphMap->Kerning(pGlyph, pNextGlyph, PixelSize) * Scale;
		float AdvanceX = (pGlyph->m_AdvanceX + Kerning.x) * Size;

		bool IsSpace = Chr == '\n' || Chr == '\t' || Chr == ' ';
		bool CanBreak = !IsSpace && (BreakWord || pCursor->m_StartOfLine);
		if(Hint.m_EffectiveAdvanceX - WordStartAdvanceX > MaxWidth || (CanBreak && pCursor->m_Advance.x + AdvanceX > MaxWidth))
		{
			Hint.m_CharCount -= NumChars;
			Hint.m_IsBroken = true;
			break;
		}

		if(Render)
		{
			CScaledGlyph Scaled;
			Scaled.m_pGlyph = pGlyph;
			Scaled.m_Advance = pCursor->m_Advance;
			Scaled.m_Size = Size;
			Scaled.m_Line = pCursor->m_LineCount - 1;
			Scaled.m_TextColor = m_TextColor;
			Scaled.m_SecondaryColor = m_TextSecondaryColor;
			Scaled.m_NumChars = NumChars;
			pCursor->m_Glyphs.add(Scaled);
		}

		pCursor->m_Advance.x += AdvanceX;
		Hint.m_GlyphCount++;

		if(IsSpace || Chr == 0)
		{
			Hint.m_EndsWithNewline = Chr == '\n';
			if(AllowNewline && Hint.m_EndsWithNewline)
			{
				// remove redundant space
				Hint.m_GlyphCount--;
				if(Render)
					pCursor->m_Glyphs.remove_index(pCursor->m_Glyphs.size()-1);
			}
			break;
		}

		Hint.m_EffectiveAdvanceX = pCursor->m_Advance.x;

		// break every char on non latin/greek characters
		if(!IsWestern(Chr))
			break;
	}

	return Hint;
}

void CTextRender::TextRefreshGlyphs(CTextCursor *pCursor)
{
	int NumQuads = pCursor->m_Glyphs.size();
	if(NumQuads <= 0)
		return;

	int NumTotalPages = m_pGlyphMap->NumTotalPages();

	if(NumTotalPages != pCursor->m_PageCountWhenDrawn)
	{
		// pages were dropped, re-render glyphs
		for(int i = 0; i < pCursor->m_Glyphs.size(); ++i)
			m_pGlyphMap->RenderGlyph(pCursor->m_Glyphs[i].m_pGlyph, true);
		pCursor->m_PageCountWhenDrawn = m_pGlyphMap->NumTotalPages();
	}

	if(NumTotalPages != m_pGlyphMap->NumTotalPages())
	{
		dbg_msg("textrender", "page %d mismatch after refresh, atlas might be too small.", NumTotalPages);
	}
}

int CTextRender::LoadFontCollection(const char *pFilename, const void *pBuf, unsigned FileSize)
{
	FT_Face FtFace;

	if(FT_New_Memory_Face(m_FTLibrary, (FT_Byte *)pBuf, (FT_Long)FileSize, -1, &FtFace))
		return -1;

	int NumFaces = FtFace->num_faces;
	FT_Done_Face(FtFace);

	int i;
	for(i = 0; i < NumFaces; ++i)
	{
		if(FT_New_Memory_Face(m_FTLibrary, (FT_Byte *)pBuf, (FT_Long)FileSize, i, &FtFace))
		{
			FT_Done_Face(FtFace);
			break;
		}

		if(m_pGlyphMap->AddFace(FtFace))
		{
			FT_Done_Face(FtFace);
			break;
		}
	}

	dbg_msg("textrender", "loaded %d faces from font file '%s'", i, (char *)pFilename);

	return 0;
}

CTextRender::CTextRender()
{
	m_pGraphics = 0;

	m_TextColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_TextSecondaryColor = vec4(0.0f, 0.0f, 0.0f, 0.3f);

	m_pGlyphMap = 0;
	m_NumVariants = 0;
	m_CurrentVariant = -1;
	m_pVariants = 0;

	mem_zero(m_apFontData, sizeof(m_apFontData));
}

void CTextRender::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	FT_Init_FreeType(&m_FTLibrary);
	m_pGlyphMap = new CGlyphMap(m_pGraphics, m_FTLibrary);
}

void CTextRender::Update()
{
	if(m_pGlyphMap)
		m_pGlyphMap->PagesAccessReset();
}

void CTextRender::Shutdown()
{
	delete m_pGlyphMap;

	FT_Done_FreeType(m_FTLibrary);

	if(m_pVariants)
		mem_free(m_pVariants);

	for(int i = 0; i < MAX_FACES; ++i)
		if(m_apFontData[i])
			mem_free(m_apFontData[i]);
}

void CTextRender::LoadFonts(IStorage *pStorage, IConsole *pConsole)
{
	CJsonParser JsonParser;
	const json_value *pJsonData = JsonParser.ParseFile("fonts/index.json", pStorage);
	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "textrender", JsonParser.Error());
		return;
	}

	// extract font file definitions
	const json_value &rFiles = (*pJsonData)["font files"];
	if(rFiles.type == json_array)
	{
		for(unsigned i = 0; i < rFiles.u.array.length && i < MAX_FACES; ++i)
		{
			char aFontName[IO_MAX_PATH_LENGTH];
			str_format(aFontName, sizeof(aFontName), "fonts/%s", (const char *)rFiles[i]);
			unsigned FileSize;
			if(pStorage->ReadFile(aFontName, IStorage::TYPE_ALL, &m_apFontData[i], &FileSize))
			{
				if(LoadFontCollection(aFontName, m_apFontData[i], FileSize))
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "failed to load font. filename='%s'", aFontName);
					pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "textrender", aBuf);
				}
			}
		}
	}

	// extract default family name
	const json_value &rDefaultFace = (*pJsonData)["default"];
	if(rDefaultFace.type == json_string)
	{
		m_pGlyphMap->SetDefaultFaceByName((const char *)rDefaultFace);
	}

	// extract fallback family names
	const json_value &rFallbackFaces = (*pJsonData)["fallbacks"];
	if(rFallbackFaces.type == json_array)
	{
		for(unsigned i = 0; i < rFallbackFaces.u.array.length; ++i)
		{
			m_pGlyphMap->AddFallbackFaceByName((const char *)rFallbackFaces[i]);
		}
	}

	// extract language variant family names
	const json_value &rVariant = (*pJsonData)["language variants"];
	if(rVariant.type == json_object)
	{
		m_NumVariants = rVariant.u.object.length;
		json_object_entry *Entries = rVariant.u.object.values;
		m_pVariants = (CFontLanguageVariant *)mem_alloc(sizeof(CFontLanguageVariant)*m_NumVariants);
		for(int i = 0; i < m_NumVariants; ++i)
		{
			char aFileName[128];
			str_format(aFileName, sizeof(aFileName), "languages/%s.json", (const char *)Entries[i].name);
			str_copy(m_pVariants[i].m_aLanguageFile, aFileName, sizeof(m_pVariants[i].m_aLanguageFile));

			json_value *pFamilyName = rVariant.u.object.values[i].value;
			if(pFamilyName->type == json_string)
				str_copy(m_pVariants[i].m_aFamilyName, pFamilyName->u.string.ptr, sizeof(m_pVariants[i].m_aFamilyName));
			else
				m_pVariants[i].m_aFamilyName[0] = 0;
		}
	}
}

void CTextRender::SetFontLanguageVariant(const char *pLanguageFile)
{
	if(!m_pGlyphMap)
		return;

	char *pFamilyName = NULL;

	if(m_pVariants)
	{
		for(int i = 0; i < m_NumVariants; ++i)
		{
			if(str_comp_filenames(pLanguageFile, m_pVariants[i].m_aLanguageFile) == 0)
			{
				pFamilyName = m_pVariants[i].m_aFamilyName;
				m_CurrentVariant = i;
				break;
			}
		}
	}

	m_pGlyphMap->SetVariantFaceByName(pFamilyName);
}

float CTextRender::TextWidth(float FontSize, const char *pText, int Length)
{
	static CTextCursor s_Cursor;
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.m_Flags = TEXTFLAG_NO_RENDER;
	s_Cursor.Reset();
	TextDeferred(&s_Cursor, pText, Length);
	return s_Cursor.m_Width;
}

void CTextRender::TextDeferred(CTextCursor *pCursor, const char *pText, int Length)
{
	if(pCursor->m_Truncated || pCursor->m_SkipTextRender)
		return;

	if(!m_pGlyphMap->GetDefaultFace())
		return;

	bool Render = !(pCursor->m_Flags & TEXTFLAG_NO_RENDER);

	// Alignment
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));
	float Size = pCursor->m_FontSize;
	int PixelSize = (int)(Size * ScreenScale.y);
	Size = PixelSize / ScreenScale.y;
	int FontSizeIndex = m_pGlyphMap->GetFontSizeIndex(PixelSize);

	// Cursor current states
	int Flags = pCursor->m_Flags;
	float MaxWidth = pCursor->m_MaxWidth;
	if(MaxWidth < 0)
		MaxWidth = INFINITY;
	int MaxLines = pCursor->m_MaxLines;
	if(MaxLines < 0)
		MaxLines = (ScreenY1-ScreenY0) / pCursor->m_FontSize;

	if(Length < 0)
		Length = str_length(pText);

	const char *pCur = (char *)pText;
	const char *pEnd = (char *)pText + Length;

	float NextAdvanceY = pCursor->m_Advance.y + pCursor->m_FontSize;
	NextAdvanceY = (int)(NextAdvanceY * ScreenScale.y) / ScreenScale.y;
	pCursor->m_NextLineAdvanceY = maximum(NextAdvanceY, pCursor->m_NextLineAdvanceY);

	while(pCur < pEnd && !pCursor->m_Truncated)
	{
		const float WordStartAdvanceX = pCursor->m_Advance.x;
		CWordWidthHint WordWidth = MakeWord(pCursor, pCur, pEnd, FontSizeIndex, Size, PixelSize, ScreenScale);
		pCursor->m_StartOfLine = false;
		if(WordWidth.m_CharCount < 0)
			break;

		// word wrapping
		if(WordWidth.m_EffectiveAdvanceX > MaxWidth)
		{
			const int NumGlyphs = pCursor->m_Glyphs.size();
			// do not let space create new line.
			if(WordWidth.m_GlyphCount == 1 && (*pCur == ' ' || *pCur == '\n' || *pCur == '\t'))
			{
				if(Render)
					pCursor->m_Glyphs.remove_index(NumGlyphs-1);
				pCursor->m_Advance.x = WordStartAdvanceX;
			}
			else
			{
				if(pCursor->m_LineCount < MaxLines)
				{
					pCursor->m_LineCount++;
					float AdvanceY = pCursor->m_Advance.y;
					pCursor->m_Advance.y = pCursor->m_LineSpacing + pCursor->m_NextLineAdvanceY;
					pCursor->m_Advance.x -= WordStartAdvanceX;

					float NextAdvanceY = pCursor->m_Advance.y + pCursor->m_FontSize;
					NextAdvanceY = (int)(NextAdvanceY * ScreenScale.y) / ScreenScale.y;
					pCursor->m_NextLineAdvanceY = maximum(NextAdvanceY, pCursor->m_NextLineAdvanceY);

					if(Render)
					{
						const int WordStartGlyphIndex = NumGlyphs - WordWidth.m_GlyphCount;
						for(int i = NumGlyphs - 1; i >= WordStartGlyphIndex; --i)
						{
							pCursor->m_Glyphs[i].m_Advance.x -= WordStartAdvanceX;
							pCursor->m_Glyphs[i].m_Advance.y += pCursor->m_Advance.y - AdvanceY;
							pCursor->m_Glyphs[i].m_Line = pCursor->m_LineCount - 1;
						}
					}

					pCursor->m_StartOfLine = false;
				}
				else
				{
					pCursor->m_Truncated = true;
				}
			}
		}

		pCursor->m_Width = maximum(pCursor->m_Advance.x, pCursor->m_Width);

		// newline \n
		bool ForceNewLine = WordWidth.m_EndsWithNewline && (Flags & TEXTFLAG_ALLOW_NEWLINE);
		if(ForceNewLine || WordWidth.m_IsBroken)
		{
			if(pCursor->m_LineCount < MaxLines)
			{
				pCursor->m_LineCount++;
				pCursor->m_Advance.y = pCursor->m_LineSpacing + pCursor->m_NextLineAdvanceY;
				pCursor->m_Advance.x = 0;
				pCursor->m_StartOfLine = true;

				float NextAdvanceY = pCursor->m_Advance.y + pCursor->m_FontSize;
				NextAdvanceY = (int)(NextAdvanceY * ScreenScale.y) / ScreenScale.y;
				pCursor->m_NextLineAdvanceY = maximum(NextAdvanceY, pCursor->m_NextLineAdvanceY);
			}
			else
			{
				pCursor->m_Truncated = true;
			}
		}

		pCur += WordWidth.m_CharCount;
	}

	pCursor->m_Height = pCursor->m_NextLineAdvanceY + 0.35f * Size;
	pCursor->m_CharCount = pCur - pText;

	// insert ellipsis at the end
	if(pCursor->m_Truncated && pCursor->m_Flags & TEXTFLAG_ELLIPSIS)
	{
		const char aEllipsis[] = "…";
		if(pCursor->m_Glyphs.size() > 0)
		{
			CScaledGlyph *pLastGlyph = &pCursor->m_Glyphs[pCursor->m_Glyphs.size()-1];
			pCursor->m_Advance.x = pLastGlyph->m_Advance.x + pLastGlyph->m_pGlyph->m_AdvanceX * pLastGlyph->m_Size;
			pCursor->m_Advance.y = pLastGlyph->m_Advance.y;
		}

		int OldMaxWidth = pCursor->m_MaxWidth;
		pCursor->m_MaxWidth = -1;
		CWordWidthHint WordWidth = MakeWord(pCursor, aEllipsis, aEllipsis+sizeof(aEllipsis), FontSizeIndex, Size, PixelSize, ScreenScale);
		pCursor->m_MaxWidth = OldMaxWidth;
		if(WordWidth.m_EffectiveAdvanceX > MaxWidth)
		{
			int NumDots = WordWidth.m_GlyphCount;
			int NumGlyphs = pCursor->m_Glyphs.size() - NumDots;
			float BackAdvanceX = 0;
			for(int i = NumGlyphs - 1; i > 1; --i)
			{
				CScaledGlyph *pScaled = &pCursor->m_Glyphs[i];
				if(pScaled->m_Advance.x + pScaled->m_pGlyph->m_AdvanceX * pScaled->m_Size > MaxWidth)
				{
					BackAdvanceX = pCursor->m_Advance.x - pScaled->m_Advance.x;
					pCursor->m_CharCount -= pScaled->m_NumChars;
					pCursor->m_Glyphs.remove_index(i);
				}
				else
					break;
			}
			for(int i = pCursor->m_Glyphs.size() - NumDots; i < pCursor->m_Glyphs.size(); ++i)
				pCursor->m_Glyphs[i].m_Advance.x -= BackAdvanceX;
		}
	}

	TextRefreshGlyphs(pCursor);
}

void CTextRender::TextNewline(CTextCursor *pCursor)
{
	if(pCursor->m_Truncated || pCursor->m_SkipTextRender)
		return;

	// Alignment
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	int MaxLines = pCursor->m_MaxLines;
	if(MaxLines < 0)
		MaxLines = (ScreenY1-ScreenY0) / pCursor->m_FontSize;

	if(pCursor->m_LineCount >= MaxLines)
	{
		pCursor->m_LineCount = MaxLines;
		pCursor->m_Truncated = true;
		return;
	}

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));
	pCursor->m_LineCount++;
	pCursor->m_Advance.y = pCursor->m_LineSpacing + pCursor->m_NextLineAdvanceY;
	pCursor->m_Advance.x = 0;
	pCursor->m_StartOfLine = true;
	float NextAdvanceY = pCursor->m_Advance.y + pCursor->m_FontSize;
	NextAdvanceY = (int)(NextAdvanceY * ScreenScale.y) / ScreenScale.y;
	pCursor->m_NextLineAdvanceY = NextAdvanceY;
}

void CTextRender::TextAdvance(CTextCursor *pCursor, float AdvanceX)
{
	// Alignment
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));

	int LineWidth = pCursor->m_Advance.x + AdvanceX;
	float MaxWidth = pCursor->m_MaxWidth;
	if(MaxWidth < 0)
		MaxWidth = INFINITY;
	if(LineWidth > MaxWidth)
	{
		TextNewline(pCursor);
		pCursor->m_Advance.x = LineWidth - MaxWidth;
	}
	else
	{
		pCursor->m_Advance.x = LineWidth;
	}

	pCursor->m_Advance.x = (int)(pCursor->m_Advance.x * ScreenScale.x) / ScreenScale.x;
}

void CTextRender::DrawText(CTextCursor *pCursor, vec2 Offset, int Texture, bool IsSecondary, float Alpha, int StartGlyph = 0, int NumGlyphs = -1)
{
	int NumQuads = pCursor->m_Glyphs.size();
	if(NumQuads <= 0)
		return;

	if(NumGlyphs < 0)
		NumGlyphs = NumQuads;

	int EndGlyphs = StartGlyph + NumGlyphs;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));

	int HorizontalAlign = pCursor->m_Align & TEXTALIGN_MASK_HORI;
	CTextBoundingBox AlignBox = pCursor->AlignedBoundingBox();
	vec2 AlignOffset = vec2(AlignBox.x, AlignBox.y);

	vec4 LastColor = vec4(-1, -1, -1, -1);
	Graphics()->TextureSet(m_pGlyphMap->GetTexture(Texture));
	Graphics()->QuadsBegin();

	int Line = -1;
	float LineOffset = 0.0f;
	vec2 Anchor = pCursor->m_CursorPos + AlignOffset;

	for(int i = NumQuads - 1; i >= 0; --i)
	{
		const CScaledGlyph& rScaled = pCursor->m_Glyphs[i];
		const CGlyph *pGlyph = rScaled.m_pGlyph;

		if(Line != rScaled.m_Line)
		{
			Line = rScaled.m_Line;
			if(HorizontalAlign == TEXTALIGN_RIGHT)
				LineOffset = pCursor->m_Width - (rScaled.m_Advance.x + pGlyph->m_AdvanceX * rScaled.m_Size);
			else if(HorizontalAlign == TEXTALIGN_CENTER)
				LineOffset = (pCursor->m_Width - (rScaled.m_Advance.x + pGlyph->m_AdvanceX * rScaled.m_Size)) / 2.0f;
			else
				LineOffset = 0.0f;
		}

		if(pGlyph->m_AtlasIndex < 0 || i < StartGlyph || i >= EndGlyphs)
			continue;

		m_pGlyphMap->TouchPage(pGlyph->m_AtlasIndex);

		vec4 Color;
		if(IsSecondary)
		{
			Color = rScaled.m_SecondaryColor;
			if(Texture == 1)
				Color.a *= rScaled.m_TextColor.a;
		}
		else
		{
			Color = rScaled.m_TextColor;
		}

		if(Color != LastColor)
		{
			Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a * Alpha);
			LastColor = Color;
		}

		Graphics()->QuadsSetSubset(pGlyph->m_aUvCoords[0], pGlyph->m_aUvCoords[1], pGlyph->m_aUvCoords[2], pGlyph->m_aUvCoords[3]);

		float AnchorX = (int)((Anchor.x + LineOffset) * ScreenScale.x) / ScreenScale.x;
		float AnchorY = (int)(Anchor.y * ScreenScale.y) / ScreenScale.y;
		vec2 QuadPosition = vec2(AnchorX, AnchorY) + rScaled.m_Advance + vec2(pGlyph->m_BearingX, pGlyph->m_BearingY) * rScaled.m_Size + Offset / ScreenScale;
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(QuadPosition.x, QuadPosition.y, pGlyph->m_Width * rScaled.m_Size, pGlyph->m_Height * rScaled.m_Size);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();
}

void CTextRender::TextPlain(CTextCursor *pCursor, const char *pText, int Length)
{
	TextDeferred(pCursor, pText, Length);
	DrawTextPlain(pCursor, 1.0f, 0, -1);
}

void CTextRender::TextOutlined(CTextCursor *pCursor, const char *pText, int Length)
{
	TextDeferred(pCursor, pText, Length);
	DrawTextOutlined(pCursor, 1.0f, 0, -1);
}

void CTextRender::TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset)
{
	TextDeferred(pCursor, pText, Length);
	DrawTextShadowed(pCursor, ShadowOffset, 1.0f, 0, -1);
}

void CTextRender::DrawTextPlain(CTextCursor *pCursor, float Alpha, int StartGlyph, int NumGlyphs)
{
	TextRefreshGlyphs(pCursor);
	DrawText(pCursor, vec2(0, 0), 0, false, Alpha, StartGlyph, NumGlyphs);
}

void CTextRender::DrawTextOutlined(CTextCursor *pCursor, float Alpha, int StartGlyph, int NumGlyphs)
{
	TextRefreshGlyphs(pCursor);
	DrawText(pCursor, vec2(0, 0), 1, true, Alpha, StartGlyph, NumGlyphs);
	DrawText(pCursor, vec2(0, 0), 0, false, Alpha, StartGlyph, NumGlyphs);
}

void CTextRender::DrawTextShadowed(CTextCursor *pCursor, vec2 ShadowOffset, float Alpha, int StartGlyph, int NumGlyphs)
{
	TextRefreshGlyphs(pCursor);
	DrawText(pCursor, ShadowOffset, 0, true, Alpha, StartGlyph, NumGlyphs);
	DrawText(pCursor, vec2(0, 0), 0, false, Alpha, StartGlyph, NumGlyphs);
}

int CTextRender::CharToGlyph(CTextCursor *pCursor, int NumChars, float *pLineWidth)
{
	int CursorChars = 0;
	int NumGlyphs = pCursor->m_Glyphs.size();
	if(NumGlyphs == 0 || NumChars == 0)
	{
		if(pLineWidth)
			*pLineWidth = 0.0f;
		return 0;
	}

	int GlyphIndex = -1;
	for(int i = 0; i < NumGlyphs; ++i)
	{
		CursorChars += pCursor->m_Glyphs[i].m_NumChars;
		if(CursorChars > NumChars)
		{
			GlyphIndex = i;
			break;
		}
	}

	int LastGlyphIndex = GlyphIndex;

	if(GlyphIndex < 0)
	{
		GlyphIndex = NumGlyphs;
		LastGlyphIndex = GlyphIndex - 1;
	}

	if(pLineWidth)
	{
		const int Line = pCursor->m_Glyphs[LastGlyphIndex].m_Line;

		for(; LastGlyphIndex < NumGlyphs; ++LastGlyphIndex)
		{
			if(LastGlyphIndex + 1 >= NumGlyphs)
				break;

			if(pCursor->m_Glyphs[LastGlyphIndex].m_Line > Line)
			{
				LastGlyphIndex -= 1;
				break;
			}
		}

		const CScaledGlyph& rScaled = pCursor->m_Glyphs[LastGlyphIndex];
		*pLineWidth = rScaled.m_Advance.x + rScaled.m_pGlyph->m_AdvanceX * rScaled.m_Size;
	}

	return GlyphIndex;
}

vec2 CTextRender::CaretPosition(CTextCursor *pCursor, int NumChars)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));
	float Size = pCursor->m_FontSize;
	int PixelSize = (int)(Size * ScreenScale.y);
	Size = PixelSize / ScreenScale.y;

	int NumGlyphs = pCursor->m_Glyphs.size();
	float LineWidth;
	int GlyphIndex = CharToGlyph(pCursor, NumChars, &LineWidth);

	int HorizontalAlign = pCursor->m_Align & TEXTALIGN_MASK_HORI;
	int VerticalAlign = pCursor->m_Align & TEXTALIGN_MASK_VERT;

	vec2 Offset = vec2(0,0);
	float LineOffset = 0.0f;

	if(HorizontalAlign == TEXTALIGN_RIGHT)
	{
		Offset.x = -pCursor->m_Width;
		LineOffset = pCursor->m_Width - LineWidth;
	}
	else if(HorizontalAlign == TEXTALIGN_CENTER)
	{
		Offset.x = -pCursor->m_Width / 2.0f;
		LineOffset = (pCursor->m_Width - LineWidth) / 2.0f;
	}

	if(VerticalAlign == TEXTALIGN_BOTTOM)
		Offset.y = -pCursor->m_Height + Size * 1.35f;
	else if(VerticalAlign == TEXTALIGN_MIDDLE)
		Offset.y = -pCursor->m_Height / 2.0f + Size * 0.675f;

	if(GlyphIndex == 0 || NumGlyphs == 0)
		return pCursor->m_CursorPos + Offset;

	if(GlyphIndex < NumGlyphs)
		return pCursor->m_CursorPos + pCursor->m_Glyphs[GlyphIndex].m_Advance + Offset;

	CScaledGlyph *pLastScaled = &pCursor->m_Glyphs[NumGlyphs-1];
	return pCursor->m_CursorPos + pLastScaled->m_Advance + Offset
		+ vec2(pLastScaled->m_pGlyph->m_AdvanceX + LineOffset, 0) * pLastScaled->m_Size;
}

IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }
