/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
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
	m_Ack = 0;
	m_PeerAck = 0;
	m_RemoteClosed = 0;

	m_State = NET_CONNSTATE_OFFLINE;
	m_LastSendTime = 0;
	m_LastRecvTime = 0;
	m_LastUpdateTime = 0;
	m_Token = NET_TOKEN_NONE;
	m_PeerToken = NET_TOKEN_NONE;
	mem_zero(&m_PeerAddr, sizeof(m_PeerAddr));

	m_Buffer.Init();

	mem_zero(&m_Construct, sizeof(m_Construct));
}

void CNetConnection::SetToken(TOKEN Token)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return;

	m_Token = Token;
}

TOKEN CNetConnection::GenerateToken(const NETADDR *pPeerAddr)
{
	return random_int() & NET_TOKEN_MASK;
}

const char *CNetConnection::ErrorString()
{
	return m_ErrorString;
}

void CNetConnection::SetError(const char *pString)
{
	str_copy(m_ErrorString, pString, sizeof(m_ErrorString));
}

void CNetConnection::Init(CNetBase *pNetBase, bool BlockCloseMsg)
{
	Reset();
	ResetStats();

	m_pNetBase = pNetBase;
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

		if(IsSeqInBackroom(pResend->m_Sequence, Ack))
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
	m_Construct.m_Token = m_PeerToken;
	m_pNetBase->SendPacket(&m_PeerAddr, &m_Construct);

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
	if(m_Construct.m_DataSize + DataSize + NET_MAX_CHUNKHEADERSIZE > (int)sizeof(m_Construct.m_aChunkData) || m_Construct.m_NumChunks == NET_MAX_PACKET_CHUNKS)
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
	m_pNetBase->SendControlMsg(&m_PeerAddr, m_PeerToken, m_Ack, ControlMsg, pExtra, ExtraSize);
}

void CNetConnection::SendPacketConnless(const char *pData, int DataSize)
{
	m_pNetBase->SendPacketConnless(&m_PeerAddr, m_PeerToken, m_Token, pData, DataSize);
}

