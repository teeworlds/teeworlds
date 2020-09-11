/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <engine/external/json-parser/json.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include "textrender.h"

int CAtlas::TrySection(int Index, int Width, int Height)
{
	ivec3 Section = m_Sections[Index];
	int CurX = Section.x;
	int CurY = Section.y;

	int FitWidth = Width;

	if(CurX + Width > m_Width - 1) return -1;

	for(int i = Index; i < m_Sections.size(); ++i)
	{
		if(FitWidth <= 0) break;

		Section = m_Sections[i];
		if(Section.y > CurY) CurY = Section.y;
		if(CurY + Height > m_Height - 1) return -1;
		FitWidth -= Section.l;
	}

	return CurY;
}

void CAtlas::Init(int Index, int X, int Y, int Width, int Height)
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
}

ivec2 CAtlas::Add(int Width, int Height)
{
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

		if(Section->x >= Previous->x + Previous->l) break;
		
		int Shrink = Previous->x + Previous->l - Section->x;
		Section->x += Shrink;
		Section->l -= Shrink;
		if(Section->l > 0) break;

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

void CGlyphMap::Grow(unsigned char *pIn, unsigned char *pOut, int w, int h)
{
	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++)
		{
			int c = pIn[y*w+x];

			for(int sy = -1; sy <= 1; sy++)
				for(int sx = -1; sx <= 1; sx++)
				{
					int GetX = x+sx;
					int GetY = y+sy;
					if(GetX >= 0 && GetY >= 0 && GetX < w && GetY < h)
					{
						int Index = GetY*w+GetX;
						if(pIn[Index] > c)
							c = pIn[Index];
					}
				}

			pOut[y*w+x] = c;
		}
}

void CGlyphMap::InitTexture(int Width, int Height)
{
	for(int y = 0; y < PAGE_COUNT; ++y)
	{
		for(int x = 0; x < PAGE_COUNT; ++x)
		{
			m_aAtlasPages[y*PAGE_COUNT+x].Init(m_NumTotalPages++, x * PAGE_SIZE, y * PAGE_SIZE, PAGE_SIZE, PAGE_SIZE);
		}
	}
	m_Glyphs.clear();

	int TextureSize = Width*Height;

	void *pMem = mem_alloc(TextureSize, 1);
	mem_zero(pMem, TextureSize);

	for(int i = 0; i < 2; i++)
	{
		if(m_aTextures[i].IsValid())
			m_pGraphics->UnloadTexture(&m_aTextures[i]);

		m_aTextures[i] = m_pGraphics->LoadTextureRaw(Width, Height, CImageInfo::FORMAT_ALPHA, pMem, CImageInfo::FORMAT_ALPHA, IGraphics::TEXLOAD_NOMIPMAPS);
	}
	dbg_msg("pGlyphMap", "memory usage: %d", TextureSize);
	mem_free(pMem);
}

int CGlyphMap::FitGlyph(int Width, int Height, ivec2 *Position)
{
	for(int i = 0; i < PAGE_COUNT*PAGE_COUNT; ++i)
	{
		*Position = m_aAtlasPages[i].Add(Width, Height);
		if(Position->x >= 0 && Position->y >= 0)
			return i;
	}
	
	// out of space, drop a page
	int LeastAccess = INT_MAX;
	int Atlas = 0;
	for(int i = 0; i < PAGE_COUNT*PAGE_COUNT; ++i)
	{
		int PageAccess = m_aAtlasPages[i].GetAccess();
		if(PageAccess < LeastAccess)
		{
			LeastAccess = PageAccess;
			Atlas = i;
		}
	}

	int X = m_aAtlasPages[Atlas].GetOffsetX();
	int Y = m_aAtlasPages[Atlas].GetOffsetY();
	int W = m_aAtlasPages[Atlas].GetWidth();
	int H = m_aAtlasPages[Atlas].GetHeight();

	unsigned char *pMem = (unsigned char *)mem_alloc(W*H, 1);
	mem_zero(pMem, W*H);

	UploadGlyph(0, X, Y, W, H, pMem);

	m_aAtlasPages[Atlas].Init(m_NumTotalPages++, X, Y, W, H);
	*Position = m_aAtlasPages[Atlas].Add(Width, Height);

	dbg_msg("pGlyphMap", "glyph map is full, dropping atlas %d, total pages: %u", Atlas, m_NumTotalPages);
	return Atlas;
}

