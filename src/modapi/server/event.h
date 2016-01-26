#ifndef MODAPI_SERVER_EVENT_H
#define MODAPI_SERVER_EVENT_H

#include <base/vmath.h>
#include <mod/defines.h>

class IServer;
class CGameContext;

class CModAPI_WorldEvent
{
private:
	IServer* m_pServer;
	CGameContext* m_pGameServer;
	int m_WorldID;

protected:
	int GenerateMask();

public:
	CModAPI_WorldEvent(CGameContext* pGameServer, int WorldID);
	
	IServer* Server();
	CGameContext* GameServer();
	int WorldID() const;
};

/* ANIMATED TEXT EVENT ************************************************/

class CModAPI_WorldEvent_AnimatedText : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_AnimatedText(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos, int ItemLayer, const char* pText, int Size, vec4 Color, int Alignment, int AnimationID, int Duration, vec2 Offset);
};

/* SOUND **************************************************************/

class CModAPI_WorldEvent_Sound : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_Sound(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos, int Sound);
	void Send(vec2 Pos, int Sound, int Mask);
};

/* SPAWN EFFECT *******************************************************/

class CModAPI_WorldEvent_SpawnEffect : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_SpawnEffect(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos);
};

/* HAMMERHIT EFFECT ***************************************************/

class CModAPI_WorldEvent_HammerHitEffect : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_HammerHitEffect(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos);
};

/* DAMAGE INDICATOR ***************************************************/

class CModAPI_WorldEvent_DamageIndicator : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_DamageIndicator(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos, float Angle, int Amount);
};

/* DEATH EFFECT *******************************************************/

class CModAPI_WorldEvent_DeathEffect : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_DeathEffect(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos, int ClientID);
};

/* EXPLOSION EFFECT ***************************************************/

class CModAPI_WorldEvent_ExplosionEffect : public CModAPI_WorldEvent
{
public:
	CModAPI_WorldEvent_ExplosionEffect(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos);
};

/* EXPLOSION **********************************************************/

class CModAPI_WorldEvent_Explosion : public CModAPI_WorldEvent_ExplosionEffect
{
public:
	CModAPI_WorldEvent_Explosion(CGameContext* pGameServer, int WorldID = MOD_WORLD_DEFAULT);
	void Send(vec2 Pos, int Owner, int Weapon, int MaxDamage);
};

#endif
