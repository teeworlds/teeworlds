/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "config.h"
#include "network.h"

void CConsoleNetConnection::Reset()
{
	m_State = NET_CONNSTATE_OFFLINE;
	mem_zero(&m_PeerAddr, sizeof(m_PeerAddr));
	m_aErrorString[0] = 0;

	m_aBuffer[0] = 0;
	m_pBufferPos = 0;
	m_Timeout = 0;
}

void CConsoleNetConnection::Init(NETSOCKET Socket, const NETADDR *pAddr)
{
	Reset();

	m_Socket = Socket;
	net_tcp_set_non_blocking(m_Socket);

	m_LastRecvTime = time_get();

	m_PeerAddr = *pAddr;
	m_State = NET_CONNSTATE_ONLINE;
}

void CConsoleNetConnection::Init(NETSOCKET Socket)
{
	Reset();

	m_Socket = Socket;
	net_tcp_set_non_blocking(m_Socket);

	m_LastRecvTime = time_get();
}

int CConsoleNetConnection::Connect(const NETADDR *pAddr)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return -1;

	// init connection
	Reset();
	m_PeerAddr = *pAddr;
	net_tcp_connect(m_Socket, pAddr);
	m_State = NET_CONNSTATE_ONLINE;
	return 0;
}

void CConsoleNetConnection::Disconnect(const char *pReason)
{
	if(State() == NET_CONNSTATE_OFFLINE)
		return;

	if(pReason)
	{
		char aBuf[sizeof(pReason) + 4];
		str_format(aBuf, sizeof(aBuf), "%s", pReason);
		Send(aBuf);
	}

	net_tcp_close(m_Socket);

	Reset();
}

int CConsoleNetConnection::Update()
{
	if(m_Timeout && time_get() > m_LastRecvTime + m_Timeout * time_freq())
	{
		m_State = NET_CONNSTATE_ERROR;
		str_copy(m_aErrorString, "timeout", sizeof(m_aErrorString));
		return -1;
	}

	if(State() == NET_CONNSTATE_ONLINE)
	{
		char aBuf[NET_MAX_PACKETSIZE];

		int Bytes = net_tcp_recv(m_Socket, aBuf, sizeof(aBuf) - 1);

		if(Bytes > 0)
		{
			aBuf[Bytes - 1] = 0;

			if(!m_pBufferPos)
				m_aBuffer[0] = 0;
			else if(m_pBufferPos != m_aBuffer)
				mem_move(m_pBufferPos, m_aBuffer, str_length(m_pBufferPos) + 1); // +1 for the \0
			m_pBufferPos = m_aBuffer;

			str_append(m_aBuffer, aBuf, sizeof(m_aBuffer));

			m_LastRecvTime = time_get();
		}
		else if(Bytes < 0)
		{
			if(net_would_block()) // no data received
				return 0;

			m_State = NET_CONNSTATE_ERROR; // error
			str_copy(m_aErrorString, "connection failure", sizeof(m_aErrorString));
			return -1;
		}
		else
		{
			m_State = NET_CONNSTATE_ERROR;
			str_copy(m_aErrorString, "remote end closed the connection", sizeof(m_aErrorString));
			return -1;
		}
	}

	return 0;
}

int CConsoleNetConnection::Recv(char *pLine, int MaxLength)
{
	if(State() == NET_CONNSTATE_ONLINE)
	{
		if(m_pBufferPos && *m_pBufferPos)
		{
			char *pResult = m_pBufferPos;

			while(*m_pBufferPos && *m_pBufferPos != '\r' && *m_pBufferPos != '\n')
				m_pBufferPos++;

			if(*m_pBufferPos) // haven't reached the end of the buffer?
			{
				if(*m_pBufferPos == '\r' && *(m_pBufferPos + 1) == '\n')
				{
					*m_pBufferPos = 0;
					m_pBufferPos += 2;
				}
				else
				{
					*m_pBufferPos = 0;
					m_pBufferPos++;
				}
			}
			else
			{
				m_pBufferPos = 0;
			}

			str_copy(pLine, pResult, MaxLength);
			return 1;
		}
	}
	return 0;
}

int CConsoleNetConnection::Send(const char *pLine)
{
	if(State() != NET_CONNSTATE_ONLINE)
		return -1;

	int Length = str_length(pLine);
	char aBuf[1024];
	str_copy(aBuf, pLine, sizeof(aBuf) - 2);
	aBuf[Length + 1] = '\n';
	aBuf[Length + 2] = '\0';

	if(net_tcp_send(m_Socket, aBuf, Length + 2) < 0)
	{
		m_State = NET_CONNSTATE_ERROR;
		str_copy(m_aErrorString, "Failed to send packet", sizeof(m_aErrorString));
		return -1;
	}
	
	return 0;
}

