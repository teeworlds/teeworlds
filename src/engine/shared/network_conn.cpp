/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "config.h"
#include "network.h"

void CNetConnection::ResetStats()
{
	mem_zero(&m_Stats, sizeof(m_Stats));
}

void CNetConnection::Reset()
{
	m_Sequence = 0;
	m_UnknownAck = false;
	m_Ack = 0;
	m_PeerAck = 0;
	m_RemoteClosed = 0;

	m_State = NET_CONNSTATE_OFFLINE;
	m_LastSendTime = 0;
	m_LastRecvTime = 0;
	m_LastUpdateTime = 0;
	m_UseToken = true;
	m_Token = 0;
	mem_zero(&m_PeerAddr, sizeof(m_PeerAddr));

	m_Buffer.Init();

	mem_zero(&m_Construct, sizeof(m_Construct));
}

const char *CNetConnection::ErrorString()
{
	return m_ErrorString;
}

void CNetConnection::SetError(const char *pString)
{
	str_copy(m_ErrorString, pString, sizeof(m_ErrorString));
}

void CNetConnection::Init(NETSOCKET Socket, bool BlockCloseMsg)
{
	Reset();
	ResetStats();

	m_Socket = Socket;
	m_BlockCloseMsg = BlockCloseMsg;
	mem_zero(m_ErrorString, sizeof(m_ErrorString));
}

void CNetConnection::AckChunks(int Ack)
{
	while(1)
	{
		CNetChunkResend *pResend = m_Buffer.First();
		if(!pResend)
			break;

		if(CNetBase::IsSeqInBackroom(pResend->m_Sequence, Ack))
			m_Buffer.PopFirst();
		else
			break;
	}
}

void CNetConnection::SignalResend()
{
	m_Construct.m_Flags |= NET_PACKETFLAG_RESEND;
}

int CNetConnection::Flush()
{
	int NumChunks = m_Construct.m_NumChunks;
	if(!NumChunks && !m_Construct.m_Flags)
		return 0;

	// send of the packets
	m_Construct.m_Ack = m_Ack;
	if(m_UseToken)
	{
		m_Construct.m_Flags |= NET_PACKETFLAG_TOKEN;
		m_Construct.m_Token = m_Token;
	}
	CNetBase::SendPacket(m_Socket, &m_PeerAddr, &m_Construct);

	// update send times
	m_LastSendTime = time_get();

	// clear construct so we can start building a new package
	mem_zero(&m_Construct, sizeof(m_Construct));
	return NumChunks;
}

int CNetConnection::QueueChunkEx(int Flags, int DataSize, const void *pData, int Sequence)
{
	unsigned char *pChunkData;

	// check if we have space for it, if not, flush the connection
	if(m_Construct.m_DataSize + DataSize + NET_MAX_CHUNKHEADERSIZE > (int)sizeof(m_Construct.m_aChunkData))
		Flush();

	// pack all the data
	CNetChunkHeader Header;
	Header.m_Flags = Flags;
	Header.m_Size = DataSize;
	Header.m_Sequence = Sequence;
	pChunkData = &m_Construct.m_aChunkData[m_Construct.m_DataSize];
	pChunkData = Header.Pack(pChunkData);
	mem_copy(pChunkData, pData, DataSize);
	pChunkData += DataSize;

	//
	m_Construct.m_NumChunks++;
	m_Construct.m_DataSize = (int)(pChunkData-m_Construct.m_aChunkData);

	// set packet flags aswell

	if(Flags&NET_CHUNKFLAG_VITAL && !(Flags&NET_CHUNKFLAG_RESEND))
	{
		// save packet if we need to resend
		CNetChunkResend *pResend = m_Buffer.Allocate(sizeof(CNetChunkResend)+DataSize);
		if(pResend)
		{
			pResend->m_Sequence = Sequence;
			pResend->m_Flags = Flags;
			pResend->m_DataSize = DataSize;
			pResend->m_pData = (unsigned char *)(pResend+1);
			pResend->m_FirstSendTime = time_get();
			pResend->m_LastSendTime = pResend->m_FirstSendTime;
			mem_copy(pResend->m_pData, pData, DataSize);
		}
		else
		{
			// out of buffer
			Disconnect("too weak connection (out of buffer)");
			return -1;
		}
	}

	return 0;
}

int CNetConnection::QueueChunk(int Flags, int DataSize, const void *pData)
{
	if(Flags&NET_CHUNKFLAG_VITAL)
		m_Sequence = (m_Sequence+1)%NET_MAX_SEQUENCE;
	return QueueChunkEx(Flags, DataSize, pData, m_Sequence);
}

