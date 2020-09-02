/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <engine/external/json-parser/json.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include "textrender.h"

CGlyphMap::CGlyphMap()
{
	m_DefaultFace = NULL;
	m_VariantFace = NULL;

	mem_zero(m_aFallbackFaces, sizeof(m_aFallbackFaces));

	mem_zero(m_aFtFaces, sizeof(m_aFtFaces));

	for (unsigned i = 0; i < NUM_FONT_SIZES; ++i)
	{
		m_aSizes[i].m_FontSize = -1;
	}

	m_NumFtFaces = 0;
	m_NumFallbackFaces = 0;
}

FT_Face CGlyphMap::GetCharFace(int Chr)
{
	if (!m_DefaultFace || FT_Get_Char_Index(m_DefaultFace, (FT_ULong)Chr)) 
		return m_DefaultFace;

	if (m_VariantFace && FT_Get_Char_Index(m_VariantFace, (FT_ULong)Chr)) 
		return m_VariantFace;

	for (int i = 0; i < m_NumFallbackFaces; ++i)
		if (m_aFallbackFaces[i] && FT_Get_Char_Index(m_aFallbackFaces[i], (FT_ULong)Chr))
			return m_aFallbackFaces[i];

	return m_DefaultFace;
}

int CGlyphMap::AddFace(FT_Face Face)
{
	if (m_NumFtFaces >= MAX_FACES) 
		return -1;

	m_aFtFaces[m_NumFtFaces++] = Face;
	if (!m_DefaultFace) m_DefaultFace = Face;

	return 0; 
}

void CGlyphMap::AddFallbackFaceByName(const char *pFamilyName)
{
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

bool CGlyphMap::SetVariantFaceByName(const char *pFamilyName)
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
		return true;
	}
	return false;
}

struct CQuadChar
{
	float m_aUvs[4];
	IGraphics::CQuadItem m_QuadItem;
};

int CTextRender::GetFontSizeIndex(int Pixelsize)
{
	for(unsigned i = 0; i < NUM_FONT_SIZES; i++)
	{
		if(aFontSizes[i] >= Pixelsize)
			return i;
	}

	return NUM_FONT_SIZES-1;
}

void CTextRender::Grow(unsigned char *pIn, unsigned char *pOut, int w, int h)
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

void CTextRender::InitTexture(CFontSizeData *pSizeData, int CharWidth, int CharHeight, int Xchars, int Ychars, int LangVariant)
{
	static int FontMemoryUsage = 0;
	int Width = CharWidth*Xchars;
	int Height = CharHeight*Ychars;
	void *pMem = mem_alloc(Width*Height, 1);
	mem_zero(pMem, Width*Height);

	for(int i = 0; i < 2; i++)
	{
		if(pSizeData->m_aTextures[i].IsValid())
		{
			Graphics()->UnloadTexture(&(pSizeData->m_aTextures[i]));
			FontMemoryUsage -= pSizeData->m_TextureWidth*pSizeData->m_TextureHeight;
		}

		pSizeData->m_aTextures[i] = Graphics()->LoadTextureRaw(Width, Height, CImageInfo::FORMAT_ALPHA, pMem, CImageInfo::FORMAT_ALPHA, IGraphics::TEXLOAD_NOMIPMAPS);
		FontMemoryUsage += Width*Height;
	}

	pSizeData->m_NumXChars = Xchars;
	pSizeData->m_NumYChars = Ychars;
	pSizeData->m_TextureWidth = Width;
	pSizeData->m_TextureHeight = Height;
	pSizeData->m_CurrentCharacter = 0;
	pSizeData->m_LanguageVariant = LangVariant;

	dbg_msg("pFont", "created texture for font size %d, memory usage: %d", pSizeData->m_FontSize, FontMemoryUsage);

	mem_free(pMem);
}

int CTextRender::AdjustOutlineThicknessToFontSize(int OutlineThickness, int FontSize)
{
	if(FontSize > 36)
		OutlineThickness *= 4;
	else if(FontSize >= 18)
		OutlineThickness *= 2;
	return OutlineThickness;
}

void CTextRender::IncreaseTextureSize(CFontSizeData *pSizeData)
{
	if(pSizeData->m_TextureWidth < pSizeData->m_TextureHeight)
		pSizeData->m_NumXChars <<= 1;
	else
		pSizeData->m_NumYChars <<= 1;
	InitTexture(pSizeData, pSizeData->m_CharMaxWidth, pSizeData->m_CharMaxHeight, pSizeData->m_NumXChars, pSizeData->m_NumYChars, pSizeData->m_LanguageVariant);
}

