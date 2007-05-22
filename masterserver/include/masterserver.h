#ifndef _MASTERSERVER_H
#define _MASTERSERVER_H

#include <baselib/network.h>
#include "serverinfo.h"

using namespace baselib;

#define HEARTBEAT_SIZE 216
#define HEARTBEAT_SIGNATURE 'TWHB'
#define HEARTBEAT_LIFETIME 10
#define MAXSERVERS 1024
#define SERVERINFOOUT_SIZE 212
#define SERVERINFOHEADER_SIZE 12
#define MASTERSERVER_VERSION 0

class CMasterServer
{
	CServerInfo m_Servers[MAXSERVERS];
	int m_ServerCount;
	socket_udp4 m_UDPSocket;
	socket_tcp4 m_TCPSocket;
	int m_CurrentTime;
	char m_ServerListPacket[MAXSERVERS * SERVERINFOOUT_SIZE + SERVERINFOHEADER_SIZE];
	int m_ServerListPacketSize;
	bool m_ServerListPacketIsOld;

	void ListenForServerListPolls();
	void BuildServerListPacket();
    void ListenForHeartBeats();
    void ProcessHeartBeat(CServerInfo info);
    CServerInfo *FindServerInfo(int32 ip, int32 port);
    CServerInfo *GetUnusedSlot();
    void CleanUpServerList();
public:
	CMasterServer()
	{
		m_ServerCount = 0;
		m_ServerListPacketIsOld = true;
	}

	void Init(int port);
	void Shutdown(); 
		
	void Tick();
};

#endif