void CGlyphMap::UploadGlyph(int TextureIndex, int PosX, int PosY, int Width, int Height, const unsigned char *pData)
{
	m_pGraphics->LoadTextureRawSub(m_aTextures[TextureIndex], PosX, PosY,
		Width, Height, CImageInfo::FORMAT_ALPHA, pData);
}

bool CGlyphMap::RenderGlyph(CGlyph *pGlyph, bool Render)
{
	FT_Bitmap *pBitmap;
	int x = 1;
	int y = 1;
	unsigned int px, py;

	FT_Face GlyphFace;
	int GlyphIndex = GetCharGlyph(pGlyph->m_ID, &GlyphFace);
	int FontSize = aFontSizes[pGlyph->m_FontSizeIndex];
	FT_Set_Pixel_Sizes(GlyphFace, 0, FontSize);

	if(FT_Load_Glyph(GlyphFace, GlyphIndex, (Render ? FT_LOAD_RENDER : FT_LOAD_BITMAP_METRICS_ONLY)|FT_LOAD_NO_BITMAP))
	{
		dbg_msg("pGlyphMap", "error loading glyph %d", pGlyph->m_ID);
		return false;
	}

	pBitmap = &GlyphFace->glyph->bitmap; // ignore_convention

	// adjust spacing
	int OutlineThickness = AdjustOutlineThicknessToFontSize(1, FontSize);
	x += OutlineThickness;
	y += OutlineThickness;

	unsigned int Width = pBitmap->width + x * 2;
	unsigned int Height = pBitmap->rows + y * 2;

	int AtlasIndex = -1;
	int Page = -1;

	if(Render)
	{
		int BitmapSize = Width * Height;

		// prepare glyph data
		mem_zero(s_aGlyphData, BitmapSize);

		if(pBitmap->pixel_mode == FT_PIXEL_MODE_GRAY) // ignore_convention
		{
			for(py = 0; py < pBitmap->rows; py++) // ignore_convention
				for(px = 0; px < pBitmap->width; px++) // ignore_convention
					s_aGlyphData[(py+y)*Width+px+x] = pBitmap->buffer[py*pBitmap->width+px]; // ignore_convention
		}
		else if(pBitmap->pixel_mode == FT_PIXEL_MODE_MONO) // ignore_convention
		{
			for(py = 0; py < pBitmap->rows; py++) // ignore_convention
				for(px = 0; px < pBitmap->width; px++) // ignore_convention
				{
					if(pBitmap->buffer[py*pBitmap->width+px/8]&(1<<(7-(px%8)))) // ignore_convention
						s_aGlyphData[(py+y)*Width+px+x] = 255;
				}
		}

		// find space in atlas
		ivec2 Position;
		AtlasIndex = FitGlyph(Width, Height, &Position);
		Page = m_aAtlasPages[AtlasIndex].GetPageID();

		UploadGlyph(0, Position.x, Position.y, (int)Width, (int)Height, s_aGlyphData);

		if(OutlineThickness == 1)
		{
			Grow(s_aGlyphData, s_aGlyphDataOutlined, Width, Height);
			UploadGlyph(1, Position.x, Position.y, (int)Width, (int)Height, s_aGlyphDataOutlined);
		}
		else
		{
			for(int i = OutlineThickness; i > 0; i-=2)
			{
				Grow(s_aGlyphData, s_aGlyphDataOutlined, Width, Height);
				Grow(s_aGlyphDataOutlined, s_aGlyphData, Width, Height);
			}
			UploadGlyph(1, Position.x, Position.y, (int)Width, (int)Height, s_aGlyphData);
		}

		m_aAtlasPages[AtlasIndex].Touch();

		float Uscale = 1.0f / TEXTURE_SIZE;
		float Vscale = 1.0f / TEXTURE_SIZE;
		pGlyph->m_aUvs[0] = Position.x * Uscale;
		pGlyph->m_aUvs[1] = Position.y * Vscale;
		pGlyph->m_aUvs[2] = pGlyph->m_aUvs[0] + Width * Uscale;
		pGlyph->m_aUvs[3] = pGlyph->m_aUvs[1] + Height * Vscale;
	}

	float Scale = 1.0f / FontSize;

	pGlyph->m_AtlasIndex = AtlasIndex;
	pGlyph->m_PageID = Page;
	pGlyph->m_Face = GlyphFace;

	pGlyph->m_Height = Height * Scale;
	pGlyph->m_Width = Width * Scale;
	pGlyph->m_Offset = vec2(GlyphFace->glyph->bitmap_left-2, FontSize - GlyphFace->glyph->bitmap_top) * Scale; // ignore_convention
	pGlyph->m_AdvanceX = (GlyphFace->glyph->advance.x>>6) * Scale; // ignore_convention
	pGlyph->m_Rendered = Render;

	return true;
}

