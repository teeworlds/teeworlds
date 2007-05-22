#ifndef _SERVERINFO_H
#define _SERVERINFO_H

#include <baselib/network.h>

#include "common.h"
#include "network.h"

class CServerInfo
{
	int32 m_Version;
	int32 m_IP;
	int32 m_Port;
	int32 m_Players;
	int32 m_MaxPlayers;
	char m_Name[128];
	char m_Map[64];

	int m_LastRefresh;

public:
	int32 IP() const { return m_IP; }
	int32 Port() const { return m_Port; }
	int32 Players() const { return m_Players; }
	int32 MaxPlayers() const { return m_MaxPlayers; };
	const char *Name() const { return m_Name; }
	const char *Map() const { return m_Map; }

	void Refresh(int time) { m_LastRefresh = time; }
	int LastRefresh() { return m_LastRefresh; }

	void SetAddress(baselib::netaddr4 *addr)
	{
		m_IP = addr->ip[0] << 24;
		m_IP |= addr->ip[1] << 16;
		m_IP |= addr->ip[2] << 8;
		m_IP |= addr->ip[3];

		m_Port = addr->port;
	}
	
	char *Serialize(char *buffer) const
	{
		buffer = WriteInt32(buffer, m_Version);
		buffer = WriteInt32(buffer, m_IP);
		buffer = WriteInt32(buffer, m_Port);
		buffer = WriteInt32(buffer, m_Players);
		buffer = WriteInt32(buffer, m_MaxPlayers);
		buffer = WriteFixedString(buffer, m_Name, sizeof(m_Name));
		buffer = WriteFixedString(buffer, m_Map, sizeof(m_Map));

		return buffer;
	}

	char *Deserialize(char *buffer)
	{
		buffer = ReadInt32(buffer, &m_Version);
		buffer = ReadInt32(buffer, &m_IP);
		buffer = ReadInt32(buffer, &m_Port);
		buffer = ReadInt32(buffer, &m_Players);
		buffer = ReadInt32(buffer, &m_MaxPlayers);
		buffer = ReadFixedString(buffer, m_Name, sizeof(m_Name));
		buffer = ReadFixedString(buffer, m_Map, sizeof(m_Map));

		return buffer;
	}
};

#endif
