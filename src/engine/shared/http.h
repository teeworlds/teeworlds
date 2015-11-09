#ifndef ENGINE_SHARED_HTTP_H
#define ENGINE_SHARED_HTTP_H

class IHttpBase
{
	enum
	{
		HTTP_MAX_HEADER_FIELDS=16
	};

	bool m_Finish;

protected:
	struct CHttpField
	{
		char m_aKey[128];
		char m_aValue[128];
	};

	CHttpField m_aFields[HTTP_MAX_HEADER_FIELDS];
	int m_FieldNum;

	IHttpBase();

	bool IsFinished() const { return m_Finish; }
	virtual bool Finish() { m_Finish = true; return true; }

public:
	virtual ~IHttpBase();

	void AddField(const char *pKey, const char *pValue);
	void AddField(const char *pKey, int Value);
	const char *GetField(const char *pKey) const;
};

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
	
	CRequest();
	void Init(int Method, const char *pURI);

	int AddToHeader(char *pCur, const char *pData, int Size);
	int GenerateHeader();
	bool InitBody(int Size, const char *pContentType);
	bool Finish();

	int GetData(char *pBuf, int MaxSize);
	void MoveCursor(int Bytes) { m_pCur += Bytes; }

	const char *GetFilename(const char *pFilename) const;

public:
	enum
	{
		HTTP_NONE = 0,
		HTTP_GET,
		HTTP_POST,
		HTTP_PUT
	};

	virtual ~CRequest();

	bool SetBody(const char *pData, int Size, const char *pContentType);
	
	//const char *GetURI() const { return m_aURI; }
};

class CResponse : public IHttpBase
{
	friend class CHttpConnection;
	
	char *m_pData;
	int m_HeaderSize;
	int m_BufferSize;
	int m_Size;
	int m_StatusCode;
	
	CResponse();

	int ParseHeader();
	bool ResizeBuffer(int NeededSize);

	bool Write(char *pData, int Size);
	bool Finish();

public:
	virtual ~CResponse();

	const char *GetBody() const { return IsFinished() ? m_pData + m_HeaderSize : 0; }
	int BodySize() const { return IsFinished() ? m_Size - m_HeaderSize : -1; }
	int StatusCode() const { return IsFinished() ? m_StatusCode : -1; }
};

class CHttpConnection
{
	friend class CHttpClient;

public:
	typedef void (*FHttpCallback)(CResponse *pResponse, bool Error, void *pUserData);
	
private:
	enum
	{
		STATE_NONE = 0,
		STATE_CONNECTING,
		STATE_SENDING,
		STATE_RECEIVING,
		STATE_DONE,
		STATE_ERROR
	};

	NETSOCKET m_Socket;
	
	char m_aAddr[256];
	int m_Port;
	int m_State;
	FHttpCallback m_pfnCallback;

	int64 m_LastActionTime;

	CResponse m_Response;
	CRequest m_Request;
	void *m_pUserData;

	int SetState(int State, const char *pMsg = 0);
	void Call();

public:
	CHttpConnection(const char *pAddr, FHttpCallback pfnCallback);
	~CHttpConnection();

	int Update();

	CRequest *CreateRequest(int Method, const char *pURI);

	//bool Error() { return m_State == STATE_ERROR; }
	CResponse *Response() { return &m_Response; }
	void *UserData() { return m_pUserData; }

	void SetUserData(void *pUserData) { m_pUserData = pUserData; }
};

class CHttpClient
{
	enum
	{
		HTTP_MAX_CONNECTIONS = 16
	};

	CHttpConnection *m_apConnections[HTTP_MAX_CONNECTIONS];

public:
	CHttpClient();
	virtual ~CHttpClient();

	bool Send(CHttpConnection *pCon);
	void Update();
};

#endif
