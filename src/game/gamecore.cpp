/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "gamecore.h"

const char *CTuningParams::s_apNames[] =
{
	#define MACRO_TUNING_PARAM(Name,ScriptName,Value) #ScriptName,
	#include "tuning.h"
	#undef MACRO_TUNING_PARAM
};


bool CTuningParams::Set(int Index, float Value)
{
	if(Index < 0 || Index >= Num())
		return false;
	((CTuneParam *)this)[Index] = Value;
	return true;
}

bool CTuningParams::Get(int Index, float *pValue) const
{
	if(Index < 0 || Index >= Num())
		return false;
	*pValue = (float)((CTuneParam *)this)[Index];
	return true;
}

bool CTuningParams::Set(const char *pName, float Value)
{
	for(int i = 0; i < Num(); i++)
		if(str_comp_nocase(pName, GetName(i)) == 0)
			return Set(i, Value);
	return false;
}

bool CTuningParams::Get(const char *pName, float *pValue) const
{
	for(int i = 0; i < Num(); i++)
		if(str_comp_nocase(pName, GetName(i)) == 0)
			return Get(i, pValue);
	return false;
}


float VelocityRamp(float Value, float Start, float Range, float Curvature)
{
	if(Value < Start)
		return 1.0f;
	return 1.0f/powf(Curvature, (Value-Start)/Range);
}

const float CCharacterCore::PHYS_SIZE = 28.0f;

void CCharacterCore::Init(CWorldCore *pWorld, CCollision *pCollision)
{
	m_pWorld = pWorld;
	m_pCollision = pCollision;
	m_DivingGear = false;
}

void CCharacterCore::Reset()
{
	m_Pos = vec2(0,0);
	m_Vel = vec2(0,0);
	m_HookDragVel = vec2(0,0);
	m_HookPos = vec2(0,0);
	m_HookDir = vec2(0,0);
	m_HarpoonDragVel = vec2(0,0);
	m_HookTick = 0;
	m_HookState = HOOK_IDLE;
	m_HookedPlayer = -1;
	m_Jumped = 0;
	m_TriggeredEvents = 0;
	m_Death = false;
	m_DivingGear = false;
}

