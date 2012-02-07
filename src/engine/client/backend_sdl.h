
#include "SDL.h"
#include "SDL_opengl.h"

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

	#include <AGL/agl.h>

	struct SGLContext
	{
		AGLContext m_Context;
	};

	static SGLContext GL_GetCurrentContext()
	{
		SGLContext Context;
		Context.m_Context = aglGetCurrentContext();
		return Context;
	}

	static void GL_MakeCurrent(const SGLContext &Context) { aglSetCurrentContext(Context.m_Context); }
	static void GL_ReleaseContext(const SGLContext &Context) { aglSetCurrentContext(NULL); }
	static void GL_SwapBuffers(const SGLContext &Context) { aglSwapBuffers(Context.m_Context); }
		
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
	GLuint m_aTextures[CCommandBuffer::MAX_TEXTURES];
	static int TexFormatToOpenGLFormat(int TexFormat);

	void SetState(const CCommandBuffer::SState &State);

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
		CMD_INIT = CCommandBuffer::CMDGROUP_PLATFORM,
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
public:
	virtual int Init(const char *pName, int Width, int Height, int FsaaSamples, int Flags);
	virtual int Shutdown();

	virtual void Minimize();
	virtual void Maximize();
	virtual int WindowActive();
	virtual int WindowOpen();
};
