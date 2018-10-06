#include "network.h"

#include "packer.h"
#include "protocol.h"


unsigned DeriveLegacyToken(unsigned Token)
{
	// Clear the highest bit to get rid of negative numbers.
	Token &= ~0x80000000;
	if(Token < 2)
	{
		Token += 2;
	}
	return Token;
}

static void AddChunk(CNetPacketConstruct *pPacket, int Sequence, const void *pData, int DataSize)
{
	dbg_assert(pPacket->m_DataSize + NET_MAX_CHUNKHEADERSIZE + DataSize <= (int)sizeof(pPacket->m_aChunkData), "too much data");

	CNetChunkHeader Header;
	Header.m_Flags = Sequence >= 0 ? NET_CHUNKFLAG_VITAL : 0;
	Header.m_Size = DataSize;
	Header.m_Sequence = Sequence >= 0 ? Sequence : 0;

	unsigned char *pPacketData = &pPacket->m_aChunkData[pPacket->m_DataSize];
	const unsigned char *pChunkStart = pPacketData;
	pPacketData = Header.Pack(pPacketData);
	pPacket->m_DataSize += pPacketData - pChunkStart;
	mem_copy(pPacketData, pData, DataSize);
	pPacket->m_DataSize += DataSize;
	pPacket->m_NumChunks++;
}

static int System(int MsgID)
{
	return (MsgID << 1) | 1;
}

void ConstructLegacyHandshake(CNetPacketConstruct *pPacket1, CNetPacketConstruct *pPacket2, unsigned LegacyToken)
{
	pPacket1->m_Flags = NET_PACKETFLAG_CONTROL;
	pPacket1->m_Ack = 0;
	pPacket1->m_NumChunks = 0;
	pPacket1->m_DataSize = 1;
	pPacket1->m_aChunkData[0] = NET_CTRLMSG_CONNECTACCEPT;

	pPacket2->m_Flags = 0;
	pPacket2->m_Ack = 0;
	pPacket2->m_NumChunks = 0;
	pPacket2->m_DataSize = 0;

	CPacker Packer;

	Packer.Reset();
	Packer.AddInt(System(NETMSG_MAP_CHANGE));
	Packer.AddString("dm1", 0);
	Packer.AddInt(0xf2159e6e);
	Packer.AddInt(5805);
	AddChunk(pPacket2, 1, Packer.Data(), Packer.Size());

	Packer.Reset();
	Packer.AddInt(System(NETMSG_CON_READY));
	AddChunk(pPacket2, 2, Packer.Data(), Packer.Size());

	for(int i = -2; i <= 0; i++)
	{
		Packer.Reset();
		Packer.AddInt(System(NETMSG_SNAPEMPTY));
		Packer.AddInt(LegacyToken + i);
		Packer.AddInt(i == -2 ? LegacyToken + i + 1 : 1);
		AddChunk(pPacket2, -1, Packer.Data(), Packer.Size());
	}
}

bool DecodeLegacyHandshake(const void *pData, int DataSize, unsigned *pLegacyToken)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pData, DataSize);
	int MsgID = Unpacker.GetInt();
	int Token = Unpacker.GetInt();
	if(Unpacker.Error() || MsgID != System(NETMSG_INPUT))
	{
		return false;
	}
	*pLegacyToken = Token;
	return true;
}
