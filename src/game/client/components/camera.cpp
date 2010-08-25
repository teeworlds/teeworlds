#include <engine/shared/config.h>

#include <base/math.h>
#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include "camera.h"
#include "controls.h"

const float ZoomStep = 0.75f;
void CCamera::ConZoomPlus(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	if(g_Config.m_ClRaceCheats == 1 || ((CCamera *)pUserData)->m_pClient->m_IsRace)
		((CCamera *)pUserData)->m_Zoom *= 1/ZoomStep;
}
void CCamera::ConZoomMinus(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	if(g_Config.m_ClRaceCheats == 1 || ((CCamera *)pUserData)->m_pClient->m_IsRace)
		((CCamera *)pUserData)->m_Zoom *= ZoomStep;
}
void CCamera::ConZoomReset(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	if(g_Config.m_ClRaceCheats == 1 || ((CCamera *)pUserData)->m_pClient->m_IsRace)
		((CCamera *)pUserData)->m_Zoom = 1.0f;
}
void CCamera::ConCameraFree(IConsole::IResult *pResult, void *pUserData, int ClientID) {
	if(!((CCamera *)pUserData)->m_pClient->m_Snap.m_Spectate && (g_Config.m_ClRaceCheats == 1 || ((CCamera *)pUserData)->m_pClient->m_IsRace))
		((CCamera *)pUserData)->m_Free = ((CCamera *)pUserData)->m_Free ? false : true;
}


CCamera::CCamera()
{
	m_Zoom = 1.0f;
	m_Free = false;
	m_Follow = -1;
}

void CCamera::OnRender()
{
	//vec2 center;
	
	// update camera center		
	if(m_pClient->m_Snap.m_Spectate || m_Free) {
		if(m_Follow == -1) {
			m_Center = m_pClient->m_pControls->m_MousePos;
		} else {
			CGameClient::CSnapState::CCharacterInfo* Char = getCharacter(m_Follow);
			if(Char == 0) {
				m_Follow = -1;
				m_Free = false;
				OnRender();
			} else {
				float l = length(m_pClient->m_pControls->m_MousePos);
				float DeadZone = g_Config.m_ClMouseDeadzone;
				float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
				vec2 CameraOffset(0, 0);

				float OffsetAmount = max(l-DeadZone, 0.0f) * FollowFactor;
				if(l > 0.0001f) // make sure that this isn't 0
					CameraOffset = normalize(m_pClient->m_pControls->m_MousePos)*OffsetAmount;
				vec2 Position = mix(vec2(Char->m_Prev.m_X, Char->m_Prev.m_Y), vec2(Char->m_Cur.m_X, Char->m_Cur.m_Y), Client()->IntraGameTick());
				m_Center = Position + CameraOffset;
			}
		}
	}
	else
	{
		float l = length(m_pClient->m_pControls->m_MousePos);
		float DeadZone = g_Config.m_ClMouseDeadzone;
		float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
		vec2 CameraOffset(0, 0);
		float OffsetAmount = max(l-DeadZone, 0.0f) * FollowFactor;
		if(l > 0.0001f) // make sure that this isn't 0
			CameraOffset = normalize(m_pClient->m_pControls->m_MousePos)*OffsetAmount;
		m_Center = m_pClient->m_LocalCharacterPos + CameraOffset;
	}
}

void CCamera::OnConsoleInit()
{
	Console()->Register("zoom+", "", CFGFLAG_CLIENT, ConZoomPlus, this, "Zoom increse", 0);
	Console()->Register("zoom-", "", CFGFLAG_CLIENT, ConZoomMinus, this, "Zoom decrese", 0);
	Console()->Register("zoom", "", CFGFLAG_CLIENT, ConZoomReset, this, "Zoom reset", 0);
	Console()->Register("camera_free", "", CFGFLAG_CLIENT, ConCameraFree, this, "Free camera On/Off", 0);
}

CGameClient::CSnapState::CCharacterInfo* CCamera::getCharacter(int Id) {
	const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, Id);
	if(pInfo != 0 && pInfo->m_Team != -1) {
		return &m_pClient->m_Snap.m_aCharacters[Id];
	}
	return 0;
}

bool CCamera::SetFollow(int Id) {
	if(m_pClient->m_Snap.m_Spectate) {
		if(Id == -1) {
			m_Follow = -1;
			return false;
		}
		if(Id >= 0 || Id < MAX_CLIENTS) {
			m_Follow = Id;
			return true;
		} 
	}
	return false;
}