// TODO: Refactor: move this into a pFont class
void CTextRender::InitIndex(CGlyphMap *pFont, int Index)
{
	CFontSizeData *pSizeData = &pFont->m_aSizes[Index];

	pSizeData->m_FontSize = aFontSizes[Index];

	int GlyphSize;
	for(GlyphSize = 1; GlyphSize < pSizeData->m_FontSize << 1; GlyphSize <<= 1);

	pSizeData->m_CharMaxWidth = GlyphSize;
	pSizeData->m_CharMaxHeight = GlyphSize;

	InitTexture(pSizeData, pSizeData->m_CharMaxWidth, pSizeData->m_CharMaxHeight, 8, 8, m_CurrentVariant);
}

CFontSizeData *CTextRender::GetSize(CGlyphMap *pFont, int Pixelsize)
{
	int Index = GetFontSizeIndex(Pixelsize);
	CFontSizeData *pSizeData = &pFont->m_aSizes[Index];
	if(pSizeData->m_FontSize != aFontSizes[Index])
		InitIndex(pFont, Index);
	else if(pSizeData->m_LanguageVariant != m_CurrentVariant)
		InitTexture(pSizeData, pSizeData->m_CharMaxWidth, pSizeData->m_CharMaxHeight, pSizeData->m_NumXChars, pSizeData->m_NumYChars, m_CurrentVariant);
	return &pFont->m_aSizes[Index];
}


void CTextRender::UploadGlyph(CFontSizeData *pSizeData, int Texnum, int SlotID, int Chr, const void *pData)
{
	int x = (SlotID%pSizeData->m_NumXChars) * (pSizeData->m_TextureWidth/pSizeData->m_NumXChars);
	int y = (SlotID/pSizeData->m_NumXChars) * (pSizeData->m_TextureHeight/pSizeData->m_NumYChars);

	Graphics()->LoadTextureRawSub(pSizeData->m_aTextures[Texnum], x, y,
		pSizeData->m_TextureWidth/pSizeData->m_NumXChars,
		pSizeData->m_TextureHeight/pSizeData->m_NumYChars,
		CImageInfo::FORMAT_ALPHA, pData);
	/*
	glBindTexture(GL_TEXTURE_2D, pSizeData->m_aTextures[Texnum]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
		pSizeData->m_TextureWidth/pSizeData->m_NumXChars,
		pSizeData->m_TextureHeight/pSizeData->m_NumYChars,
		m_FontTextureFormat, GL_UNSIGNED_BYTE, pData);*/
}

// 32k of data used for rendering glyphs
unsigned char ms_aGlyphData[(1024/8) * (1024/8)];
unsigned char ms_aGlyphDataOutlined[(1024/8) * (1024/8)];

int CTextRender::GetSlot(CFontSizeData *pSizeData)
{
	int CharCount = pSizeData->m_NumXChars*pSizeData->m_NumYChars;
	if(pSizeData->m_CurrentCharacter < CharCount)
	{
		int i = pSizeData->m_CurrentCharacter;
		pSizeData->m_CurrentCharacter++;
		return i;
	}

	// kick out the oldest
	// TODO: remove this linear search
	{
		int Oldest = 0;
		for(int i = 1; i < CharCount; i++)
		{
			if(pSizeData->m_aCharacters[i].m_TouchTime < pSizeData->m_aCharacters[Oldest].m_TouchTime)
				Oldest = i;
		}

		if(time_get()-pSizeData->m_aCharacters[Oldest].m_TouchTime < time_freq() &&
			(pSizeData->m_NumXChars < MAX_CHARACTERS || pSizeData->m_NumYChars < MAX_CHARACTERS))
		{
			IncreaseTextureSize(pSizeData);
			return GetSlot(pSizeData);
		}

		return Oldest;
	}
}

