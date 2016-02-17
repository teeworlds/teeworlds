/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/teecomp.h>

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


	float FontSize = 18.0f + 20.0f * g_Config.m_ClNameplatesSize / 100.0f;
	// render name plate
	if(!pPlayerInfo->m_Local)
	{
		//TextRender()->TextColor
		float a = 1.0f;
		if(g_Config.m_ClNameplatesAlways == 0)
			a = clamp(1-powf(distance(m_pClient->m_pControls->m_TargetPos, Position)/200.0f,16.0f), 0.0f, 1.0f);

		char aName[256];
		if(!g_Config.m_TcNameplateScore)
			str_format(aName, 256, "%s", m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName);
		else
			str_format(aName, 256, "%s (%d)", m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName, pPlayerInfo->m_Score);
		float tw = TextRender()->TextWidth(0, FontSize, aName, -1);

		bool IsTeamplay;
		IsTeamplay = m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS;
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.5f*a);
		if(g_Config.m_ClNameplatesTeamcolors && IsTeamplay)
		{
			vec3 Col = CTeecompUtils::GetTeamColor(
				m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_Team,
				m_pClient->m_Snap.m_pLocalInfo ? m_pClient->m_Snap.m_pLocalInfo->m_Team : TEAM_RED,
				g_Config.m_TcColoredTeesTeam1,
				g_Config.m_TcColoredTeesTeam2,
				g_Config.m_TcColoredTeesMethod);
			TextRender()->TextColor(Col.r, Col.g, Col.b, a);
		}
		else // FFA or no colored plates
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, a);
		TextRender()->Text(0, Position.x-tw/2.0f, Position.y-FontSize-38.0f, FontSize, aName, -1);
		
		if(g_Config.m_Debug || g_Config.m_ClNameplateClientID) // render client id when in debug aswell
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf),"%d", pPlayerInfo->m_ClientID);
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, Position.x-tw/2.0f, Position.y-(FontSize*2.0f)-38.0f, FontSize, aBuf, -1);
		}

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}
}

void CNamePlates::OnRender()
{
	if (!g_Config.m_ClNameplates || g_Config.m_ClClearAll)
		return;
	
	TextRender()->BatchBegin();

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
	
	TextRender()->BatchEnd();
}
