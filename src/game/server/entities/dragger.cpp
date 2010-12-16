/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/config.h>
#include <engine/server.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "dragger.h"

//////////////////////////////////////////////////
// CDragger
//////////////////////////////////////////////////

const int LENGTH=700;

CDragger::CDragger(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW, int CatchedTeam, int Layer, int Number)
: CEntity(pGameWorld, NETOBJTYPE_LASER)
{
	m_Layer = Layer;
	m_Number = Number;
	m_Pos = Pos;
	m_Strength = Strength;
	m_EvalTick = Server()->Tick();
	m_NW = NW;
	m_CatchedTeam = CatchedTeam;
	GameWorld()->InsertEntity(this);
}

void CDragger::Move()
{
	if(m_Target && m_Target->m_Alive && (m_Target->m_Super || (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[m_Target->Team()])))
		m_Target = 0;
	if(m_Target)
		return;
	CCharacter *Ents[16];
	int Num = GameServer()->m_World.FindEntities(m_Pos,LENGTH, (CEntity**)Ents, 16, NETOBJTYPE_CHARACTER);
	int Id=-1;
	int MinLen=0;
	for (int i = 0; i < Num; i++)
	{
		m_Target = Ents[i];
		if(m_Target->Team() != m_CatchedTeam) continue;
		if(m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[m_Target->Team()]) continue;
		int Res = m_NW ? GameServer()->Collision()->IntersectNoLaserNW(m_Pos, m_Target->m_Pos, 0, 0) :
			GameServer()->Collision()->IntersectNoLaser(m_Pos, m_Target->m_Pos, 0, 0);

		if (Res==0)
		{
			int Len=length(m_Target->m_Pos - m_Pos);
			if (MinLen==0 || MinLen>Len)
			{
				MinLen=Len;
				Id=i;
			}
		}
	}
	m_Target = Id != -1 ? Ents[Id] : 0;
}

void CDragger::Drag()
{
	if (m_Target)
	{

		int Res = 0;
		if (!m_NW)
			Res = GameServer()->Collision()->IntersectNoLaser(m_Pos, m_Target->m_Pos, 0, 0);
		else
			Res = GameServer()->Collision()->IntersectNoLaserNW(m_Pos, m_Target->m_Pos, 0, 0);
		if (Res || length(m_Pos-m_Target->m_Pos)>700)
		{
			m_Target=0;
		}
		else
			if (length(m_Pos-m_Target->m_Pos)>28)
			{
				vec2 Temp = m_Target->m_Core.m_Vel +(normalize(m_Pos-m_Target->m_Pos)*m_Strength);
				if(Temp.x > 0 && ((m_Target->m_TileIndex == TILE_STOP && m_Target->m_TileFlags == ROTATION_270) || (m_Target->m_TileIndexL == TILE_STOP && m_Target->m_TileFlagsL == ROTATION_270) || (m_Target->m_TileIndexL == TILE_STOPS && (m_Target->m_TileFlagsL == ROTATION_90 || m_Target->m_TileFlagsL ==ROTATION_270)) || (m_Target->m_TileIndexL == TILE_STOPA) || (m_Target->m_TileFIndex == TILE_STOP && m_Target->m_TileFFlags == ROTATION_270) || (m_Target->m_TileFIndexL == TILE_STOP && m_Target->m_TileFFlagsL == ROTATION_270) || (m_Target->m_TileFIndexL == TILE_STOPS && (m_Target->m_TileFFlagsL == ROTATION_90 || m_Target->m_TileFFlagsL == ROTATION_270)) || (m_Target->m_TileFIndexL == TILE_STOPA) || (m_Target->m_TileSIndex == TILE_STOP && m_Target->m_TileSFlags == ROTATION_270) || (m_Target->m_TileSIndexL == TILE_STOP && m_Target->m_TileSFlagsL == ROTATION_270) || (m_Target->m_TileSIndexL == TILE_STOPS && (m_Target->m_TileSFlagsL == ROTATION_90 || m_Target->m_TileSFlagsL == ROTATION_270)) || (m_Target->m_TileSIndexL == TILE_STOPA)))
					Temp.x = 0;
				if(Temp.x < 0 && ((m_Target->m_TileIndex == TILE_STOP && m_Target->m_TileFlags == ROTATION_90) || (m_Target->m_TileIndexR == TILE_STOP && m_Target->m_TileFlagsR == ROTATION_90) || (m_Target->m_TileIndexR == TILE_STOPS && (m_Target->m_TileFlagsR == ROTATION_90 || m_Target->m_TileFlagsR == ROTATION_270)) || (m_Target->m_TileIndexR == TILE_STOPA) || (m_Target->m_TileFIndex == TILE_STOP && m_Target->m_TileFFlags == ROTATION_90) || (m_Target->m_TileFIndexR == TILE_STOP && m_Target->m_TileFFlagsR == ROTATION_90) || (m_Target->m_TileFIndexR == TILE_STOPS && (m_Target->m_TileFFlagsR == ROTATION_90 || m_Target->m_TileFFlagsR == ROTATION_270)) || (m_Target->m_TileFIndexR == TILE_STOPA) || (m_Target->m_TileSIndex == TILE_STOP && m_Target->m_TileSFlags == ROTATION_90) || (m_Target->m_TileSIndexR == TILE_STOP && m_Target->m_TileSFlagsR == ROTATION_90) || (m_Target->m_TileSIndexR == TILE_STOPS && (m_Target->m_TileSFlagsR == ROTATION_90 || m_Target->m_TileSFlagsR == ROTATION_270)) || (m_Target->m_TileSIndexR == TILE_STOPA)))
					Temp.x = 0;
				if(Temp.y < 0 && ((m_Target->m_TileIndex == TILE_STOP && m_Target->m_TileFlags == ROTATION_180) || (m_Target->m_TileIndexB == TILE_STOP && m_Target->m_TileFlagsB == ROTATION_180) || (m_Target->m_TileIndexB == TILE_STOPS && (m_Target->m_TileFlagsB == ROTATION_0 || m_Target->m_TileFlagsB == ROTATION_180)) || (m_Target->m_TileIndexB == TILE_STOPA) || (m_Target->m_TileFIndex == TILE_STOP && m_Target->m_TileFFlags == ROTATION_180) || (m_Target->m_TileFIndexB == TILE_STOP && m_Target->m_TileFFlagsB == ROTATION_180) || (m_Target->m_TileFIndexB == TILE_STOPS && (m_Target->m_TileFFlagsB == ROTATION_0 || m_Target->m_TileFFlagsB == ROTATION_180)) || (m_Target->m_TileFIndexB == TILE_STOPA) || (m_Target->m_TileSIndex == TILE_STOP && m_Target->m_TileSFlags == ROTATION_180) || (m_Target->m_TileSIndexB == TILE_STOP && m_Target->m_TileSFlagsB == ROTATION_180) || (m_Target->m_TileSIndexB == TILE_STOPS && (m_Target->m_TileSFlagsB == ROTATION_0 || m_Target->m_TileSFlagsB == ROTATION_180)) || (m_Target->m_TileSIndexB == TILE_STOPA)))
					Temp.y = 0;
				if(Temp.y > 0 && ((m_Target->m_TileIndex == TILE_STOP && m_Target->m_TileFlags == ROTATION_0) || (m_Target->m_TileIndexT == TILE_STOP && m_Target->m_TileFlagsT == ROTATION_0) || (m_Target->m_TileIndexT == TILE_STOPS && (m_Target->m_TileFlagsT == ROTATION_0 || m_Target->m_TileFlagsT == ROTATION_180)) || (m_Target->m_TileIndexT == TILE_STOPA) || (m_Target->m_TileFIndex == TILE_STOP && m_Target->m_TileFFlags == ROTATION_0) || (m_Target->m_TileFIndexT == TILE_STOP && m_Target->m_TileFFlagsT == ROTATION_0) || (m_Target->m_TileFIndexT == TILE_STOPS && (m_Target->m_TileFFlagsT == ROTATION_0 || m_Target->m_TileFFlagsT == ROTATION_180)) || (m_Target->m_TileFIndexT == TILE_STOPA) || (m_Target->m_TileSIndex == TILE_STOP && m_Target->m_TileSFlags == ROTATION_0) || (m_Target->m_TileSIndexT == TILE_STOP && m_Target->m_TileSFlagsT == ROTATION_0) || (m_Target->m_TileSIndexT == TILE_STOPS && (m_Target->m_TileSFlagsT == ROTATION_0 || m_Target->m_TileSFlagsT == ROTATION_180)) || (m_Target->m_TileSIndexT == TILE_STOPA)))
					Temp.y = 0;
				m_Target->m_Core.m_Vel = Temp;
			}
	}
}

