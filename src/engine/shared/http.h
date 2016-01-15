#ifndef ENGINE_SHARED_HTTP_H
#define ENGINE_SHARED_HTTP_H

#include <base/tl/array.h>

#include <engine/external/http-parser/http_parser.h>

#include <engine/engine.h>

enum
{
	HTTP_PRIORITY_HIGH,
	HTTP_PRIORITY_LOW
};

class IHttpBase
{
	enum
	{
		HTTP_MAX_HEADER_FIELDS=16
	};

	bool m_Finalized;

protected:
	struct CHttpField
	{
		char m_aKey[128];
		char m_aValue[128];
	};

	CHttpField m_aFields[HTTP_MAX_HEADER_FIELDS];
	int m_FieldNum;

	IHttpBase();

	bool IsFinalized() const { return m_Finalized; }
	virtual bool Finalize() { m_Finalized = true; return true; }

public:
	virtual ~IHttpBase();

	void AddField(CHttpField Field);
	void AddField(const char *pKey, const char *pValue);
	void AddField(const char *pKey, int Value);
	const char *GetField(const char *pKey) const;
};

class IResponse : public IHttpBase
{
	friend class CHttpConnection;

	http_parser m_Parser;

	CHttpField m_CurField;

	bool m_LastWasValue;
	bool m_Complete;
	bool m_Close;

	static int OnHeaderField(http_parser *pParser, const char *pData, size_t Len);
	static int OnHeaderValue(http_parser *pParser, const char *pData, size_t Len);
	static int OnMessageComplete(http_parser *pParser);

	bool Write(char *pData, int Size);

protected:
	http_parser_settings m_ParserSettings;

	int m_Size;

	IResponse();
	virtual bool Finalize();

public:
	virtual ~IResponse();

	int Size() const { return IsFinalized() ? m_Size : -1; }
	unsigned StatusCode() const { return IsFinalized() ? m_Parser.status_code : -1; }

	virtual bool IsFile() const = 0;
};

class CBufferResponse : public IResponse
{
	char *m_pData;
	int m_BufferSize;

	static int OnBody(http_parser *pParser, const char *pData, size_t Len);
	static int OnHeadersComplete(http_parser *pParser);

	bool ResizeBuffer(int NeededSize);

public:
	CBufferResponse();
	virtual ~CBufferResponse();

	const char *GetBody() const { return IsFinalized() ? m_pData : 0; }

	bool IsFile() const { return false; }
};


class CFileResponse : public IResponse
{
	IOHANDLE m_File;
	unsigned m_Crc;
	char m_aFilename[512];

	static int OnBody(http_parser *pParser, const char *pData, size_t Len);

	bool Finalize();

public:
	CFileResponse(IOHANDLE File, const char *pFilename);
	virtual ~CFileResponse();

	unsigned GetCrc() const { return m_Crc; }
	const char *GetPath() const { return m_aFilename; }
	const char *GetFilename() const;

	bool IsFile() const { return true; }
};

typedef void(*FHttpCallback)(IResponse *pResponse, bool Error, void *pUserData);

class IRequest : public IHttpBase
{
protected:
	friend class CHttpConnection;

	char m_aURI[256];
	int m_Method;
	int m_State;

	char m_aHeader[1024*8];

	char *m_pCur;
	char *m_pEnd;

	IRequest(int Method, const char *pURI, int State);

	int AddToHeader(char *pCur, const char *pData, int Size);
	int GenerateHeader();

	virtual bool Finalize() = 0;

	virtual int GetData(char *pBuf, int MaxSize) = 0;

public:
	enum
	{
		HTTP_GET = 0,
		HTTP_POST,
		HTTP_PUT
	};

	virtual ~IRequest();
	
	//const char *GetURI() const { return m_aURI; }
};

class CBufferRequest : public IRequest
{
	enum
	{
		STATE_HEADER = 0,
		STATE_BODY
	};

	char *m_pBody;
	int m_BodySize;

	int GetData(char *pBuf, int MaxSize);
	bool Finalize();

public:
	CBufferRequest(int Method, const char *pURI);
	virtual ~CBufferRequest();

	bool SetBody(const char *pData, int Size, const char *pContentType);
};

class CFileRequest : public IRequest
{
	enum
	{
		STATE_HEADER = 0,
		STATE_FILE_HEADER,
		STATE_FILE_BODY,
		STATE_FILE_FOOTER
	};

	IOHANDLE m_File;

	char m_aUploadHeader[256];
	char m_aUploadFooter[256];

	const char *GetFilename(const char *pFilename) const;

	int GetData(char *pBuf, int MaxSize);
	bool Finalize();

public:
	CFileRequest(const char *pURI);
	virtual ~CFileRequest();

	bool SetFile(IOHANDLE File, const char *pFilename, const char *pUploadName);
};

class CRequestInfo
{
	friend class CHttpClient;
	friend class CHttpConnection;

	char m_aAddr[256];

	CHostLookup m_Lookup;
	IRequest *m_pRequest;
	IResponse *m_pResponse;

	int m_Priority;

	FHttpCallback m_pfnCallback;
	void *m_pUserData;

public:
	CRequestInfo(const char *pAddr);
	CRequestInfo(const char *pAddr, IOHANDLE File, const char *pFilename);

	virtual ~CRequestInfo();

	void SetPriority(int Priority) { m_Priority = Priority; }
	void SetCallback(FHttpCallback pfnCallback, void *pUserData = 0);
	void ExecuteCallback(IResponse *pResponse, bool Error);
};

class CHttpConnection
{
	enum
	{
		HTTP_CHUNK_SIZE=1024*4,
		HTTP_MAX_SPEED=1024*100
	};

	int64 m_LastDataTime;

	int m_ID;

	NETSOCKET m_Socket;
	NETADDR m_Addr;
	
	int m_State;
	int64 m_LastActionTime;

	CRequestInfo *m_pInfo;

	int64 Interval() const;

	void Reset();
	void Close();

	bool SetState(int State, const char *pMsg = 0);

public:
	enum
	{
		STATE_OFFLINE = 0,
		STATE_CONNECTING,
		STATE_SENDING,
		STATE_RECEIVING,
		STATE_WAITING,
		STATE_ERROR
	};

	CHttpConnection();
	virtual ~CHttpConnection();

	void SetID(int ID) { m_ID = ID; }
	bool Update();

	bool Connect(NETADDR Addr);
	bool SetRequest(CRequestInfo *pInfo);

	int State() { return m_State; }
	bool CompareAddr(NETADDR Addr);

	const CRequestInfo* GetInfo() const { return m_pInfo; }
};

class CHttpClient
{
	enum
	{
		HTTP_MAX_CONNECTIONS = 4,
		HTTP_MAX_LOW_PRIORITY_CONNECTIONS=2
	};

	CHttpConnection m_aConnections[HTTP_MAX_CONNECTIONS];
	array<CRequestInfo*> m_lPendingRequests;

	IEngine *m_pEngine;

	CHttpConnection *GetConnection(NETADDR Addr);

public:
	CHttpClient();
	virtual ~CHttpClient();

	void Init(IEngine *pEngine) { m_pEngine = pEngine; }

	void Send(CRequestInfo *pInfo, IRequest *pRequest);
	void FetchRequest(int Priority, int Max);
	void Update();
};

#endif
