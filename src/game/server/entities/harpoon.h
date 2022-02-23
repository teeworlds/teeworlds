#ifndef GAME_SERVER_ENTITIES_HARPOON_H
#define GAME_SERVER_ENTITIES_HARPOON_H

#include <game/server/entity.h>

class CHarpoon : public CEntity
{
public:
	CHarpoon(CGameWorld* pGameWorld, vec2 Pos, vec2 Direction, int Owner, CCharacter* This);
	
	vec2 GetPos(float Tick, vec2 Pos, vec2 Direction, int Curvature, int Speed);
	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient); 
	void RemoveHarpoon();
	void DeallocateOwner();
	void DeallocateVictim();
	void FillInfo(CNetObj_Harpoon* pHarpoon, int SnappingClient);
	void FillPlayerDragInfo(CNetObj_HarpoonDragPlayer* pHarpoon, int SnappingClient);
	CCharacter* GetOwner() {
		if (m_pOwnerChar)
			return m_pOwnerChar;
		else return 0x0;
	}
	int m_Status;

	int m_DeathTick;

private:
	vec2 m_Direction;
	int m_StartTick;
	CCharacter* m_pOwnerChar;
	CCharacter* m_pVictim;
	CEntity* m_pVictimEntity;
	int m_SpawnTick;
	vec2 m_DragVel;
	int m_Owner;
	vec2 m_Angle;

	float m_Size;
};

#endif