int CTextRender::RenderGlyph(CGlyphMap *pFont, CFontSizeData *pSizeData, int Chr, int ReplacingSlot)
{
	FT_Bitmap *pBitmap;
	int SlotID = 0;
	int SlotW = pSizeData->m_TextureWidth / pSizeData->m_NumXChars;
	int SlotH = pSizeData->m_TextureHeight / pSizeData->m_NumYChars;
	int SlotSize = SlotW*SlotH;
	int x = 1;
	int y = 1;
	unsigned int px, py;

	FT_Face CharFace = pFont->GetCharFace(Chr);
	FT_Set_Pixel_Sizes(CharFace, 0, pSizeData->m_FontSize);

	if(FT_Load_Char(CharFace, Chr, FT_LOAD_RENDER|FT_LOAD_NO_BITMAP))
	{
		dbg_msg("pFont", "error loading glyph %d", Chr);
		return -1;
	}

	pBitmap = &CharFace->glyph->bitmap; // ignore_convention

	// fetch slot
	if (ReplacingSlot >= 0)
		SlotID = ReplacingSlot;
	else
		SlotID = GetSlot(pSizeData);

	if(SlotID < 0)
		return -1;

	// adjust spacing
	int OutlineThickness = AdjustOutlineThicknessToFontSize(1, pSizeData->m_FontSize);
	x += OutlineThickness;
	y += OutlineThickness;

	// prepare glyph data
	mem_zero(ms_aGlyphData, SlotSize);

	if(pBitmap->pixel_mode == FT_PIXEL_MODE_GRAY) // ignore_convention
	{
		for(py = 0; py < pBitmap->rows; py++) // ignore_convention
			for(px = 0; px < pBitmap->width; px++) // ignore_convention
				ms_aGlyphData[(py+y)*SlotW+px+x] = pBitmap->buffer[py*pBitmap->pitch+px]; // ignore_convention
	}
	else if(pBitmap->pixel_mode == FT_PIXEL_MODE_MONO) // ignore_convention
	{
		for(py = 0; py < pBitmap->rows; py++) // ignore_convention
			for(px = 0; px < pBitmap->width; px++) // ignore_convention
			{
				if(pBitmap->buffer[py*pBitmap->pitch+px/8]&(1<<(7-(px%8)))) // ignore_convention
					ms_aGlyphData[(py+y)*SlotW+px+x] = 255;
			}
	}

	/*for(py = 0; py < SlotW; py++)
		for(px = 0; px < SlotH; px++)
			ms_aGlyphData[py*SlotW+px] = 255;*/

	// upload the glyph
	UploadGlyph(pSizeData, 0, SlotID, Chr, ms_aGlyphData);

	if(OutlineThickness == 1)
	{
		Grow(ms_aGlyphData, ms_aGlyphDataOutlined, SlotW, SlotH);
		UploadGlyph(pSizeData, 1, SlotID, Chr, ms_aGlyphDataOutlined);
	}
	else
	{
		for(int i = OutlineThickness; i > 0; i-=2)
		{
			Grow(ms_aGlyphData, ms_aGlyphDataOutlined, SlotW, SlotH);
			Grow(ms_aGlyphDataOutlined, ms_aGlyphData, SlotW, SlotH);
		}
		UploadGlyph(pSizeData, 1, SlotID, Chr, ms_aGlyphData);
	}

	// set char info
	{
		CGlyph *pFontchr = &pSizeData->m_aCharacters[SlotID];
		float Scale = 1.0f/pSizeData->m_FontSize;
		float Uscale = 1.0f/pSizeData->m_TextureWidth;
		float Vscale = 1.0f/pSizeData->m_TextureHeight;
		int Height = pBitmap->rows + OutlineThickness*2 + 2; // ignore_convention
		int Width = pBitmap->width + OutlineThickness*2 + 2; // ignore_convention

		pFontchr->m_ID = Chr;
		pFontchr->m_Height = Height * Scale;
		pFontchr->m_Width = Width * Scale;
		pFontchr->m_OffsetX = (CharFace->glyph->bitmap_left-2) * Scale; // ignore_convention
		pFontchr->m_OffsetY = (pSizeData->m_FontSize - CharFace->glyph->bitmap_top) * Scale; // ignore_convention
		pFontchr->m_AdvanceX = (CharFace->glyph->advance.x>>6) * Scale; // ignore_convention

		pFontchr->m_aUvs[0] = (SlotID%pSizeData->m_NumXChars) / (float)(pSizeData->m_NumXChars);
		pFontchr->m_aUvs[1] = (SlotID/pSizeData->m_NumXChars) / (float)(pSizeData->m_NumYChars);
		pFontchr->m_aUvs[2] = pFontchr->m_aUvs[0] + Width*Uscale;
		pFontchr->m_aUvs[3] = pFontchr->m_aUvs[1] + Height*Vscale;
	}

	return SlotID;
}

