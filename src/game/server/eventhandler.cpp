/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "eventhandler.h"
#include "gamecontext.h"
#include "player.h"

CEventHandler::CEventHandler()
{
	m_pGameServer = 0;
	m_NumEvents = 0;
	m_DataSize = 0;
}

void CEventHandler::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
}

CNetEvent_Common *CEventHandler::CreateEvent(int Type, int Size, int Mask)
{
	if(Size < sizeof(CNetEvent_Common))
		return 0;
	if(m_NumEvents == MAX_EVENTS || m_DataSize+Size > MAX_DATASIZE)
		return 0;

	CNetEvent_Common *pEvent = (CNetEvent_Common *) &m_aData[m_DataSize];
	m_DataSize += Size;

	CEventInfo *pInfo = &m_aEvents[m_NumEvents++];
	pInfo->m_pEvent = pEvent;
	pInfo->m_Type = Type;
	pInfo->m_Size = Size;
	pInfo->m_ClientMask = Mask;

	return pEvent;
}

void CEventHandler::Clear()
{
	m_NumEvents = 0;
	m_DataSize = 0;
}

void CEventHandler::Snap(int SnappingClient) const
{
	for(int i = 0; i < m_NumEvents; i++)
	{
		const CEventInfo *pInfo = &m_aEvents[i];

		if(SnappingClient == -1)
			SnapEvent(pInfo, i);
		else if(CmaskIsSet(pInfo->m_ClientMask, SnappingClient))
		{
			vec2 ViewPos = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos;
			vec2 EventPos = vec2(pInfo->m_pEvent->m_X, pInfo->m_pEvent->m_Y);
			if(distance(ViewPos, EventPos) <= MAX_SNAP_RANGE)
				SnapEvent(pInfo, i);
		}
	}
}

void CEventHandler::SnapEvent(const CEventInfo *pInfo, int EventId) const
{
	void *d = GameServer()->Server()->SnapNewItem(pInfo->m_Type, EventId, pInfo->m_Size);
	if(d)
		mem_copy(d, pInfo->m_pEvent, pInfo->m_Size);
}
