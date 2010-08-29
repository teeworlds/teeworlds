#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "projectile.h"

CProjectile::CProjectile(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeze,
		bool Explosive,
		float Force,
		int SoundImpact,
		int Weapon)
: CEntity(pGameWorld, NETOBJTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_StartPos = Pos;
	m_Direction = Dir;
	m_StartDir = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	//m_Damage = Damage;
	m_Freeze = Freeze;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;
	m_BouncePos=vec2(0,0);
	m_ReBouncePos=vec2(0,0);
	m_LastBounce=vec2(0,0);
	m_PrevLastBounce=vec2(0,0);
	m_LastRestart = 0;

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	if(m_LifeSpan > -2)
		GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;
	
	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;
			
		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;
			
		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}
	
	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

void CProjectile::SetBouncing(int Value)
{
	m_Bouncing = Value;
}

void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	vec2 NewPos;
	vec2 Speed = CurPos - PrevPos;
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos,false);
	CCharacter *OwnerChar;
	

	
	if(m_Owner >= 0)
		OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, (m_Freeze) ? 1.0f : 6.0f, ColPos, OwnerChar);

	if(m_LifeSpan > -1)
		m_LifeSpan--;
	
	if( (TargetChr && (g_Config.m_SvHit || m_Owner == -1 || TargetChr == OwnerChar)) || Collide)//TODO:TEAM
	{
		//if(OwnerChar != 0 && OwnerChar->Team() != TargetChr->Team()) return;
		if(m_Explosive/*??*/ && (!TargetChr || (TargetChr && !m_Freeze)))
		{
			GameServer()->CreateExplosion(ColPos, m_Owner, m_Weapon, (m_Owner == -1)?true:false);
			GameServer()->CreateSound(ColPos, m_SoundImpact);
		}
		else if(TargetChr && m_Freeze)
			TargetChr->Freeze(Server()->TickSpeed()*3);
		if (Collide && m_Bouncing != 0)
		{
			m_StartTick = Server()->Tick();
			m_Pos = NewPos;
			if(g_Config.m_SvShotgunReset > m_LastRestart)
			{
				m_Pos = m_StartPos;
				m_Direction = m_StartDir;
				m_StartTick = Server()->Tick();
			}
			if (m_Bouncing == 1)
			{
				m_PrevLastBounce.x = m_LastBounce.x;
				m_LastBounce.x = m_Pos.x;
				if(!m_BouncePos.x)
					m_BouncePos.x=m_Pos.x;
				else if (!m_ReBouncePos.x)
					m_ReBouncePos.x=m_Pos.x;
				m_Direction.x =- m_Direction.x;
				if(!((m_PrevLastBounce.x+1 == m_BouncePos.x || m_PrevLastBounce.x-1 == m_BouncePos.x || m_PrevLastBounce.x == m_BouncePos.x) && (m_LastBounce.x == m_ReBouncePos.x || m_LastBounce.x+1 == m_ReBouncePos.x || m_LastBounce.x-1 == m_ReBouncePos.x)) && !((m_LastBounce.x == m_BouncePos.x || m_LastBounce.x+1 == m_BouncePos.x || m_LastBounce.x-1 == m_BouncePos.x) && (m_PrevLastBounce.x+1 == m_ReBouncePos.x || m_PrevLastBounce.x-1 == m_ReBouncePos.x || m_PrevLastBounce.x == m_ReBouncePos.x)))
				{
					/*int bx=(int)m_BouncePos.x;
					int rbx=(int)m_ReBouncePos.x;
					int lbx=(int)m_LastBounce.x;
					int plbx=(int)m_PrevLastBounce.x;
					dbg_msg("m_BouncePos","%d",bx);
					dbg_msg("m_ReBouncePos","%d",rbx);
					dbg_msg("m_LastBounce","%d",lbx);
					dbg_msg("m_PrevLastBounce","%d",plbx);
					m_Pos.x=m_AvgPos.x;*/
					g_Config.m_SvShotgunReset++;
					dbg_msg("CrazyShotgun","Warning Horizontal Crazy Shotgun Out of bounds");
					/*int x=(int)m_Pos.x;
					dbg_msg("RePos","%d",x);*/
				}
			}
			else if (m_Bouncing == 2)
			{
				m_PrevLastBounce.y = m_LastBounce.y;
				m_LastBounce.y = m_Pos.y;
				if(!m_BouncePos.y)
					m_BouncePos.y=m_Pos.y;
				else if (!m_ReBouncePos.y)
					m_ReBouncePos.y=m_Pos.y;
				m_Direction.y =- m_Direction.y;
				if(!((m_PrevLastBounce.y+1 == m_BouncePos.y || m_PrevLastBounce.y-1 == m_BouncePos.y || m_PrevLastBounce.y == m_BouncePos.y) && (m_LastBounce.y == m_ReBouncePos.y || m_LastBounce.y+1 == m_ReBouncePos.y || m_LastBounce.y-1 == m_ReBouncePos.y)) && !((m_LastBounce.y == m_BouncePos.y || m_LastBounce.y+1 == m_BouncePos.y || m_LastBounce.y-1 == m_BouncePos.y) && (m_PrevLastBounce.y+1 == m_ReBouncePos.y || m_PrevLastBounce.y-1 == m_ReBouncePos.y || m_PrevLastBounce.y == m_ReBouncePos.y)))
				{
					/*int by=(int)m_BouncePos.y;
					int rby=(int)m_ReBouncePos.y;
					int lby=(int)m_LastBounce.y;
					int plby=(int)m_PrevLastBounce.y;
					dbg_msg("m_BouncePos","%d",by);
					dbg_msg("m_ReBouncePos","%d",rby);
					dbg_msg("m_LastBounce","%d",lby);
					dbg_msg("m_PrevLastBounce","%d",plby);
					m_Pos=m_AvgPos;*/
					g_Config.m_SvShotgunReset++;
					dbg_msg("CrazyShotgun","Warning Vertical Crazy Shotgun Out of bounds");
					/*int y=(int)m_Pos.y;
					dbg_msg("RePos","%d",y);*/
				}
			}
			m_Pos += m_Direction;
		}
		else if (m_Weapon == WEAPON_GUN)
		{
			GameServer()->CreateDamageInd(CurPos, -atan2(m_Direction.x, m_Direction.y), 10);
			GameServer()->m_World.DestroyEntity(this);
		}
		else
			if (!m_Freeze)
				GameServer()->m_World.DestroyEntity(this);
	}
	 if (m_LifeSpan == -1)  		 
	{
		GameServer()->m_World.DestroyEntity(this);
	}



}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	
	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_Id, sizeof(CNetObj_Projectile)));
	FillInfo(pProj);
}
