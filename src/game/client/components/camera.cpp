/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <base/math.h>
#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include "camera.h"
#include "controls.h"

CCamera::CCamera()
{
	m_WasSpectator = false;
}

void CCamera::OnRender()
{
	//vec2 center;
	m_Zoom = 1.0f;

	// update camera center		
	if(m_pClient->m_Snap.m_Spectate && (!m_pClient->m_Snap.m_pSpectatorInfo || m_pClient->m_Snap.m_pSpectatorInfo->m_SpectatorID == SPEC_FREEVIEW))
	{
		if(!m_WasSpectator)
		{
			m_pClient->m_pControls->ClampMousePos();
			m_WasSpectator = true;
		}
		m_Center = m_pClient->m_pControls->m_MousePos;
	}
	else
	{
		if(m_WasSpectator)
		{
			m_pClient->m_pControls->ClampMousePos();
			m_WasSpectator = false;
		}

		vec2 CameraOffset(0, 0);

		float l = length(m_pClient->m_pControls->m_MousePos);
		if(l > 0.0001f) // make sure that this isn't 0
		{
			float DeadZone = g_Config.m_ClMouseDeadzone;
			float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
			float OffsetAmount = max(l-DeadZone, 0.0f) * FollowFactor;

			CameraOffset = normalize(m_pClient->m_pControls->m_MousePos)*OffsetAmount;
		}
		
		if(m_pClient->m_Snap.m_Spectate)
			m_Center = m_pClient->m_Snap.m_SpectatorPos + CameraOffset;
		else
			m_Center = m_pClient->m_LocalCharacterPos + CameraOffset;
	}
}
