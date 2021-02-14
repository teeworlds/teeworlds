/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_EFFECTS_H
#define GAME_CLIENT_COMPONENTS_EFFECTS_H
#include <game/client/component.h>

class CEffects : public CComponent
{
	struct CDamageIndInfo
	{
		vec2 m_Pos;
		int m_HealthAmount;
		int m_ArmorAmount;
		float m_SumSin;
		float m_SumCos;
		int m_NumEvents;
	};

	struct CDamageIndState
	{
		int m_LastDamageTick;
		int m_Variation;
		int m_LastDamage;
	};

	bool m_Add50hz;
	bool m_Add100hz;

	CDamageIndInfo m_aClientDamage[MAX_CLIENTS];
	CDamageIndState m_aClientDamageState[MAX_CLIENTS];
	void CreateDamageIndicator(CDamageIndInfo *pDamage, int ClientID);

public:
	CEffects();

	void AirJump(vec2 Pos);
	void DamageIndicator(vec2 Pos, int HealthAmount, int ArmorAmount, float Angle, int ClientID);
	void PowerupShine(vec2 Pos, vec2 Size);
	void SmokeTrail(vec2 Pos, vec2 Vel);
	void SkidTrail(vec2 Pos, vec2 Vel);
	void BulletTrail(vec2 Pos);
	void PlayerSpawn(vec2 Pos);
	void PlayerDeath(vec2 Pos, int ClientID);
	void Explosion(vec2 Pos);
	void HammerHit(vec2 Pos);

	void ApplyDamageIndicators();

	virtual void OnReset();
	virtual void OnRender();
	float GetEffectsSpeed();
};
#endif
