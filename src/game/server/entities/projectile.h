/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PROJECTILE_H
#define GAME_SERVER_ENTITIES_PROJECTILE_H

#include <game/server/entity.h>

class CProjectile : public CEntity
{
private:
	/* Identity */
	int m_Type;
	int m_Owner;
	vec2 m_Dir;

	/* State */
	int m_StartTick;

protected:
	/* Functions */
	virtual void OnLifeOver(vec2 Pos) = 0;
	virtual bool OnCharacterHit(vec2 Pos, class CCharacter *pChar) = 0;
	virtual bool OnGroundHit(vec2 Pos) = 0;

public:
	/* Constructor */
	CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Dir, vec2 Pos);

	/* Getters */
	int GetType() const		{ return m_Type; }
	int GetOwner() const	{ return m_Owner; }
	vec2 GetDir() const		{ return m_Dir; }
	virtual float GetCurvature() = 0;
	virtual float GetSpeed() = 0;
	virtual float GetLifeSpan() = 0;

	/* CEntity functions */
	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	/* Other functions */
	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj) const;
};

#endif
