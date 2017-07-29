#include <base/system.h>

#include "config.h"
#include "http.h"

CHttpConnection::CHttpConnection()
	: m_LastDataTime(-1), m_State(STATE_OFFLINE), m_LastActionTime(-1), m_pInfo(0), m_BufferBytes(0), m_BufferOffset(0)
{
	mem_zero(&m_Addr, sizeof(m_Addr));
}

CHttpConnection::~CHttpConnection()
{
	Close();
	Reset();
}

void CHttpConnection::Reset()
{
	if(m_pInfo)
		delete m_pInfo;
	m_pInfo = 0;
}

void CHttpConnection::Close()
{
	if(m_State != STATE_OFFLINE)
		net_tcp_close(m_Socket);

	mem_zero(&m_Addr, sizeof(m_Addr));
	m_State = STATE_OFFLINE;
}

int64 CHttpConnection::Interval() const
{
	return time_freq() / (HTTP_MAX_SPEED / HTTP_CHUNK_SIZE);
}

bool CHttpConnection::SetState(int State, const char *pMsg)
{
	if(pMsg && g_Config.m_Debug)
		dbg_msg("http/conn", "%d: %s", m_ID, pMsg);
	bool Error = State == STATE_ERROR;
	if(Error)
		State = STATE_OFFLINE;

	if(State == STATE_WAITING || State == STATE_OFFLINE)
	{
		if(m_pInfo)
			m_pInfo->ExecuteCallback(m_pInfo->m_pResponse, Error);
		Reset();
	}
	if(State == STATE_OFFLINE)
	{
		if(g_Config.m_Debug)
			dbg_msg("http/conn", "%d: disconnecting", m_ID);
		Close();
	}
	if(State == STATE_SENDING || State == STATE_RECEIVING)
	{
		m_LastDataTime = time_get() - Interval();
	}
	m_State = State;
	return !Error;
}

bool CHttpConnection::CompareAddr(NETADDR Addr)
{
	if(m_State == STATE_OFFLINE)
		return false;
	return net_addr_comp(&m_Addr, &Addr) == 0;
}

bool CHttpConnection::Connect(NETADDR Addr)
{
	if(m_State != STATE_OFFLINE)
		return false;

	NETADDR BindAddr = { 0 };
	BindAddr.type = NETTYPE_IPV4;
	m_Socket = net_tcp_create(BindAddr);
	if(m_Socket.type == NETTYPE_INVALID)
		return SetState(STATE_ERROR, "error: could not create socket");

	net_set_non_blocking(m_Socket);

	m_Addr = Addr;
	net_tcp_connect(m_Socket, &m_Addr);
	m_LastActionTime = time_get();

	char aBuf[256];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&m_Addr, aAddrStr, sizeof(aAddrStr), true);
	str_format(aBuf, sizeof(aBuf), "connecting to %s", aAddrStr);

	SetState(STATE_CONNECTING, aBuf);
	return true;
}

bool CHttpConnection::SetRequest(CRequestInfo *pInfo)
{
	if(m_State != STATE_WAITING && m_State != STATE_CONNECTING)
		return false;
	m_pInfo = pInfo;
	if(pInfo->m_pRequest->Finalize())
	{
		m_LastActionTime = time_get();
		int NewState = m_State == STATE_CONNECTING ? STATE_CONNECTING : STATE_SENDING;
		return SetState(NewState, "new request");
	}
	return SetState(STATE_ERROR, "error: incomplete request");
}

bool CHttpConnection::Update()
{
	int Timeout = m_State == STATE_WAITING ? 30 : 10;
	if(m_State != STATE_OFFLINE && time_get() - m_LastActionTime > time_freq() * Timeout)
		return SetState(STATE_ERROR, "error: timeout");

	if(m_State == STATE_SENDING || m_State == STATE_RECEIVING)
	{
		if(time_get() - m_LastDataTime < Interval())
			return 0;
		else
			m_LastDataTime += Interval();
	}

	switch(m_State)
	{
		case STATE_CONNECTING:
		{
			int Result = net_socket_write_wait(m_Socket, 0);
			if(Result > 0)
			{
				m_LastActionTime = time_get();
				int NewState = m_pInfo ? STATE_SENDING : STATE_WAITING;
				return SetState(NewState, "connected");
			}
			else if(Result == -1)
				return SetState(STATE_ERROR, "error: could not connect");
		}
		break;

		case STATE_SENDING:
		{
			if(m_BufferBytes == 0)
			{
				m_BufferBytes = m_pInfo->m_pRequest->GetData(m_aBuffer, sizeof(m_aBuffer));
				m_BufferOffset = 0;
			}
			if(m_BufferBytes < 0)
				return SetState(STATE_ERROR, "error: could not read request data");
			else if(m_BufferBytes > 0)
			{
				int Size = net_tcp_send(m_Socket, m_aBuffer + m_BufferOffset, m_BufferBytes);
				if(Size < 0)
					return SetState(STATE_ERROR, "error: sending data");
				m_BufferBytes -= Size;
				m_BufferOffset += Size;

				m_LastActionTime = time_get();
			}
			else // Bytes = 0
				return SetState(STATE_RECEIVING, "sent request"); 
		}
		break;

		case STATE_RECEIVING:
		{
			bool ForceClose = false;
			m_BufferBytes = net_tcp_recv(m_Socket, m_aBuffer, sizeof(m_aBuffer));
			if(m_BufferBytes < 0)
			{
				if(!net_would_block())
					return SetState(STATE_ERROR, "error: receiving data");
			}
			else if(m_BufferBytes >= 0)
			{
				m_LastActionTime = time_get();
				if(!m_pInfo->m_pResponse->Write(m_aBuffer, m_BufferBytes))
					return SetState(STATE_ERROR, "error: parsing http");
			}

			if(m_BufferBytes == 0)
				ForceClose = true;
			if(m_pInfo->m_pResponse->m_Complete)
			{
				if(!m_pInfo->m_pResponse->Finalize())
					return SetState(STATE_ERROR, "error: incomplete response");
				m_BufferBytes = 0;
				return SetState((m_pInfo->m_pResponse->m_Close || ForceClose) ? STATE_OFFLINE : STATE_WAITING, "received response");
			}
			else if(ForceClose)
				return SetState(STATE_ERROR, "error: remote closed");
		}
		break;

		case STATE_WAITING:
		{
			int Bytes = net_tcp_recv(m_Socket, m_aBuffer, sizeof(m_aBuffer));
			if(Bytes < 0)
			{
				if (!net_would_block())
					return SetState(STATE_ERROR, "error: waiting");
			}
			else if(Bytes == 0)
				return SetState(STATE_OFFLINE, "remote closed");
		}
		break;
	}
	
	return 0;
}
