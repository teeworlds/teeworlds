/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include <game/server/entities/weapon.h>

#include "projectile.h"
#include "laser.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
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


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
	m_MaxHealth = 10;
	m_MaxArmor = 10;
	InitWeapons(pWorld);
}

void
CCharacter::InitWeapons(CGameWorld *pWorld)
{
	m_apWeapons[WEAPON_HAMMER]  = new CHammer(pWorld,this);
	m_apWeapons[WEAPON_GUN]     = new CGun(pWorld,this);
	m_apWeapons[WEAPON_GRENADE] = new CGrenadeLauncher(pWorld,this);
	m_apWeapons[WEAPON_SHOTGUN] = new CShotgun(pWorld,this);
	m_apWeapons[WEAPON_RIFLE]   = new CLaserRifle(pWorld,this);
	m_apWeapons[WEAPON_NINJA]   = new CNinja(pWorld,this);
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_PlayerState = PLAYERSTATE_UNKNOWN;
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_pActiveWeapon = m_apWeapons[WEAPON_GUN];
	m_pLastWeapon = m_apWeapons[WEAPON_GUN];
	m_QueuedWeapon = -1;
	
	m_pPlayer = pPlayer;
	m_Pos = Pos;
	
	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));
	
	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);

	return true;
}

void CCharacter::Destroy()
{
	int i;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	for (i = 0; i < NUM_WEAPONS; i++)
		delete m_apWeapons[i];
	m_Alive = false;
}

void CCharacter::SetWeapon(int Weapon)
{
	if(Weapon == m_pActiveWeapon->WeaponType())
		return;
		
	m_pLastWeapon = m_pActiveWeapon;
	m_QueuedWeapon = -1;

	if(Weapon < 0 || Weapon >= NUM_WEAPONS)
		Weapon = 0;
	m_pActiveWeapon = m_apWeapons[Weapon];

	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}


void CCharacter::HandleNinja()
{
	m_apWeapons[WEAPON_NINJA]->Update();
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_apWeapons[WEAPON_NINJA]->m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_pActiveWeapon->WeaponType();
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
			if(m_apWeapons[WantedWeapon]->m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_apWeapons[WantedWeapon]->m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_pActiveWeapon->WeaponType() && m_apWeapons[WantedWeapon]->m_Got)
		m_QueuedWeapon = WantedWeapon;
	
	DoWeaponSwitch();
}

bool CCharacter::WillFire()
{
	return (CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		|| (m_pActiveWeapon->m_FullAuto && (m_LatestInput.m_Fire&1) && m_pActiveWeapon->CanFire());
}

void CCharacter::FireWeapon()
{
	// check for ammo
	if(!m_pActiveWeapon->CanFire())
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
		return;
	}

	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));
	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;
	
	m_ReloadTimer = m_pActiveWeapon->FireWeapon(ProjStartPos,Direction);
	m_AttackTick = Server()->Tick();
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();
	
	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}


	if(!m_ReloadTimer)
	{
		DoWeaponSwitch();
		// fire Weapon, if wanted
		if (WillFire())
		{
			FireWeapon();
		}
	}
	
	m_pActiveWeapon->RegenAmmo(m_ReloadTimer);
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	return m_apWeapons[Weapon]->GiveWeapon(Ammo);
}

void CCharacter::GiveNinja()
{
	m_apWeapons[WEAPON_NINJA]->GiveWeapon(-1);

	m_pLastWeapon = m_pActiveWeapon;
	m_pActiveWeapon = m_apWeapons[WEAPON_NINJA];

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
}

void CCharacter::EndNinja()
{
	m_apWeapons[WEAPON_NINJA]->m_Got = false;
	m_pActiveWeapon = m_pLastWeapon;

	if(m_pActiveWeapon == m_apWeapons[WEAPON_NINJA])
		m_pActiveWeapon = m_apWeapons[WEAPON_GUN];

	SetWeapon(m_pActiveWeapon->WeaponType());
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();
		
	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;
	
	// or are not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;	
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));
	
	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		if (!m_ReloadTimer && WillFire())
			FireWeapon();
	}
	
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::Tick()
{
	if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());
		
		m_pPlayer->m_ForceBalanced = false;
	}

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);
	
	// handle death-tiles and leaving gamelayer
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	}
	
	// handle Weapons
	HandleWeapons();

	m_PlayerState = m_Input.m_PlayerState;

	// Previnput
	m_PrevInput = m_Input;
	return;
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
	
	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	
	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;
	
	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

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

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());
	
	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);
	
	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);

	
	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}
	
	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= m_MaxHealth)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= m_MaxArmor)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
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
	
	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
	
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;
	
	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage)
		return false;

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

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

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, CmaskOne(From));

	// check for death
	if(m_Health <= 0)
	{
		Die(From, Weapon);
		
		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}
	
		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;
	
	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;
	
	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
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
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;
	
	pCharacter->m_Weapon = m_pActiveWeapon->WeaponType();
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient)
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_pActiveWeapon->m_Ammo > 0)
			pCharacter->m_AmmoCount = m_pActiveWeapon->m_Ammo;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerState = m_PlayerState;
}
