#pragma once

#include "graphics_threaded.h"

#if defined(CONF_PLATFORM_MACOSX)
	#include <objc/objc-runtime.h>

	class semaphore
	{
		SDL_sem *sem;
	public:
		semaphore() { sem = SDL_CreateSemaphore(0); }
		~semaphore() { SDL_DestroySemaphore(sem); }
		void wait() { SDL_SemWait(sem); }
		void signal() { SDL_SemPost(sem); }
	};

	class CAutoreleasePool
	{
	private:
		id m_Pool;

	public:
		CAutoreleasePool()
		{
			Class NSAutoreleasePoolClass = (Class) objc_getClass("NSAutoreleasePool");
			m_Pool = class_createInstance(NSAutoreleasePoolClass, 0);
			SEL selector = sel_registerName("init");
			((id (*)(id, SEL))objc_msgSend)(m_Pool, selector);
		}

		~CAutoreleasePool()
		{
			SEL selector = sel_registerName("drain");
			((id (*)(id, SEL))objc_msgSend)(m_Pool, selector);
		}
	};
#endif


// basic threaded backend, abstract, missing init and shutdown functions
class CGraphicsBackend_Threaded : public IGraphicsBackend
{
public:
	// constructed on the main thread, the rest of the functions is run on the render thread
	class ICommandProcessor
	{
	public:
		virtual ~ICommandProcessor() {}
		virtual void RunBuffer(CCommandBuffer *pBuffer) = 0;
	};

	CGraphicsBackend_Threaded();

	virtual void RunBuffer(CCommandBuffer *pBuffer);
	virtual bool IsIdle() const;
	virtual void WaitForIdle();

protected:
	void StartProcessor(ICommandProcessor *pProcessor);
	void StopProcessor();

private:
	ICommandProcessor *m_pProcessor;
	CCommandBuffer * volatile m_pBuffer;
	volatile bool m_Shutdown;
	semaphore m_Activity;
	semaphore m_BufferDone;
	void *m_pThread;

	static void ThreadFunc(void *pUser);
};

// takes care of implementation independent operations
class CCommandProcessorFragment_General
{
	void Cmd_Nop();
	void Cmd_Signal(const CCommandBuffer::CSignalCommand *pCommand);
public:
	bool RunCommand(const CCommandBuffer::CCommand * pBaseCommand);
};

// takes care of opengl related rendering
class CCommandProcessorFragment_OpenGL
{
	class CTexture
	{
	public:
		enum
		{
			STATE_EMPTY = 0,
			STATE_TEX2D = 1,
			STATE_TEX3D = 2,

			MIN_GL_MAX_3D_TEXTURE_SIZE = 64,																					// GL_MAX_3D_TEXTURE_SIZE must be at least 64 according to the standard
			MAX_ARRAYSIZE_TEX3D = IGraphics::NUMTILES_DIMENSION * IGraphics::NUMTILES_DIMENSION / MIN_GL_MAX_3D_TEXTURE_SIZE,	// = 4
		};
		GLuint m_Tex2D;
		GLuint m_Tex3D[MAX_ARRAYSIZE_TEX3D];
		int m_State;
		int m_Format;
		int m_MemSize;
	};
	CTexture m_aTextures[CCommandBuffer::MAX_TEXTURES];
	volatile int *m_pTextureMemoryUsage;
	int m_MaxTexSize;
	int m_Max3DTexSize;
	int m_TextureArraySize;

public:
	enum
	{
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM_OPENGL,
	};

	struct CInitCommand : public CCommandBuffer::CCommand
	{
		CInitCommand() : CCommand(CMD_INIT) {}
		volatile int *m_pTextureMemoryUsage;
		int *m_pTextureArraySize;
	};

private:
	static int TexFormatToOpenGLFormat(int TexFormat);
	static unsigned char Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp);
	static void *Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData);

	void SetState(const CCommandBuffer::CState &State);

	void Cmd_Init(const CInitCommand *pCommand);
	void Cmd_Texture_Update(const CCommandBuffer::CTextureUpdateCommand *pCommand);
	void Cmd_Texture_Destroy(const CCommandBuffer::CTextureDestroyCommand *pCommand);
	void Cmd_Texture_Create(const CCommandBuffer::CTextureCreateCommand *pCommand);
	void Cmd_Clear(const CCommandBuffer::CClearCommand *pCommand);
	void Cmd_Render(const CCommandBuffer::CRenderCommand *pCommand);
	void Cmd_Screenshot(const CCommandBuffer::CScreenshotCommand *pCommand);

public:
	CCommandProcessorFragment_OpenGL();

	bool RunCommand(const CCommandBuffer::CCommand * pBaseCommand);
};

// takes care of sdl related commands
class CCommandProcessorFragment_SDL
{
	// SDL stuff
	SDL_Window *m_pWindow;
	SDL_GLContext m_GLContext;
public:
	enum
	{
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM_SDL,
		CMD_SHUTDOWN,
	};

	struct CInitCommand : public CCommandBuffer::CCommand
	{
		CInitCommand() : CCommand(CMD_INIT) {}
		SDL_Window *m_pWindow;
		SDL_GLContext m_GLContext;
	};

	struct CShutdownCommand : public CCommandBuffer::CCommand
	{
		CShutdownCommand() : CCommand(CMD_SHUTDOWN) {}
	};

private:
	void Cmd_Init(const CInitCommand *pCommand);
	void Cmd_Shutdown(const CShutdownCommand *pCommand);
	void Cmd_Swap(const CCommandBuffer::CSwapCommand *pCommand);
	void Cmd_VSync(const CCommandBuffer::CVSyncCommand *pCommand);
	void Cmd_VideoModes(const CCommandBuffer::CVideoModesCommand *pCommand);
public:
	CCommandProcessorFragment_SDL();

	bool RunCommand(const CCommandBuffer::CCommand *pBaseCommand);
};

// command processor impelementation, uses the fragments to combine into one processor
class CCommandProcessor_SDL_OpenGL : public CGraphicsBackend_Threaded::ICommandProcessor
{
	CCommandProcessorFragment_OpenGL m_OpenGL;
	CCommandProcessorFragment_SDL m_SDL;
	CCommandProcessorFragment_General m_General;
 public:
	virtual void RunBuffer(CCommandBuffer *pBuffer);
};

// graphics backend implemented with SDL and OpenGL
class CGraphicsBackend_SDL_OpenGL : public CGraphicsBackend_Threaded
{
	SDL_Window *m_pWindow;
	SDL_GLContext m_GLContext;
	ICommandProcessor *m_pProcessor;
	volatile int m_TextureMemoryUsage;
	int m_NumScreens;
	int m_TextureArraySize;
public:
	virtual int Init(const char *pName, int *pScreen, int *pWindowWidth, int *pWindowHeight, int *pScreenWidth, int *pScreenHeight, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight);
	virtual int Shutdown();

	virtual int MemoryUsage() const;
	virtual int GetTextureArraySize() const { return m_TextureArraySize; }

	virtual int GetNumScreens() const { return m_NumScreens; }

	virtual void Minimize();
	virtual void Maximize();
	virtual bool Fullscreen(bool State);		// on=true/off=false
	virtual void SetWindowBordered(bool State);	// on=true/off=false
	virtual bool SetWindowScreen(int Index);
	virtual int GetWindowScreen();
	virtual bool GetDesktopResolution(int Index, int *pDesktopWidth, int* pDesktopHeight);
	virtual int WindowActive();
	virtual int WindowOpen();
};
