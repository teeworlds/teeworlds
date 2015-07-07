/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <generated/server_data.h>
#include <game/server/weapons/grenade.h>
#include <game/server/weapons/gun.h>
#include <game/server/weapons/hammer.h>
#include <game/server/weapons/laser.h>
#include <game/server/weapons/ninja.h>
#include <game/server/weapons/shotgun.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include "character.h"


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

CCharacter::CCharacter(CGameWorld *pWorld, CPlayer *pPlayer)
: CEntity(pWorld, ENTTYPE_CHARACTER, vec2(0, 0), ms_PhysSize)
{
	m_pPlayer = pPlayer;
	m_Alive = false;
}

CCharacter::~CCharacter()
{
	for(int i = 0; i < NUM_WEAPONS; i++)
	{
		delete m_apWeapons[i];
		m_apWeapons[i] = 0;
	}
}

void CCharacter::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CCharacter::Destroy()
{
	m_Alive = false;
	GameWorld()->GetCore()->m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_pPlayer->UnsetCharacter();
}

void CCharacter::Spawn(vec2 Pos)
{
	m_Pos = Pos;

	m_Alive = true;
	m_Health = 0;
	m_Armor = 0;

	m_DamageTaken = 0;
	m_DamageTakenTick = -1;
	m_EmoteStopTick = -1;

	for(int i = 0; i < NUM_WEAPONS; i++)
		m_apWeapons[i] = 0;
	GiveWeapon(WEAPON_HAMMER, -1);

	m_ActiveWeapon = WEAPON_HAMMER;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;

	m_ReloadTimer = 0;
	m_AttackTick = -1;
	m_LastNoAmmoSound = -1;

	m_LastAction = -1;
	m_InputsCount = 0;

	m_Core.Reset();
	m_Core.Init(GameWorld()->GetCore(), GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameWorld()->GetCore()->m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_TriggeredEvents = 0;

	m_ReckoningTick = -1;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_pController->OnCharacterSpawn(this);
}

void CCharacter::Die(int Killer, int Weapon)
{
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_pPlayer->GetCID();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = ModeSpecial;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());

	GameWorld()->DestroyEntity(this);
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	// update core speed
	m_Core.m_Vel += Force;

	// check friendly fire
	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From))
		return false;

	// player only inflicts half damage on himself
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	// make sure that the damage indicators don't group together
	if(Server()->Tick() > m_DamageTakenTick+Server()->TickSpeed()/2)
		m_DamageTaken = 0;

	// create healthmod indicator
	GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*pi/12, Dmg);

	// update health/armor
	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	// update damage taken
	m_DamageTaken++;
	m_DamageTakenTick = Server()->Tick();

	// create damage hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS ||  GameServer()->m_apPlayers[i]->m_DeadSpecMode) &&
				GameServer()->m_apPlayers[i]->GetSpectatorID() == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death :(
	if(m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if(pChr)
				pChr->SetEmote(EMOTE_HAPPY, Server()->TickSpeed());
		}

		return false;
	}

	// create pain sound
	if(Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	// set pain emote
	m_EmoteType = EMOTE_PAIN;
	m_EmoteStopTick = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::SetEmote(int Emote, int DurationTicks)
{
	m_EmoteType = Emote;
	m_EmoteStopTick = Server()->Tick() + DurationTicks;
}

CWeapon *CCharacter::GetActiveWeapon()
{
	return m_apWeapons[m_ActiveWeapon];
}

void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || GetActiveWeapon()->IsAlwaysActive())
		return;

	// switch weapon
	if(SetWeapon(m_QueuedWeapon))
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_apWeapons[WantedWeapon])
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_apWeapons[WantedWeapon])
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_apWeapons[WantedWeapon])
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	CWeapon *pWeapon = GetActiveWeapon();

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(pWeapon->IsHoldable() && (m_LatestInput.m_Fire&1) && pWeapon->GetAmmo() > 0)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(pWeapon->GetAmmo() == 0)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));
	vec2 ProjStartPos = m_Pos+Direction*GetProximityRadius()*0.75f;
	GetActiveWeapon()->Fire(Direction, ProjStartPos);

	m_AttackTick = Server()->Tick();
}

