#ifndef ENGINE_SHARED_HTTP_H
#define ENGINE_SHARED_HTTP_H

#include <base/tl/array.h>

#include <engine/external/http-parser/http_parser.h>

#include <engine/engine.h>

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
	bool Finalize();

protected:
	http_parser_settings m_ParserSettings;

	int m_Size;

	IResponse();

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

	static int OnBody(http_parser *pParser, const char *pData, size_t Len);

public:
	CFileResponse(IOHANDLE File);
	virtual ~CFileResponse();

	bool IsFile() const { return true; }
};

typedef void(*FHttpCallback)(IResponse *pResponse, bool Error, void *pUserData);

class CRequest : public IHttpBase
{
	friend class CHttpConnection;

	enum
	{
		STATE_HEADER=0,
		STATE_BODY
	};

	char m_aURI[256];
	int m_Method;
	int m_State;

	char m_aHeader[1024*8];
	char *m_pBody;
	int m_BodySize;

	char *m_pCur;
	char *m_pEnd;

	int AddToHeader(char *pCur, const char *pData, int Size);
	int GenerateHeader();
	bool InitBody(int Size, const char *pContentType);

	bool Finalize();

	int GetData(char *pBuf, int MaxSize);
	void MoveCursor(int Bytes) { m_pCur += Bytes; }

	const char *GetFilename(const char *pFilename) const;

public:
	enum
	{
		HTTP_GET = 0,
		HTTP_POST,
		HTTP_PUT
	};

	CRequest(int Method, const char *pURI);
	virtual ~CRequest();

	bool SetBody(const char *pData, int Size, const char *pContentType);
	
	//const char *GetURI() const { return m_aURI; }
};

class CRequestInfo
{
	friend class CHttpClient;
	friend class CHttpConnection;

	char m_aAddr[256];

	CHostLookup m_Lookup;
	CRequest *m_pRequest;
	IResponse *m_pResponse;

	FHttpCallback m_pfnCallback;
	void *m_pUserData;

public:
	CRequestInfo(const char *pAddr);
	CRequestInfo(const char *pAddr, IOHANDLE File);

	virtual ~CRequestInfo();

	void SetCallback(FHttpCallback pfnCallback, void *pUserData = 0);
	void ExecuteCallback(IResponse *pResponse, bool Error);
};

class CHttpConnection
{
private:
	int m_ID;

	NETSOCKET m_Socket;
	NETADDR m_Addr;
	
	int m_State;
	int64 m_LastActionTime;

	CRequestInfo *m_pInfo;

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
};

class CHttpClient
{
	enum
	{
		HTTP_MAX_CONNECTIONS = 4
	};

	CHttpConnection m_aConnections[HTTP_MAX_CONNECTIONS];
	array<CRequestInfo*> m_lPendingRequests;

	IEngine *m_pEngine;

	CHttpConnection *GetConnection(NETADDR Addr);

public:
	CHttpClient();
	virtual ~CHttpClient();

	void Init(IEngine *pEngine) { m_pEngine = pEngine; }

	void Send(CRequestInfo *pInfo, CRequest *pRequest);
	void Update();
};

#endif