bool CGlyphMap::SetFaceByName(FT_Face *pFace, const char *pFamilyName)
{
	FT_Face Face = NULL;
	char aFamilyStyleName[128];

	if(pFamilyName != NULL)
	{
		for(int i = 0; i < m_NumFtFaces; ++i)
		{
			str_format(aFamilyStyleName, 128, "%s %s", m_aFtFaces[i]->family_name, m_aFtFaces[i]->style_name);
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

CGlyphMap::CGlyphMap(IGraphics *pGraphics)
{
	m_pGraphics = pGraphics;

	m_DefaultFace = NULL;
	m_VariantFace = NULL;

	mem_zero(m_aFallbackFaces, sizeof(m_aFallbackFaces));
	mem_zero(m_aFtFaces, sizeof(m_aFtFaces));

	m_NumFtFaces = 0;
	m_NumFallbackFaces = 0;
	m_NumTotalPages = 0;

	InitTexture(TEXTURE_SIZE, TEXTURE_SIZE);
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
	if(!m_DefaultFace) m_DefaultFace = Face;

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

void CGlyphMap::GetGlyph(int Chr, int FontSizeIndex, CGlyph *pGlyph, bool Render)
{
	CGlyph Glyph;
	Glyph.m_FontSizeIndex = FontSizeIndex;
	Glyph.m_ID = Chr;
	
	sorted_array<CGlyph>::range r = ::find_binary(m_Glyphs.all(), Glyph);

	// couldn't find glyph, render a new one
	if(r.empty())
	{
		if(RenderGlyph(&Glyph, Render))
		{
			m_Glyphs.add(Glyph);
			mem_copy(pGlyph, &Glyph, sizeof(*pGlyph));
			return;
		}
		pGlyph->m_ID = -1;
		return;
	}

	CGlyph *pMatch = &r.front();

	if(Render)
	{
		if(!pMatch->m_Rendered || m_aAtlasPages[pMatch->m_AtlasIndex].GetPageID() != pMatch->m_PageID)
		{
			// render the glyph if it haven't yet
			// or, re-render glyph if the page is dropped
			RenderGlyph(pMatch, true);
		}
		else
		{
			// otherwise touch the page
			m_aAtlasPages[pMatch->m_AtlasIndex].Touch();
		}
	}

	mem_copy(pGlyph, pMatch, sizeof(*pGlyph));
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

int CGlyphMap::GetFontSizeIndex(int PixelSize)
{
	for(unsigned i = 0; i < NUM_FONT_SIZES; i++)
	{
		if(aFontSizes[i] >= PixelSize)
			return i;
	}

	return NUM_FONT_SIZES-1;
}

void CGlyphMap::PagesAccessReset()
{
	for(int i = 0; i < PAGE_COUNT * PAGE_COUNT; ++i)
		m_aAtlasPages[i].Update();
}

CWordWidthHint CTextRender::MakeWord(CTextCursor *pCursor, const char *pText, const char *pEnd, int FontSizeIndex, float Size, int PixelSize)
{
	CWordWidthHint Hint;
	const char *pCur = pText;
	int NextChr = str_utf8_decode(&pCur);
	CGlyph NextGlyph;
	if(NextChr)
		if (NextChr == '\n' || NextChr == '\t')
			m_pGlyphMap->GetGlyph(' ', FontSizeIndex, &NextGlyph, true);
		else
			m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, &NextGlyph, true);
	else
		NextGlyph.m_Face = NULL;
	
	float Scale = 1.0f/PixelSize;
	float LineWidth = pCursor->m_MaxWidth;
	if(LineWidth < 0)
		LineWidth = INFINITY;

	Hint.m_CharCount = 0;
	Hint.m_GlyphCount = 0;
	Hint.m_EffectiveAdvanceX = pCursor->m_Advance.x;
	Hint.m_EndsWithNewline = false;
	Hint.m_EndsWithSpace = false;
	Hint.m_IsBroken = false;

	float WordStartAdvance = pCursor->m_Advance.x;

	while(1)
	{
		int Chr = NextChr;
		CGlyph Glyph = NextGlyph;
		Hint.m_CharCount = pCur - pText;

		if(Chr <= 0 || pCur > pEnd)
		{
			Hint.m_CharCount--;
			break;
		}

		NextChr = str_utf8_decode(&pCur);
		if(NextChr)
			if (NextChr == '\n' || NextChr == '\t')
				m_pGlyphMap->GetGlyph(' ', FontSizeIndex, &NextGlyph, true);
			else
				m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, &NextGlyph, true);
		else
			NextGlyph.m_Face = NULL;

		vec2 Kerning = m_pGlyphMap->Kerning(&Glyph, &NextGlyph, PixelSize) * Scale;
		float AdvanceX = (Glyph.m_AdvanceX + Kerning.x) * Size;

		bool IsSpace = Chr == '\n' || Chr == '\t' || Chr == ' ';
		if(!IsSpace && pCursor->m_Advance.x + AdvanceX - WordStartAdvance > LineWidth)
		{
			Hint.m_CharCount--;
			Hint.m_IsBroken = true;
			break;
		}
		
		CQuadGlyph QuadGlyph;
		mem_copy(QuadGlyph.m_aUvs, Glyph.m_aUvs, sizeof(QuadGlyph.m_aUvs));
		QuadGlyph.m_Offset = pCursor->m_Advance + Glyph.m_Offset * Size;
		QuadGlyph.m_Width = Glyph.m_Width * Size;
		QuadGlyph.m_Height = Glyph.m_Height * Size;
		QuadGlyph.m_Line = pCursor->m_LineCount - 1;
		QuadGlyph.m_TextColor = vec4(m_TextR, m_TextG, m_TextB, m_TextA);
		QuadGlyph.m_SecondaryColor = vec4(m_TextSecondaryR, m_TextSecondaryG, m_TextSecondaryB, m_TextSecondaryA);

		pCursor->m_Glyphs.add(QuadGlyph);
		pCursor->m_Advance.x += AdvanceX;

		Hint.m_GlyphCount++;

		if (IsSpace)
		{
			Hint.m_EndsWithSpace = true;
			Hint.m_EndsWithNewline = Chr == '\n';
			break;
		}

		Hint.m_EffectiveAdvanceX = pCursor->m_Advance.x;
		if(!isWestern(Chr))
			break;
	}

	return Hint;
}

int CTextRender::LoadFontCollection(const char *pFilename)
{
	FT_Face FtFace;

	if(FT_New_Face(m_FTLibrary, pFilename, -1, &FtFace))
		return -1;

	int NumFaces = FtFace->num_faces;
	FT_Done_Face(FtFace);

	int i;
	for(i = 0; i < NumFaces; ++i)
	{
		if(FT_New_Face(m_FTLibrary, pFilename, i, &FtFace))
			break;

		if(m_pGlyphMap->AddFace(FtFace))
			break;
	}

	dbg_msg("textrender", "loaded %d faces from font file '%s'", i, pFilename);

	return 0;
}

CTextRender::CTextRender()
{
	m_pGraphics = 0;

	m_TextR = 1.0f;
	m_TextG = 1.0f;
	m_TextB = 1.0f;
	m_TextA = 1.0f;
	m_TextSecondaryR = 0.0f;
	m_TextSecondaryG = 0.0f;
	m_TextSecondaryB = 0.0f;
	m_TextSecondaryA = 0.3f;

	m_pGlyphMap = 0;
	m_NumVariants = 0;
	m_CurrentVariant = -1;
	m_paVariants = 0;

	// GL_LUMINANCE can be good for debugging
	//m_FontTextureFormat = GL_ALPHA;
}

void CTextRender::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	FT_Init_FreeType(&m_FTLibrary);
	m_pGlyphMap = new CGlyphMap(m_pGraphics);
}

void CTextRender::Update()
{
	if(m_pGlyphMap) m_pGlyphMap->PagesAccessReset();
}

void CTextRender::Shutdown()
{
	delete m_pGlyphMap;
	if(m_paVariants) mem_free(m_paVariants);
}

void CTextRender::LoadFonts(IStorage *pStorage, IConsole *pConsole)
{
	// read file data into buffer
	const char *pFilename = "fonts/index.json";
	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "textrender", "couldn't open fonts index file");
		return;
	}
	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize, 1);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);

	if(pJsonData == 0)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, pFilename, aError);
		return;
	}

	// extract font file definitions
	const json_value &rFiles = (*pJsonData)["font files"];
	if(rFiles.type == json_array)
	{
		for(unsigned i = 0; i < rFiles.u.array.length; ++i)
		{
			char aFontName[IO_MAX_PATH_LENGTH];
			str_format(aFontName, sizeof(aFontName), "fonts/%s", (const char *)rFiles[i]);
			char aFilename[IO_MAX_PATH_LENGTH];
			IOHANDLE File = pStorage->OpenFile(aFontName, IOFLAG_READ, IStorage::TYPE_ALL, aFilename, sizeof(aFilename));
			if(File)
			{
				io_close(File);
				if(LoadFontCollection(aFilename))
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
		m_paVariants = (CFontLanguageVariant *)mem_alloc(sizeof(CFontLanguageVariant)*m_NumVariants, 1);
		for(int i = 0; i < m_NumVariants; ++i)
		{
			char aFileName[128];
			str_format(aFileName, sizeof(aFileName), "languages/%s.json", (const char *)Entries[i].name);
			strncpy(m_paVariants[i].m_aLanguageFile, aFileName, sizeof(m_paVariants[i].m_aLanguageFile));
			
			json_value *pFamilyName = rVariant.u.object.values[i].value;
			if(pFamilyName->type == json_string)
				strncpy(m_paVariants[i].m_aFamilyName, pFamilyName->u.string.ptr, sizeof(m_paVariants[i].m_aFamilyName));
			else
				m_paVariants[i].m_aFamilyName[0] = 0;
		}
	}

	json_value_free(pJsonData);
}

void CTextRender::SetFontLanguageVariant(const char *pLanguageFile)
{
	if(!m_pGlyphMap) return;	

	char *FamilyName = NULL;

	if(m_paVariants)
	{
		for(int i = 0; i < m_NumVariants; ++i)
		{
			if(str_comp_filenames(pLanguageFile, m_paVariants[i].m_aLanguageFile) == 0)
			{
				FamilyName = m_paVariants[i].m_aFamilyName;
				m_CurrentVariant = i;
				break;
			}
		}
	}

	m_pGlyphMap->SetVariantFaceByName(FamilyName);
}

void CTextRender::SetCursor(CTextCursor *pCursor, float x, float y, int Flags)
{
	pCursor->m_BoundingBox.m_Max = vec2(0, 0);
	pCursor->m_BoundingBox.m_Min = vec2(0, 0);
	pCursor->m_Flags = Flags;
	pCursor->m_CursorPos = vec2(x, y);
	pCursor->m_Advance = vec2(0, 0);
	pCursor->m_LineCount = 1;
	pCursor->m_CharCount = 0;
	pCursor->m_GlyphCount = 0;
	pCursor->m_PageCountWhenDrawn = -1;
	pCursor->m_Truncated = false;

	pCursor->m_Glyphs.set_size(0);
}

void CTextRender::TextColor(float r, float g, float b, float a)
{
	m_TextR = r;
	m_TextG = g;
	m_TextB = b;
	m_TextA = a;
}

void CTextRender::TextSecondaryColor(float r, float g, float b, float a)
{
	m_TextSecondaryR = r;
	m_TextSecondaryG = g;
	m_TextSecondaryB = b;
	m_TextSecondaryA = a;
}

float CTextRender::TextWidth(float FontSize, const char *pText, int Length)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	int ScreenWidth = Graphics()->ScreenWidth();
	int ScreenHeight = Graphics()->ScreenHeight();
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	vec2 ScreenScale = vec2(ScreenWidth/(ScreenX1-ScreenX0), ScreenHeight/(ScreenY1-ScreenY0));
	int PixelSize = (int)(FontSize * ScreenScale.y);
	float Size = PixelSize / ScreenScale.y;
	int FontSizeIndex = m_pGlyphMap->GetFontSizeIndex(PixelSize);
	float Scale = 1.0f/PixelSize;

	if(Length < 0)
		Length = str_length(pText);

	const char *pCur = (char *)pText;
	const char *pEnd = pCur + Length;
	int NextChr = str_utf8_decode(&pCur);
	if(NextChr == '\n' || NextChr == '\t')
		NextChr = ' ';
	CGlyph NextGlyph;
	m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, &NextGlyph, false);
	float Width = 0.0f;
	while(pCur <= pEnd)
	{
		int Chr = NextChr;
		CGlyph Glyph = NextGlyph;
		NextChr = str_utf8_decode(&pCur);
		if(NextChr == '\n')
			NextChr = ' ';
		if(NextChr)
			m_pGlyphMap->GetGlyph(NextChr, FontSizeIndex, &NextGlyph, false);
		if(Chr == 0 || Glyph.m_ID <= 0)
			break;

		vec2 Kerning = m_pGlyphMap->Kerning(&Glyph, &NextGlyph, PixelSize) * Scale;
		Width += (Glyph.m_AdvanceX + Kerning.x) * Size;
	}
	return Width;
}