void CNetConnection::SendControlWithToken(int ControlMsg)
{
	m_LastSendTime = time_get();
	m_pNetBase->SendControlMsgWithToken(&m_PeerAddr, m_PeerToken, 0, ControlMsg, m_Token, true);
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

int CNetConnection::Connect(NETADDR *pAddr)
{
	if(State() != NET_CONNSTATE_OFFLINE)
		return -1;

	// init connection
	Reset();
	m_LastRecvTime = time_get();
	m_PeerAddr = *pAddr;
	m_PeerToken = NET_TOKEN_NONE;
	SetToken(GenerateToken(pAddr));
	mem_zero(m_ErrorString, sizeof(m_ErrorString));
	m_State = NET_CONNSTATE_TOKEN;
	SendControlWithToken(NET_CTRLMSG_TOKEN);
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

	int64 Now = time_get();

	if(pPacket->m_Token == NET_TOKEN_NONE || pPacket->m_Token != m_Token)
		return 0;

	// check if resend is requested
	if(pPacket->m_Flags&NET_PACKETFLAG_RESEND)
		Resend();

	if(pPacket->m_Flags&NET_PACKETFLAG_CONNLESS)
		return 1;

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

				char Str[128] = {0};
				if(pPacket->m_DataSize > 1)
				{
					// make sure to sanitize the error string form the other party
					if(pPacket->m_DataSize < 128)
						str_copy(Str, (char *)&pPacket->m_aChunkData[1], pPacket->m_DataSize);
					else
						str_copy(Str, (char *)&pPacket->m_aChunkData[1], sizeof(Str));
					str_sanitize_strong(Str);
				}

				if(!m_BlockCloseMsg)
				{
					// set the error string
					SetError(Str);
				}

				if(Config()->m_Debug)
					dbg_msg("conn", "closed reason='%s'", Str);
			}
			return 0;
		}
		else
		{
			if(CtrlMsg == NET_CTRLMSG_TOKEN)
			{
				m_PeerToken = pPacket->m_ResponseToken;

				if(State() == NET_CONNSTATE_TOKEN)
				{
					m_LastRecvTime = Now;
					m_State = NET_CONNSTATE_CONNECT;
					SendControlWithToken(NET_CTRLMSG_CONNECT);
					dbg_msg("connection", "got token, replying, token=%x mytoken=%x", m_PeerToken, m_Token);
				}
				else if(Config()->m_Debug)
					dbg_msg("connection", "got token, token=%x", m_PeerToken);
			}
			else
			{
				if(State() == NET_CONNSTATE_OFFLINE)
				{
					if(CtrlMsg == NET_CTRLMSG_CONNECT)
					{
						// send response and init connection
						TOKEN Token = m_Token;
						Reset();
						mem_zero(m_ErrorString, sizeof(m_ErrorString));
						m_State = NET_CONNSTATE_PENDING;
						m_PeerAddr = *pAddr;
						m_PeerToken = pPacket->m_ResponseToken;
						m_Token = Token;
						m_LastSendTime = Now;
						m_LastRecvTime = Now;
						m_LastUpdateTime = Now;
						SendControl(NET_CTRLMSG_ACCEPT, 0, 0);
						if(Config()->m_Debug)
							dbg_msg("connection", "got connection, sending accept");
					}
				}
				else if(State() == NET_CONNSTATE_CONNECT)
				{
					// connection made
					if(CtrlMsg == NET_CTRLMSG_ACCEPT)
					{
						m_LastRecvTime = Now;
						m_State = NET_CONNSTATE_ONLINE;
						if(Config()->m_Debug)
							dbg_msg("connection", "got accept. connection online");
					}
				}
			}
		}
	}
	else
	{
		if(State() == NET_CONNSTATE_PENDING)
		{
			m_LastRecvTime = Now;
			m_State = NET_CONNSTATE_ONLINE;
			if(Config()->m_Debug)
				dbg_msg("connection", "connecting online");
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

	if(State() == NET_CONNSTATE_OFFLINE || State() == NET_CONNSTATE_ERROR)
		return 0;

	// check for timeout
	if(State() != NET_CONNSTATE_OFFLINE &&
		State() != NET_CONNSTATE_TOKEN &&
		(Now-m_LastRecvTime) > time_freq()*10)
	{
		m_State = NET_CONNSTATE_ERROR;
		SetError("Timeout");
	}
	else if(State() == NET_CONNSTATE_TOKEN && (Now - m_LastRecvTime) > time_freq() * 5)
	{
		m_State = NET_CONNSTATE_ERROR;
		SetError("Unable to connect to the server");
	}

	// fix resends
	if(m_Buffer.First())
	{
		CNetChunkResend *pResend = m_Buffer.First();

		// check if we have some really old stuff laying around and abort if not acked
		if(Now-pResend->m_FirstSendTime > time_freq()*10)
		{
			m_State = NET_CONNSTATE_ERROR;
			SetError("Too weak connection (not acked for 10 seconds)");
		}
		else
		{
			// resend packet if we haven't got it acked in 1 second
			if(Now-pResend->m_LastSendTime > time_freq())
				ResendChunk(pResend);
		}
	}

	// send keep alives if nothing has happend for 250ms
	if(State() == NET_CONNSTATE_ONLINE)
	{
		if(time_get()-m_LastSendTime > time_freq()/2) // flush connection after 500ms if needed
		{
			int NumFlushedChunks = Flush();
			if(NumFlushedChunks && Config()->m_Debug)
				dbg_msg("connection", "flushed connection due to timeout. %d chunks.", NumFlushedChunks);
		}

		if(time_get()-m_LastSendTime > time_freq())
			SendControl(NET_CTRLMSG_KEEPALIVE, 0, 0);
	}
	else if(State() == NET_CONNSTATE_TOKEN)
	{
		if(time_get()-m_LastSendTime > time_freq()/2) // send a new token request every 500ms
			SendControlWithToken(NET_CTRLMSG_TOKEN);
	}
	else if(State() == NET_CONNSTATE_CONNECT)
	{
		if(time_get()-m_LastSendTime > time_freq()/2) // send a new connect every 500ms
			SendControlWithToken(NET_CTRLMSG_CONNECT);
	}
	else if(State() == NET_CONNSTATE_PENDING)
	{
		if(time_get()-m_LastSendTime > time_freq()/2) // send a new connect/accept every 500ms
			SendControl(NET_CTRLMSG_ACCEPT, 0, 0);
	}

	return 0;
}

int CNetConnection::IsSeqInBackroom(int Seq, int Ack)
{
	int Bottom = (Ack - NET_MAX_SEQUENCE / 2);
	if(Bottom < 0)
	{
		if(Seq <= Ack)
			return 1;
		if(Seq >= (Bottom + NET_MAX_SEQUENCE))
			return 1;
	}
	else
	{
		if(Seq <= Ack && Seq >= Bottom)
			return 1;
	}

	return 0;
}