void CNetConnection::SendControl(int ControlMsg, const void *pExtra, int ExtraSize)
{
	// send the control message
	m_LastSendTime = time_get();
	bool UseToken = m_UseToken && ControlMsg != NET_CTRLMSG_CONNECT;
	CNetBase::SendControlMsg(m_Socket, &m_PeerAddr, m_Ack, UseToken, m_Token, ControlMsg, pExtra, ExtraSize);
}

void CNetConnection::ResendChunk(CNetChunkResend *pResend)
{
	QueueChunkEx(pResend->m_Flags|NET_CHUNKFLAG_RESEND, pResend->m_DataSize, pResend->m_pData, pResend->m_Sequence);
	pResend->m_LastSendTime = time_get();
}

void CNetConnection::Resend()
{
	for(CNetChunkResend *pResend = m_Buffer.First(); pResend; pResend = m_Buffer.Next(pResend))
		ResendChunk(pResend);
}

void CNetConnection::SendConnect()
{
	unsigned char aConnect[512] = {0};
	uint32_to_be(&aConnect[4], m_Token);
	SendControl(NET_CTRLMSG_CONNECT, aConnect, sizeof(aConnect));
}

int CNetConnection::Connect(NETADDR *pAddr)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return -1;

	// init connection
	Reset();
	m_PeerAddr = *pAddr;
	mem_zero(m_ErrorString, sizeof(m_ErrorString));
	m_State = NET_CONNSTATE_CONNECT;
	secure_random_fill(&m_Token, sizeof(m_Token));
	SendConnect();
	return 0;
}

int CNetConnection::Accept(NETADDR *pAddr, unsigned Token)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return -1;

	// init connection
	Reset();
	m_PeerAddr = *pAddr;
	mem_zero(m_ErrorString, sizeof(m_ErrorString));
	m_State = NET_CONNSTATE_ONLINE;
	m_LastRecvTime = time_get();
	m_Token = Token;
	if(g_Config.m_Debug)
	{
		dbg_msg("connection", "connecting online");
	}
	return 0;
}

int CNetConnection::AcceptLegacy(NETADDR *pAddr)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return -1;

	// init connection
	Reset();
	m_PeerAddr = *pAddr;
	mem_zero(m_ErrorString, sizeof(m_ErrorString));
	m_State = NET_CONNSTATE_ONLINE;
	m_LastRecvTime = time_get();

	m_Token = 0;
	m_UseToken = false;
	m_UnknownAck = true;
	m_Sequence = NET_COMPATIBILITY_SEQ;

	if(g_Config.m_Debug)
	{
		dbg_msg("connection", "legacy connecting online");
	}
	return 0;
}

void CNetConnection::Disconnect(const char *pReason)
{
	if(State() == NET_CONNSTATE_OFFLINE)
		return;

	if(m_RemoteClosed == 0)
	{
		if(pReason)
			SendControl(NET_CTRLMSG_CLOSE, pReason, str_length(pReason)+1);
		else
			SendControl(NET_CTRLMSG_CLOSE, 0, 0);

		if(pReason != m_ErrorString)
		{
			if(pReason)
				str_copy(m_ErrorString, pReason, sizeof(m_ErrorString));
			else
				m_ErrorString[0] = 0;
		}
	}

	Reset();
}

