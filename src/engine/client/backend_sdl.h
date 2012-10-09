#pragma once

#include "graphics_threaded.h"


// platform dependent implementations for transfering render context from the main thread to the graphics thread
// TODO: when SDL 1.3 comes, this can be removed
#if defined(CONF_FAMILY_WINDOWS)
	struct SGLContext
	{
		HDC m_hDC;
		HGLRC m_hGLRC;
	};

	static SGLContext GL_GetCurrentContext()
	{
		SGLContext Context;
		Context.m_hDC = wglGetCurrentDC();
		Context.m_hGLRC = wglGetCurrentContext();
		return Context;
	}

	static void GL_MakeCurrent(const SGLContext &Context) { wglMakeCurrent(Context.m_hDC, Context.m_hGLRC); }
	static void GL_ReleaseContext(const SGLContext &Context) { wglMakeCurrent(NULL, NULL); }
	static void GL_SwapBuffers(const SGLContext &Context) { SwapBuffers(Context.m_hDC); }
#elif defined(CONF_PLATFORM_MACOSX)

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

	struct SGLContext
	{
		id m_Context;
	};

	static SGLContext GL_GetCurrentContext()
	{
		SGLContext Context;
		Class NSOpenGLContextClass = (Class) objc_getClass("NSOpenGLContext");
		SEL selector = sel_registerName("currentContext");
		Context.m_Context = objc_msgSend((objc_object*) NSOpenGLContextClass, selector);
		return Context;
	}

	static void GL_MakeCurrent(const SGLContext &Context)
	{
		SEL selector = sel_registerName("makeCurrentContext");
		objc_msgSend(Context.m_Context, selector);
	}

	static void GL_ReleaseContext(const SGLContext &Context)
	{
		Class NSOpenGLContextClass = (Class) objc_getClass("NSOpenGLContext");
		SEL selector = sel_registerName("clearCurrentContext");
		objc_msgSend((objc_object*) NSOpenGLContextClass, selector);
	}

	static void GL_SwapBuffers(const SGLContext &Context)
	{
		SEL selector = sel_registerName("flushBuffer");
		objc_msgSend(Context.m_Context, selector);
	}

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

#elif defined(CONF_FAMILY_UNIX)

	#include <GL/glx.h>

	struct SGLContext
	{
		Display *m_pDisplay;
		GLXDrawable m_Drawable;
		GLXContext m_Context;
	};

	static SGLContext GL_GetCurrentContext()
	{
		SGLContext Context;
		Context.m_pDisplay = glXGetCurrentDisplay();
		Context.m_Drawable = glXGetCurrentDrawable();
		Context.m_Context = glXGetCurrentContext();
		return Context;
	}

	static void GL_MakeCurrent(const SGLContext &Context) { glXMakeCurrent(Context.m_pDisplay, Context.m_Drawable, Context.m_Context); }
	static void GL_ReleaseContext(const SGLContext &Context) { glXMakeCurrent(Context.m_pDisplay, None, 0x0); }
	static void GL_SwapBuffers(const SGLContext &Context) { glXSwapBuffers(Context.m_pDisplay, Context.m_Drawable); }
#else
	#error missing implementation
#endif


// basic threaded backend, abstract, missing init and shutdown functions
class CGraphicsBackend_Threaded : public IGraphicsBackend
{
public:
	// constructed on the main thread, the rest of the functions is runned on the render thread
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
	SGLContext m_GLContext;
public:
	enum
	{
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM_SDL,
		CMD_SHUTDOWN,
	};

	struct SCommand_Init : public CCommandBuffer::SCommand
	{
		SCommand_Init() : SCommand(CMD_INIT) {}
		SGLContext m_Context;
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
	SDL_Surface *m_pScreenSurface;
	ICommandProcessor *m_pProcessor;
	SGLContext m_GLContext;
	volatile int m_TextureMemoryUsage;
public:
	virtual int Init(const char *pName, int *Width, int *Height, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight);
	virtual int Shutdown();

	virtual int MemoryUsage() const;

	virtual void Minimize();
	virtual void Maximize();
	virtual int WindowActive();
	virtual int WindowOpen();
};
