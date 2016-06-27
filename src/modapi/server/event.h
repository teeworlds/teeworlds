#ifndef MODAPI_SERVER_EVENT_H
#define MODAPI_SERVER_EVENT_H

#include <base/vmath.h>
#include <mod/defines.h>

class IServer;
class CGameContext;

#define MODAPI_EVENT_HEADER(TypeName) public:\
	TypeName& Client(int WorldID); \
	TypeName& Mask(int Mask); \
	TypeName& World(int WorldID);

class CModAPI_Event
{
protected:
	IServer* m_pServer;
	CGameContext* m_pGameServer;

protected:
	int m_Mask;
	
	int GetMask06();
	int GetMask07();
	int GetMask07ModAPI();

public:
	CModAPI_Event(CGameContext* pGameServer);
	
	IServer* Server();
	CGameContext* GameServer() { return m_pGameServer; }
};

/* WEAPONPICKUP *******************************************************/

class CModAPI_Event_WeaponPickup : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_WeaponPickup)

public:
	CModAPI_Event_WeaponPickup(CGameContext* pGameServer);
	void Send(int WeaponID);
};

/* BROADCAST **********************************************************/

class CModAPI_Event_Broadcast : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_Broadcast)

public:
	enum
	{
		ALT_NONE = 0,
		ALT_CHAT,
		ALT_MOTD
	};

public:
	CModAPI_Event_Broadcast(CGameContext* pGameServer);
	void Send(const char* pText, int Alternative);
};

/* CHAT **********************************************************/

class CModAPI_Event_Chat : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_Chat)

public:
	CModAPI_Event_Chat(CGameContext* pGameServer);
	void Send(int From, int Team, const char* pText);
};

/* MOTD ***************************************************************/

class CModAPI_Event_MOTD : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_MOTD)

public:
	CModAPI_Event_MOTD(CGameContext* pGameServer);
	void Send(const char* pText);
};

/* ANIMATED TEXT EVENT ************************************************/

class CModAPI_Event_AnimatedText : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_AnimatedText)

public:
	CModAPI_Event_AnimatedText(CGameContext* pGameServer);
	void Send(vec2 Pos, int ItemLayer, const char* pText, int Size, vec4 Color, int Alignment, int AnimationID, int Duration, vec2 Offset);
};

/* SOUND **************************************************************/

class CModAPI_Event_Sound : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_Sound)

public:
	CModAPI_Event_Sound(CGameContext* pGameServer);
	void Send(vec2 Pos, int Sound);
};

/* SPAWN EFFECT *******************************************************/

class CModAPI_Event_SpawnEffect : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_SpawnEffect)

public:
	CModAPI_Event_SpawnEffect(CGameContext* pGameServer);
	void Send(vec2 Pos);
};

/* HAMMERHIT EFFECT ***************************************************/

class CModAPI_Event_HammerHitEffect : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_HammerHitEffect)

public:
	CModAPI_Event_HammerHitEffect(CGameContext* pGameServer);
	void Send(vec2 Pos);
};

/* DAMAGE INDICATOR ***************************************************/

class CModAPI_Event_DamageIndicator : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_DamageIndicator)

public:
	CModAPI_Event_DamageIndicator(CGameContext* pGameServer);
	void Send(vec2 Pos, float Angle, int Amount);
};

/* DEATH EFFECT *******************************************************/

class CModAPI_Event_DeathEffect : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_DeathEffect)

public:
	CModAPI_Event_DeathEffect(CGameContext* pGameServer);
	void Send(vec2 Pos, int ClientID);
};

/* EXPLOSION EFFECT ***************************************************/

class CModAPI_Event_ExplosionEffect : public CModAPI_Event
{
MODAPI_EVENT_HEADER(CModAPI_Event_ExplosionEffect)

public:
	CModAPI_Event_ExplosionEffect(CGameContext* pGameServer);
	void Send(vec2 Pos);
};

#endif
