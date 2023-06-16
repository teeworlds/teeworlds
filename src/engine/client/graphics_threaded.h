/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_GRAPHICS_THREADED_H
#define ENGINE_CLIENT_GRAPHICS_THREADED_H

#include <stdint.h>

#include <pnglite.h>

#include <engine/graphics.h>

class CCommandBuffer
{
	class CBuffer
	{
		unsigned char *m_pData;
		unsigned m_Size;
		unsigned m_Used;
	public:
		CBuffer(unsigned BufferSize)
		{
			m_Size = BufferSize;
			m_pData = new unsigned char[m_Size];
			m_Used = 0;
		}

		~CBuffer()
		{
			delete [] m_pData;
			m_pData = 0x0;
			m_Used = 0;
			m_Size = 0;
		}

		void Reset()
		{
			m_Used = 0;
		}

		void *Alloc(unsigned Requested, unsigned Alignment = 8) // TODO: use alignof(std::max_align_t)
		{
			size_t Offset = Alignment - (reinterpret_cast<uintptr_t>(m_pData + m_Used) % Alignment);
			if(Requested + Offset + m_Used > m_Size)
				return 0;

			void *pPtr = &m_pData[m_Used + Offset];
			m_Used += Requested + Offset;
			return pPtr;
		}

		unsigned char *DataPtr() { return m_pData; }
		unsigned DataSize() const { return m_Size; }
		unsigned DataUsed() const { return m_Used; }
	};

public:
	CBuffer m_CmdBuffer;
	CBuffer m_DataBuffer;

	enum
	{
		MAX_TEXTURES=1024*4,
	};

	enum
	{
		// commadn groups
		CMDGROUP_CORE = 0, // commands that everyone has to implement
		CMDGROUP_PLATFORM_OPENGL = 10000, // commands specific to a platform
		CMDGROUP_PLATFORM_SDL = 20000,

		//
		CMD_NOP = CMDGROUP_CORE,

		//
		CMD_RUNBUFFER,

		// syncronization
		CMD_SIGNAL,

		// texture commands
		CMD_TEXTURE_CREATE,
		CMD_TEXTURE_DESTROY,
		CMD_TEXTURE_UPDATE,

		// rendering
		CMD_CLEAR,
		CMD_RENDER,

		// swap
		CMD_SWAP,

		// misc
		CMD_VSYNC,
		CMD_SCREENSHOT,

	};

	enum
	{
		TEXFORMAT_INVALID = 0,
		TEXFORMAT_RGB,
		TEXFORMAT_RGBA,
		TEXFORMAT_ALPHA,

		TEXFLAG_NOMIPMAPS = 1,
		TEXFLAG_COMPRESSED = 2,
		TEXFLAG_QUALITY = 4,
		TEXFLAG_TEXTURE3D = 8,
		TEXFLAG_TEXTURE2D = 16,
		TEXTFLAG_LINEARMIPMAPS = 32,
	};

	enum
	{
		//
		PRIMTYPE_INVALID = 0,
		PRIMTYPE_LINES,
		PRIMTYPE_QUADS,
	};

	enum
	{
		BLEND_NONE = 0,
		BLEND_ALPHA,
		BLEND_ADDITIVE,
	};

	struct CPoint { float x, y; };
	struct CTexCoord { float u, v, i; };
	struct CColor { float r, g, b, a; };

	struct CVertex
	{
		CPoint m_Pos;
		CTexCoord m_Tex;
		CColor m_Color;
	};

	struct CCommand
	{
	public:
		CCommand(unsigned Cmd) :
			m_Cmd(Cmd), m_pNext(0) {}
		unsigned m_Cmd;
		CCommand *m_pNext;
	};
	CCommand *m_pCmdBufferHead;
	CCommand *m_pCmdBufferTail;

	struct CState
	{
		int m_BlendMode;
		int m_WrapModeU;
		int m_WrapModeV;
		int m_Texture;
		int m_TextureArrayIndex;
		int m_Dimension;
		CPoint m_ScreenTL;
		CPoint m_ScreenBR;

		// clip
		bool m_ClipEnable;
		int m_ClipX;
		int m_ClipY;
		int m_ClipW;
		int m_ClipH;
	};

	struct CClearCommand : public CCommand
	{
		CClearCommand() : CCommand(CMD_CLEAR) {}
		CColor m_Color;
	};

	struct CSignalCommand : public CCommand
	{
		CSignalCommand() : CCommand(CMD_SIGNAL) {}
		semaphore *m_pSemaphore;
	};

	struct CRunBufferCommand : public CCommand
	{
		CRunBufferCommand() : CCommand(CMD_RUNBUFFER) {}
		CCommandBuffer *m_pOtherBuffer;
	};

