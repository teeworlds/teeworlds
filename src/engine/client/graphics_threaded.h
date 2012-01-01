#pragma once

#include <base/tl/threading.h>

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

		void *Alloc(unsigned Requested)
		{
			if(Requested + m_Used > m_Size)
				return 0;
			void *pPtr = &m_pData[m_Used];
			m_Used += Requested;
			return pPtr;
		}

		unsigned char *DataPtr() { return m_pData; }
		unsigned DataSize() { return m_Size; }
		unsigned DataUsed() { return m_Used; }
	};

	CBuffer m_CmdBuffer;
	CBuffer m_DataBuffer;
public:
	enum
	{
		MAX_TEXTURES=1024*4,
	};

	enum
	{
		//
		CMD_NOP = 0,

		//
		CMD_INIT,
		CMD_SHUTDOWN,

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
		CMD_SCREENSHOT,
		CMD_VIDEOMODES,
	};

	enum
	{
		TEXFORMAT_INVALID = 0,
		TEXFORMAT_RGB,
		TEXFORMAT_RGBA,
		TEXFORMAT_ALPHA,

		TEXFLAG_NOMIPMAPS = 1,
	};

	enum
	{
		INITFLAG_FULLSCREEN = 1,
		INITFLAG_VSYNC = 2,
		INITFLAG_RESIZABLE = 4,
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

	struct SPoint { float x, y, z; };
	struct STexCoord { float u, v; };
	struct SColor { float r, g, b, a; };

	struct SVertex
	{
		SPoint m_Pos;
		STexCoord m_Tex;
		SColor m_Color;
	};

	struct SCommand
	{
	public:
		SCommand(unsigned Cmd) : m_Cmd(Cmd), m_Size(0) {}
		unsigned m_Cmd;
		unsigned m_Size;
	};

	struct SState
	{
		int m_BlendMode;
		int m_Texture;
		SPoint m_ScreenTL;
		SPoint m_ScreenBR;

		// clip
		bool m_ClipEnable;
		int m_ClipX;
		int m_ClipY;
		int m_ClipW;
		int m_ClipH;
	};
		
	struct SCommand_Clear : public SCommand
	{
		SCommand_Clear() : SCommand(CMD_CLEAR) {}
		SColor m_Color;
	};

	struct SCommand_Init : public SCommand
	{
		SCommand_Init() : SCommand(CMD_INIT) {}
		
		char m_aName[256];

		int m_ScreenWidth;
		int m_ScreenHeight;
		int m_FsaaSamples;
		int m_Flags;

		volatile int *m_pResult;
	};

	struct SCommand_Shutdown : public SCommand
	{
		SCommand_Shutdown() : SCommand(CMD_SHUTDOWN) {}
	};
		
	struct SCommand_Signal : public SCommand
	{
		SCommand_Signal() : SCommand(CMD_SIGNAL) {}
		semaphore *m_pSemaphore;
	};

	struct SCommand_RunBuffer : public SCommand
	{
		SCommand_RunBuffer() : SCommand(CMD_RUNBUFFER) {}
		CCommandBuffer *m_pOtherBuffer;
	};

	struct SCommand_Render : public SCommand
	{
		SCommand_Render() : SCommand(CMD_RENDER) {}
		SState m_State;
		unsigned m_PrimType;
		unsigned m_PrimCount;
		SVertex *m_pVertices; // you should use the command buffer data to allocate vertices for this command
	};

	struct SCommand_Screenshot : public SCommand
	{
		SCommand_Screenshot() : SCommand(CMD_SCREENSHOT) {}
		CImageInfo *m_pImage; // processor will fill this out, the one who adds this command must free the data as well
	};

	struct SCommand_VideoModes : public SCommand
	{
		SCommand_VideoModes() : SCommand(CMD_VIDEOMODES) {}

		CVideoMode *m_pModes; // processor will fill this in
		int m_MaxModes; // maximum of modes the processor can write to the m_pModes
		int *m_pNumModes; // processor will write to this pointer
	};

	struct SCommand_Swap : public SCommand
	{
		SCommand_Swap() : SCommand(CMD_SWAP) {}
	};

	struct SCommand_Texture_Create : public SCommand
	{
		SCommand_Texture_Create() : SCommand(CMD_TEXTURE_CREATE) {}

		// texture information
		int m_Slot;

		int m_Width;
		int m_Height;
		int m_Format;
		int m_StoreFormat;
		int m_Flags;
		void *m_pData; // will be freed by the command processor
	};

	struct SCommand_Texture_Update : public SCommand
	{
		SCommand_Texture_Update() : SCommand(CMD_TEXTURE_UPDATE) {}

		// texture information
		int m_Slot;

		int m_X;
		int m_Y;
		int m_Width;
		int m_Height;
		int m_Format;
		void *m_pData; // will be freed by the command processor
	};


	struct SCommand_Texture_Destroy : public SCommand
	{
		SCommand_Texture_Destroy() : SCommand(CMD_TEXTURE_DESTROY) {}

		// texture information
		int m_Slot;
	};
	
	//
	CCommandBuffer(unsigned CmdBufferSize, unsigned DataBufferSize)
	: m_CmdBuffer(CmdBufferSize), m_DataBuffer(DataBufferSize)
	{
	}

	void *AllocData(unsigned WantedSize)
	{
		return m_DataBuffer.Alloc(WantedSize);
	}

	template<class T>
	void AddCommand(const T &Command)
	{
		SCommand *pCmd = (SCommand *)m_CmdBuffer.Alloc(sizeof(Command));
		if(!pCmd)
			return;

		mem_copy(pCmd, &Command, sizeof(Command));
		pCmd->m_Size = sizeof(Command);
	}

	SCommand *GetCommand(unsigned *pIndex)
	{
		if(*pIndex >= m_CmdBuffer.DataUsed())
			return NULL;

		SCommand *pCommand = (SCommand *)&m_CmdBuffer.DataPtr()[*pIndex];
		*pIndex += pCommand->m_Size;
		return pCommand;
	}

	void Reset()
	{
		m_CmdBuffer.Reset();
		m_DataBuffer.Reset();
	}	
};

