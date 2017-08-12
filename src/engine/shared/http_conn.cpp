#include <base/system.h>

#include "config.h"
#include "http.h"

CHttpConnection::CHttpConnection()
	: m_State(STATE_OFFLINE), m_LastActionTime(-1), m_LastDataTime(-1), m_pInfo(0), m_BufferBytes(0), m_BufferOffset(0),
	m_DataInterval(time_freq() / (HTTP_MAX_SPEED / HTTP_CHUNK_SIZE))
{
	mem_zero(&m_Addr, sizeof(m_Addr));
}

CHttpConnection::~CHttpConnection()
{
	if(m_pInfo)
		m_pInfo->ExecuteCallback(m_pInfo->m_pResponse, true);
	Close();
	Reset();
}

void CHttpConnection::Reset()
{
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

bool CHttpConnection::SetState(int State, const char *pMsg)
{
	if(pMsg && g_Config.m_Debug)
		dbg_msg("http/conn", "%d: %s", m_ID, pMsg);
	bool Error = State == STATE_ERROR;
	if(Error)
	{
		State = STATE_OFFLINE;
		if(m_pInfo)
			m_pInfo->ExecuteCallback(m_pInfo->m_pResponse, true);
		Reset();
	}

	if(State == STATE_OFFLINE)
	{
		if(g_Config.m_Debug)
			dbg_msg("http/conn", "%d: disconnecting", m_ID);
		Close();
	}
	m_State = State;
	return !Error;
}

bool CHttpConnection::CompareAddr(NETADDR Addr) const
{
	if(m_State == STATE_OFFLINE)
		return false;
	return net_addr_comp(&m_Addr, &Addr) == 0;
}

bool CHttpConnection::Connect(NETADDR Addr)
{
	if(m_State != STATE_OFFLINE || !m_pInfo)
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

	return SetState(STATE_CONNECTING, aBuf);
}

bool CHttpConnection::SetRequest(CRequestInfo *pInfo)
{
	if(IsActive())
		return false;
	m_pInfo = pInfo;
	if(pInfo->m_pRequest->Finalize())
	{
		m_LastActionTime = time_get();
		if(g_Config.m_Debug)
			dbg_msg("http/conn", "%d: %s", m_ID, "new request");
		if(m_State == STATE_OFFLINE)
			return true;
		m_LastDataTime = time_get() - m_DataInterval;
		return SetState(STATE_SENDING);
	}
	return SetState(STATE_ERROR, "error: incomplete request");
}

bool CHttpConnection::Update()
{
	char aBuf[128];
	int64 IdleTime = time_get() - m_LastActionTime;
	if(IsActive() && IdleTime > time_freq() * HTTP_TIMEOUT)
		return SetState(STATE_ERROR, "error: timeout");

	// TODO: improve bandwidth limiting
	switch(m_State)
	{
		case STATE_CONNECTING:
		{
			if(net_socket_write_wait(m_Socket, 0) == 0)
				break;
			int Error = net_socket_error(m_Socket);
			if(Error)
			{
				str_format(aBuf, sizeof(aBuf), "error: could not connect (%d)", Error);
				return SetState(STATE_ERROR, aBuf);
			}

			m_LastActionTime = time_get();
			m_LastDataTime = time_get() - m_DataInterval;
			return SetState(STATE_SENDING, "connected");
		}
		break;

		case STATE_SENDING:
		{
			if(time_get() - m_LastDataTime < m_DataInterval)
				return 0;

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
				{
					if(net_would_block())
						break;
					str_format(aBuf, sizeof(aBuf), "error: sending data (%d)", net_errno());
					return SetState(STATE_ERROR, aBuf);
				}
				m_BufferBytes -= Size;
				m_BufferOffset += Size;

				m_LastActionTime = time_get();
				m_LastDataTime += m_DataInterval;
			}
			else // Bytes = 0
				return SetState(STATE_RECEIVING, "sent request");
		}
		break;

		case STATE_RECEIVING:
		{
			if(time_get() - m_LastDataTime < m_DataInterval)
				return 0;

			m_BufferBytes = net_tcp_recv(m_Socket, m_aBuffer, sizeof(m_aBuffer));
			if(m_BufferBytes < 0)
			{
				if(net_would_block())
					break;
				str_format(aBuf, sizeof(aBuf), "error: receiving data (%d)", net_errno());
				return SetState(STATE_ERROR, aBuf);
			}

			m_LastActionTime = time_get();
			m_LastDataTime += m_DataInterval;
			if(!m_pInfo->m_pResponse->Write(m_aBuffer, m_BufferBytes))
				return SetState(STATE_ERROR, "error: parsing http");

			bool ForceClose = !m_BufferBytes;
			if(m_pInfo->m_pResponse->m_Complete)
			{
				if(!m_pInfo->m_pResponse->Finalize())
					return SetState(STATE_ERROR, "error: incomplete response");
				m_BufferBytes = 0;

				bool Disconnect = m_pInfo->m_pResponse->m_Close || ForceClose;
				m_pInfo->ExecuteCallback(m_pInfo->m_pResponse, false);
				Reset();
				return SetState(Disconnect ? STATE_OFFLINE : STATE_WAITING, "received response");
			}
			else if(ForceClose)
				return SetState(STATE_ERROR, "error: remote closed");
		}
		break;

		case STATE_WAITING:
		{
			if(IdleTime > time_freq() * HTTP_KEEPALIVE_TIME)
				return SetState(STATE_OFFLINE, "closing idle connection");

			int Bytes = net_tcp_recv(m_Socket, m_aBuffer, sizeof(m_aBuffer));
			if(Bytes < 0 && !net_would_block())
			{
				str_format(aBuf, sizeof(aBuf), "error: waiting (%d)", net_errno());
				return SetState(STATE_ERROR, aBuf);
			}
			else if(Bytes == 0)
				return SetState(STATE_OFFLINE, "remote closed");
		}
		break;
	}
	
	return 0;
}
