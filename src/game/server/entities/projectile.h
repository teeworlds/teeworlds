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
		int Weapon
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
	vec2 m_AvgPos;
	vec2 m_BouncePos;
	vec2 m_ReBouncePos;
	vec2 m_LastBounce;
	vec2 m_PrevLastBounce;
	vec2 m_StartPos;
	vec2 m_StartDir;
	int m_LastRestart;
};

#endif
