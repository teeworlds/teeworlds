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

	if (CurX + Width > m_Width - 1) return -1;

	for (int i = Index; i < m_Sections.size(); ++i)
	{
		if (FitWidth <= 0) break;

		Section = m_Sections[i];
		if (Section.y > CurY) CurY = Section.y;
		if (CurY + Height > m_Height - 1) return -1;
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
	
	for (int i = 0; i < m_Sections.size(); ++i)
	{
		int y = TrySection(i, Width, Height);
		if (y >= 0)
		{
			ivec3 Section = m_Sections[i];
			int NewHeight = y + Height;
			if ((NewHeight < BestHeight) || ((NewHeight == BestHeight) && (Section.l > 0 && Section.l < BestWidth)))
			{
				BestHeight = NewHeight;
				BestWidth = Section.l;
				BestSectionIndex = i;
				Position.x = Section.x;
				Position.y = y;
			}
		}
	}

	if (BestSectionIndex < 0)
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

	for (int i = BestSectionIndex + 1; i < m_Sections.size(); ++i)
	{
		ivec3 *Section = &m_Sections[i];
		ivec3 *Previous = &m_Sections[i-1];

		if (Section->x >= Previous->x + Previous->l) break;
		
		int Shrink = Previous->x + Previous->l - Section->x;
		Section->x += Shrink;
		Section->l -= Shrink;
		if (Section->l > 0) break;

		m_Sections.remove_index(i);
		i -= 1;
	}


    for (int i = 0; i < m_Sections.size()-1; ++i)
    {
        ivec3 *Section = &m_Sections[i];
        ivec3 *Next = &m_Sections[i+1];
        if( Section->y == Next->y )
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
					if (GetX >= 0 && GetY >= 0 && GetX < w && GetY < h)
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
	m_NumTotalPages = 0;

	for (int y = 0; y < PAGE_COUNT; ++y)
	{
		for (int x = 0; x < PAGE_COUNT; ++x)
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
	for (int i = 0; i < PAGE_COUNT*PAGE_COUNT; ++i)
	{
		*Position = m_aAtlasPages[i].Add(Width, Height);
		if (Position->x >= 0 && Position->y >= 0)
			return i;
	}
	
	// out of space, drop a page
	int LeastAccess = INT_MAX;
	int Atlas = 0;
	for (int i = 0; i < PAGE_COUNT*PAGE_COUNT; ++i)
	{
		int PageAccess = m_aAtlasPages[i].GetAccess();
		if (LeastAccess < PageAccess)
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
	return Atlas;
}

void CGlyphMap::UploadGlyph(int TextureIndex, int PosX, int PosY, int Width, int Height, const unsigned char *pData)
{
	m_pGraphics->LoadTextureRawSub(m_aTextures[TextureIndex], PosX, PosY,
		Width, Height, CImageInfo::FORMAT_ALPHA, pData);
}

bool CGlyphMap::RenderGlyph(int Chr, int FontSizeIndex, CGlyph *pGlyph)
{
	FT_Bitmap *pBitmap;
	int x = 1;
	int y = 1;
	unsigned int px, py;

	FT_Face CharFace;
	int GlyphIndex = GetCharGlyph(Chr, &CharFace);
	int FontSize = aFontSizes[FontSizeIndex];
	FT_Set_Pixel_Sizes(CharFace, 0, FontSize);

	if(FT_Load_Glyph(CharFace, GlyphIndex, FT_LOAD_RENDER|FT_LOAD_NO_BITMAP))
	{
		dbg_msg("pGlyphMap", "error loading glyph %d", Chr);
		return false;
	}

	pBitmap = &CharFace->glyph->bitmap; // ignore_convention

	// adjust spacing
	int OutlineThickness = AdjustOutlineThicknessToFontSize(1, FontSize);
	x += OutlineThickness;
	y += OutlineThickness;

	unsigned int Width = pBitmap->width + x * 2;
	unsigned int Height = pBitmap->rows + y * 2;

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
	int AtlasIndex = FitGlyph(Width, Height, &Position);
	int Page = m_aAtlasPages[AtlasIndex].GetPageID();

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

	// set char info
	float Scale = 1.0f / FontSize;
	float Uscale = 1.0f / TEXTURE_SIZE;
	float Vscale = 1.0f / TEXTURE_SIZE;

	pGlyph->m_ID = Chr;
	pGlyph->m_FontSizeIndex = FontSizeIndex;
	pGlyph->m_AtlasIndex = AtlasIndex;
	pGlyph->m_PageID = Page;

	pGlyph->m_Height = Height * Scale;
	pGlyph->m_Width = Width * Scale;
	pGlyph->m_OffsetX = (CharFace->glyph->bitmap_left-2) * Scale; // ignore_convention
	pGlyph->m_OffsetY = (FontSize - CharFace->glyph->bitmap_top) * Scale; // ignore_convention
	pGlyph->m_AdvanceX = (CharFace->glyph->advance.x>>6) * Scale; // ignore_convention

	pGlyph->m_aUvs[0] = Position.x * Uscale;
	pGlyph->m_aUvs[1] = Position.y * Vscale;
	pGlyph->m_aUvs[2] = pGlyph->m_aUvs[0] + Width * Uscale;
	pGlyph->m_aUvs[3] = pGlyph->m_aUvs[1] + Height * Vscale;

	return true;
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

	InitTexture(TEXTURE_SIZE, TEXTURE_SIZE);
}

int CGlyphMap::GetCharGlyph(int Chr, FT_Face *pFace)
{
	int GlyphIndex = FT_Get_Char_Index(m_DefaultFace, (FT_ULong)Chr);
	*pFace = m_DefaultFace;

	if (!m_DefaultFace || GlyphIndex)
		return GlyphIndex;

	if (m_VariantFace)
	{
		GlyphIndex = FT_Get_Char_Index(m_VariantFace, (FT_ULong)Chr);
		if (GlyphIndex)
		{
			*pFace = m_VariantFace;
			return GlyphIndex;
		}
	}

	for (int i = 0; i < m_NumFallbackFaces; ++i)
	{
		if (m_aFallbackFaces[i])
		{
			GlyphIndex = FT_Get_Char_Index(m_aFallbackFaces[i], (FT_ULong)Chr);
			*pFace = m_aFallbackFaces[i];
			return GlyphIndex;
		}
	}

	return 0;
}

int CGlyphMap::AddFace(FT_Face Face)
{
	if (m_NumFtFaces == MAX_FACES) 
		return -1;

	m_aFtFaces[m_NumFtFaces++] = Face;
	if (!m_DefaultFace) m_DefaultFace = Face;

	return 0; 
}

void CGlyphMap::AddFallbackFaceByName(const char *pFamilyName)
{
	if (m_NumFallbackFaces == MAX_FACES)
		return;

	char aFamilyStyleName[128];
	FT_Face Face = NULL;
	for (int i = 0; i < m_NumFtFaces; ++i)
	{
		str_format(aFamilyStyleName, 128, "%s %s", m_aFtFaces[i]->family_name, m_aFtFaces[i]->style_name);
		if (str_comp(pFamilyName, aFamilyStyleName) == 0)
		{
			Face = m_aFtFaces[i];
			break;
		}

		if (!Face && str_comp(pFamilyName, m_aFtFaces[i]->family_name) == 0)
		{
			Face = m_aFtFaces[i];
		}
	}

	m_aFallbackFaces[m_NumFallbackFaces++] = Face;
}

void CGlyphMap::SetVariantFaceByName(const char *pFamilyName)
{
	FT_Face Face = NULL;
	if (pFamilyName != NULL)
	{
		for (int i = 0; i < m_NumFtFaces; ++i)
		{
			if (str_comp(pFamilyName, m_aFtFaces[i]->family_name) == 0)
			{
				Face = m_aFtFaces[i];
				break;
			}
		}
	}

	if (m_VariantFace != Face)
	{
		m_VariantFace = Face;
		InitTexture(TEXTURE_SIZE, TEXTURE_SIZE);
	}
	return;
}

CGlyph *CGlyphMap::GetGlyph(int Chr, int FontSizeIndex)
{
	CGlyph Glyph;
	Glyph.m_FontSizeIndex = FontSizeIndex;
	Glyph.m_ID = Chr;
	
	sorted_array<CGlyph>::range r = ::find_binary(m_Glyphs.all(), Glyph);
	
	CGlyph *pGlyph = NULL;

	// couldn't find glyph, render a new one
	if(r.empty())
	{
		if (RenderGlyph(Chr, FontSizeIndex, &Glyph))
		{
			int Index = m_Glyphs.add(Glyph);
			pGlyph = &m_Glyphs[Index];
		}
	}
	else
	{
		pGlyph = &r.front();

		if (m_aAtlasPages[pGlyph->m_AtlasIndex].GetPageID() != pGlyph->m_PageID)
		{
			// re-render glyph if the page is dropped
			RenderGlyph(Chr, FontSizeIndex, pGlyph);
		}
		else
		{
			// otherwise touch the page
			m_aAtlasPages[pGlyph->m_AtlasIndex].Touch();
		}
	}

	return pGlyph;
}

vec2 CGlyphMap::Kerning(int Left, int Right)
{
	FT_Vector Kerning = {0,0};
	FT_Face LeftFace;
	GetCharGlyph(Left, &LeftFace);
	FT_Face RightFace;
	GetCharGlyph(Right, &RightFace);

	if (LeftFace == RightFace)
		FT_Get_Kerning(LeftFace, Left, Right, FT_KERNING_DEFAULT, &Kerning);

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

struct CQuadChar
{
	float m_aUvs[4];
	IGraphics::CQuadItem m_QuadItem;
};

int CTextRender::LoadFontCollection(const char *pFilename)
{
	FT_Face FtFace;

	if(FT_New_Face(m_FTLibrary, pFilename, -1, &FtFace))
		return -1;

	int NumFaces = FtFace->num_faces;
	FT_Done_Face(FtFace);

	int i;
	for (i = 0; i < NumFaces; ++i)
	{
		if(FT_New_Face(m_FTLibrary, pFilename, i, &FtFace))
			break;

		if (m_pGlyphMap->AddFace(FtFace))
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
	m_TextOutlineR = 0.0f;
	m_TextOutlineG = 0.0f;
	m_TextOutlineB = 0.0f;
	m_TextOutlineA = 0.3f;

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
	if (m_pGlyphMap) m_pGlyphMap->PagesAccessReset();
}

void CTextRender::Shutdown()
{
	delete m_pGlyphMap;
	if (m_paVariants) mem_free(m_paVariants);
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
			if (pFamilyName->type == json_string)
				strncpy(m_paVariants[i].m_aFamilyName, pFamilyName->u.string.ptr, sizeof(m_paVariants[i].m_aFamilyName));
			else
				m_paVariants[i].m_aFamilyName[0] = 0;
		}
	}

	json_value_free(pJsonData);
}

void CTextRender::SetFontLanguageVariant(const char *pLanguageFile)
{
	if (!m_pGlyphMap) return;	

	char *FamilyName = NULL;

	if (m_paVariants)
	{
		for (int i = 0; i < m_NumVariants; ++i)
		{
			if (str_comp_filenames(pLanguageFile, m_paVariants[i].m_aLanguageFile) == 0)
			{
				FamilyName = m_paVariants[i].m_aFamilyName;
				m_CurrentVariant = i;
				break;
			}
		}
	}

	m_pGlyphMap->SetVariantFaceByName(FamilyName);
}

void CTextRender::SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags)
{
	mem_zero(pCursor, sizeof(*pCursor));
	pCursor->m_FontSize = FontSize;
	pCursor->m_StartX = x;
	pCursor->m_StartY = y;
	pCursor->m_X = x;
	pCursor->m_Y = y;
	pCursor->m_LineCount = 1;
	pCursor->m_LineWidth = -1.0f;
	pCursor->m_Flags = Flags;
	pCursor->m_GlyphCount = 0;
	pCursor->m_CharCount = 0;
}

void CTextRender::Text(void *pFontSetV, float x, float y, float Size, const char *pText, float LineWidth, bool MultiLine)
{
	CTextCursor Cursor;
	SetCursor(&Cursor, x, y, Size, TEXTFLAG_RENDER);
	if(!MultiLine)
		Cursor.m_Flags |= TEXTFLAG_STOP_AT_END;
	Cursor.m_LineWidth = LineWidth;
	TextEx(&Cursor, pText, -1);
}

float CTextRender::TextWidth(void *pFontSetV, float Size, const char *pText, int StrLength, float LineWidth)
{
	CTextCursor Cursor;
	SetCursor(&Cursor, 0, 0, Size, 0);
	Cursor.m_LineWidth = LineWidth;
	TextEx(&Cursor, pText, StrLength);
	return Cursor.m_X;
}

int CTextRender::TextLineCount(void *pFontSetV, float Size, const char *pText, float LineWidth)
{
	CTextCursor Cursor;
	SetCursor(&Cursor, 0, 0, Size, 0);
	Cursor.m_LineWidth = LineWidth;
	TextEx(&Cursor, pText, -1);
	return Cursor.m_LineCount;
}

void CTextRender::TextColor(float r, float g, float b, float a)
{
	m_TextR = r;
	m_TextG = g;
	m_TextB = b;
	m_TextA = a;
}

void CTextRender::TextOutlineColor(float r, float g, float b, float a)
{
	m_TextOutlineR = r;
	m_TextOutlineG = g;
	m_TextOutlineB = b;
	m_TextOutlineA = a;
}

void CTextRender::TextShadowed(CTextCursor *pCursor, const char *pText, int Length, vec2 ShadowOffset,
							vec4 ShadowColor, vec4 TextColor_)
{
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	float FakeToScreenX, FakeToScreenY;

	// to correct coords, convert to screen coords, round, and convert back
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	FakeToScreenX = (Graphics()->ScreenWidth()/(ScreenX1-ScreenX0));
	FakeToScreenY = (Graphics()->ScreenHeight()/(ScreenY1-ScreenY0));
	ShadowOffset.x /= FakeToScreenX;
	ShadowOffset.y /= FakeToScreenY;

	CQuadChar aTextQuads[1024];
	int TextQuadCount = 0;
	IGraphics::CTextureHandle FontTexture;

	TextDeferredRenderEx(pCursor, pText, Length, aTextQuads, sizeof(aTextQuads)/sizeof(aTextQuads[0]),
			&TextQuadCount, &FontTexture);

	Graphics()->TextureSet(FontTexture);

	Graphics()->QuadsBegin();

	// shadow pass
	Graphics()->SetColor(ShadowColor.r, ShadowColor.g, ShadowColor.b, ShadowColor.a);

	for(int i = 0; i < TextQuadCount; i++)
	{
		const CQuadChar& q = aTextQuads[i];
		Graphics()->QuadsSetSubset(q.m_aUvs[0], q.m_aUvs[1], q.m_aUvs[2], q.m_aUvs[3]);

		IGraphics::CQuadItem QuadItem = q.m_QuadItem;
		QuadItem.m_X += ShadowOffset.x;
		QuadItem.m_Y += ShadowOffset.y;
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}

	// text pass
	Graphics()->SetColor(TextColor_.r, TextColor_.g, TextColor_.b, TextColor_.a);

	for(int i = 0; i < TextQuadCount; i++)
	{
		const CQuadChar& q = aTextQuads[i];
		Graphics()->QuadsSetSubset(q.m_aUvs[0], q.m_aUvs[1], q.m_aUvs[2], q.m_aUvs[3]);
		Graphics()->QuadsDrawTL(&q.m_QuadItem, 1);
	}

	Graphics()->QuadsEnd();
}

void CTextRender::TextDeferredRenderEx(CTextCursor *pCursor, const char *pText, int Length,
									CQuadChar* aQuadChar, int QuadCharMaxCount, int* pQuadCharCount,
									IGraphics::CTextureHandle* pFontTexture)
{
	CGlyphMap *pFont = m_pGlyphMap;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	float FakeToScreenX, FakeToScreenY;
	int ActualX, ActualY;

	int ActualSize;
	int GotNewLine = 0;
	float DrawX = 0.0f, DrawY = 0.0f;
	int LineCount = 0;
	float CursorX, CursorY;

	float Size = pCursor->m_FontSize;

	// to correct coords, convert to screen coords, round, and convert back
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	FakeToScreenX = (Graphics()->ScreenWidth()/(ScreenX1-ScreenX0));
	FakeToScreenY = (Graphics()->ScreenHeight()/(ScreenY1-ScreenY0));
	ActualX = (int)(pCursor->m_X * FakeToScreenX);
	ActualY = (int)(pCursor->m_Y * FakeToScreenY);

	CursorX = ActualX / FakeToScreenX;
	CursorY = ActualY / FakeToScreenY;

	// same with size
	ActualSize = (int)(Size * FakeToScreenY);
	Size = ActualSize / FakeToScreenY;

	if(!pFont || !pFont->GetDefaultFace())
		return;

	int FontSizeIndex = pFont->GetFontSizeIndex(ActualSize);
	int FontSize = aFontSizes[FontSizeIndex];
	*pFontTexture = pFont->GetTexture(0);

	float Scale = 1.0f / FontSize;

	// set length
	if(Length < 0)
		Length = str_length(pText);

	const char *pCurrent = (char *)pText;
	const char *pEnd = pCurrent+Length;
	DrawX = CursorX;
	DrawY = CursorY;
	LineCount = pCursor->m_LineCount;

	while(pCurrent < pEnd && (pCursor->m_MaxLines < 1 || LineCount <= pCursor->m_MaxLines))
	{
		int NewLine = 0;
		const char *pBatchEnd = pEnd;
		if(pCursor->m_LineWidth > 0 && !(pCursor->m_Flags&TEXTFLAG_STOP_AT_END))
		{
			int Wlen = min(WordLength((char *)pCurrent), (int)(pEnd-pCurrent));
			CTextCursor Compare = *pCursor;
			Compare.m_X = DrawX;
			Compare.m_Y = DrawY;
			Compare.m_Flags &= ~TEXTFLAG_RENDER;
			Compare.m_LineWidth = -1;
			TextDeferredRenderEx(&Compare, pCurrent, Wlen, aQuadChar, QuadCharMaxCount, pQuadCharCount,
									pFontTexture);

			if(Compare.m_X-DrawX > pCursor->m_LineWidth)
			{
				// word can't be fitted in one line, cut it
				CTextCursor Cutter = *pCursor;
				Cutter.m_GlyphCount = 0;
				Cutter.m_X = DrawX;
				Cutter.m_Y = DrawY;
				Cutter.m_Flags &= ~TEXTFLAG_RENDER;
				Cutter.m_Flags |= TEXTFLAG_STOP_AT_END;

				TextDeferredRenderEx(&Cutter, (const char *)pCurrent, Wlen, aQuadChar, QuadCharMaxCount,
										pQuadCharCount, pFontTexture);
				Wlen = Cutter.m_GlyphCount;
				NewLine = 1;

				if(Wlen <= 3) // if we can't place 3 chars of the word on this line, take the next
					Wlen = 0;
			}
			else if(Compare.m_X-pCursor->m_StartX > pCursor->m_LineWidth)
			{
				NewLine = 1;
				Wlen = 0;
			}

			pBatchEnd = pCurrent + Wlen;
		}

		const char *pTmp = pCurrent;
		int NextCharacter = str_utf8_decode(&pTmp);
		while(pCurrent < pBatchEnd)
		{
			pCursor->m_CharCount += pTmp-pCurrent;
			int Character = NextCharacter;
			pCurrent = pTmp;
			NextCharacter = str_utf8_decode(&pTmp);

			if(Character == '\n')
			{
				DrawX = pCursor->m_StartX;
				DrawY += Size;
				DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
				DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
				++LineCount;
				if(pCursor->m_MaxLines > 0 && LineCount > pCursor->m_MaxLines)
					break;
				continue;
			}

			CGlyph *pChr = pFont->GetGlyph(Character, FontSizeIndex);
			if(pChr)
			{
				vec2 Kern = pFont->Kerning(Character, NextCharacter) * Scale;
				DrawY += Kern.y;
				float Advance = pChr->m_AdvanceX + Kern.x * Scale;
				if(pCursor->m_Flags&TEXTFLAG_STOP_AT_END && DrawX+Advance*Size-pCursor->m_StartX > pCursor->m_LineWidth)
				{
					// we hit the end of the line, no more to render or count
					pCurrent = pEnd;
					break;
				}

				if(pCursor->m_Flags&TEXTFLAG_RENDER)
				{
					dbg_assert(*pQuadCharCount < QuadCharMaxCount, "aQuadChar size is too small");

					CQuadChar QuadChar;
					memmove(QuadChar.m_aUvs, pChr->m_aUvs, sizeof(pChr->m_aUvs));

					IGraphics::CQuadItem QuadItem(DrawX+pChr->m_OffsetX*Size,
													DrawY+pChr->m_OffsetY*Size,
													pChr->m_Width*Size,
													pChr->m_Height*Size);
					QuadChar.m_QuadItem = QuadItem;
					aQuadChar[(*pQuadCharCount)++] = QuadChar;
				}

				DrawX += Advance*Size;
				pCursor->m_GlyphCount++;
			}
		}

		if(NewLine)
		{
			DrawX = pCursor->m_StartX;
			DrawY += Size;
			GotNewLine = 1;
			DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
			DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
			++LineCount;
		}
	}

	pCursor->m_X = DrawX;
	pCursor->m_LineCount = LineCount;

	if(GotNewLine)
		pCursor->m_Y = DrawY;
}

void CTextRender::TextEx(CTextCursor *pCursor, const char *pText, int Length)
{
	CGlyphMap *pFont = m_pGlyphMap;

	//dbg_msg("textrender", "rendering text '%s'", text);

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	float FakeToScreenX, FakeToScreenY;
	int ActualX, ActualY;

	int ActualSize;
	int i;
	int GotNewLine = 0;
	float DrawX = 0.0f, DrawY = 0.0f;
	int LineCount = 0;
	float CursorX, CursorY;

	float Size = pCursor->m_FontSize;

	// to correct coords, convert to screen coords, round, and convert back
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	FakeToScreenX = (Graphics()->ScreenWidth()/(ScreenX1-ScreenX0));
	FakeToScreenY = (Graphics()->ScreenHeight()/(ScreenY1-ScreenY0));
	ActualX = (int)(pCursor->m_X * FakeToScreenX);
	ActualY = (int)(pCursor->m_Y * FakeToScreenY);

	CursorX = ActualX / FakeToScreenX;
	CursorY = ActualY / FakeToScreenY;

	// same with size
	ActualSize = (int)(Size * FakeToScreenY);
	Size = ActualSize / FakeToScreenY;

	if(!pFont || !pFont->GetDefaultFace())
		return;

	int FontSizeIndex = pFont->GetFontSizeIndex(ActualSize);
	int FontSize = aFontSizes[FontSizeIndex];

	float Scale = 1.0f / FontSize;

	// set length
	if(Length < 0)
		Length = str_length(pText);

	// if we don't want to render, we can just skip the first outline pass
	i = 1;
	if(pCursor->m_Flags&TEXTFLAG_RENDER)
		i = 0;

	for(;i < 2; i++)
	{
		const char *pCurrent = (char *)pText;
		const char *pEnd = pCurrent+Length;
		DrawX = CursorX;
		DrawY = CursorY;
		LineCount = pCursor->m_LineCount;

		if(pCursor->m_Flags&TEXTFLAG_RENDER)
		{
			// TODO: Make this better
			if (i == 0)
				Graphics()->TextureSet(pFont->GetTexture(1));
			else
				Graphics()->TextureSet(pFont->GetTexture(0));

			Graphics()->QuadsBegin();
			if (i == 0)
				Graphics()->SetColor(m_TextOutlineR, m_TextOutlineG, m_TextOutlineB, m_TextOutlineA*m_TextA);
			else
				Graphics()->SetColor(m_TextR, m_TextG, m_TextB, m_TextA);
		}

		while(pCurrent < pEnd && (pCursor->m_MaxLines < 1 || LineCount <= pCursor->m_MaxLines))
		{
			int NewLine = 0;
			const char *pBatchEnd = pEnd;
			if(pCursor->m_LineWidth > 0 && !(pCursor->m_Flags&TEXTFLAG_STOP_AT_END))
			{
				int Wlen = min(WordLength((char *)pCurrent), (int)(pEnd-pCurrent));
				CTextCursor Compare = *pCursor;
				Compare.m_X = DrawX;
				Compare.m_Y = DrawY;
				Compare.m_Flags &= ~TEXTFLAG_RENDER;
				Compare.m_LineWidth = -1;
				TextEx(&Compare, pCurrent, Wlen);

				if(Compare.m_X-DrawX > pCursor->m_LineWidth)
				{
					// word can't be fitted in one line, cut it
					CTextCursor Cutter = *pCursor;
					Cutter.m_GlyphCount = 0;
					Cutter.m_X = DrawX;
					Cutter.m_Y = DrawY;
					Cutter.m_Flags &= ~TEXTFLAG_RENDER;
					Cutter.m_Flags |= TEXTFLAG_STOP_AT_END;

					TextEx(&Cutter, (const char *)pCurrent, Wlen);
					Wlen = Cutter.m_GlyphCount;
					NewLine = 1;

					if(Wlen <= 3) // if we can't place 3 chars of the word on this line, take the next
						Wlen = 0;
				}
				else if(Compare.m_X-pCursor->m_StartX > pCursor->m_LineWidth)
				{
					NewLine = 1;
					Wlen = 0;
				}

				pBatchEnd = pCurrent + Wlen;
			}

			const char *pTmp = pCurrent;
			int NextCharacter = str_utf8_decode(&pTmp);
			while(pCurrent < pBatchEnd)
			{
				pCursor->m_CharCount += pTmp-pCurrent;
				int Character = NextCharacter;
				pCurrent = pTmp;
				NextCharacter = str_utf8_decode(&pTmp);

				if(Character == '\n')
				{
					DrawX = pCursor->m_StartX;
					DrawY += Size;
					GotNewLine = 1;
					DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
					DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
					++LineCount;
					if(pCursor->m_MaxLines > 0 && LineCount > pCursor->m_MaxLines)
						break;
					continue;
				}

				CGlyph *pChr = pFont->GetGlyph(Character, FontSizeIndex);
				if(pChr)
				{
					vec2 Kern = pFont->Kerning(Character, NextCharacter) * Scale;
					DrawY += Kern.y;
					float Advance = pChr->m_AdvanceX + Kern.x * Scale;
					if(pCursor->m_Flags&TEXTFLAG_STOP_AT_END && DrawX+Advance*Size-pCursor->m_StartX > pCursor->m_LineWidth)
					{
						// we hit the end of the line, no more to render or count
						pCurrent = pEnd;
						break;
					}

					if(pCursor->m_Flags&TEXTFLAG_RENDER)
					{
						Graphics()->QuadsSetSubset(pChr->m_aUvs[0], pChr->m_aUvs[1], pChr->m_aUvs[2], pChr->m_aUvs[3]);
						IGraphics::CQuadItem QuadItem(DrawX+pChr->m_OffsetX*Size, DrawY+pChr->m_OffsetY*Size, pChr->m_Width*Size, pChr->m_Height*Size);
						Graphics()->QuadsDrawTL(&QuadItem, 1);
					}

					DrawX += Advance*Size;
					pCursor->m_GlyphCount++;
				}
			}

			if(NewLine)
			{
				DrawX = pCursor->m_StartX;
				DrawY += Size;
				GotNewLine = 1;
				DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
				DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
				++LineCount;
			}
		}

		if(pCursor->m_Flags&TEXTFLAG_RENDER)
			Graphics()->QuadsEnd();
	}

	pCursor->m_X = DrawX;
	pCursor->m_LineCount = LineCount;

	if(GotNewLine)
		pCursor->m_Y = DrawY;
}

float CTextRender::TextGetLineBaseY(const CTextCursor *pCursor)
{
	CGlyphMap *pFont = m_pGlyphMap;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	float Size = pCursor->m_FontSize;

	// to correct coords, convert to screen coords, round, and convert back
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

	float FakeToScreenY = (Graphics()->ScreenHeight()/(ScreenY1-ScreenY0));
	int ActualY = (int)(pCursor->m_Y * FakeToScreenY);
	float CursorY = ActualY / FakeToScreenY;

	// same with size
	int ActualSize = (int)(Size * FakeToScreenY);
	Size = ActualSize / FakeToScreenY;

	if(!pFont || !pFont->GetDefaultFace())
		return 0;

	int FontSizeIndex = pFont->GetFontSizeIndex(ActualSize);

	CGlyph *pChr = pFont->GetGlyph(' ', FontSizeIndex);
	return CursorY + pChr->m_OffsetY*Size + pChr->m_Height*Size;
}

IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }
