/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "laser.h"

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_Type = Type;
	GameWorld()->InsertEntity(this);
	DoBounce();
}


bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *Hit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, m_Bounces != 0 ? 0: OwnerChar, m_Owner);
	if(!Hit || (Hit == OwnerChar && g_Config.m_SvOldLaser))
		return false;
	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	if (m_Type == 1 && g_Config.m_SvHit)
	{
		vec2 Temp;
		if(!g_Config.m_SvOldLaser)
			Temp = Hit->m_Core.m_Vel + normalize(m_PrevPos - Hit->m_Core.m_Pos) * 10;
		else
			Temp = Hit->m_Core.m_Vel + normalize(OwnerChar->m_Core.m_Pos-Hit->m_Core.m_Pos)*10;
		if(Temp.x > 0 && ((Hit->m_TileIndex == TILE_STOP && Hit->m_TileFlags == ROTATION_270) || (Hit->m_TileIndexL == TILE_STOP && Hit->m_TileFlagsL == ROTATION_270) || (Hit->m_TileIndexL == TILE_STOPS && (Hit->m_TileFlagsL == ROTATION_90 || Hit->m_TileFlagsL ==ROTATION_270)) || (Hit->m_TileIndexL == TILE_STOPA) || (Hit->m_TileFIndex == TILE_STOP && Hit->m_TileFFlags == ROTATION_270) || (Hit->m_TileFIndexL == TILE_STOP && Hit->m_TileFFlagsL == ROTATION_270) || (Hit->m_TileFIndexL == TILE_STOPS && (Hit->m_TileFFlagsL == ROTATION_90 || Hit->m_TileFFlagsL == ROTATION_270)) || (Hit->m_TileFIndexL == TILE_STOPA) || (Hit->m_TileSIndex == TILE_STOP && Hit->m_TileSFlags == ROTATION_270) || (Hit->m_TileSIndexL == TILE_STOP && Hit->m_TileSFlagsL == ROTATION_270) || (Hit->m_TileSIndexL == TILE_STOPS && (Hit->m_TileSFlagsL == ROTATION_90 || Hit->m_TileSFlagsL == ROTATION_270)) || (Hit->m_TileSIndexL == TILE_STOPA)))
			Temp.x = 0;
		if(Temp.x < 0 && ((Hit->m_TileIndex == TILE_STOP && Hit->m_TileFlags == ROTATION_90) || (Hit->m_TileIndexR == TILE_STOP && Hit->m_TileFlagsR == ROTATION_90) || (Hit->m_TileIndexR == TILE_STOPS && (Hit->m_TileFlagsR == ROTATION_90 || Hit->m_TileFlagsR == ROTATION_270)) || (Hit->m_TileIndexR == TILE_STOPA) || (Hit->m_TileFIndex == TILE_STOP && Hit->m_TileFFlags == ROTATION_90) || (Hit->m_TileFIndexR == TILE_STOP && Hit->m_TileFFlagsR == ROTATION_90) || (Hit->m_TileFIndexR == TILE_STOPS && (Hit->m_TileFFlagsR == ROTATION_90 || Hit->m_TileFFlagsR == ROTATION_270)) || (Hit->m_TileFIndexR == TILE_STOPA) || (Hit->m_TileSIndex == TILE_STOP && Hit->m_TileSFlags == ROTATION_90) || (Hit->m_TileSIndexR == TILE_STOP && Hit->m_TileSFlagsR == ROTATION_90) || (Hit->m_TileSIndexR == TILE_STOPS && (Hit->m_TileSFlagsR == ROTATION_90 || Hit->m_TileSFlagsR == ROTATION_270)) || (Hit->m_TileSIndexR == TILE_STOPA)))
			Temp.x = 0;
		if(Temp.y < 0 && ((Hit->m_TileIndex == TILE_STOP && Hit->m_TileFlags == ROTATION_180) || (Hit->m_TileIndexB == TILE_STOP && Hit->m_TileFlagsB == ROTATION_180) || (Hit->m_TileIndexB == TILE_STOPS && (Hit->m_TileFlagsB == ROTATION_0 || Hit->m_TileFlagsB == ROTATION_180)) || (Hit->m_TileIndexB == TILE_STOPA) || (Hit->m_TileFIndex == TILE_STOP && Hit->m_TileFFlags == ROTATION_180) || (Hit->m_TileFIndexB == TILE_STOP && Hit->m_TileFFlagsB == ROTATION_180) || (Hit->m_TileFIndexB == TILE_STOPS && (Hit->m_TileFFlagsB == ROTATION_0 || Hit->m_TileFFlagsB == ROTATION_180)) || (Hit->m_TileFIndexB == TILE_STOPA) || (Hit->m_TileSIndex == TILE_STOP && Hit->m_TileSFlags == ROTATION_180) || (Hit->m_TileSIndexB == TILE_STOP && Hit->m_TileSFlagsB == ROTATION_180) || (Hit->m_TileSIndexB == TILE_STOPS && (Hit->m_TileSFlagsB == ROTATION_0 || Hit->m_TileSFlagsB == ROTATION_180)) || (Hit->m_TileSIndexB == TILE_STOPA)))
			Temp.y = 0;
		if(Temp.y > 0 && ((Hit->m_TileIndex == TILE_STOP && Hit->m_TileFlags == ROTATION_0) || (Hit->m_TileIndexT == TILE_STOP && Hit->m_TileFlagsT == ROTATION_0) || (Hit->m_TileIndexT == TILE_STOPS && (Hit->m_TileFlagsT == ROTATION_0 || Hit->m_TileFlagsT == ROTATION_180)) || (Hit->m_TileIndexT == TILE_STOPA) || (Hit->m_TileFIndex == TILE_STOP && Hit->m_TileFFlags == ROTATION_0) || (Hit->m_TileFIndexT == TILE_STOP && Hit->m_TileFFlagsT == ROTATION_0) || (Hit->m_TileFIndexT == TILE_STOPS && (Hit->m_TileFFlagsT == ROTATION_0 || Hit->m_TileFFlagsT == ROTATION_180)) || (Hit->m_TileFIndexT == TILE_STOPA) || (Hit->m_TileSIndex == TILE_STOP && Hit->m_TileSFlags == ROTATION_0) || (Hit->m_TileSIndexT == TILE_STOP && Hit->m_TileSFlagsT == ROTATION_0) || (Hit->m_TileSIndexT == TILE_STOPS && (Hit->m_TileSFlagsT == ROTATION_0 || Hit->m_TileSFlagsT == ROTATION_180)) || (Hit->m_TileSIndexT == TILE_STOPA)))
			Temp.y = 0;
		Hit->m_Core.m_Vel = Temp;
	}
	else if (m_Type == 0)
	{
		Hit->UnFreeze();
	}
	return true;
}

void CLaser::DoBounce()
{
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
	//m_Owner = -1;
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
	CCharacter * SnappingChar = GameServer()->GetPlayerChar(SnappingClient);
	CCharacter * OwnerChar = 0;
	if(m_Owner >= 0)
		OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!SnappingChar || !OwnerChar)
		return;
	if(SnappingChar->m_Alive && OwnerChar->m_Alive && SnappingChar->Team() != OwnerChar->Team())
		return;
	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
