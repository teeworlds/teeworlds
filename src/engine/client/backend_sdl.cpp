
#include "SDL.h"
#include "SDL_opengl.h"
#include <base/detect.h>

#if defined(CONF_PLATFORM_MACOSX)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <base/tl/threading.h>

#include "graphics_threaded.h"
#include "backend_sdl.h"

// ------------ CGraphicsBackend_Threaded

void CGraphicsBackend_Threaded::ThreadFunc(void *pUser)
{
	CGraphicsBackend_Threaded *pThis = (CGraphicsBackend_Threaded *)pUser;

	while(!pThis->m_Shutdown)
	{
		#ifdef CONF_PLATFORM_MACOSX
			CAutoreleasePool AutoreleasePool;
		#endif
		pThis->m_Activity.wait();
		if(pThis->m_pBuffer)
		{
			pThis->m_pProcessor->RunBuffer(pThis->m_pBuffer);
			sync_barrier();
			pThis->m_pBuffer = 0x0;
			pThis->m_BufferDone.signal();
		}
	}
}

CGraphicsBackend_Threaded::CGraphicsBackend_Threaded()
{
	m_pBuffer = 0x0;
	m_pProcessor = 0x0;
	m_pThread = 0x0;
}

void CGraphicsBackend_Threaded::StartProcessor(ICommandProcessor *pProcessor)
{
	m_Shutdown = false;
	m_pProcessor = pProcessor;
	m_pThread = thread_create(ThreadFunc, this);
	m_BufferDone.signal();
}

void CGraphicsBackend_Threaded::StopProcessor()
{
	m_Shutdown = true;
	m_Activity.signal();
	thread_wait(m_pThread);
	thread_destroy(m_pThread);
}

void CGraphicsBackend_Threaded::RunBuffer(CCommandBuffer *pBuffer)
{
	WaitForIdle();
	m_pBuffer = pBuffer;
	m_Activity.signal();
}

bool CGraphicsBackend_Threaded::IsIdle() const
{
	return m_pBuffer == 0x0;
}

void CGraphicsBackend_Threaded::WaitForIdle()
{
	while(m_pBuffer != 0x0)
		m_BufferDone.wait();
}


// ------------ CCommandProcessorFragment_General

void CCommandProcessorFragment_General::Cmd_Signal(const CCommandBuffer::SCommand_Signal *pCommand)
{
	pCommand->m_pSemaphore->signal();
}

bool CCommandProcessorFragment_General::RunCommand(const CCommandBuffer::SCommand * pBaseCommand)
{
	switch(pBaseCommand->m_Cmd)
	{
	case CCommandBuffer::CMD_NOP: break;
	case CCommandBuffer::CMD_SIGNAL: Cmd_Signal(static_cast<const CCommandBuffer::SCommand_Signal *>(pBaseCommand)); break;
	default: return false;
	}

	return true;
}

// ------------ CCommandProcessorFragment_OpenGL

int CCommandProcessorFragment_OpenGL::TexFormatToOpenGLFormat(int TexFormat)
{
	if(TexFormat == CCommandBuffer::TEXFORMAT_RGB) return GL_RGB;
	if(TexFormat == CCommandBuffer::TEXFORMAT_ALPHA) return GL_ALPHA;
	if(TexFormat == CCommandBuffer::TEXFORMAT_RGBA) return GL_RGBA;
	return GL_RGBA;
}

unsigned char CCommandProcessorFragment_OpenGL::Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp)
{
	int Value = 0;
	for(int x = 0; x < ScaleW; x++)
		for(int y = 0; y < ScaleH; y++)
			Value += pData[((v+y)*w+(u+x))*Bpp+Offset];
	return Value/(ScaleW*ScaleH);
}