CGlyph *CTextRender::GetChar(CGlyphMap *pFont, CFontSizeData *pSizeData, int Chr)
{
	CGlyph *pFontchr = NULL;

	// search for the character
	// TODO: remove this linear search
	int i;
	for(i = 0; i < pSizeData->m_CurrentCharacter; i++)
	{
		if(pSizeData->m_aCharacters[i].m_ID == Chr)
		{
			pFontchr = &pSizeData->m_aCharacters[i];
			break;
		}
	}

	// check if we need to render the character
	if(!pFontchr)
	{
		int Index = RenderGlyph(pFont, pSizeData, Chr);
		if(Index >= 0)
			pFontchr = &pSizeData->m_aCharacters[Index];
	}

	// touch the character
	// TODO: don't call time_get here
	if(pFontchr)
		pFontchr->m_TouchTime = time_get();

	return pFontchr;
}

// must only be called from the rendering function as the pFont must be set to the correct size
void CTextRender::RenderSetup(CGlyphMap *pFont, int size)
{
	FT_Set_Pixel_Sizes(pFont->GetDefaultFace(), 0, size);
}

vec2 CTextRender::Kerning(CGlyphMap *pFont, int Left, int Right)
{
	FT_Vector Kerning = {0,0};
	FT_Face LeftFace = pFont->GetCharFace(Left);
	FT_Face RightFace = pFont->GetCharFace(Right);
	if (LeftFace == RightFace)
		FT_Get_Kerning(LeftFace, Left, Right, FT_KERNING_DEFAULT, &Kerning);

	vec2 Vec;
	Vec.x = (float)(Kerning.x>>6);
	Vec.y = (float)(Kerning.y>>6);
	return Vec;
}

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

		if (m_pDefaultFont->AddFace(FtFace))
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

	m_pDefaultFont = 0;
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
	m_pDefaultFont = new CGlyphMap();
}

void CTextRender::Shutdown()
{
	delete m_pDefaultFont;
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
			m_pDefaultFont->AddFallbackFaceByName((const char *)rFallbackFaces[i]);
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
	if (!m_pDefaultFont) return;	

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

	m_pDefaultFont->SetVariantFaceByName(FamilyName);
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
	CGlyphMap *pFont = pCursor->m_pFont;
	CFontSizeData *pSizeData = NULL;

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

	// fetch pFont data
	if(!pFont)
		pFont = m_pDefaultFont;

	if(!pFont || !pFont->GetDefaultFace())
		return;

	pSizeData = GetSize(pFont, ActualSize);
	RenderSetup(pFont, ActualSize);
	*pFontTexture = pSizeData->m_aTextures[0];

	float Scale = 1.0f/pSizeData->m_FontSize;

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

			CGlyph *pChr = GetChar(pFont, pSizeData, Character);
			if(pChr)
			{
				vec2 Kern = Kerning(pFont, Character, NextCharacter)*Scale;
				DrawY += Kern.y;
				float Advance = pChr->m_AdvanceX + Kern.x *Scale;
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
	CGlyphMap *pFont = pCursor->m_pFont;
	CFontSizeData *pSizeData = NULL;

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

	// fetch pFont data
	if(!pFont)
		pFont = m_pDefaultFont;

	if(!pFont || !pFont->GetDefaultFace())
		return;

	pSizeData = GetSize(pFont, ActualSize);
	RenderSetup(pFont, ActualSize);

	float Scale = 1.0f/pSizeData->m_FontSize;

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
				Graphics()->TextureSet(pSizeData->m_aTextures[1]);
			else
				Graphics()->TextureSet(pSizeData->m_aTextures[0]);

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

				CGlyph *pChr = GetChar(pFont, pSizeData, Character);
				if(pChr)
				{
					vec2 Kern = Kerning(pFont, Character, NextCharacter)*Scale;
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
	CGlyphMap *pFont = pCursor->m_pFont;
	CFontSizeData *pSizeData = NULL;

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

	// fetch pFont data
	if(!pFont)
		pFont = m_pDefaultFont;

	if(!pFont || !pFont->GetDefaultFace())
		return 0;

	pSizeData = GetSize(pFont, ActualSize);
	RenderSetup(pFont, ActualSize);
	CGlyph *pChr = GetChar(pFont, pSizeData, ' ');
	return CursorY + pChr->m_OffsetY*Size + pChr->m_Height*Size;
}

IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }
