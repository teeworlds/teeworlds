/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

#include <engine/shared/protocol.h>
#include <generated/protocol.h>

class CEventHandler
{
private:
	/* Types */
	struct CEventInfo
	{
		const CNetEvent_Common *m_pEvent;
		int m_Type;
		int m_Size;
		int m_ClientMask;
	};

	/* Constants */
	static const int MAX_EVENTS = MAX_CLIENTS * 8;
	static const int MAX_DATASIZE = MAX_EVENTS * 32;
	static const int MAX_SNAP_RANGE = 1500;

	/* Identity */
	class CGameContext *m_pGameServer;

	/* State */
	CEventInfo m_aEvents[MAX_EVENTS];
	unsigned char m_aData[MAX_DATASIZE];
	int m_NumEvents;
	int m_DataSize;

	/* Functions */
	CNetEvent_Common *CreateEvent(int Type, int Size, int Mask);
	void SnapEvent(const CEventInfo *pInfo, int EventId) const;

public:
	/* Constructor */
	CEventHandler();

	/* Getters / Setters */
	CGameContext *GameServer() const { return m_pGameServer; }
	void SetGameServer(CGameContext *pGameServer);

	/* Functions */
	template <class T> T *Create(int Type, int Mask=-1) { return (T *) CreateEvent(Type, sizeof(T), Mask); }
	void Clear();
	void Snap(int SnappingClient) const;
};

#endif
