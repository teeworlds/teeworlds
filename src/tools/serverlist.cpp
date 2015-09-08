#include <base/system.h>
#include <game/version.h>

#include <http_parser.h>

enum
{
	BUFFER_SIZE=1024,
};

// Fuck all error checking, right?

void SendRequest(NETSOCKET Socket, const char *pHostname)
{
	char aBuf[BUFFER_SIZE];
	// E: Buf too small.
	str_format(aBuf, sizeof(aBuf),
		"GET /teeworlds/serverlist/0.6 HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Teeworlds_serverlist/" GAME_VERSION "\r\n"
		"Connection: close\r\n"
		"\r\n", pHostname);
	// E: Buf not completely sent.
	net_tcp_send(Socket, aBuf, str_length(aBuf)); // exclude zero termination
}

void LineCallback(const char *pLine)
{
	dbg_msg("lc", "%s", pLine);
}

struct CReceiveCallbackData
{
	char m_aBuf[BUFFER_SIZE+1];
	size_t m_BufSize;
	bool m_Invalid;
	void (*m_LineCallback)(const char *pLine);
};

int ReceiveCallback(http_parser *pParser, const char *pData, size_t DataSize)
{
	CReceiveCallbackData *pCbData = (CReceiveCallbackData *)pParser->data;

	for(size_t i = 0; i < DataSize; i++)
	{
		if(pCbData->m_Invalid)
		{
			if(pData[i] == '\n')
			{
				pCbData->m_BufSize = 0;
				pCbData->m_Invalid = false;
			}
			continue;
		}

		if(pData[i] < 32)
		{
			// Null termination.
			pCbData->m_aBuf[pCbData->m_BufSize] = '\0';
			pCbData->m_LineCallback(pCbData->m_aBuf);
			pCbData->m_BufSize = 0;
			if(pData[i] != '\n')
			{
				pCbData->m_Invalid = true;
			}
			continue;
		}

		// Leave one byte for zero termination.
		if(pCbData->m_BufSize == sizeof(pCbData->m_aBuf) - 1)
		{
			pCbData->m_Invalid = true;
			continue;
		}

		pCbData->m_aBuf[pCbData->m_BufSize] = pData[i];
		pCbData->m_BufSize++;
	}

	return 0;
}

int ReceiveStatusCallback(http_parser *pParser, const char *pData, size_t DataSize)
{
	dbg_msg("status", "%d", pParser->status_code);
	return 0;
}

void Receive(NETSOCKET Socket, http_parser *pParser)
{
	http_parser_settings Settings = { 0 };
	Settings.on_body = ReceiveCallback;
	Settings.on_status = ReceiveStatusCallback;

	CReceiveCallbackData CbData = { 0 };
	CbData.m_LineCallback = LineCallback;
	pParser->data = &CbData;

	int Read;
	do
	{
		char aBuf[BUFFER_SIZE];
		Read = net_tcp_recv(Socket, aBuf, sizeof(aBuf));
		if(Read < 0)
		{
			// Errors don't happen, remember?
			return;
		}

		http_parser_execute(pParser, &Settings, aBuf, Read);
	}
	while(Read != 0);
}

void Run(const char *pHostname)
{
	NETADDR BindAddr = { 0 };
	BindAddr.type = NETTYPE_IPV4;
	NETSOCKET Socket = net_tcp_create(BindAddr);

	NETADDR Addr = { 0 };
	// E: Hostname not found
	net_host_lookup(pHostname, &Addr, NETTYPE_IPV4);

	Addr.port = 80;

	// E: Conn refused
	net_tcp_connect(&Socket, &Addr);

	SendRequest(Socket, pHostname);

	http_parser Parser;
	http_parser_init(&Parser, HTTP_RESPONSE);

	Receive(Socket, &Parser);
}

int main(int argc, char **argv)
{
	dbg_logger_stdout();
	net_init();

	for(int i = 1; i < argc; i++)
	{
		Run(argv[i]);
	}

	if(argc <= 1)
	{
		Run("master1.teeworlds.com");
		Run("master2.teeworlds.com");
		Run("master3.teeworlds.com");
		Run("master4.teeworlds.com");
	}
}
