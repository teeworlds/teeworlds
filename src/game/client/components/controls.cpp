#include <base/math.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>

#include "controls.h"

CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));
}

void CControls::OnReset()
{
	m_LastData.m_Direction = 0;
	m_LastData.m_Hook = 0;
	// simulate releasing the fire button
	if((m_LastData.m_Fire&1) != 0)
		m_LastData.m_Fire++;
	m_LastData.m_Fire &= INPUT_STATE_MASK;
	m_LastData.m_Jump = 0;
	m_InputData = m_LastData;
	
	m_InputDirectionLeft = 0;
	m_InputDirectionRight = 0;
}

void CControls::OnPlayerDeath()
{
	m_LastData.m_WantedWeapon = m_InputData.m_WantedWeapon = 0;
}

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	int *v = (int *)pUserData;
	if(((*v)&1) != pResult->GetInteger(0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_pVariable;
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
		*pSet->m_pVariable = pSet->m_Value;
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet->m_pVariable);
	pSet->m_pControls->m_InputData.m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionLeft, "Move left");
	Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionRight, "Move right");
	Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Jump, "Jump");
	Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Hook, "Hook");
	Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, &m_InputData.m_Fire, "Fire");

	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 1};  Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to hammer"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 2};  Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to gun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 3};  Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to shotgun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 4};  Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to grenade"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 5};  Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to rifle"); }

	{ static CInputSet s_Set = {this, &m_InputData.m_NextWeapon, 0};  Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to next weapon"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_PrevWeapon, 0};  Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to previous weapon"); }
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
    if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
    {
    	CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
        if(g_Config.m_ClAutoswitchWeapons)
        	m_InputData.m_WantedWeapon = pMsg->m_Weapon+1;
    }
}

int CControls::SnapInput(int *pData)
{
	static int64 LastSendTime = 0;
	bool Send = false;
	
	// update player state
	if(m_pClient->m_pChat->IsActive())
		m_InputData.m_PlayerState = PLAYERSTATE_CHATTING;
	else if(m_pClient->m_pMenus->IsActive())
		m_InputData.m_PlayerState = PLAYERSTATE_IN_MENU;
	else
		m_InputData.m_PlayerState = PLAYERSTATE_PLAYING;
	
	if(m_LastData.m_PlayerState != m_InputData.m_PlayerState)
		Send = true;
		
	m_LastData.m_PlayerState = m_InputData.m_PlayerState;
	
	// we freeze the input if chat or menu is activated
	if(m_InputData.m_PlayerState != PLAYERSTATE_PLAYING)
	{
		OnReset();
			
		mem_copy(pData, &m_InputData, sizeof(m_InputData));

		// send once a second just to be sure
		if(time_get() > LastSendTime + time_freq())
			Send = true;
	}
	else
	{
		
		m_InputData.m_TargetX = (int)m_MousePos.x;
		m_InputData.m_TargetY = (int)m_MousePos.y;
		if(!m_InputData.m_TargetX && !m_InputData.m_TargetY)
		{
			m_InputData.m_TargetX = 1;
			m_MousePos.x = 1;
		}
			
		// set direction
		m_InputData.m_Direction = 0;
		if(m_InputDirectionLeft && !m_InputDirectionRight)
			m_InputData.m_Direction = -1;
		if(!m_InputDirectionLeft && m_InputDirectionRight)
			m_InputData.m_Direction = 1;

		// stress testing
		if(g_Config.m_DbgStress)
		{
			float t = Client()->LocalTime();
			mem_zero(&m_InputData, sizeof(m_InputData));

			m_InputData.m_Direction = ((int)t/2)&1;
			m_InputData.m_Jump = ((int)t);
			m_InputData.m_Fire = ((int)(t*10));
			m_InputData.m_Hook = ((int)(t*2))&1;
			m_InputData.m_WantedWeapon = ((int)t)%NUM_WEAPONS;
			m_InputData.m_TargetX = (int)(sinf(t*3)*100.0f);
			m_InputData.m_TargetY = (int)(cosf(t*3)*100.0f);
		}

		// check if we need to send input
		if(m_InputData.m_Direction != m_LastData.m_Direction) Send = true;
		else if(m_InputData.m_Jump != m_LastData.m_Jump) Send = true;
		else if(m_InputData.m_Fire != m_LastData.m_Fire) Send = true;
		else if(m_InputData.m_Hook != m_LastData.m_Hook) Send = true;
		else if(m_InputData.m_PlayerState != m_LastData.m_PlayerState) Send = true;
		else if(m_InputData.m_WantedWeapon != m_LastData.m_WantedWeapon) Send = true;
		else if(m_InputData.m_NextWeapon != m_LastData.m_NextWeapon) Send = true;
		else if(m_InputData.m_PrevWeapon != m_LastData.m_PrevWeapon) Send = true;

		// send at at least 10hz
		if(time_get() > LastSendTime + time_freq()/25)
			Send = true;
	}
	
	// copy and return size	
	m_LastData = m_InputData;
	
	if(!Send)
		return 0;
		
	LastSendTime = time_get();
	mem_copy(pData, &m_InputData, sizeof(m_InputData));
	return sizeof(m_InputData);	
}

void CControls::OnRender()
{
	// update target pos
	if(m_pClient->m_Snap.m_pGameobj && !(m_pClient->m_Snap.m_pGameobj->m_Paused || m_pClient->m_Snap.m_Spectate))
		m_TargetPos = m_pClient->m_LocalCharacterPos + m_MousePos;
}

bool CControls::OnMouseMove(float x, float y)
{
	if(m_pClient->m_Snap.m_pGameobj && m_pClient->m_Snap.m_pGameobj->m_Paused)
		return false;
	m_MousePos += vec2(x, y); // TODO: ugly

	ClampMousePos();

	return true;
}

void CControls::ClampMousePos()
{
	//
	float CameraMaxDistance = 200.0f;
	float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
	float DeadZone = g_Config.m_ClMouseDeadzone;
	float MouseMax = min(CameraMaxDistance/FollowFactor + DeadZone, (float)g_Config.m_ClMouseMaxDistance);
	
	//vec2 camera_offset(0, 0);

	if(m_pClient->m_Snap.m_Spectate)
	{
		if(m_MousePos.x < 200.0f) m_MousePos.x = 200.0f;
		if(m_MousePos.y < 200.0f) m_MousePos.y = 200.0f;
		if(m_MousePos.x > Collision()->GetWidth()*32-200.0f) m_MousePos.x = Collision()->GetWidth()*32-200.0f;
		if(m_MousePos.y > Collision()->GetHeight()*32-200.0f) m_MousePos.y = Collision()->GetHeight()*32-200.0f;
		
		m_TargetPos = m_MousePos;
	}
	else
	{
		float l = length(m_MousePos);
		
		if(l > MouseMax)
		{
			m_MousePos = normalize(m_MousePos)*MouseMax;
			l = MouseMax;
		}
		
		//float offset_amount = max(l-deadzone, 0.0f) * follow_factor;
		//if(l > 0.0001f) // make sure that this isn't 0
			//camera_offset = normalize(mouse_pos)*offset_amount;
	}
}
