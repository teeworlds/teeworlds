/* Webapp class by Sushi and Redix */

#include <game/data.h>
#include "http_con.h"
#include "request.h"
#include "response.h"

// TODO: support for chunked transfer-encoding?
// TODO: move http directory somewhere else?
CHttpConnection::CHttpConnection() : m_State(STATE_NONE), m_Type(-1),
	m_LastActionTime(-1), m_pResponse(0), m_pRequest(0), m_pUserData(0) { }

CHttpConnection::~CHttpConnection()
{
	if(m_pResponse)
		delete m_pResponse;
	if(m_pRequest)
		delete m_pRequest;
	if(m_pUserData)
		delete m_pUserData;
	Close();
}

bool CHttpConnection::Create(NETADDR Addr, int Type)
{
	m_Addr = Addr;
	m_Socket = net_tcp_create(m_Addr);
	if(m_Socket.type == NETTYPE_INVALID)
		return false;

	net_set_non_blocking(m_Socket);
	m_State = STATE_CONNECT;
	m_Type = Type;
	return true;
}

void CHttpConnection::CloseFiles()
{
	m_pRequest->CloseFiles();
	m_pResponse->CloseFiles();
}

void CHttpConnection::Close()
{
	net_tcp_close(m_Socket);
	m_State = STATE_NONE;
}

int CHttpConnection::SetState(int State, const char *pMsg)
{
	m_State = State;
	if(pMsg)
		dbg_msg("http", "%s (%d : %s)", pMsg, m_Type, m_pRequest->GetURI());
	if(m_State == STATE_DONE)
		return 1;
	if(m_State == STATE_ERROR)
		return -1;
	return 0;
}

int CHttpConnection::Update()
{
	if(time_get() - m_LastActionTime > time_freq() * 5 && m_LastActionTime != -1)
		return SetState(STATE_ERROR, "error: timeout");

	switch(m_State)
	{
		case STATE_CONNECT:
		{
			if(time_get() < m_pRequest->StartTime() && m_pRequest->StartTime() != -1)
				return 0;

			m_State = STATE_WAIT;
			if(net_tcp_connect(m_Socket, &m_Addr) != 0)
			{
				if(net_in_progress())
				{
					m_LastActionTime = time_get();
					return 0;
				}
				return SetState(STATE_ERROR, "error: could not connect");
			}
			return 0;
		}

		case STATE_WAIT:
		{
			int Result = net_socket_write_wait(m_Socket, 0);
			if(Result == 1)
			{
				m_LastActionTime = time_get();
				if(m_pRequest->Finish())
					return SetState(STATE_SEND, "connected");
				return SetState(STATE_ERROR, "error: incomplete request");
			}
			if(Result == -1)
			{
				return SetState(STATE_ERROR, "error: could not connect");
			}
			return 0;
		}

		case STATE_SEND:
		{
			char aData[1024] = {0};
			int Bytes = m_pRequest->GetData(aData, sizeof(aData));
			if(Bytes > 0)
			{
				int Size = net_tcp_send(m_Socket, aData, Bytes);
				if(Size < 0)
					return SetState(STATE_ERROR, "error: sending data");
				m_LastActionTime = time_get();

				if(Size > Bytes)
					m_pRequest->MoveCursor(Bytes-Size);
			}
			else if(Bytes == 0)
			{
				return SetState(STATE_RECV, "sent request");
			}
			else
			{
				return SetState(STATE_ERROR, "error: request is incomplete");
			}
		}

		case STATE_RECV:
		{
			char aBuf[1024] = {0};
			int Bytes = net_tcp_recv(m_Socket, aBuf, sizeof(aBuf));

			if(Bytes > 0)
			{
				m_LastActionTime = time_get();
				if(m_pResponse->Write(aBuf, Bytes) == -1)
					return SetState(STATE_ERROR, "error: could not read the header");
			}
			else if(Bytes < 0)
			{
				if(net_would_block())
					return 0;
				return SetState(STATE_ERROR, "connection error");
			}
			else
			{
				if(m_pResponse->Finish())
					return SetState(STATE_DONE, "received response");
				return SetState(STATE_ERROR, "error: wrong content-length");
			}
			return 0;
		}

		default:
			return SetState(STATE_ERROR, "unknown error");
	}
}
