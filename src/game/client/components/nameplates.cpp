/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "nameplates.h"
#include "controls.h"

void CNamePlates::RenderNameplate(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CNetObj_PlayerInfo *pPlayerInfo
	)
{
	float IntraTick = Client()->IntraGameTick();
	
	vec2 Position = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), IntraTick);
	
	// render name plate
	if(!pPlayerInfo->m_Local)
	{
		//TextRender()->TextColor
		float a = 1;
		if(g_Config.m_ClNameplatesAlways == 0)
			a = clamp(1-powf(distance(m_pClient->m_pControls->m_TargetPos, Position)/200.0f,16.0f), 0.0f, 1.0f);
			
		const char *pName = m_pClient->m_aClients[pPlayerInfo->m_ClientId].m_aName;
		float tw = TextRender()->TextWidth(0, 28.0f, pName, -1);
		TextRender()->TextColor(1,1,1,a);
		TextRender()->Text(0, Position.x-tw/2.0f, Position.y-60, 28.0f, pName, -1);
		
		if(g_Config.m_Debug) // render client id when in debug aswell
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf),"%d", pPlayerInfo->m_ClientId);
			TextRender()->Text(0, Position.x, Position.y-90, 28.0f, aBuf, -1);
		}

		TextRender()->TextColor(1,1,1,1);
	}
}

void CNamePlates::OnRender()
{
	if (!g_Config.m_ClNameplates)
		return;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// only render active characters
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const void *pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);

		if(pInfo)
		{
			RenderNameplate(
				&m_pClient->m_Snap.m_aCharacters[i].m_Prev,
				&m_pClient->m_Snap.m_aCharacters[i].m_Cur,
				(const CNetObj_PlayerInfo *)pInfo);
		}
	}
}
