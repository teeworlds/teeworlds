// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "laser.h"

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int type)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_Type = type;
	GameWorld()->InsertEntity(this);
	DoBounce();
}


bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *Hit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, OwnerChar);
	//TODO: this plaice need to fix, so laser will freeze more then one character
	if(!Hit)
		return false;
	if(OwnerChar->Team() != Hit->Team()) return false;
	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	if ((m_Type == 1 && g_Config.m_SvHit))
	{
			Hit->m_Core.m_Vel+=normalize(m_PrevPos - Hit->m_Core.m_Pos) * 10;
	}
	else if (m_Type == 0)
	{
		Hit->UnFreeze();
	}
	return true;
}

void CLaser::DoBounce()
{
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	m_EvalTick = Server()->Tick();
	
	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}
	m_PrevPos = m_Pos;
	vec2 To = m_Pos + m_Dir * m_Energy;
	vec2 OrgTo = To;
	vec2 Coltile;
	
	int res;
	res = GameServer()->Collision()->IntersectLine(m_Pos, To, &Coltile, &To,false);
	
	if(res)
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;
			
			int f;
			if(res == -1) {
				f = GameServer()->Collision()->GetTile(round(Coltile.x), round(Coltile.y)); 
				GameServer()->Collision()->SetCollisionAt(round(Coltile.x), round(Coltile.y), CCollision::COLFLAG_SOLID); 
			}
			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			if(res == -1) {
				GameServer()->Collision()->SetCollisionAt(round(Coltile.x), round(Coltile.y), f); 
			}
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);
			
			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
			m_Bounces++;
			
			if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
				m_Energy = -1;
				
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}
	m_Owner = -1;
}
	
void CLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