void *CCommandProcessorFragment_OpenGL::Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData)
{
	unsigned char *pTmpData;
	int ScaleW = Width/NewWidth;
	int ScaleH = Height/NewHeight;

	int Bpp = 3;
	if(Format == CCommandBuffer::TEXFORMAT_RGBA)
		Bpp = 4;

	pTmpData = (unsigned char *)mem_alloc(NewWidth*NewHeight*Bpp, 1);

	int c = 0;
	for(int y = 0; y < NewHeight; y++)
		for(int x = 0; x < NewWidth; x++, c++)
		{
			pTmpData[c*Bpp] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 0, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+1] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 1, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+2] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 2, ScaleW, ScaleH, Bpp);
			if(Bpp == 4)
				pTmpData[c*Bpp+3] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 3, ScaleW, ScaleH, Bpp);
		}

	return pTmpData;
}

void CCommandProcessorFragment_OpenGL::SetState(const CCommandBuffer::SState &State)
{
	// blend
	switch(State.m_BlendMode)
	{
	case CCommandBuffer::BLEND_NONE:
		glDisable(GL_BLEND);
		break;
	case CCommandBuffer::BLEND_ALPHA:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case CCommandBuffer::BLEND_ADDITIVE:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	default:
		dbg_msg("render", "unknown blendmode %d\n", State.m_BlendMode);
	};

	// clip
	if(State.m_ClipEnable)
	{
		glScissor(State.m_ClipX, State.m_ClipY, State.m_ClipW, State.m_ClipH);
		glEnable(GL_SCISSOR_TEST);
	}
	else
		glDisable(GL_SCISSOR_TEST);
	
	// texture
	if(State.m_Texture >= 0 && State.m_Texture < CCommandBuffer::MAX_TEXTURES)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_aTextures[State.m_Texture].m_Tex);
	}
	else
		glDisable(GL_TEXTURE_2D);

	switch(State.m_WrapMode)
	{
	case CCommandBuffer::WRAP_REPEAT:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		break;
	case CCommandBuffer::WRAP_CLAMP:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		break;
	default:
		dbg_msg("render", "unknown wrapmode %d\n", State.m_WrapMode);
	};

	// screen mapping
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(State.m_ScreenTL.x, State.m_ScreenBR.x, State.m_ScreenBR.y, State.m_ScreenTL.y, 1.0f, 10.f);
}

void CCommandProcessorFragment_OpenGL::Cmd_Init(const SCommand_Init *pCommand)
{
	m_pTextureMemoryUsage = pCommand->m_pTextureMemoryUsage;
}

void CCommandProcessorFragment_OpenGL::Cmd_Texture_Update(const CCommandBuffer::SCommand_Texture_Update *pCommand)
{
	glBindTexture(GL_TEXTURE_2D, m_aTextures[pCommand->m_Slot].m_Tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, pCommand->m_X, pCommand->m_Y, pCommand->m_Width, pCommand->m_Height,
		TexFormatToOpenGLFormat(pCommand->m_Format), GL_UNSIGNED_BYTE, pCommand->m_pData);
	mem_free(pCommand->m_pData);
}

void CCommandProcessorFragment_OpenGL::Cmd_Texture_Destroy(const CCommandBuffer::SCommand_Texture_Destroy *pCommand)
{
	glDeleteTextures(1, &m_aTextures[pCommand->m_Slot].m_Tex);
	*m_pTextureMemoryUsage -= m_aTextures[pCommand->m_Slot].m_MemSize;
}