void CTextRender::TextDeferred(CTextCursor *pCursor, const char *pText, int Length)
{
	if(pCursor->m_Truncated)
		return;

	if(!m_pGlyphMap->GetDefaultFace())
		return;

	// Sizes
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
	CTextBoundingBox BoundingBox = pCursor->m_BoundingBox;
	int Flags = pCursor->m_Flags;
	float LineWidth = pCursor->m_MaxWidth;
	if(LineWidth < 0)
		LineWidth = INFINITY;
	int MaxLines = pCursor->m_MaxLines;
	if(MaxLines < 0)
		MaxLines = INT32_MAX;

	if(Length < 0)
		Length = str_length(pText);
	
	pCursor->m_Glyphs.hint_size(Length); // Reduce allocation

	if(Flags & TEXTFLAG_FIT_FONTSIZE)
	{
		Flags &= ~(TEXTFLAG_ALLOW_NEWLINE);
		float TotalWidth = TextWidth(PixelSize, pText, Length);
		if(TotalWidth > LineWidth)
		{
			// Scale fontsize to fit width
			Size = max(Size * LineWidth / TotalWidth, min(pCursor->m_MinFontSize, pCursor->m_FontSize));
			PixelSize = (int)(Size * ScreenScale.y);
			Size = PixelSize / ScreenScale.y;
			FontSizeIndex = m_pGlyphMap->GetFontSizeIndex(PixelSize);
			pCursor->m_FontSize = Size;
		}
	}
	
	const char *pCur = (char *)pText;
	const char *pEnd = (char *)pText + Length;

	float WordStartAdvanceX = pCursor->m_Advance.x;
	CWordWidthHint WordWidth = MakeWord(pCursor, pCur, pEnd, FontSizeIndex, Size, PixelSize);
	const char *pWordEnd = pCur + WordWidth.m_CharCount;

	while(pWordEnd <= pEnd && WordWidth.m_CharCount > 0)
	{
		if(WordWidth.m_EffectiveAdvanceX > LineWidth)
		{
			int NumGlyphs = pCursor->m_Glyphs.size();
			int WordStartGlyphIndex = NumGlyphs - WordWidth.m_GlyphCount;
			// Do not let space create new line.
			if(WordWidth.m_GlyphCount == 1 && WordWidth.m_EndsWithSpace)
			{
				pCursor->m_Glyphs.remove_index(NumGlyphs-1);
				pCursor->m_Advance.x = WordStartAdvanceX;
				WordWidth.m_EffectiveAdvanceX = 0;
			}
			else
			{
				pCursor->m_LineCount++;
				pCursor->m_Advance.y += Size;
				pCursor->m_Advance.x -= WordStartAdvanceX;

				if(pCursor->m_LineCount > MaxLines)
				{
					for(int i = NumGlyphs - 1; i >= WordStartGlyphIndex; --i)
						pCursor->m_Glyphs.remove_index(i);
					pCursor->m_Truncated = true;
				}
				else
				{
					for(int i = NumGlyphs - 1; i >= WordStartGlyphIndex; --i)
					{
						pCursor->m_Glyphs[i].m_Offset.x -= WordStartAdvanceX;
						pCursor->m_Glyphs[i].m_Offset.y += Size;
						pCursor->m_Glyphs[i].m_Line = pCursor->m_LineCount - 1;
					}
				}

				WordWidth.m_EffectiveAdvanceX -= WordStartAdvanceX;
			}
		}

		bool ForceNewLine = WordWidth.m_EndsWithNewline && (Flags & TEXTFLAG_ALLOW_NEWLINE);
		if(ForceNewLine || WordWidth.m_IsBroken)
		{
			pCursor->m_LineCount++;
			pCursor->m_Advance.y += Size;
			pCursor->m_Advance.x = 0;
		}

		BoundingBox.m_Max.x = max(WordWidth.m_EffectiveAdvanceX, BoundingBox.m_Max.x);
		WordStartAdvanceX = pCursor->m_Advance.x;

		pCur = pWordEnd;
		WordWidth = MakeWord(pCursor, pCur, pEnd, FontSizeIndex, Size, PixelSize);
		pWordEnd = pWordEnd + WordWidth.m_CharCount;

		if(pCursor->m_LineCount > MaxLines)
		{
			pCursor->m_LineCount--;
			if(WordWidth.m_CharCount > 0)
			{
				int NumGlyphs = pCursor->m_Glyphs.size();
				int WordStartGlyphIndex = NumGlyphs - WordWidth.m_GlyphCount;
				for(int i = NumGlyphs - 1; i >= WordStartGlyphIndex; --i)
					pCursor->m_Glyphs.remove_index(i);
				pCursor->m_Truncated = true;
			}
			break;
		}
	}

	BoundingBox.m_Max.y = pCursor->m_LineCount * Size;
	pCursor->m_BoundingBox = BoundingBox;
	pCursor->m_CharCount = pCur - pText;

	pCursor->m_PageCountWhenDrawn = m_pGlyphMap->NumTotalPages();
}

