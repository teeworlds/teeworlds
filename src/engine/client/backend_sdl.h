#pragma once

#include "graphics_threaded.h"
#include "SDL.h"
#include "SDL_opengl.h"


// Use SDL semaphores on Mac OS X, because (unnamed) posix semaphores are not available
#if defined(CONF_PLATFORM_MACOSX)
	class semaphore
	{
		SDL_sem *sem;
	public:
		semaphore() { sem = SDL_CreateSemaphore(0); }
		~semaphore() { SDL_DestroySemaphore(sem); }
		void wait() { SDL_SemWait(sem); }
		void signal() { SDL_SemPost(sem); }
	};

	#include <objc/objc-runtime.h>
	
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
			objc_msgSend(m_Pool, selector);
		}

		~CAutoreleasePool()
		{
			SEL selector = sel_registerName("drain");
			objc_msgSend(m_Pool, selector);
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
	void Cmd_Signal(const CCommandBuffer::SCommand_Signal *pCommand);
public:
	bool RunCommand(const CCommandBuffer::SCommand * pBaseCommand);
};

// takes care of opengl related rendering
class CCommandProcessorFragment_OpenGL
{
	struct CTexture
	{
		GLuint m_Tex;
		int m_MemSize;
	};
	CTexture m_aTextures[CCommandBuffer::MAX_TEXTURES];
	volatile int *m_pTextureMemoryUsage;

public:
	enum
	{
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM_OPENGL,
	};

	struct SCommand_Init : public CCommandBuffer::SCommand
	{
		SCommand_Init() : SCommand(CMD_INIT) {}
		volatile int *m_pTextureMemoryUsage;
	};

private:
	static int TexFormatToOpenGLFormat(int TexFormat);
	static unsigned char Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp);
	static void *Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData);

	void SetState(const CCommandBuffer::SState &State);

	void Cmd_Init(const SCommand_Init *pCommand);
	void Cmd_Texture_Update(const CCommandBuffer::SCommand_Texture_Update *pCommand);
	void Cmd_Texture_Destroy(const CCommandBuffer::SCommand_Texture_Destroy *pCommand);
	void Cmd_Texture_Create(const CCommandBuffer::SCommand_Texture_Create *pCommand);
	void Cmd_Clear(const CCommandBuffer::SCommand_Clear *pCommand);
	void Cmd_Render(const CCommandBuffer::SCommand_Render *pCommand);
	void Cmd_Screenshot(const CCommandBuffer::SCommand_Screenshot *pCommand);

public:
	CCommandProcessorFragment_OpenGL();

	bool RunCommand(const CCommandBuffer::SCommand * pBaseCommand);
};

// takes care of sdl related commands
class CCommandProcessorFragment_SDL
{
	// SDL stuff
	SDL_GLContext m_GLContext;
	SDL_Window * m_pWindow;
public:
	enum
	{
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM_SDL,
		CMD_SHUTDOWN,
	};

	struct SCommand_Init : public CCommandBuffer::SCommand
	{
		SCommand_Init() : SCommand(CMD_INIT) {}
		SDL_GLContext m_GLContext;
		SDL_Window * m_pWindow;
	};

	struct SCommand_Shutdown : public CCommandBuffer::SCommand
	{
		SCommand_Shutdown() : SCommand(CMD_SHUTDOWN) {}
	};

private:
	void Cmd_Init(const SCommand_Init *pCommand);
	void Cmd_Shutdown(const SCommand_Shutdown *pCommand);
	void Cmd_Swap(const CCommandBuffer::SCommand_Swap *pCommand);
	void Cmd_VideoModes(const CCommandBuffer::SCommand_VideoModes *pCommand);
public:
	CCommandProcessorFragment_SDL();

	bool RunCommand(const CCommandBuffer::SCommand *pBaseCommand);
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
public:
	virtual int Init(const char *pName, int *Width, int *Height, int Screen, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight);
	virtual int Shutdown();

	virtual int MemoryUsage() const;

	virtual void Minimize();
	virtual void Maximize();
	virtual void GrabWindow(bool);
	virtual void WarpMouse(int x, int y);
	virtual int WindowActive();
	virtual int WindowOpen();
	virtual int GetNumScreens();
};