	struct CRenderCommand : public CCommand
	{
		CRenderCommand() : CCommand(CMD_RENDER) {}
		CState m_State;
		unsigned m_PrimType;
		unsigned m_PrimCount;
		CVertex *m_pVertices; // you should use the command buffer data to allocate vertices for this command
	};

	struct CScreenshotCommand : public CCommand
	{
		CScreenshotCommand() : CCommand(CMD_SCREENSHOT) {}
		int m_X, m_Y, m_W, m_H; // specify rectangle size, -1 if fullscreen (width/height)
		CImageInfo *m_pImage; // processor will fill this out, the one who adds this command must free the data as well
	};

	struct CSwapCommand : public CCommand
	{
		CSwapCommand() : CCommand(CMD_SWAP) {}

		int m_Finish;
	};

	struct CVSyncCommand : public CCommand
	{
		CVSyncCommand() : CCommand(CMD_VSYNC) {}

		int m_VSync;
		bool *m_pRetOk;
	};

	struct CTextureCreateCommand : public CCommand
	{
		CTextureCreateCommand() : CCommand(CMD_TEXTURE_CREATE) {}

		// texture information
		int m_Slot;

		int m_Width;
		int m_Height;
		int m_PixelSize;
		int m_Format;
		int m_StoreFormat;
		int m_Flags;
		void *m_pData; // will be freed by the command processor
	};

	struct CTextureUpdateCommand : public CCommand
	{
		CTextureUpdateCommand() : CCommand(CMD_TEXTURE_UPDATE) {}

		// texture information
		int m_Slot;

		int m_X;
		int m_Y;
		int m_Width;
		int m_Height;
		int m_Format;
		void *m_pData; // will be freed by the command processor
	};


	struct CTextureDestroyCommand : public CCommand
	{
		CTextureDestroyCommand() : CCommand(CMD_TEXTURE_DESTROY) {}

		// texture information
		int m_Slot;
	};

	//
	CCommandBuffer(unsigned CmdBufferSize, unsigned DataBufferSize) :
		m_CmdBuffer(CmdBufferSize), m_DataBuffer(DataBufferSize), m_pCmdBufferHead(0), m_pCmdBufferTail(0)
	{
	}

	void *AllocData(unsigned WantedSize)
	{
		return m_DataBuffer.Alloc(WantedSize);
	}

	template<class T>
	bool AddCommand(const T &Command)
	{
		// make sure that we don't do something stupid like ->AddCommand(&Cmd);
		(void)static_cast<const CCommand *>(&Command);

		// allocate and copy the command into the buffer
		T *pCmd = (T *)m_CmdBuffer.Alloc(sizeof(*pCmd), 8); // TODO: use alignof(T)
		if(!pCmd)
			return false;
		*pCmd = Command;
		pCmd->m_pNext = 0;

		if(m_pCmdBufferTail)
			m_pCmdBufferTail->m_pNext = pCmd;
		if(!m_pCmdBufferHead)
			m_pCmdBufferHead = pCmd;
		m_pCmdBufferTail = pCmd;

		return true;
	}

	CCommand *Head()
	{
		return m_pCmdBufferHead;
	}

	void Reset()
	{
		m_pCmdBufferHead = m_pCmdBufferTail = 0;
		m_CmdBuffer.Reset();
		m_DataBuffer.Reset();
	}
};

// interface for the graphics backend
// all these functions are called on the main thread
class IGraphicsBackend
{
public:
	enum
	{
		INITFLAG_FULLSCREEN = 1,
		INITFLAG_VSYNC = 2,
		INITFLAG_RESIZABLE = 4,
		INITFLAG_BORDERLESS = 8,
		INITFLAG_X11XRANDR = 16,
		INITFLAG_HIGHDPI = 32,
	};

	virtual ~IGraphicsBackend() {}

	virtual int Init(const char *pName, int *Screen, int *pWindowWidth, int *pWindowHeight, int *pScreenWidth, int *pScreenHeight, int FsaaSamples, int Flags, int *pDesktopWidth, int *pDesktopHeight) = 0;
	virtual int Shutdown() = 0;

	virtual int MemoryUsage() const = 0;
	virtual int GetTextureArraySize() const = 0;

	virtual int GetNumScreens() const = 0;

	virtual void Minimize() = 0;
	virtual void Maximize() = 0;
	virtual bool Fullscreen(bool State) = 0;
	virtual void SetWindowBordered(bool State) = 0;
	virtual bool SetWindowScreen(int Index) = 0;
	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen) = 0;
	virtual bool GetDesktopResolution(int Index, int *pDesktopWidth, int* pDesktopHeight) = 0;
	virtual int GetWindowScreen() = 0;
	virtual int WindowActive() = 0;
	virtual int WindowOpen() = 0;

	virtual void RunBuffer(CCommandBuffer *pBuffer) = 0;
	virtual bool IsIdle() const = 0;
	virtual void WaitForIdle() = 0;
};