void CTextRender::TextOutlined(CTextCursor *pCursor, const char *pText, int Length)
{
	TextDeferred(pCursor, pText, Length);
	DrawTextOutlined(pCursor);
}

void CTextRender::TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset)
{
	TextDeferred(pCursor, pText, Length);
	DrawTextShadowed(pCursor, ShadowOffset);
}

void CTextRender::DrawTextOutlined(CTextCursor *pCursor)
{
	int NumQuads = pCursor->m_Glyphs.size();
	if(NumQuads <= 0)
		return;

	int HorizontalAlign = pCursor->m_Align & TEXTALIGN_MASK_HORI;
	CTextBoundingBox AlignedBox = pCursor->AlignedBoundingBox();
	float BoxWidth = AlignedBox.Width();

	for(int pass = 0; pass < 2; ++pass)
	{
		Graphics()->TextureSet(m_pGlyphMap->GetTexture(1-pass));
		Graphics()->QuadsBegin();

		int Line = -1;
		vec2 LineOffset = vec2(0, 0);
		for(int i = NumQuads - 1; i >= 0; --i)
		{
			const CQuadGlyph& Quad = pCursor->m_Glyphs[i];
			if(pass == 0)
				Graphics()->SetColor(Quad.m_SecondaryColor.r, Quad.m_SecondaryColor.g, Quad.m_SecondaryColor.b, Quad.m_SecondaryColor.a * Quad.m_TextColor.a);
			else
				Graphics()->SetColor(Quad.m_TextColor.r, Quad.m_TextColor.g, Quad.m_TextColor.b, Quad.m_TextColor.a);
		
			if(Line != Quad.m_Line)
			{
				Line = Quad.m_Line;
				if(HorizontalAlign == TEXTALIGN_RIGHT)
					LineOffset.x = BoxWidth - (Quad.m_Offset.x + Quad.m_Width);
				else if(HorizontalAlign == TEXTALIGN_CENTER)
					LineOffset.x = (BoxWidth - (Quad.m_Offset.x + Quad.m_Width)) / 2.0f;
				else
					LineOffset.x = 0;
			}
			Graphics()->QuadsSetSubset(Quad.m_aUvs[0], Quad.m_aUvs[1], Quad.m_aUvs[2], Quad.m_aUvs[3]);

			vec2 QuadPosition = pCursor->m_CursorPos + Quad.m_Offset + AlignedBox.m_Min + LineOffset;
			IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(QuadPosition.x, QuadPosition.y, Quad.m_Width, Quad.m_Height);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
		Graphics()->QuadsEnd();
	}
}

void CTextRender::DrawTextShadowed(CTextCursor *pCursor, vec2 ShadowOffset)
{
	int NumQuads = pCursor->m_Glyphs.size();

	if(NumQuads <= 0)
		return;

	int HorizontalAlign = pCursor->m_Align & TEXTALIGN_MASK_HORI;
	CTextBoundingBox AlignedBox = pCursor->AlignedBoundingBox();
	float BoxWidth = AlignedBox.Width();

	Graphics()->TextureSet(m_pGlyphMap->GetTexture(0));
	Graphics()->QuadsBegin();

	int Line = -1;
	vec2 LineOffset = vec2(0, 0);
	for(int i = NumQuads - 1; i >= 0; --i)
	{
		const CQuadGlyph& Quad = pCursor->m_Glyphs[i];
		Graphics()->SetColor(Quad.m_SecondaryColor.r, Quad.m_SecondaryColor.g, Quad.m_SecondaryColor.b, Quad.m_SecondaryColor.a);
		if(Line != Quad.m_Line)
		{
			Line = Quad.m_Line;
			if(HorizontalAlign == TEXTALIGN_RIGHT)
				LineOffset.x = BoxWidth - (Quad.m_Offset.x + Quad.m_Width);
			else if(HorizontalAlign == TEXTALIGN_CENTER)
				LineOffset.x = (BoxWidth - (Quad.m_Offset.x + Quad.m_Width)) / 2.0f;
			else
				LineOffset.x = 0;
		}
		Graphics()->QuadsSetSubset(Quad.m_aUvs[0], Quad.m_aUvs[1], Quad.m_aUvs[2], Quad.m_aUvs[3]);

		vec2 QuadPosition = pCursor->m_CursorPos + Quad.m_Offset + AlignedBox.m_Min + LineOffset + ShadowOffset;
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(QuadPosition.x, QuadPosition.y, Quad.m_Width, Quad.m_Height);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}

	Graphics()->SetColor(m_TextR, m_TextG, m_TextB, m_TextA);
	Line = -1;
	LineOffset = vec2(0, 0);
	for(int i = NumQuads - 1; i >= 0; --i)
	{
		const CQuadGlyph& Quad = pCursor->m_Glyphs[i];
		Graphics()->SetColor(Quad.m_TextColor.r, Quad.m_TextColor.g, Quad.m_TextColor.b, Quad.m_TextColor.a);
		if(Line != Quad.m_Line)
		{
			Line = Quad.m_Line;
			if(HorizontalAlign == TEXTALIGN_RIGHT)
				LineOffset.x = BoxWidth - (Quad.m_Offset.x + Quad.m_Width);
			else if(HorizontalAlign == TEXTALIGN_CENTER)
				LineOffset.x = (BoxWidth - (Quad.m_Offset.x + Quad.m_Width)) / 2.0f;
			else
				LineOffset.x = 0;
		}
		Graphics()->QuadsSetSubset(Quad.m_aUvs[0], Quad.m_aUvs[1], Quad.m_aUvs[2], Quad.m_aUvs[3]);

		vec2 QuadPosition = pCursor->m_CursorPos + Quad.m_Offset + AlignedBox.m_Min + LineOffset;
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(QuadPosition.x, QuadPosition.y, Quad.m_Width, Quad.m_Height);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();
}

IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }
