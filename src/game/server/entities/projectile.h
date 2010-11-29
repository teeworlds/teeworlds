/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

class CProjectile : public CEntity
{
public:
	CProjectile
	(
		CGameWorld *pGameWorld,
		int Type,
		int Owner,
		vec2 Pos,
		vec2 Dir,
		int Span,
		bool Freeeze,
		bool Explosive,
		float Force,
		int SoundImpact,
		int Weapon,
		int Layer = 0,
		int Number = 0
	);

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);
	void SetBouncing(int Value);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	
private:
	vec2 m_Direction;
	int m_LifeSpan;
	int m_Owner;
	int m_Type;
	//int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	int m_StartTick;
	bool m_Explosive;
	int m_Bouncing;
	bool m_Freeze;
	bool m_Collised;
};

#endif