class CGraphics_Threaded : public IEngineGraphics
{
	enum
	{
		NUM_CMDBUFFERS = 2,

		MAX_VERTICES = 32*1024,
		MAX_TEXTURES = 1024*4,

		DRAWING_QUADS=1,
		DRAWING_LINES=2
	};

	CCommandBuffer::CState m_State;
	IGraphicsBackend *m_pBackend;

	CCommandBuffer *m_apCommandBuffers[NUM_CMDBUFFERS];
	CCommandBuffer *m_pCommandBuffer;
	unsigned m_CurrentCommandBuffer;

	//
	class IStorage *m_pStorage;
	class CConfig *m_pConfig;
	class IConsole *m_pConsole;

	CCommandBuffer::CVertex m_aVertices[MAX_VERTICES];
	int m_NumVertices;

	CCommandBuffer::CColor m_aColor[4];
	CCommandBuffer::CTexCoord m_aTexture[4];

	bool m_RenderEnable;

	float m_Rotation;
	int m_Drawing;
	bool m_DoScreenshot;
	char m_aScreenshotName[128];

	CTextureHandle m_InvalidTexture;

	int m_TextureArrayIndex;
	int m_aTextureIndices[MAX_TEXTURES];
	int m_FirstFreeTexture;
	int m_TextureMemoryUsage;

	void FlushVertices();
	void AddVertices(int Count);
	void Rotate4(const CCommandBuffer::CPoint &rCenter, CCommandBuffer::CVertex *pPoints);

	void KickCommandBuffer();

	bool LoadPNGImpl(CImageInfo *pImg, png_read_callback_t pfnPngReadFunc, void *pPngReadUserData, const char *pFilename);

	int IssueInit();
	int InitWindow();
public:
	CGraphics_Threaded();

	virtual void ClipEnable(int x, int y, int w, int h);
	virtual void ClipDisable();

	virtual void BlendNone();
	virtual void BlendNormal();
	virtual void BlendAdditive();

	virtual void WrapNormal();
	virtual void WrapClamp();
	virtual void WrapMode(int WrapU, int WrapV);

	virtual int MemoryUsage() const;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY);
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY);

	virtual void LinesBegin();
	virtual void LinesEnd();
	virtual void LinesDraw(const CLineItem *pArray, int Num);

	virtual int UnloadTexture(IGraphics::CTextureHandle *pIndex);
	virtual IGraphics::CTextureHandle LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags);
	virtual int LoadTextureRawSub(IGraphics::CTextureHandle TextureID, int x, int y, int Width, int Height, int Format, const void *pData);

	// simple uncompressed RGBA loaders
	virtual IGraphics::CTextureHandle LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags);
	virtual bool LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType);
	virtual bool LoadPNG(CImageInfo *pImg, const unsigned char *pData, unsigned Size, const char *pContext);

	void ScreenshotDirect(const char *pFilename);

	virtual void TextureSet(CTextureHandle TextureID);

	virtual void Clear(float r, float g, float b);

	virtual void QuadsBegin();
	virtual void QuadsEnd();
	virtual void QuadsSetRotation(float Angle);

	virtual void SetColorVertex(const CColorVertex *pArray, int Num);
	virtual void SetColor(float r, float g, float b, float a);
	virtual void SetColor4(const vec4 &TopLeft, const vec4 &TopRight, const vec4 &BottomLeft, const vec4 &BottomRight);

	void TilesetFallbackSystem(int TextureIndex);
	virtual void QuadsSetSubset(float TlU, float TlV, float BrU, float BrV, int TextureIndex = -1);
	virtual void QuadsSetSubsetFree(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3, int TextureIndex = -1);

	virtual void QuadsDraw(CQuadItem *pArray, int Num);
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num);
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num);
	virtual void QuadsText(float x, float y, float Size, const char *pText);

	virtual int GetNumScreens() const;
	virtual void Minimize();
	virtual void Maximize();
	virtual bool Fullscreen(bool State);
	virtual void SetWindowBordered(bool State);
	virtual bool SetWindowScreen(int Index);
	virtual int GetWindowScreen();

	virtual int WindowActive();
	virtual int WindowOpen();

	virtual int Init();
	virtual void Shutdown();

	virtual void ReadBackbuffer(unsigned char **ppPixels, int x, int y, int w, int h);
	virtual void TakeScreenshot(const char *pFilename);
	virtual void Swap();
	virtual bool SetVSync(bool State);

	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen);

	// syncronization
	virtual void InsertSignal(semaphore *pSemaphore);
	virtual bool IsIdle() const;
	virtual void WaitForIdle();
};

extern IGraphicsBackend *CreateGraphicsBackend();

#endif // ENGINE_CLIENT_GRAPHICS_THREADED_H