void CDragger::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CDragger::Tick()
{
	if (Server()->Tick()%int(Server()->TickSpeed()*0.15f)==0)
	{
		int Flags;
		m_EvalTick=Server()->Tick();
		int index = GameServer()->Collision()->IsCp(m_Pos.x,m_Pos.y, &Flags);
		if (index)
		{
			m_Core=GameServer()->Collision()->CpSpeed(index, Flags);
		}
		m_Pos+=m_Core;
		Move();
	}
	Drag();
	return;


}

void CDragger::Snap(int SnappingClient)
{
	if (m_Target)
	{
		if(NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient,m_Target->m_Pos))
			return;
	}
	else
		if(NetworkClipped(SnappingClient,m_Pos))
			return;

	CCharacter * Char = GameServer()->GetPlayerChar(SnappingClient);
	int Tick = (Server()->Tick()%Server()->TickSpeed())%11;
	if (Char && Char->m_Alive && (m_Layer == LAYER_SWITCH && !GameServer()->Collision()->m_pSwitchers[m_Number].m_Status[Char->Team()] && (!Tick))) return;
	if(Char && Char->m_Alive && m_Target &&  m_Target->m_Alive && Char->Team() != m_Target->Team()) return;

	CNetObj_Laser *obj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_Id, sizeof(CNetObj_Laser)));

	obj->m_X = (int)m_Pos.x;
	obj->m_Y = (int)m_Pos.y;
	if (m_Target)
	{
		obj->m_FromX = (int)m_Target->m_Pos.x;
		obj->m_FromY = (int)m_Target->m_Pos.y;
	}
	else
	{
		obj->m_FromX = (int)m_Pos.x;
		obj->m_FromY = (int)m_Pos.y;
	}


	int StartTick = m_EvalTick;
	if (StartTick < Server()->Tick() - 4)
		StartTick = Server()->Tick() - 4;
	else if (StartTick>Server()->Tick())
		StartTick = Server()->Tick();
	obj->m_StartTick = StartTick;
}

CDraggerTeam::CDraggerTeam(CGameWorld *pGameWorld, vec2 Pos, float Strength, bool NW, int Layer, int Number){
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		m_Draggers[i] = new CDragger(pGameWorld, Pos, Strength, NW, i, Layer, Number);
	}
}

//CDraggerTeam::~CDraggerTeam() {
	//for(int i = 0; i < MAX_CLIENTS; ++i) {
	//	delete m_Draggers[i];
	//}
//}