void CCommandProcessorFragment_OpenGL::Cmd_Texture_Create(const CCommandBuffer::SCommand_Texture_Create *pCommand)
{
	int Width = pCommand->m_Width;
	int Height = pCommand->m_Height;
	void *pTexData = pCommand->m_pData;

	// resample if needed
	if(pCommand->m_Format == CCommandBuffer::TEXFORMAT_RGBA || pCommand->m_Format == CCommandBuffer::TEXFORMAT_RGB)
	{
		int MaxTexSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTexSize);
		if(Width > MaxTexSize || Height > MaxTexSize)
		{
			do
			{
				Width>>=1;
				Height>>=1;
			}
			while(Width > MaxTexSize || Height > MaxTexSize);

			void *pTmpData = Rescale(pCommand->m_Width, pCommand->m_Height, Width, Height, pCommand->m_Format, static_cast<const unsigned char *>(pCommand->m_pData));
			mem_free(pTexData);
			pTexData = pTmpData;
		}
		else if(Width > 16 && Height > 16 && (pCommand->m_Flags&CCommandBuffer::TEXFLAG_QUALITY) == 0)
		{
			Width>>=1;
			Height>>=1;

			void *pTmpData = Rescale(pCommand->m_Width, pCommand->m_Height, Width, Height, pCommand->m_Format, static_cast<const unsigned char *>(pCommand->m_pData));
			mem_free(pTexData);
			pTexData = pTmpData;
		}
	}

	int Oglformat = TexFormatToOpenGLFormat(pCommand->m_Format);
	int StoreOglformat = TexFormatToOpenGLFormat(pCommand->m_StoreFormat);

	if(pCommand->m_Flags&CCommandBuffer::TEXFLAG_COMPRESSED)
	{
		switch(StoreOglformat)
		{
			case GL_RGB: StoreOglformat = GL_COMPRESSED_RGB_ARB; break;
			case GL_ALPHA: StoreOglformat = GL_COMPRESSED_ALPHA_ARB; break;
			case GL_RGBA: StoreOglformat = GL_COMPRESSED_RGBA_ARB; break;
			default: StoreOglformat = GL_COMPRESSED_RGBA_ARB;
		}
	}
	glGenTextures(1, &m_aTextures[pCommand->m_Slot].m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_aTextures[pCommand->m_Slot].m_Tex);

	if(pCommand->m_Flags&CCommandBuffer::TEXFLAG_NOMIPMAPS)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, StoreOglformat, Width, Height, 0, Oglformat, GL_UNSIGNED_BYTE, pTexData);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, StoreOglformat, Width, Height, Oglformat, GL_UNSIGNED_BYTE, pTexData);
	}

	// calculate memory usage
	m_aTextures[pCommand->m_Slot].m_MemSize = Width*Height*pCommand->m_PixelSize;
	while(Width > 2 && Height > 2)
	{
		Width>>=1;
		Height>>=1;
		m_aTextures[pCommand->m_Slot].m_MemSize += Width*Height*pCommand->m_PixelSize;
	}
	*m_pTextureMemoryUsage += m_aTextures[pCommand->m_Slot].m_MemSize;

	mem_free(pTexData);
}