class ICommandProcessor
{
public:
	virtual ~ICommandProcessor() {}
	virtual void RunBuffer(CCommandBuffer *pBuffer) = 0;
};


class CCommandProcessorHandler
{
	ICommandProcessor *m_pProcessor;
	CCommandBuffer * volatile m_pBuffer;
	volatile bool m_Shutdown;
	semaphore m_Activity;
	semaphore m_BufferDone;
	void *m_pThread;

	static void ThreadFunc(void *pUser);

public:
	CCommandProcessorHandler();
	void Start(ICommandProcessor *pProcessor);
	void Stop();

	void RunBuffer(CCommandBuffer *pBuffer);
	bool IsIdle() const { return m_pBuffer == 0; }
	void WaitForIdle();
};

class CGraphics_Threaded : public IEngineGraphics
{
	CCommandBuffer::SState m_State;
	CCommandProcessorHandler m_Handler;
	ICommandProcessor *m_pProcessor;

	CCommandBuffer *m_apCommandBuffers[2];
	CCommandBuffer *m_pCommandBuffer;
	unsigned m_CurrentCommandBuffer;

	//
	class IStorage *m_pStorage;
	class IConsole *m_pConsole;

	enum
	{
		MAX_VERTICES = 32*1024,
		MAX_TEXTURES = 1024*4,
		
		DRAWING_QUADS=1,
		DRAWING_LINES=2
	};

	CCommandBuffer::SVertex m_aVertices[MAX_VERTICES];
	int m_NumVertices;

	CCommandBuffer::SColor m_aColor[4];
	CCommandBuffer::STexCoord m_aTexture[4];

	bool m_RenderEnable;

	float m_Rotation;
	int m_Drawing;
	bool m_DoScreenshot;
	char m_aScreenshotName[128];

	int m_InvalidTexture;

	struct CTexture
	{
		int m_State;
		int m_MemSize;
		int m_Flags;
		int m_Next;
	};

	CTexture m_aTextures[MAX_TEXTURES];
	int m_FirstFreeTexture;
	int m_TextureMemoryUsage;

	void FlushVertices();
	void AddVertices(int Count);
	void Rotate4(const CCommandBuffer::SPoint &rCenter, CCommandBuffer::SVertex *pPoints);

	static unsigned char Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp);
	static unsigned char *Rescale(int Width, int Height, int NewWidth, int NewHeight, int Format, const unsigned char *pData);

	void KickCommandBuffer();

	int IssueInit();
	int InitWindow();
public:
	CGraphics_Threaded();

	virtual void ClipEnable(int x, int y, int w, int h);
	virtual void ClipDisable();

	virtual void BlendNone();
	virtual void BlendNormal();
	virtual void BlendAdditive();

	virtual int MemoryUsage() const;

	virtual void MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY);
	virtual void GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY);

	virtual void LinesBegin();
	virtual void LinesEnd();
	virtual void LinesDraw(const CLineItem *pArray, int Num);

	virtual int UnloadTexture(int Index);
	virtual int LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags);
	virtual int LoadTextureRawSub(int TextureID, int x, int y, int Width, int Height, int Format, const void *pData);

	// simple uncompressed RGBA loaders
	virtual int LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags);
	virtual int LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType);

	void ScreenshotDirect(const char *pFilename);

	virtual void TextureSet(int TextureID);

	virtual void Clear(float r, float g, float b);

	virtual void QuadsBegin();
	virtual void QuadsEnd();
	virtual void QuadsSetRotation(float Angle);

	virtual void SetColorVertex(const CColorVertex *pArray, int Num);
	virtual void SetColor(float r, float g, float b, float a);

	virtual void QuadsSetSubset(float TlU, float TlV, float BrU, float BrV);
	virtual void QuadsSetSubsetFree(
		float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3);

	virtual void QuadsDraw(CQuadItem *pArray, int Num);
	virtual void QuadsDrawTL(const CQuadItem *pArray, int Num);
	virtual void QuadsDrawFreeform(const CFreeformItem *pArray, int Num);
	virtual void QuadsText(float x, float y, float Size, float r, float g, float b, float a, const char *pText);

	virtual void Minimize();
	virtual void Maximize();

	virtual int WindowActive();
	virtual int WindowOpen();

	virtual bool Init();
	virtual void Shutdown();

	virtual void TakeScreenshot(const char *pFilename);
	virtual void Swap();

	virtual int GetVideoModes(CVideoMode *pModes, int MaxModes);

	// syncronization
	virtual void InsertSignal(semaphore *pSemaphore);
	virtual bool IsIdle();
	virtual void WaitForIdle();
};