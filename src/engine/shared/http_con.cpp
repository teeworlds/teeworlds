#include <base/system.h>

#include "http.h"

CHttpConnection::CHttpConnection(const char *pAddr, FHttpCallback pfnCallback)
	: m_State(STATE_NONE), m_pfnCallback(pfnCallback), m_LastActionTime(-1), m_pUserData(0)
{
	str_copy(m_aAddr, pAddr, sizeof(m_aAddr));
	
	m_Port = 80;
	for(int k = 0; m_aAddr[k]; k++)
	{
		if(m_aAddr[k] == ':')
		{
			m_Port = str_toint(m_aAddr+k+1);
			m_aAddr[k] = 0;
			break;
		}
	}
}

CHttpConnection::~CHttpConnection()
{
	net_tcp_close(m_Socket);
}

CRequest *CHttpConnection::CreateRequest(int Method, const char *pURI)
{
	if(m_Request.m_Method == CRequest::HTTP_NONE)
	{
		m_Request.Init(Method, pURI);
		m_Request.AddField("Host", m_aAddr);
	}
	return &m_Request;
}

int CHttpConnection::SetState(int State, const char *pMsg)
{
	m_State = State;
	if(pMsg)
		dbg_msg("http/conn", "%s", pMsg);
	if(m_State == STATE_DONE)
	{
		net_tcp_close(m_Socket);
		return 1;
	}
	if(m_State == STATE_ERROR)
	{
		net_tcp_close(m_Socket);
		return -1;
	}
	return 0;
}

void CHttpConnection::Call()
{
	bool Error = m_State == STATE_ERROR;
	if(m_State == STATE_DONE || Error)
		m_pfnCallback(&m_Response, Error, m_pUserData);
}

int CHttpConnection::Update()
{
	if(time_get() - m_LastActionTime > time_freq() * 5 && m_LastActionTime > 0)
		return SetState(STATE_ERROR, "error: timeout");

	switch(m_State)
	{
		case STATE_NONE:
		{
			if(!m_Request.Finish())
				return SetState(STATE_ERROR, "error: incomplete request");

			NETADDR BindAddr = { 0 };
			BindAddr.type = NETTYPE_IPV4;
			m_Socket = net_tcp_create(BindAddr);
			if(m_Socket.type == NETTYPE_INVALID)
				return SetState(STATE_ERROR, "error: could not create socket");

			net_set_non_blocking(m_Socket);

			// TODO: move to thread
			NETADDR Addr;
			if(net_host_lookup(m_aAddr, &Addr, NETTYPE_IPV4) != 0)
				return SetState(STATE_ERROR, "error: could not find the address");
			Addr.port = m_Port;
			
			char aAddrStr[NETADDR_MAXSTRSIZE];
			net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
			dbg_msg("http/conn", "addr: %s", aAddrStr);

			net_tcp_connect(m_Socket, &Addr);
			m_LastActionTime = time_get();
			SetState(STATE_CONNECTING, "connecting");
		}

		case STATE_CONNECTING:
		{
			int Result = net_socket_write_wait(m_Socket, 0);
			if(Result == 0)
				break;
			if(Result == -1)
				return SetState(STATE_ERROR, "error: could not connect");
			m_LastActionTime = time_get();
			SetState(STATE_SENDING, "connected");
		}

		case STATE_SENDING:
		{
			char aData[1024] = {0};
			int Bytes = m_Request.GetData(aData, sizeof(aData));
			if(Bytes < 0)
				return SetState(STATE_ERROR, "error: could not read request data");
			if(Bytes > 0)
			{
				int Size = net_tcp_send(m_Socket, aData, Bytes);
				if(Size < 0)
				{
					if(net_would_block())
						break;
					return SetState(STATE_ERROR, "error: sending data");
				}
				m_LastActionTime = time_get();

				// resend if needed
				if(Size < Bytes)
					m_Request.MoveCursor(Size - Bytes);
				break;
			}
			SetState(STATE_RECEIVING, "sent request");
		}

		case STATE_RECEIVING:
		{
			char aBuf[1024] = {0};
			int Bytes = net_tcp_recv(m_Socket, aBuf, sizeof(aBuf));
			if(Bytes < 0)
			{
				if(net_would_block())
					break;
				return SetState(STATE_ERROR, "error: receiving data");
			}
			if(Bytes > 0)
			{
				m_LastActionTime = time_get();
				if(!m_Response.Write(aBuf, Bytes))
					return SetState(STATE_ERROR, "error: could not read the response header");
				break;
			}
			if(!m_Response.Finish())
				return SetState(STATE_ERROR, "error: incomplete response");
			return SetState(STATE_DONE, "received response");
		}

		case STATE_DONE:
			return 1;
		
		case STATE_ERROR:
			return -1;

		default:
			return SetState(STATE_ERROR, "unknown error");
	}
	
	return 0;
}