void CCommandProcessorFragment_OpenGL::Cmd_Clear(const CCommandBuffer::SCommand_Clear *pCommand)
{
	glClearColor(pCommand->m_Color.r, pCommand->m_Color.g, pCommand->m_Color.b, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CCommandProcessorFragment_OpenGL::Cmd_Render(const CCommandBuffer::SCommand_Render *pCommand)
{
	SetState(pCommand->m_State);
	
	glVertexPointer(3, GL_FLOAT, sizeof(CCommandBuffer::SVertex), (char*)pCommand->m_pVertices);
	glTexCoordPointer(2, GL_FLOAT, sizeof(CCommandBuffer::SVertex), (char*)pCommand->m_pVertices + sizeof(float)*3);
	glColorPointer(4, GL_FLOAT, sizeof(CCommandBuffer::SVertex), (char*)pCommand->m_pVertices + sizeof(float)*5);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	switch(pCommand->m_PrimType)
	{
	case CCommandBuffer::PRIMTYPE_QUADS:
		glDrawArrays(GL_QUADS, 0, pCommand->m_PrimCount*4);
		break;
	case CCommandBuffer::PRIMTYPE_LINES:
		glDrawArrays(GL_LINES, 0, pCommand->m_PrimCount*2);
		break;
	default:
		dbg_msg("render", "unknown primtype %d\n", pCommand->m_Cmd);
	};
}

void CCommandProcessorFragment_OpenGL::Cmd_Screenshot(const CCommandBuffer::SCommand_Screenshot *pCommand)
{
	// fetch image data
	GLint aViewport[4] = {0,0,0,0};
	glGetIntegerv(GL_VIEWPORT, aViewport);

	int w = aViewport[2];
	int h = aViewport[3];

	// we allocate one more row to use when we are flipping the texture
	unsigned char *pPixelData = (unsigned char *)mem_alloc(w*(h+1)*3, 1);
	unsigned char *pTempRow = pPixelData+w*h*3;

	// fetch the pixels
	GLint Alignment;
	glGetIntegerv(GL_PACK_ALIGNMENT, &Alignment);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0,0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);
	glPixelStorei(GL_PACK_ALIGNMENT, Alignment);

	// flip the pixel because opengl works from bottom left corner
	for(int y = 0; y < h/2; y++)
	{
		mem_copy(pTempRow, pPixelData+y*w*3, w*3);
		mem_copy(pPixelData+y*w*3, pPixelData+(h-y-1)*w*3, w*3);
		mem_copy(pPixelData+(h-y-1)*w*3, pTempRow,w*3);
	}

	// fill in the information
	pCommand->m_pImage->m_Width = w;
	pCommand->m_pImage->m_Height = h;
	pCommand->m_pImage->m_Format = CImageInfo::FORMAT_RGB;
	pCommand->m_pImage->m_pData = pPixelData;
}

CCommandProcessorFragment_OpenGL::CCommandProcessorFragment_OpenGL()
{
	mem_zero(m_aTextures, sizeof(m_aTextures));
	m_pTextureMemoryUsage = 0;
}

bool CCommandProcessorFragment_OpenGL::RunCommand(const CCommandBuffer::SCommand * pBaseCommand)
{
	switch(pBaseCommand->m_Cmd)
	{
	case CMD_INIT: Cmd_Init(static_cast<const SCommand_Init *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_TEXTURE_CREATE: Cmd_Texture_Create(static_cast<const CCommandBuffer::SCommand_Texture_Create *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_TEXTURE_DESTROY: Cmd_Texture_Destroy(static_cast<const CCommandBuffer::SCommand_Texture_Destroy *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_TEXTURE_UPDATE: Cmd_Texture_Update(static_cast<const CCommandBuffer::SCommand_Texture_Update *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_CLEAR: Cmd_Clear(static_cast<const CCommandBuffer::SCommand_Clear *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_RENDER: Cmd_Render(static_cast<const CCommandBuffer::SCommand_Render *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_SCREENSHOT: Cmd_Screenshot(static_cast<const CCommandBuffer::SCommand_Screenshot *>(pBaseCommand)); break;
	default: return false;
	}

	return true;
}


// ------------ CCommandProcessorFragment_SDL

void CCommandProcessorFragment_SDL::Cmd_Init(const SCommand_Init *pCommand)
{
	m_GLContext = pCommand->m_GLContext;
	m_pWindow = pCommand->m_pWindow;
	SDL_GL_MakeCurrent(m_pWindow, m_GLContext);

	// set some default settings
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);
	glDepthMask(0);
}

void CCommandProcessorFragment_SDL::Cmd_Shutdown(const SCommand_Shutdown *pCommand)
{
	// Release the context from this thread
	SDL_GL_MakeCurrent(NULL, NULL);
}

void CCommandProcessorFragment_SDL::Cmd_Swap(const CCommandBuffer::SCommand_Swap *pCommand)
{
	SDL_GL_SwapWindow(m_pWindow);

	if(pCommand->m_Finish)
		glFinish();
}

void CCommandProcessorFragment_SDL::Cmd_VideoModes(const CCommandBuffer::SCommand_VideoModes *pCommand)
{
	SDL_DisplayMode mode;
	int maxModes = SDL_GetNumDisplayModes(pCommand->m_Screen),
		numModes = 0;
	for(int i = 0; i < maxModes; i++)
	{
		if(SDL_GetDisplayMode(pCommand->m_Screen, i, &mode) < 0)
		{
			dbg_msg("gfx", "unable to get display mode: %s", SDL_GetError());
			continue;
		}
		bool Skip = false;
		for(int j = 0; j < numModes; j++)
		{
			if(pCommand->m_pModes[j].m_Width == mode.w && pCommand->m_pModes[j].m_Height == mode.h)
			{
				Skip = true; break;
			}
		}
		if(Skip)
			continue;

		pCommand->m_pModes[numModes].m_Width = mode.w;
		pCommand->m_pModes[numModes].m_Height = mode.h;
		pCommand->m_pModes[numModes].m_Red = 8;
		pCommand->m_pModes[numModes].m_Green = 8;
		pCommand->m_pModes[numModes].m_Blue = 8;
		numModes++;

	}
	*pCommand->m_pNumModes = numModes;
}

CCommandProcessorFragment_SDL::CCommandProcessorFragment_SDL()
{
}

bool CCommandProcessorFragment_SDL::RunCommand(const CCommandBuffer::SCommand *pBaseCommand)
{
	switch(pBaseCommand->m_Cmd)
	{
	case CCommandBuffer::CMD_SWAP: Cmd_Swap(static_cast<const CCommandBuffer::SCommand_Swap *>(pBaseCommand)); break;
	case CCommandBuffer::CMD_VIDEOMODES: Cmd_VideoModes(static_cast<const CCommandBuffer::SCommand_VideoModes *>(pBaseCommand)); break;
	case CMD_INIT: Cmd_Init(static_cast<const SCommand_Init *>(pBaseCommand)); break;
	case CMD_SHUTDOWN: Cmd_Shutdown(static_cast<const SCommand_Shutdown *>(pBaseCommand)); break;
	default: return false;
	}

	return true;
}

// ------------ CCommandProcessor_SDL_OpenGL

void CCommandProcessor_SDL_OpenGL::RunBuffer(CCommandBuffer *pBuffer)
{
	unsigned CmdIndex = 0;
	while(1)
	{
		const CCommandBuffer::SCommand *pBaseCommand = pBuffer->GetCommand(&CmdIndex);
		if(pBaseCommand == 0x0)
			break;
		
		if(m_OpenGL.RunCommand(pBaseCommand))
			continue;
		
		if(m_SDL.RunCommand(pBaseCommand))
			continue;

		if(m_General.RunCommand(pBaseCommand))
			continue;
		
		dbg_msg("graphics", "unknown command %d", pBaseCommand->m_Cmd);
	}
}

// ------------ CGraphicsBackend_SDL_OpenGL

int CGraphicsBackend_SDL_OpenGL::Init(const char *pName, int *Width, int *Height, int Screen, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO))
	{
		if(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			dbg_msg("gfx", "unable to init SDL video: %s", SDL_GetError());
			return -1;
		}

		#ifdef CONF_FAMILY_WINDOWS
			if(!getenv("SDL_VIDEO_WINDOW_POS") && !getenv("SDL_VIDEO_CENTERED")) // ignore_convention
				putenv("SDL_VIDEO_WINDOW_POS=8,27"); // ignore_convention
		#endif
	}

	SDL_Rect ScreenBounds;
	if(SDL_GetDisplayBounds(Screen, &ScreenBounds) < 0)
	{
		dbg_msg("gfx", "unable to get current screen bounds: %s", SDL_GetError());
		return -1;
	}

	if(*Width == 0 || *Height == 0)
	{
		*Width = ScreenBounds.w;
		*Height = ScreenBounds.h; 
	}

	*pDesktopWidth = ScreenBounds.w;
	*pDesktopHeight = ScreenBounds.h;

	if(FsaaSamples)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, FsaaSamples);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	// set flags
	int SdlFlags = SDL_WINDOW_OPENGL;
	if(Flags&IGraphicsBackend::INITFLAG_RESIZABLE)
		SdlFlags |= SDL_WINDOW_RESIZABLE;
	if(Flags&IGraphicsBackend::INITFLAG_BORDERLESS)
		SdlFlags |= SDL_WINDOW_BORDERLESS;
	if(Flags&IGraphicsBackend::INITFLAG_FULLSCREEN)
		SdlFlags |= SDL_WINDOW_FULLSCREEN;

	dbg_assert(!(Flags&IGraphicsBackend::INITFLAG_BORDERLESS)
		|| !(Flags&IGraphicsBackend::INITFLAG_FULLSCREEN),
		"only one of borderless and fullscreen may be activated at the same time");

	// CreateWindow apparently doesn't care about the window position in fullscreen
	m_pWindow = SDL_CreateWindow(
		pName,
		SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),
		SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),
		*Width,
		*Height,
		SdlFlags
	);

	if(m_pWindow == NULL)
	{
		dbg_msg("gfx", "unable to create window: %s", SDL_GetError());
		return -1;
	}

#if 0
	int RenderFlags = SDL_RENDERER_ACCELERATED;
	if(Flags&IGraphicsBackend::INITFLAG_VSYNC)
		RenderFlags |= SDL_RENDERER_PRESENTVSYNC;
#endif

	m_GLContext = SDL_GL_CreateContext(m_pWindow);

	if(m_GLContext == NULL)
	{
		dbg_msg("gfx", "unable to create renderer: %s", SDL_GetError());
		return -1;
	}

	// release the current GL context from this thread
	SDL_GL_MakeCurrent(NULL, NULL);

	// start the command processor
	m_pProcessor = new CCommandProcessor_SDL_OpenGL;
	StartProcessor(m_pProcessor);

	// issue init commands for OpenGL and SDL
	CCommandBuffer CmdBuffer(1024, 512);
	CCommandProcessorFragment_OpenGL::SCommand_Init CmdOpenGL;
	CmdOpenGL.m_pTextureMemoryUsage = &m_TextureMemoryUsage;
	CmdBuffer.AddCommand(CmdOpenGL);
	CCommandProcessorFragment_SDL::SCommand_Init CmdSDL;
	CmdSDL.m_GLContext = m_GLContext;
	CmdSDL.m_pWindow = m_pWindow;
	CmdBuffer.AddCommand(CmdSDL);
	RunBuffer(&CmdBuffer);
	WaitForIdle();

	return 0;
}

int CGraphicsBackend_SDL_OpenGL::Shutdown()
{
	// issue a shutdown command
	CCommandBuffer CmdBuffer(1024, 512);
	CCommandProcessorFragment_SDL::SCommand_Shutdown Cmd;
	CmdBuffer.AddCommand(Cmd);
	RunBuffer(&CmdBuffer);
	WaitForIdle();
			
	// stop and delete the processor
	StopProcessor();
	delete m_pProcessor;
	m_pProcessor = 0;

	SDL_GL_DeleteContext(m_GLContext);
	SDL_DestroyWindow(m_pWindow);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	return 0;
}

int CGraphicsBackend_SDL_OpenGL::MemoryUsage() const
{
	return m_TextureMemoryUsage;
}

void CGraphicsBackend_SDL_OpenGL::Minimize()
{
	SDL_MinimizeWindow(m_pWindow);
}

void CGraphicsBackend_SDL_OpenGL::Maximize()
{
	SDL_MaximizeWindow(m_pWindow);
}

void CGraphicsBackend_SDL_OpenGL::GrabWindow(bool grab)
{
	SDL_SetWindowGrab(m_pWindow, grab ? SDL_TRUE : SDL_FALSE);
}

void CGraphicsBackend_SDL_OpenGL::WarpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(m_pWindow, x, y);
}

int CGraphicsBackend_SDL_OpenGL::WindowActive()
{
	return SDL_GetWindowFlags(m_pWindow)&SDL_WINDOW_INPUT_FOCUS;
}

int CGraphicsBackend_SDL_OpenGL::WindowOpen()
{
	return SDL_GetWindowFlags(m_pWindow)&SDL_WINDOW_SHOWN;
}

int CGraphicsBackend_SDL_OpenGL::GetNumScreens()
{
	int num = SDL_GetNumVideoDisplays();
	if(num < 1)
		num = 1;
	return num;
}


IGraphicsBackend *CreateGraphicsBackend() { return new CGraphicsBackend_SDL_OpenGL; }