bool CCharacter::GotWeapon(int Weapon) const
{
	return Weapon >= 0 && Weapon < NUM_WEAPONS && m_apWeapons[Weapon];
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(m_apWeapons[Weapon])
	{
		// refill weapon
		return m_apWeapons[Weapon]->IncreaseAmmo(Ammo);
	}
	else
	{
		// add weapon
		if(Weapon == WEAPON_HAMMER)
			m_apWeapons[Weapon] = new CWeaponHammer(this, Ammo);
		else if(Weapon == WEAPON_GUN)
			m_apWeapons[Weapon] = new CWeaponGun(this, Ammo);
		else if(Weapon == WEAPON_SHOTGUN)
			m_apWeapons[Weapon] = new CWeaponShotgun(this, Ammo);
		else if(Weapon == WEAPON_GRENADE)
			m_apWeapons[Weapon] = new CWeaponGrenade(this, Ammo);
		else if(Weapon == WEAPON_LASER)
			m_apWeapons[Weapon] = new CWeaponLaser(this, Ammo);
		else if(Weapon == WEAPON_NINJA)
			m_apWeapons[Weapon] = new CWeaponNinja(this, Ammo);
		else
			return false;

		if(m_apWeapons[Weapon]->IsAlwaysActive())
			SetWeapon(Weapon);

		return true;
	}
}

void CCharacter::RemoveWeapon(int Weapon)
{
	if(m_apWeapons[Weapon])
	{
		delete m_apWeapons[Weapon];
		m_apWeapons[Weapon] = 0;
	}

	if(m_ActiveWeapon == Weapon)
	{
		SetWeapon(m_LastWeapon);
		m_LastWeapon = m_ActiveWeapon;
	}
}

bool CCharacter::SetWeapon(int Weapon)
{
	// check we got that weapon
	if(!GotWeapon(Weapon))
	{
		// find a weapon we got
		Weapon = -1;
		for(int i = 0; i < NUM_WEAPONS; i++)
		{
			if(m_apWeapons[i])
			{
				Weapon = i;
				break;
			}
		}

		// we got no weapon anymore
		if(Weapon == -1)
		{
			GiveWeapon(WEAPON_HAMMER, -1);
			Weapon = WEAPON_HAMMER;
		}
	}

	if(Weapon == m_ActiveWeapon)
		return false;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = Weapon;
	m_apWeapons[Weapon]->SetActive();

	return true;
}

void CCharacter::SetReloadTimer(int ReloadTimer)
{
	m_ReloadTimer = ReloadTimer;
}

CCharacter::CInputCount CCharacter::CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_InputsCount++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_InputsCount > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	// handle death-tiles and leaving gamelayer
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+GetProximityRadius()/3.f, m_Pos.y-GetProximityRadius()/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+GetProximityRadius()/3.f, m_Pos.y+GetProximityRadius()/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-GetProximityRadius()/3.f, m_Pos.y-GetProximityRadius()/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-GetProximityRadius()/3.f, m_Pos.y+GetProximityRadius()/3.f)&CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	}

	// tick active weapon
	GetActiveWeapon()->Tick();

	// check reload timer
	if(m_ReloadTimer)
		m_ReloadTimer--;

	// fire active weapon
	FireWeapon();
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	// save some core info temporarily
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	// update core
	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	// debug: check if core is stuck
	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		} StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	// fetch triggered events
	m_TriggeredEvents |= m_Core.m_TriggeredEvents;

	// reset character pos if spectating
	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reckoning for up to 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	if(!m_Alive)
		return;

	if(m_AttackTick > -1)
		++m_AttackTick;
	if(m_DamageTakenTick > -1)
		++m_DamageTakenTick;
	if(m_ReckoningTick > -1)
		++m_ReckoningTick;
	if(m_LastAction > -1)
		++m_LastAction;
	if(m_EmoteStopTick > -1)
		++m_EmoteStopTick;

	GetActiveWeapon()->TickPaused();
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the core
	if(m_ReckoningTick < 0 || GameWorld()->IsPaused())
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if(m_EmoteStopTick < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStopTick = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;
	pCharacter->m_TriggeredEvents = m_TriggeredEvents;

	pCharacter->m_Weapon = GetActiveWeapon()->GetWeaponID();
	pCharacter->m_AttackTick = max(0, m_AttackTick);

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->GetSpectatorID()))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		pCharacter->m_AmmoCount = GetActiveWeapon()->SnapAmmo();
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%250) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}
}

void CCharacter::PostSnap()
{
	m_TriggeredEvents = 0;
}