void CCharacterCore::HandleSwimming(vec2 TargetDirection)
{
	vec2 ProposedVel, MaxSpeed;
	//MaxSpeed.x = m_DivingGear ? TargetDirection.x * m_pWorld->m_Tuning.m_LiquidDivingCursorMaxSpeed : TargetDirection.x * m_pWorld->m_Tuning.m_LiquidCursorMaxSpeed;
	//MaxSpeed.y = m_DivingGear ? TargetDirection.y * m_pWorld->m_Tuning.m_LiquidDivingCursorMaxSpeed : TargetDirection.y * m_pWorld->m_Tuning.m_LiquidCursorMaxSpeed;
	ProposedVel.x = m_Vel.x + TargetDirection.x;
	ProposedVel.y = m_Vel.y + TargetDirection.y;

	if (length(ProposedVel) > m_pWorld->m_Tuning.m_LiquidDivingCursorMaxSpeed)
	{
		ProposedVel = normalize(ProposedVel);
		ProposedVel *= m_pWorld->m_Tuning.m_LiquidDivingCursorMaxSpeed;
	}
	m_Vel = ProposedVel;
	/*if (TargetDirection.x > 0)
	{
		if (!(m_Vel.x > MaxSpeed.x))
		{
			m_Vel.x = clamp(ProposedVel.x, -100.0f, MaxSpeed.x);
		}
	}
	else if (TargetDirection.x < 0)
	{
		if (!(m_Vel.x < MaxSpeed.x))
		{
			m_Vel.x = clamp(ProposedVel.x, MaxSpeed.x, 100.0f);
		}
	}
	if (TargetDirection.y > 0)
	{
		if (!(m_Vel.y > MaxSpeed.y))
		{
			m_Vel.y = clamp(ProposedVel.y, -100.0f, MaxSpeed.y);
		}
	}
	else if (TargetDirection.y < 0)
	{
		if (!(m_Vel.y < MaxSpeed.y))
		{
			m_Vel.y = clamp(ProposedVel.y, MaxSpeed.y, 100.0f);
		}
	}*/
}
void CCharacterCore::Tick(bool UseInput)
{
	m_TriggeredEvents = 0;

	// get ground state
	const bool Grounded =
		m_pCollision->CheckPoint(m_Pos.x+PHYS_SIZE/2, m_Pos.y+PHYS_SIZE/2+5)
		|| m_pCollision->CheckPoint(m_Pos.x-PHYS_SIZE/2, m_Pos.y+PHYS_SIZE/2+5);

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	if(!(IsInWater()))
		m_Vel.y += m_pWorld->m_Tuning.m_Gravity;
	//else
		//m_Vel.y += 0.1f * m_pWorld->m_Tuning.m_Gravity;
		
	float MaxSpeed = Grounded ? m_pWorld->m_Tuning.m_GroundControlSpeed : m_pWorld->m_Tuning.m_AirControlSpeed;
	if (IsInWater())
	{
		MaxSpeed = m_pWorld->m_Tuning.m_LiquidDivingMaxSpeed;
	}
	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;
	m_DirectionVertical = m_Input.m_DirectionVertical;
	// handle input
	if(UseInput)
	{
		m_Direction = m_Input.m_Direction;
		
		m_Angle = (int)(angle(vec2(m_Input.m_TargetX, m_Input.m_TargetY))*256.0f);

		// handle jump
		if(m_Input.m_Jump)
		{
			
			if (!( IsInWater() && !IsFloating() ))
			{
				if (!(m_Jumped & 1))
				{
					if (Grounded)
					{
						m_TriggeredEvents |= COREEVENTFLAG_GROUND_JUMP;
						m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse;
						m_Jumped |= 1;
					}
					else if (!(m_Jumped & 2))
					{
						m_TriggeredEvents |= COREEVENTFLAG_AIR_JUMP;
						m_Vel.y = -m_pWorld->m_Tuning.m_AirJumpImpulse;
						m_Jumped |= 3;
					}
				}
			}
			else
			{
				if (m_pWorld->m_Tuning.m_LiquidDivingCursor)
					if(!(m_pWorld->m_Tuning.m_LiquidCursorRequireDiving && !m_DivingGear))
						HandleSwimming(TargetDirection);
				//if (m_DivingGear)
				//{
					// (m_pWorld->m_Tuning.m_LiquidDivingCursor)
						//(TargetDirection);
					//else
						//sm_Vel.y = SaturatedAdd(-m_pWorld->m_Tuning.m_LiquidSwimmingTopAccel, 1.0f, m_Vel.y, -m_pWorld->m_Tuning.m_Gravity - m_pWorld->m_Tuning.m_LiquidSwimmingAccel);
				//}
			}
		}
		else
			m_Jumped &= ~1;

		// handle hook
		if(m_Input.m_Hook)
		{
			if(m_HookState == HOOK_IDLE)
			{
				m_HookState = HOOK_FLYING;
				m_HookPos = m_Pos+TargetDirection*PHYS_SIZE*1.5f;
				m_HookDir = TargetDirection;
				m_HookedPlayer = -1;
				m_HookTick = 0;
				//m_TriggeredEvents |= COREEVENTFLAG_HOOK_LAUNCH;
			}
		}
		else
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_IDLE;
			m_HookPos = m_Pos;
		}
	}

	// add the speed modification according to players wanted direction
	if (!(IsInWater() && m_pWorld->m_Tuning.m_LiquidDivingCursor))
	{
		if (m_Direction < 0)
			m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
		if (m_Direction > 0)
			m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
		if (m_Direction == 0)
			m_Vel.x *= Friction;
	}

	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(Grounded)
		m_Jumped &= ~2;

	// do hook
	if(m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if(m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if(m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		//m_TriggeredEvents |= COREEVENTFLAG_HOOK_RETRACT;
	}
	else if(m_HookState == HOOK_FLYING)
	{
		vec2 NewPos;
		NewPos = m_pCollision->TestBox(m_HookPos, vec2(1.0f, 1.0f), 8) ? m_HookPos + m_HookDir * m_pWorld->m_Tuning.m_HookFireSpeed * m_pWorld->m_Tuning.m_HookLiquidSlowdown : m_HookPos + m_HookDir * m_pWorld->m_Tuning.m_HookFireSpeed;
		if(distance(m_Pos, NewPos) > m_pWorld->m_Tuning.m_HookLength)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos-m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if(Hit)
		{
			if (Hit & CCollision::COLFLAG_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if(m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
				if(!pCharCore || pCharCore == this)
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PHYS_SIZE+2.0f)
				{
					if(m_HookedPlayer == -1 || distance(m_HookPos, pCharCore->m_Pos) < Distance)
					{
						m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}
		}

		if(m_HookState == HOOK_FLYING)
		{
			// check against ground
			if(GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;
			}
			else if(GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENTFLAG_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if(m_HookState == HOOK_GRABBED)
	{
		if(m_HookedPlayer != -1)
		{
			CCharacterCore *pCharCore = m_pWorld->m_apCharacters[m_HookedPlayer];
			if(pCharCore)
				m_HookPos = pCharCore->m_Pos;
			else
			{
				// release hook
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
				//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if(m_HookedPlayer == -1 && distance(m_HookPos, m_Pos) > 46.0f)
		{
			vec2 HookVel = normalize(m_HookPos-m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel+HookVel;

			// check if we are under the legal limit for the hook
			if(length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if(m_HookedPlayer != -1 && (m_HookTick > SERVER_TICK_SPEED+SERVER_TICK_SPEED/5 || !m_pWorld->m_apCharacters[m_HookedPlayer]))
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_RETRACTED;
			m_HookPos = m_Pos;
		}
	}

	if(m_pWorld)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
			if(!pCharCore)
				continue;

			//player *p = (player*)ent;
			if(pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

			// handle player <-> player collision
			float Distance = distance(m_Pos, pCharCore->m_Pos);
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if(m_pWorld->m_Tuning.m_PlayerCollision && Distance < PHYS_SIZE*1.25f && Distance > 0.0f)
			{
				float a = (PHYS_SIZE*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if(length(m_Vel) > 0.0001)
					Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
			}

			// handle hook influence
			if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking)
			{
				if(Distance > PHYS_SIZE*1.50f) // TODO: fix tweakable variable
				{
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);

					// add force to the hooked player
					pCharCore->m_HookDragVel += Dir*Accel*1.5f;

					// add a little bit force to the guy who has the grip
					m_HookDragVel -= Dir*Accel*0.25f;
				}
			}
		}
	}
	HandleWater(UseInput);
	// clamp the velocity to something sane
	if(length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CCharacterCore::AddDragVelocity()
{
	// Apply hook interaction velocity
	float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

	m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, m_HookDragVel.x);
	m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, m_HookDragVel.y);
}

void CCharacterCore::AddHarpoonDragVelocity()
{
	// Apply harpoon interaction velocity
	float DragSpeed = m_pWorld->m_Tuning.m_HarpoonDragSpeed;

	m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, m_HarpoonDragVel.x);
	m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, m_HarpoonDragVel.y);
}

void CCharacterCore::ResetDragVelocity()
{
	m_HookDragVel = vec2(0,0);
	m_HarpoonDragVel = vec2(0,0);
}

void CCharacterCore::HandleWater(bool UseInput)
{
	if (!IsInWater())
		return;
	
	if(IsFloating())
		m_Jumped |= 1;
	else
		m_Jumped &= ~2;
		
	if (m_DivingGear)
	{
		//Cursor Swimming
		if (m_pWorld->m_Tuning.m_LiquidDivingCursor)
		{
			if (!m_Input.m_Jump)
			{
				m_Vel.y *= m_pWorld->m_Tuning.m_LiquidVerticalDecel;
				m_Vel.x *= m_pWorld->m_Tuning.m_LiquidHorizontalDecel;
			}
			return;
		}
		// Regular
		m_Vel.y *= m_pWorld->m_Tuning.m_LiquidVerticalDecel;
		if (UseInput)
		{
			if (m_pWorld->m_Tuning.m_LiquidUseAscendDescend)
			{
				if (m_Input.m_Jump || m_Input.m_DirectionVertical == -1)
				{
					m_Vel.y -= m_pWorld->m_Tuning.m_LiquidPushOut * 2;
				}
				else if (m_Input.m_DirectionVertical == 1)
				{
					m_Vel.y -= -m_pWorld->m_Tuning.m_LiquidPushOut;
				}
				else
				{
					m_Vel.y -= m_pWorld->m_Tuning.m_LiquidPushOut;
				}
			}
			else
			{
				m_Vel.y -= m_Input.m_Jump || m_Input.m_DirectionVertical == 1 ? -m_pWorld->m_Tuning.m_LiquidPushOut : m_pWorld->m_Tuning.m_LiquidPushOut;
			}
		}
		if (absolute(m_Vel.x) > m_pWorld->m_Tuning.m_LiquidDivingGearMaxHorizontalVelocity)
		{
			m_Vel.x *= m_pWorld->m_Tuning.m_LiquidHorizontalDecel;
		}
		return;
	}
	if (m_pWorld->m_Tuning.m_LiquidDivingCursor&&!m_pWorld->m_Tuning.m_LiquidCursorRequireDiving)
	{
		if (!m_Input.m_Jump)
		{
			m_Vel.y *= m_pWorld->m_Tuning.m_LiquidVerticalDecel;
			m_Vel.x *= m_pWorld->m_Tuning.m_LiquidHorizontalDecel;
		}
		return;
	}
	m_Vel.y *= (m_pWorld->m_Tuning.m_LiquidVerticalDecel);
	if (UseInput)
	{
		if (m_pWorld->m_Tuning.m_LiquidUseAscendDescend)
		{
			if (m_Input.m_Jump || m_Input.m_DirectionVertical == -1)
			{
				m_Vel.y -= m_pWorld->m_Tuning.m_LiquidPushOut * 2;
			}
			else if (m_Input.m_DirectionVertical == 1)
			{
				m_Vel.y -= -m_pWorld->m_Tuning.m_LiquidPushOut * m_pWorld->m_Tuning.m_LiquidFractionofSwimming;
			}
			else
			{
				m_Vel.y -= m_pWorld->m_Tuning.m_LiquidPushOut;
			}
		}
		else
		{
			m_Vel.y -= m_Input.m_Jump || m_Input.m_DirectionVertical == 1 ? -m_pWorld->m_Tuning.m_LiquidPushOut * m_pWorld->m_Tuning.m_LiquidFractionofSwimming : m_pWorld->m_Tuning.m_LiquidPushOut;
		}
	}
	
	
	m_Vel.x *= (m_pWorld->m_Tuning.m_LiquidHorizontalDecel);
}
bool CCharacterCore::IsInWater()
{
	return (m_pCollision->TestBox(vec2(m_Pos.x, m_Pos.y), vec2(PHYS_SIZE, PHYS_SIZE + 1.0f) * (2.0f / 3.0f), CCollision::COLFLAG_WATER));
}

bool CCharacterCore::IsFloating()
{ 
	return (!m_pCollision->TestBox(vec2(m_Pos.x, m_Pos.y - 2.0f), vec2(PHYS_SIZE, PHYS_SIZE + 1.0f) * (2.0f / 3.0f), CCollision::COLFLAG_WATER));
}

void CCharacterCore::Move()
{
	if(!m_pWorld)
		return;
	float RampValue = VelocityRamp(length(m_Vel)*50, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);

	m_Vel.x = m_Vel.x*RampValue;

	vec2 NewPos = m_Pos;
	m_pCollision->MoveBox(&NewPos, &m_Vel, vec2(PHYS_SIZE, PHYS_SIZE), 0, &m_Death);
	

	m_Vel.x = m_Vel.x*(1.0f/RampValue);

	if(m_pWorld->m_Tuning.m_PlayerCollision)
	{
		// check player collision
		float Distance = distance(m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = m_Pos;
		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(m_Pos, NewPos, a);
			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CCharacterCore *pCharCore = m_pWorld->m_apCharacters[p];
				if(!pCharCore || pCharCore == this)
					continue;
				float D = distance(Pos, pCharCore->m_Pos);
				if(D < PHYS_SIZE && D >= 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						m_Pos = NewPos;
					return;
				}
			}
			LastPos = Pos;
		}
	}

	m_Pos = NewPos;
}

void CCharacterCore::Write(CNetObj_CharacterCore *pObjCore) const
{
	pObjCore->m_X = round_to_int(m_Pos.x);
	pObjCore->m_Y = round_to_int(m_Pos.y);

	pObjCore->m_VelX = round_to_int(m_Vel.x*256.0f);
	pObjCore->m_VelY = round_to_int(m_Vel.y*256.0f);
	pObjCore->m_HookState = m_HookState;
	pObjCore->m_HookTick = m_HookTick;
	pObjCore->m_HookX = round_to_int(m_HookPos.x);
	pObjCore->m_HookY = round_to_int(m_HookPos.y);
	pObjCore->m_HookDx = round_to_int(m_HookDir.x*256.0f);
	pObjCore->m_HookDy = round_to_int(m_HookDir.y*256.0f);
	pObjCore->m_HookedPlayer = m_HookedPlayer;
	pObjCore->m_Jumped = m_Jumped;
	pObjCore->m_Direction = m_Direction;
	pObjCore->m_DirectionVertical = m_DirectionVertical;
	pObjCore->m_Angle = m_Angle;
	pObjCore->m_DivingGear = m_DivingGear;
}

void CCharacterCore::Read(const CNetObj_CharacterCore *pObjCore)
{
	m_Pos.x = pObjCore->m_X;
	m_Pos.y = pObjCore->m_Y;
	m_Vel.x = pObjCore->m_VelX/256.0f;
	m_Vel.y = pObjCore->m_VelY/256.0f;
	m_HookState = pObjCore->m_HookState;
	m_HookTick = pObjCore->m_HookTick;
	m_HookPos.x = pObjCore->m_HookX;
	m_HookPos.y = pObjCore->m_HookY;
	m_HookDir.x = pObjCore->m_HookDx/256.0f;
	m_HookDir.y = pObjCore->m_HookDy/256.0f;
	m_HookedPlayer = pObjCore->m_HookedPlayer;
	m_Jumped = pObjCore->m_Jumped;
	m_Direction = pObjCore->m_Direction;
	m_DirectionVertical = pObjCore->m_DirectionVertical;
	m_Angle = pObjCore->m_Angle;
	m_DivingGear = pObjCore->m_DivingGear;
}

void CCharacterCore::Quantize()
{
	CNetObj_CharacterCore Core;
	Write(&Core);
	Read(&Core);
}
