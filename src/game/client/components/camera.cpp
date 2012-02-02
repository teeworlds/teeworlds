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
	m_CamType = CAMTYPE_UNDEFINED;
	m_RotationCenter = vec2(0.0f, 0.0f);
	m_MenuCenter = vec2(0.0f, 0.0f);

	m_Positions[POS_START] = vec2(500.0f, 500.0f);
	m_Positions[POS_INTERNET] = vec2(500.0f, 500.0f);
	m_Positions[POS_LAN] = vec2(1000.0f, 1000.0f);
	m_Positions[POS_FAVORITES] = vec2(2000.0f, 500.0f);
	m_Positions[POS_DEMOS] = vec2(1500.0f, 1000.0f);
	m_Positions[POS_SETTINGS] = vec2(1000.0f, 500.0f);

	m_CurrentPosition = -1;
}

void CCamera::OnRender()
{
	if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_Zoom = 1.0f;

		// update camera center
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
		{
			if(m_CamType != CAMTYPE_SPEC)
			{
				m_pClient->m_pControls->m_MousePos = m_PrevCenter;
				m_pClient->m_pControls->ClampMousePos();
				m_CamType = CAMTYPE_SPEC;
			}
			m_Center = m_pClient->m_pControls->m_MousePos;
		}
		else
		{
			if(m_CamType != CAMTYPE_PLAYER)
			{
				m_pClient->m_pControls->ClampMousePos();
				m_CamType = CAMTYPE_PLAYER;
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

			if(m_pClient->m_Snap.m_SpecInfo.m_Active)
				m_Center = m_pClient->m_Snap.m_SpecInfo.m_Position + CameraOffset;
			else
				m_Center = m_pClient->m_LocalCharacterPos + CameraOffset;
		}
	}
	else
	{
		m_Zoom = 0.7f;
		static vec2 Dir = vec2(1.0f, 0.0f);

		if(distance(m_Center, m_RotationCenter) <= (float)g_Config.m_ClRotationRadius+0.5f)
		{
			// do little rotation
			float RotPerTick = 360.0f/(float)g_Config.m_ClRotationSpeed * Client()->RenderFrameTime();
			Dir = rotate(Dir, RotPerTick);
			m_Center = m_RotationCenter+Dir*(float)g_Config.m_ClRotationRadius;
		}
		else
		{
			Dir = normalize(m_RotationCenter-m_Center);
			m_Center += Dir*(500.0f*Client()->RenderFrameTime());
			Dir = normalize(m_Center-m_RotationCenter);
		}
	}

	m_PrevCenter = m_Center;
}

void CCamera::ChangePosition(int PositionNumber)
{
	m_RotationCenter = m_Positions[PositionNumber];
	m_CurrentPosition = PositionNumber;
}

int CCamera::GetCurrentPosition()
{
	return m_CurrentPosition;
}

void CCamera::ConSetPosition(IConsole::IResult *pResult, void *pUserData)
{
	CCamera *pSelf = (CCamera *)pUserData;
	int PositionNumber = clamp(pResult->GetInteger(0), 0, 4);
	vec2 Position = vec2(pResult->GetInteger(1)*32.0f+16.0f, pResult->GetInteger(2)*32.0f+16.0f);
	pSelf->m_Positions[PositionNumber] = Position;

	// update
	if(pSelf->GetCurrentPosition() == PositionNumber)
		pSelf->ChangePosition(PositionNumber);
}

void CCamera::OnConsoleInit()
{
	Console()->Register("set_position", "iii", CFGFLAG_CLIENT, ConSetPosition, this, "Sets the rotation position");
}

void CCamera::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_OFFLINE)
		m_MenuCenter = m_Center;
	else if(NewState != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		m_Center = m_MenuCenter;
}