int CNetConnection::Feed(CNetPacketConstruct *pPacket, NETADDR *pAddr)
{
	int64 Now = time_get();

	if(m_UseToken)
	{
		if(!(pPacket->m_Flags&NET_PACKETFLAG_TOKEN))
		{
			if(!(pPacket->m_Flags&NET_PACKETFLAG_CONTROL) || pPacket->m_DataSize < 1)
			{
				if(g_Config.m_Debug)
				{
					dbg_msg("connection", "dropping msg without token");
				}
				return 0;
			}
			if(pPacket->m_aChunkData[0] == NET_CTRLMSG_CONNECTACCEPT)
			{
				if(!g_Config.m_ClAllowOldServers)
				{
					if(g_Config.m_Debug)
					{
						dbg_msg("connection", "dropping connect+accept without token");
					}
					return 0;
				}
			}
			else
			{
				if(g_Config.m_Debug)
				{
					dbg_msg("connection", "dropping ctrl msg without token");
				}
				return 0;
			}
		}
		else
		{
			if(pPacket->m_Token != m_Token)
			{
				if(g_Config.m_Debug)
				{
					dbg_msg("connection", "dropping msg with invalid token, wanted=%08x got=%08x", m_Token, pPacket->m_Token);
				}
				return 0;
			}
		}
	}

	// check if actual ack value is valid(own sequence..latest peer ack)
	if(m_Sequence >= m_PeerAck)
	{
		if(pPacket->m_Ack < m_PeerAck || pPacket->m_Ack > m_Sequence)
			return 0;
	}
	else
	{
		if(pPacket->m_Ack < m_PeerAck && pPacket->m_Ack > m_Sequence)
			return 0;
	}
	m_PeerAck = pPacket->m_Ack;

	// check if resend is requested
	if(pPacket->m_Flags&NET_PACKETFLAG_RESEND)
		Resend();

	//
	if(pPacket->m_Flags&NET_PACKETFLAG_CONTROL)
	{
		int CtrlMsg = pPacket->m_aChunkData[0];

		if(CtrlMsg == NET_CTRLMSG_CLOSE)
		{
			if(net_addr_comp(&m_PeerAddr, pAddr) == 0)
			{
				m_State = NET_CONNSTATE_ERROR;
				m_RemoteClosed = 1;

				char aStr[128] = {0};
				if(pPacket->m_DataSize > 1)
				{
					// make sure to sanitize the error string form the other party
					if(pPacket->m_DataSize < 128)
						str_copy(aStr, (char *)&pPacket->m_aChunkData[1], pPacket->m_DataSize);
					else
						str_copy(aStr, (char *)&pPacket->m_aChunkData[1], sizeof(aStr));
					str_sanitize_strong(aStr);
				}

				if(!m_BlockCloseMsg)
				{
					// set the error string
					SetError(aStr);
				}

				if(g_Config.m_Debug)
					dbg_msg("connection", "closed reason='%s'", aStr);
			}
			return 0;
		}
		else
		{
			if(State() == NET_CONNSTATE_CONNECT)
			{
				// connection made
				if(CtrlMsg == NET_CTRLMSG_CONNECTACCEPT)
				{
					if(pPacket->m_Flags&NET_PACKETFLAG_TOKEN)
					{
						if(pPacket->m_DataSize < 1+4)
						{
							if(g_Config.m_Debug)
							{
								dbg_msg("connection", "got short connect+accept, size=%d", pPacket->m_DataSize);
							}
							return 1;
						}
						m_Token = uint32_from_be(&pPacket->m_aChunkData[1]);
					}
					else
					{
						m_UseToken = false;
					}
					m_LastRecvTime = Now;
					m_State = NET_CONNSTATE_ONLINE;
					if(g_Config.m_Debug)
						dbg_msg("connection", "got connect+accept, sending accept. connection online");
				}
			}
		}
	}

	if(State() == NET_CONNSTATE_ONLINE)
	{
		m_LastRecvTime = Now;
		AckChunks(pPacket->m_Ack);
	}

	return 1;
}

int CNetConnection::Update()
{
	int64 Now = time_get();
	int64 Freq = time_freq();

	if(State() == NET_CONNSTATE_OFFLINE || State() == NET_CONNSTATE_ERROR)
		return 0;

	// check for timeout
	if(State() != NET_CONNSTATE_OFFLINE &&
		State() != NET_CONNSTATE_CONNECT &&
		(Now-m_LastRecvTime) > Freq*10)
	{
		m_State = NET_CONNSTATE_ERROR;
		SetError("Timeout");
	}

	// fix resends
	if(m_Buffer.First())
	{
		CNetChunkResend *pResend = m_Buffer.First();

		// check if we have some really old stuff laying around and abort if not acked
		if(Now-pResend->m_FirstSendTime > Freq*10)
		{
			m_State = NET_CONNSTATE_ERROR;
			SetError("Too weak connection (not acked for 10 seconds)");
		}
		else
		{
			// resend packet if we havn't got it acked in 1 second
			if(Now-pResend->m_LastSendTime > Freq)
				ResendChunk(pResend);
		}
	}

	// send keep alives if nothing has happend for 250ms
	if(State() == NET_CONNSTATE_ONLINE)
	{
		if(Now-m_LastSendTime > Freq/2) // flush connection after 500ms if needed
		{
			int NumFlushedChunks = Flush();
			if(NumFlushedChunks && g_Config.m_Debug)
				dbg_msg("connection", "flushed connection due to timeout. %d chunks.", NumFlushedChunks);
		}

		if(Now-m_LastSendTime > Freq)
			SendControl(NET_CTRLMSG_KEEPALIVE, 0, 0);
	}
	else if(State() == NET_CONNSTATE_CONNECT)
	{
		if(Now-m_LastSendTime > Freq/2) // send a new connect every 500ms
			SendConnect();
	}

	return 0;
}
