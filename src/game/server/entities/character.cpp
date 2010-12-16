/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <stdio.h>
#include <string.h>
#include <engine/shared/config.h>
#include <engine/server/server.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include <game/server/gamemodes/DDRace.h>

#include <game/server/score.h>

#include "character.h"
#include "laser.h"
#include "light.h"
#include "projectile.h"

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

// Character, "physical" m_pPlayer's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, NETOBJTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
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
	m_ActiveWeapon = WEAPON_GUN;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;
	m_pPlayer = pPlayer;
	m_Pos = Pos;
	m_OlderPos = Pos;
	m_OldPos = Pos;
	m_DDRaceState = DDRACE_NONE;
	m_PrevPos = Pos;
	m_Core.Reset();
	m_BroadCast = true;
	m_EyeEmote = true;
	m_Fly = true;
	m_LastBroadcast = 0;
	m_TeamBeforeSuper = 0;
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision(), &Controller->m_Teams.m_Core);
	m_Core.m_Pos = m_Pos;
	m_Core.m_Id = GetPlayer()->GetCID();
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;
	dbg_msg("CCHaracter::Spawn", "ID %d Player %d m_Core %d", GetPlayer()->GetCID() ,GetPlayer() ,&m_Core);
			
	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;
	if(m_pPlayer->m_RconFreeze) Freeze(-1);
	GameServer()->m_pController->OnCharacterSpawn(this);
	
	if(GetPlayer()->m_IsUsingDDRaceClient) {
		Controller->m_Teams.SendTeamsState(GetPlayer()->GetCID());
	}

	return true;
}

void CCharacter::Destroy()
{
	//GameServer()->m_World.m_Core.m_apCharacters[m_MarkedId] = 0; This caused the Marked Char for delete to always Delete ID 0 Core
	dbg_msg("CCHaracter::Destroy", "ID %d Player %d m_Core %d", GetPlayer()->GetCID() ,GetPlayer() ,&m_Core);
	m_Alive = false;
	CEntity::Destroy();
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::CanCollide(int Cid) {
	return Teams()->m_Core.CanCollide(GetPlayer()->GetCID(), Cid);
}

bool CCharacter::SameTeam(int Cid) {
	return Teams()->m_Core.SameTeam(GetPlayer()->GetCID(), Cid);
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
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;
		if(m_ActiveWeapon == WEAPON_NINJA)
			m_ActiveWeapon = WEAPON_GUN;
		Direction= normalize(vec2(0,0)) ;
		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel *= 0.2f;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[64];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, 64, NETOBJTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a m_pPlayer, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, 10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
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
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0 /*|| m_FreezeTime > 0*/)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if((FullAuto || m_Super) && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
/*		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 1 * Server()->TickSpeed();
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);*/
		// Timerstuff to avoid shrieking orchestra caused by unfreeze-plasma
		if(m_PainSoundTimer<=0)
		{
				m_PainSoundTimer = 1 * Server()->TickSpeed();
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
		}
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;
	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE, Teams()->TeamMask(Team()));

			if (!g_Config.m_SvHit) break;

			CCharacter *aEnts[64];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)aEnts,
			64, NETOBJTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *Target = aEnts[i];

				//for DDRace mod or any other mod, which needs hammer hits through the wall remove second condition
				if ((Target == this || !CanCollide(Target->GetPlayer()->GetCID())) /*|| GameServer()->Collision()->IntersectLine(ProjStartPos, Target->m_Pos, NULL, NULL)*/)
					continue;

				// set his velocity to fast upward (for now)
				GameServer()->CreateHammerHit(m_Pos, Teams()->TeamMask(Team()));
				aEnts[i]->TakeDamage(vec2(0.f, -1.f), g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage, m_pPlayer->GetCID(), m_ActiveWeapon);

				vec2 Dir;
				if (length(Target->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(Target->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);
				vec2 Temp = Target->m_Core.m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f * (m_HammerType + 1);
				if(Temp.x > 0 && ((Target->m_TileIndex == TILE_STOP && Target->m_TileFlags == ROTATION_270) || (Target->m_TileIndexL == TILE_STOP && Target->m_TileFlagsL == ROTATION_270) || (Target->m_TileIndexL == TILE_STOPS && (Target->m_TileFlagsL == ROTATION_90 || Target->m_TileFlagsL ==ROTATION_270)) || (Target->m_TileIndexL == TILE_STOPA) || (Target->m_TileFIndex == TILE_STOP && Target->m_TileFFlags == ROTATION_270) || (Target->m_TileFIndexL == TILE_STOP && Target->m_TileFFlagsL == ROTATION_270) || (Target->m_TileFIndexL == TILE_STOPS && (Target->m_TileFFlagsL == ROTATION_90 || Target->m_TileFFlagsL == ROTATION_270)) || (Target->m_TileFIndexL == TILE_STOPA) || (Target->m_TileSIndex == TILE_STOP && Target->m_TileSFlags == ROTATION_270) || (Target->m_TileSIndexL == TILE_STOP && Target->m_TileSFlagsL == ROTATION_270) || (Target->m_TileSIndexL == TILE_STOPS && (Target->m_TileSFlagsL == ROTATION_90 || Target->m_TileSFlagsL == ROTATION_270)) || (Target->m_TileSIndexL == TILE_STOPA)))
					Temp.x = 0;
				if(Temp.x < 0 && ((Target->m_TileIndex == TILE_STOP && Target->m_TileFlags == ROTATION_90) || (Target->m_TileIndexR == TILE_STOP && Target->m_TileFlagsR == ROTATION_90) || (Target->m_TileIndexR == TILE_STOPS && (Target->m_TileFlagsR == ROTATION_90 || Target->m_TileFlagsR == ROTATION_270)) || (Target->m_TileIndexR == TILE_STOPA) || (Target->m_TileFIndex == TILE_STOP && Target->m_TileFFlags == ROTATION_90) || (Target->m_TileFIndexR == TILE_STOP && Target->m_TileFFlagsR == ROTATION_90) || (Target->m_TileFIndexR == TILE_STOPS && (Target->m_TileFFlagsR == ROTATION_90 || Target->m_TileFFlagsR == ROTATION_270)) || (Target->m_TileFIndexR == TILE_STOPA) || (Target->m_TileSIndex == TILE_STOP && Target->m_TileSFlags == ROTATION_90) || (Target->m_TileSIndexR == TILE_STOP && Target->m_TileSFlagsR == ROTATION_90) || (Target->m_TileSIndexR == TILE_STOPS && (Target->m_TileSFlagsR == ROTATION_90 || Target->m_TileSFlagsR == ROTATION_270)) || (Target->m_TileSIndexR == TILE_STOPA)))
					Temp.x = 0;
				if(Temp.y < 0 && ((Target->m_TileIndex == TILE_STOP && Target->m_TileFlags == ROTATION_180) || (Target->m_TileIndexB == TILE_STOP && Target->m_TileFlagsB == ROTATION_180) || (Target->m_TileIndexB == TILE_STOPS && (Target->m_TileFlagsB == ROTATION_0 || Target->m_TileFlagsB == ROTATION_180)) || (Target->m_TileIndexB == TILE_STOPA) || (Target->m_TileFIndex == TILE_STOP && Target->m_TileFFlags == ROTATION_180) || (Target->m_TileFIndexB == TILE_STOP && Target->m_TileFFlagsB == ROTATION_180) || (Target->m_TileFIndexB == TILE_STOPS && (Target->m_TileFFlagsB == ROTATION_0 || Target->m_TileFFlagsB == ROTATION_180)) || (Target->m_TileFIndexB == TILE_STOPA) || (Target->m_TileSIndex == TILE_STOP && Target->m_TileSFlags == ROTATION_180) || (Target->m_TileSIndexB == TILE_STOP && Target->m_TileSFlagsB == ROTATION_180) || (Target->m_TileSIndexB == TILE_STOPS && (Target->m_TileSFlagsB == ROTATION_0 || Target->m_TileSFlagsB == ROTATION_180)) || (Target->m_TileSIndexB == TILE_STOPA)))
					Temp.y = 0;
				if(Temp.y > 0 && ((Target->m_TileIndex == TILE_STOP && Target->m_TileFlags == ROTATION_0) || (Target->m_TileIndexT == TILE_STOP && Target->m_TileFlagsT == ROTATION_0) || (Target->m_TileIndexT == TILE_STOPS && (Target->m_TileFlagsT == ROTATION_0 || Target->m_TileFlagsT == ROTATION_180)) || (Target->m_TileIndexT == TILE_STOPA) || (Target->m_TileFIndex == TILE_STOP && Target->m_TileFFlags == ROTATION_0) || (Target->m_TileFIndexT == TILE_STOP && Target->m_TileFFlagsT == ROTATION_0) || (Target->m_TileFIndexT == TILE_STOPS && (Target->m_TileFFlagsT == ROTATION_0 || Target->m_TileFFlagsT == ROTATION_180)) || (Target->m_TileFIndexT == TILE_STOPA) || (Target->m_TileSIndex == TILE_STOP && Target->m_TileSFlags == ROTATION_0) || (Target->m_TileSIndexT == TILE_STOP && Target->m_TileSFlagsT == ROTATION_0) || (Target->m_TileSIndexT == TILE_STOPS && (Target->m_TileSFlagsT == ROTATION_0 || Target->m_TileSFlagsT == ROTATION_180)) || (Target->m_TileSIndexT == TILE_STOPA)))
					Temp.y = 0;
				Target->m_Core.m_Vel = Temp;
				Target->UnFreeze();
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			CProjectile *Proj = new CProjectile
				(
				GameWorld(),
				WEAPON_GUN,//Type
				m_pPlayer->GetCID(),//Owner
				ProjStartPos,//Pos
				Direction,//Dir
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),//Span
				0,//Freeze
				0,//Explosive
				0,//Force
				-1,//SoundImpact
				WEAPON_GUN//Weapon
				);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			Proj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);

			Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE, Teams()->TeamMask(Team()));
		} break;

		case WEAPON_SHOTGUN:
		{
				new CLaser(&GameServer()->m_World, m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), 1);
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
			/*int ShotSpread = 2;

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(ShotSpread*2+1);

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = GetAngle(Direction);
				a += Spreading[i+2];
				float v = 1-(absolute(i)/(float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				CProjectile *Proj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					vec2(cosf(a), sinf(a))*Speed,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_Shotm_GunLifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				Proj->FillInfo(&p);

				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
			}

			Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);*/
		} break;

		case WEAPON_GRENADE:
		{
				CProjectile *Proj = new CProjectile
					(
					GameWorld(),
					WEAPON_GRENADE,//Type
					m_pPlayer->GetCID(),//Owner
					ProjStartPos,//Pos
					Direction,//Dir
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),//Span
					0,//Freeze
					true,//Explosive
					0,//Force
					SOUND_GRENADE_EXPLODE,//SoundImpact
					WEAPON_GRENADE//Weapon
					);//SoundImpact

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				Proj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
				Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, Teams()->TeamMask(Team()));
		} break;

		case WEAPON_RIFLE:
		{
				new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), 0);
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE, Teams()->TeamMask(Team()));
		} break;

		case WEAPON_NINJA:
		{
				// reset Hit objects
				m_NumObjectsHit = 0;

				m_AttackTick = Server()->Tick();
				m_Ninja.m_ActivationDir = Direction;
				//m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
				m_Ninja.m_CurrentMoveTime = 10;
				GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE, Teams()->TeamMask(Team()));
		} break;

	}

	m_AttackTick = Server()->Tick();
	/*
	if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;
	*/
	if(!m_ReloadTimer)
		m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));
	// check PainSoundTimer - Hmm, maybe sometimes shrieking can be a weapon too? ;)
	if(m_PainSoundTimer>0)
		m_PainSoundTimer--;

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();
	/*
	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	if(AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}
	*/
	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		if(!m_FreezeTime) m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

void CCharacter::GiveNinja()
{
	if(!m_aWeapons[WEAPON_NINJA].m_Got)
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	if (!m_FreezeTime) m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;
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

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != -1)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}
	if(pNewInput->m_Jump&1  && m_Super && m_Fly)
		HandleFly();

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::OnFinish()
{
	//TODO: this ugly
	float time = (float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed());
	if(time < 0.000001) return;
	CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
	char aBuf[128];
		m_CpActive=-2;
		str_format(aBuf, sizeof(aBuf), "%s finished in: %d minute(s) %5.2f second(s)", Server()->ClientName(m_pPlayer->GetCID()), (int)time/60, time-((int)time/60*60));
		if(g_Config.m_SvHideScore)
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		else
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		if(time - pData->m_BestTime < 0)
		{
			// new record \o/
			str_format(aBuf, sizeof(aBuf), "New record: %5.2f second(s) better.", fabs(time - pData->m_BestTime));
			if(g_Config.m_SvHideScore)
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
			else
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
		else if(pData->m_BestTime != 0) // tee has already finished?
		{
			if(fabs(time - pData->m_BestTime) <= 0.005)
			{
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You finished with your best time.");
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%5.2f second(s) worse, better luck next time.", fabs(pData->m_BestTime - time));
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);//this is private, sent only to the tee
			}
		}

		if(!pData->m_BestTime || time < pData->m_BestTime)
		{
			// update the score
			pData->Set(time, m_CpCurrent);

			if(str_comp_num(Server()->ClientName(m_pPlayer->GetCID()), "nameless tee", 12) != 0)
				GameServer()->Score()->SaveScore(m_pPlayer->GetCID(), time, this);
		}
		bool NeedToSendNewRecord = false;
		// update server best time
		if(GameServer()->m_pController->m_CurrentRecord == 0 || time < GameServer()->m_pController->m_CurrentRecord)
		{
			// check for nameless
			if(str_comp_num(Server()->ClientName(m_pPlayer->GetCID()), "nameless tee", 12) != 0) {
				GameServer()->m_pController->m_CurrentRecord = time;
				//dbg_msg("character", "Finish");
				NeedToSendNewRecord = true;
				
			}
				
		}

		m_DDRaceState = DDRACE_NONE;
		// set player score
		if(!pData->m_CurrentTime || pData->m_CurrentTime > time)
		{
			pData->m_CurrentTime = time;
			NeedToSendNewRecord = true;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsUsingDDRaceClient)
				{
					if(!g_Config.m_SvHideScore || i == m_pPlayer->GetCID())
					{
						CNetMsg_Sv_PlayerTime Msg;
						Msg.m_Time = time * 100.0;
						Msg.m_Cid = m_pPlayer->GetCID();
						Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
					}
				}
			}
		}
		
		if(NeedToSendNewRecord && GetPlayer()->m_IsUsingDDRaceClient) { 
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsUsingDDRaceClient)
				{
					GameServer()->SendRecord(i);
				}
			}
		}
		
		if(GetPlayer()->m_IsUsingDDRaceClient) {
			CNetMsg_Sv_DDRaceTime Msg;
			Msg.m_Time = (int)(time * 100.0f);
			Msg.m_Check = 0;
			Msg.m_Finish = 1;

				if(pData->m_BestTime)
				{
					float Diff = (time - pData->m_BestTime)*100;
					Msg.m_Check = (int)Diff;
				}
			
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
		}

		int TTime = 0-(int)time;
		if(m_pPlayer->m_Score < TTime)
			m_pPlayer->m_Score = TTime;

}

int CCharacter::Team()
{
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	return Controller->m_Teams.m_Core.Team(m_pPlayer->GetCID());
}

CGameTeams* CCharacter::Teams()
{
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	return &Controller->m_Teams;
}

void CCharacter::HandleFly()
{
	m_Core.HandleFly();
}

void CCharacter::Tick()
{
	std::list < int > Indices = GameServer()->Collision()->GetMapIndices(m_PrevPos, m_Pos);
	//dbg_msg("Indices","%d",Indices.size());
	/*if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", Controller->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());

		m_pPlayer->m_ForceBalanced = false;
	}*/
	m_Armor=(m_FreezeTime != -1)?10-(m_FreezeTime/15):0;
	if(m_Input.m_Direction != 0 || m_Input.m_Jump != 0)
		m_LastMove = Server()->Tick();

	if(m_FreezeTime > 0 || m_FreezeTime == -1)
	{
		if (m_FreezeTime % Server()->TickSpeed() == 0 || m_FreezeTime == -1)
		{
			GameServer()->CreateDamageInd(m_Pos, 0, m_FreezeTime / Server()->TickSpeed());
		}
		if(m_FreezeTime != -1)
			m_FreezeTime--;
		else
			m_Ninja.m_ActivationTick = Server()->Tick();
		m_Input.m_Direction = 0;
		m_Input.m_Jump = 0;
		m_Input.m_Hook = 0;

		//m_Input.m_Fire = 0;
		if (m_FreezeTime == 1) {
			UnFreeze();
		}
	}
	m_Core.m_Input = m_Input;
	m_Core.Tick(true);
	m_Core.m_Id = GetPlayer()->GetCID();

	m_DoSplash = false;
	if (g_Config.m_SvEndlessDrag)
		m_Core.m_HookTick = 0;
	if (m_Super && m_Core.m_Jumped > 1)
		m_Core.m_Jumped = 1;
	if (m_Super && g_Config.m_SvEndlessSuperHook)
		m_Core.m_HookTick = 0;
	/*dbg_msg("character","m_TileIndex=%d , m_TileFIndex=%d",m_TileIndex,m_TileFIndex); //REMOVE*/
	//DDRace
	char aBroadcast[128];
	m_Time = (float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed());
	CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());

	if(Server()->Tick() - m_RefreshTime >= Server()->TickSpeed())
	{
		if (m_DDRaceState == DDRACE_STARTED)
		{
			if(m_CpActive != -1 && m_CpTick > Server()->Tick() && !m_pPlayer->m_IsUsingDDRaceClient)
			{
				if(pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
				{
					float Diff = m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive];
					str_format(aBroadcast, sizeof(aBroadcast), "Checkpoint | Diff : %+5.2f", Diff);
					GameServer()->SendBroadcast(aBroadcast, m_pPlayer->GetCID());
					m_LastBroadcast = Server()->Tick();
				}
			}
			else if( g_Config.m_SvBroadcast[0] != 0 && m_BroadCast)
			{
				str_format(aBroadcast, sizeof(aBroadcast), "%s", g_Config.m_SvBroadcast);

				if(Server()->Tick() >= (m_LastBroadcast + Server()->TickSpeed()))
				{
					GameServer()->SendBroadcast(aBroadcast, m_pPlayer->GetCID());
					m_LastBroadcast = Server()->Tick();
				}
			}
		}
		else
		{
			char aTmp[128];
			if(!m_pPlayer->m_IsUsingDDRaceClient) 
			{
				if( g_Config.m_SvBroadcast[0] != 0 && (Server()->Tick() > (m_LastBroadcast + (Server()->TickSpeed() * 9))))
				{
					char aYourBest[64],aServerBest[64];
					str_format(aYourBest, sizeof(aYourBest), "Your Best:'%s%d:%s%d'", ((pData->m_BestTime / 60) < 10)?"0":"", (int)(pData->m_BestTime / 60), (((int)pData->m_BestTime % 60) < 10)?"0":"", (int)pData->m_BestTime % 60);
					
					CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
					
					str_format(aServerBest, sizeof(aServerBest), "Server Best:'%s%d:%s%d'", ((GameServer()->m_pController->m_CurrentRecord / 60) < 10)?"0":"", (int)(GameServer()->m_pController->m_CurrentRecord / 60), (((int)GameServer()->m_pController->m_CurrentRecord % 60) < 10)?"0":"", (int)GameServer()->m_pController->m_CurrentRecord % 60);
					str_format(aTmp, sizeof(aTmp), "%s %s", (GameServer()->m_pController->m_CurrentRecord)?aServerBest:"", (pData->m_BestTime)?aYourBest:"");
					GameServer()->SendBroadcast(aTmp, m_pPlayer->GetCID());
					m_LastBroadcast = Server()->Tick();
				}
			}
			else if( g_Config.m_SvBroadcast[0] != 0 && (Server()->Tick() > (m_LastBroadcast + (Server()->TickSpeed() * 9))))
			{
				str_format(aTmp, sizeof(aTmp), "%s", g_Config.m_SvBroadcast);
				GameServer()->SendBroadcast(aTmp, m_pPlayer->GetCID());
				m_LastBroadcast = Server()->Tick();
			}
		}
		m_RefreshTime = Server()->Tick();
	}
//int num =0;
	if(!Indices.empty())
		for(std::list < int >::iterator i = Indices.begin(); i != Indices.end(); i++)
			HandleTiles(*i);
	else
		HandleTiles(GameServer()->Collision()->GetPureMapIndex(m_Pos));

	// kill player when leaving gamelayer
	if((int)m_Pos.x/32 < -200 || (int)m_Pos.x/32 > GameServer()->Collision()->GetWidth()+200 ||
		(int)m_Pos.y/32 < -200 || (int)m_Pos.y/32 > GameServer()->Collision()->GetHeight()+200)
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	}

	// handle Weapons
	HandleWeapons();

	m_PlayerState = m_Input.m_PlayerState;

	// Previnput
	m_PrevInput = m_Input;
	if (!m_Doored)
	{
		m_OlderPos = m_OldPos;
		m_OldPos = m_Core.m_Pos;
	}
	m_PrevPos = m_Core.m_Pos;
	return;
}

void CCharacter::HandleTiles(int Index)
{//dbg_msg("num","%d",++num);
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	int MapIndex = Index;
	int PureMapIndex = GameServer()->Collision()->GetPureMapIndex(m_Pos);
	float Offset = 4;
	int MapIndexL = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x + (m_ProximityRadius/2)+Offset,m_Pos.y));
	int MapIndexR = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x - (m_ProximityRadius/2)-Offset,m_Pos.y));
	int MapIndexT = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x,m_Pos.y + (m_ProximityRadius/2)+Offset));
	int MapIndexB = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x,m_Pos.y - (m_ProximityRadius/2)-Offset));
	//dbg_msg("","N%d L%d R%d B%d T%d",MapIndex,MapIndexL,MapIndexR,MapIndexB,MapIndexT);
	m_TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
	m_TileFlags = GameServer()->Collision()->GetTileFlags(MapIndex);
	m_TileIndexL = GameServer()->Collision()->GetTileIndex(MapIndexL);
	m_TileFlagsL = GameServer()->Collision()->GetTileFlags(MapIndexL);
	m_TileIndexR = GameServer()->Collision()->GetTileIndex(MapIndexR);
	m_TileFlagsR = GameServer()->Collision()->GetTileFlags(MapIndexR);
	m_TileIndexB = GameServer()->Collision()->GetTileIndex(MapIndexB);
	m_TileFlagsB = GameServer()->Collision()->GetTileFlags(MapIndexB);
	m_TileIndexT = GameServer()->Collision()->GetTileIndex(MapIndexT);
	m_TileFlagsT = GameServer()->Collision()->GetTileFlags(MapIndexT);
	m_TileFIndex = GameServer()->Collision()->GetFTileIndex(MapIndex);
	m_TileFFlags = GameServer()->Collision()->GetFTileFlags(MapIndex);
	m_TileFIndexL = GameServer()->Collision()->GetFTileIndex(MapIndexL);
	m_TileFFlagsL = GameServer()->Collision()->GetFTileFlags(MapIndexL);
	m_TileFIndexR = GameServer()->Collision()->GetFTileIndex(MapIndexR);
	m_TileFFlagsR = GameServer()->Collision()->GetFTileFlags(MapIndexR);
	m_TileFIndexB = GameServer()->Collision()->GetFTileIndex(MapIndexB);
	m_TileFFlagsB = GameServer()->Collision()->GetFTileFlags(MapIndexB);
	m_TileFIndexT = GameServer()->Collision()->GetFTileIndex(MapIndexT);
	m_TileFFlagsT = GameServer()->Collision()->GetFTileFlags(MapIndexT);//
	m_TileSIndex = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndex)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileIndex(MapIndex):0:0;
	m_TileSFlags = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndex)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileFlags(MapIndex):0:0;
	m_TileSIndexL = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexL)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileIndex(MapIndexL):0:0;
	m_TileSFlagsL = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexL)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileFlags(MapIndexL):0:0;
	m_TileSIndexR = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexR)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileIndex(MapIndexR):0:0;
	m_TileSFlagsR = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexR)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileFlags(MapIndexR):0:0;
	m_TileSIndexB = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexB)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileIndex(MapIndexB):0:0;
	m_TileSFlagsB = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexB)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileFlags(MapIndexB):0:0;
	m_TileSIndexT = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexT)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileIndex(MapIndexT):0:0;
	m_TileSFlagsT = (GameServer()->Collision()->m_pSwitchers &&GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexT)].m_Status[Team()])?(Team() != TEAM_SUPER)?GameServer()->Collision()->GetDTileFlags(MapIndexT):0:0;
	//dbg_msg("Tiles","%d, %d, %d, %d, %d", m_TileSIndex, m_TileSIndexL, m_TileSIndexR, m_TileSIndexB, m_TileSIndexT);
	//Sensitivity
	int S1 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f));
	int S2 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f));
	int S3 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f));
	int S4 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f));
	int Tile1 = GameServer()->Collision()->GetTileIndex(S1);
	int Tile2 = GameServer()->Collision()->GetTileIndex(S2);
	int Tile3 = GameServer()->Collision()->GetTileIndex(S3);
	int Tile4 = GameServer()->Collision()->GetTileIndex(S4);
	int FTile1 = GameServer()->Collision()->GetFTileIndex(S1);
	int FTile2 = GameServer()->Collision()->GetFTileIndex(S2);
	int FTile3 = GameServer()->Collision()->GetFTileIndex(S3);
	int FTile4 = GameServer()->Collision()->GetFTileIndex(S4);
	//dbg_msg("","N%d L%d R%d B%d T%d",m_TileIndex,m_TileIndexL,m_TileIndexR,m_TileIndexB,m_TileIndexT);
	//dbg_msg("","N%d L%d R%d B%d T%d",m_TileFIndex,m_TileFIndexL,m_TileFIndexR,m_TileFIndexB,m_TileFIndexT);

	int cp = GameServer()->Collision()->IsCheckpoint(MapIndex);
	if(cp != -1 && m_DDRaceState == DDRACE_STARTED && cp > m_CpActive)
	{
		m_CpActive = cp;
		m_CpCurrent[cp] = m_Time;
		m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
		if(m_pPlayer->m_IsUsingDDRaceClient) {
			CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
			CNetMsg_Sv_DDRaceTime Msg;
			Msg.m_Time = (int)m_Time;
			Msg.m_Check = 0;
			Msg.m_Finish = 0;

			if(m_CpActive != -1 && m_CpTick > Server()->Tick())
			{
				if(pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
				{
					float Diff = (m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive])*100;
					Msg.m_Check = (int)Diff;
				}
			}

			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
		}
	}
	int cpf = GameServer()->Collision()->IsFCheckpoint(MapIndex);
	if(cpf != -1 && m_DDRaceState == DDRACE_STARTED && cpf > m_CpActive)
	{
		m_CpActive = cpf;
		m_CpCurrent[cpf] = m_Time;
		m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
		if(m_pPlayer->m_IsUsingDDRaceClient) {
			CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
			CNetMsg_Sv_DDRaceTime Msg;
			Msg.m_Time = (int)m_Time;
			Msg.m_Check = 0;
			Msg.m_Finish = 0;

			if(m_CpActive != -1 && m_CpTick > Server()->Tick())
			{
				if(pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
				{
					float Diff = (m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive])*100;
					Msg.m_Check = (int)Diff;
				}
			}

			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
		}
	}
	if(((m_TileIndex == TILE_BEGIN) || (m_TileFIndex == TILE_BEGIN) || FTile1 == TILE_BEGIN || FTile2 == TILE_BEGIN || FTile3 == TILE_BEGIN || FTile4 == TILE_BEGIN || Tile1 == TILE_BEGIN || Tile2 == TILE_BEGIN || Tile3 == TILE_BEGIN || Tile4 == TILE_BEGIN) && (m_DDRaceState == DDRACE_NONE || (m_DDRaceState == DDRACE_STARTED && !Team())))
	{
		bool CanBegin = true;
		if(g_Config.m_SvTeam == 1 && (Team() == TEAM_FLOCK || Teams()->Count(Team()) <= 1) ) {
			GameServer()->SendChat(-1, GetPlayer()->GetCID(),"I already told you that you must find a friend");//need to make this better
			CanBegin = false;
		}
		if(CanBegin) {
			Controller->m_Teams.OnCharacterStart(m_pPlayer->GetCID());
			m_CpActive = -2;
		} else {
			
		}
		
	}
	if(((m_TileIndex == TILE_END) || (m_TileFIndex == TILE_END) || FTile1 == TILE_END || FTile2 == TILE_END || FTile3 == TILE_END || FTile4 == TILE_END || Tile1 == TILE_END || Tile2 == TILE_END || Tile3 == TILE_END || Tile4 == TILE_END) && m_DDRaceState == DDRACE_STARTED)
	{
		Controller->m_Teams.OnCharacterFinish(m_pPlayer->GetCID());
	}
	if(((m_TileIndex == TILE_FREEZE) || (m_TileFIndex == TILE_FREEZE)) && !m_Super)
	{
		Freeze(Server()->TickSpeed()*3);
	}
	else if((m_TileIndex == TILE_UNFREEZE) || (m_TileFIndex == TILE_UNFREEZE))
	{
		UnFreeze();
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)) && m_Core.m_Vel.x > 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexL).x)
			if((int)GameServer()->Collision()->GetPos(MapIndexL).x < (int)m_Core.m_Pos.x)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.x = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)) && m_Core.m_Vel.x < 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexR).x)
			if((int)GameServer()->Collision()->GetPos(MapIndexR).x > (int)m_Core.m_Pos.x)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.x = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)) && m_Core.m_Vel.y < 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexB).y)
			if((int)GameServer()->Collision()->GetPos(MapIndexB).y > (int)m_Core.m_Pos.y)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.y = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)) && m_Core.m_Vel.y > 0)
	{
		//dbg_msg("","%f %f",GameServer()->Collision()->GetPos(MapIndex).y,m_Core.m_Pos.y);
		if((int)GameServer()->Collision()->GetPos(MapIndexT).y)
			if((int)GameServer()->Collision()->GetPos(MapIndexT).y < (int)m_Core.m_Pos.y)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.y = 0;
		m_Core.m_Jumped = 0;
	}
	// handle switch tiles
	if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHOPEN && Team() != TEAM_SUPER)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = true;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = 0;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHOPEN;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHTIMEDOPEN && Team() != TEAM_SUPER)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = true;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = Server()->Tick() + 1 + GameServer()->Collision()->GetSwitchDelay(MapIndex)*Server()->TickSpeed() ;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHTIMEDOPEN;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHTIMEDCLOSE && Team() != TEAM_SUPER)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = false;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = Server()->Tick() + 1 + GameServer()->Collision()->GetSwitchDelay(MapIndex)*Server()->TickSpeed();
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHTIMEDCLOSE;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHCLOSE && Team() != TEAM_SUPER)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = false;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = 0;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHCLOSE;
	}
	// handle speedup tiles
	if(GameServer()->Collision()->IsSpeedup(PureMapIndex) == TILE_BOOST)
	{
		vec2 Direction, MaxVel, TempVel = m_Core.m_Vel;
		int Force, MaxSpeed = 0;
		float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;
		GameServer()->Collision()->GetSpeedup(MapIndex, &Direction, &Force, &MaxSpeed);
		if(Force == 255 && MaxSpeed)
		{
			m_Core.m_Vel = Direction * (MaxSpeed/5);
		}
		else
		{
			if(MaxSpeed > 0 && MaxSpeed < 5) MaxSpeed = 5;
			//dbg_msg("speedup tile start","Direction %f %f, Force %d, Max Speed %d", (Direction).x,(Direction).y, Force, MaxSpeed);
			if(
					((Direction.x < 0) && ((int)GameServer()->Collision()->GetPos(MapIndexL).x) && ((int)GameServer()->Collision()->GetPos(MapIndexL).x < (int)m_Core.m_Pos.x)) ||
					((Direction.x > 0) && ((int)GameServer()->Collision()->GetPos(MapIndexR).x) && ((int)GameServer()->Collision()->GetPos(MapIndexR).x > (int)m_Core.m_Pos.x)) ||
					((Direction.y > 0) && ((int)GameServer()->Collision()->GetPos(MapIndexB).y) && ((int)GameServer()->Collision()->GetPos(MapIndexB).y > (int)m_Core.m_Pos.y)) ||
					((Direction.y < 0) && ((int)GameServer()->Collision()->GetPos(MapIndexT).y) && ((int)GameServer()->Collision()->GetPos(MapIndexT).y < (int)m_Core.m_Pos.y))
					)
					m_Core.m_Pos = m_PrevPos;
			
			if(MaxSpeed > 0)
			{
				if(Direction.x > 0.0000001f)
					SpeederAngle = -atan(Direction.y / Direction.x);
				else if(Direction.x < 0.0000001f)
					SpeederAngle = atan(Direction.y / Direction.x) + 2.0f * asin(1.0f);
				else if(Direction.y > 0.0000001f)
					SpeederAngle = asin(1.0f);
				else
					SpeederAngle = asin(-1.0f);

				if(SpeederAngle < 0)
					SpeederAngle = 4.0f * asin(1.0f) + SpeederAngle;

				if(TempVel.x > 0.0000001f)
					TeeAngle = -atan(TempVel.y / TempVel.x);
				else if(TempVel.x < 0.0000001f)
					TeeAngle = atan(TempVel.y / TempVel.x) + 2.0f * asin(1.0f);
				else if(TempVel.y > 0.0000001f)
					TeeAngle = asin(1.0f);
				else
					TeeAngle = asin(-1.0f);

				if(TeeAngle < 0)
					TeeAngle = 4.0f * asin(1.0f) + TeeAngle;

				TeeSpeed = sqrt(pow(TempVel.x, 2) + pow(TempVel.y, 2));

				DiffAngle = SpeederAngle - TeeAngle;
				SpeedLeft = MaxSpeed / 5.0f - cos(DiffAngle) * TeeSpeed;

				//dbg_msg("speedup tile debug","MaxSpeed %i, TeeSpeed %f, SpeedLeft %f, SpeederAngle %f, TeeAngle %f", MaxSpeed, TeeSpeed, SpeedLeft, SpeederAngle, TeeAngle);

				if(abs(SpeedLeft) > Force && SpeedLeft > 0.0000001f)
					TempVel += Direction * Force;
				else if(abs(SpeedLeft) > Force)
					TempVel += Direction * -Force;
				else
					TempVel += Direction * SpeedLeft;
			}
			else
				TempVel += Direction * Force;

			if(TempVel.x > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)))
				TempVel.x = 0;
			if(TempVel.x < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)))
				TempVel.x = 0;
			if(TempVel.y < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)))
				TempVel.y = 0;
			if(TempVel.y > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)))
				TempVel.y = 0;
			m_Core.m_Vel = TempVel;
			//dbg_msg("speedup tile end","(Direction*Force) %f %f   m_Core.m_Vel%f %f",(Direction*Force).x,(Direction*Force).y,m_Core.m_Vel.x,m_Core.m_Vel.y);
			//dbg_msg("speedup tile end","Direction %f %f, Force %d, Max Speed %d", (Direction).x,(Direction).y, Force, MaxSpeed);
		}
	}
	m_LastBooster = MapIndex;
	int z = GameServer()->Collision()->IsTeleport(MapIndex);
	if(z && ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z-1].size())
	{
		m_Core.m_HookedPlayer = -1;
		m_Core.m_HookState = HOOK_RETRACTED;
		m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_Core.m_HookState = HOOK_RETRACTED;
		int Num = (((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z-1].size());
		m_Core.m_Pos = ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z-1][(!Num)?Num:rand() % Num];
		m_Core.m_HookPos = m_Core.m_Pos;
		return;
	}
	int evilz = GameServer()->Collision()->IsEvilTeleport(MapIndex);
	if(evilz && !m_Super && ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[evilz-1].size())
	{
		m_Core.m_HookedPlayer = -1;
		m_Core.m_HookState = HOOK_RETRACTED;
		m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		m_Core.m_HookState = HOOK_RETRACTED;
		GameWorld()->ReleaseHooked(GetPlayer()->GetCID());
		int Num = (((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[evilz-1].size());
		m_Core.m_Pos = ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[evilz-1][(!Num)?Num:rand() % Num];
		m_Core.m_HookPos = m_Core.m_Pos;
		m_Core.m_Vel = vec2(0,0);
		return;
	}
	// handle death-tiles
	if((GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH)&&
			!m_Super)
		{
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
			return;
		}
}

float point_distance(vec2 point, vec2 line_start, vec2 line_end)
{
	float res = -1.0f;
	vec2 dir = normalize(line_end-line_start);
	for(int i = 0; i < length(line_end-line_start); i++)
	{
		 vec2 step = dir;
		 step.x *= i;
		 step.y *= i;
		 float dist = distance(step+line_start, point);
		 if(res < 0 || dist < res)
			  res = dist;
	}
	return res;
}

void CCharacter::ResetPos()
{
	m_Core.m_Pos = m_OlderPos;
	//core.pos-=core.vel;
	m_Core.m_Vel = vec2(0,0);
	if(m_Core.m_Jumped >= 2)
		m_Core.m_Jumped = 1;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision(), &Controller->m_Teams.m_Core);
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	if(m_Doored)
	{
		ResetPos();
		m_Doored = false;
	}
	/*if((m_Stopped&STOPPED_LEFT && m_Core.m_Vel.x > 0)||(m_Stopped&STOPPED_RIGHT && m_Core.m_Vel.x < 0))
		m_Core.m_Vel.x=0;
	if((m_Stopped&STOPPED_BOTTOM && m_Core.m_Vel.y < 0)||(m_Stopped&STOPPED_TOP && m_Core.m_Vel.y > 0))
		m_Core.m_Vel.y=0;*/
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


	if(m_pPlayer->GetTeam() == -1)
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
		if(m_Core.m_pReset || m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
			m_Core.m_pReset = false;
		}
	}
}

bool CCharacter::Freeze(int Time)
{
	if ((Time <= 1 || m_Super || m_FreezeTime == -1) && Time != -1)
		 return false;
	if (m_FreezeTick < Server()->Tick() - Server()->TickSpeed())  		 
	{
		for(int i=0;i<NUM_WEAPONS;i++)
			if(m_aWeapons[i].m_Got)
			 {
				 m_aWeapons[i].m_Ammo = 0;
			 }
		m_Armor=0;
		m_FreezeTime=Time;
		m_FreezeTick=Server()->Tick();
		return true;
	}
	return false;
}

bool CCharacter::Freeze()
{
	int Time = Server()->TickSpeed()*3;
	if (Time <= 1 || m_Super || m_FreezeTime == -1)
		 return false;
	if (m_FreezeTick < Server()->Tick() - Server()->TickSpeed())
	{
		for(int i=0;i<NUM_WEAPONS;i++)
			if(m_aWeapons[i].m_Got)
			 {
				 m_aWeapons[i].m_Ammo = 0;
			 }
		m_Armor=0;
		m_Ninja.m_ActivationTick = Server()->Tick();
		m_FreezeTime=Time;
		m_FreezeTick=Server()->Tick();
		return true;
	}
	return false;
}

bool CCharacter::UnFreeze()
{
	if (m_FreezeTime>0)
	{
		m_Armor=10;
		for(int i=0;i<NUM_WEAPONS;i++)
			if(m_aWeapons[i].m_Got)
			 {
				 m_aWeapons[i].m_Ammo = -1;
			 }
		if(!m_aWeapons[m_ActiveWeapon].m_Got)
			m_ActiveWeapon = WEAPON_GUN;
		m_FreezeTime = 0;
		m_FreezeTick = 0;
		if (m_ActiveWeapon==WEAPON_HAMMER) m_ReloadTimer = 0;
		 return true;
	}
	return false;
}

void CCharacter::GiveAllWeapons()
{
	 for(int i=1;i<NUM_WEAPONS-1;i++)
	 {
		 m_aWeapons[i].m_Got = true;
		 if(!m_FreezeTime) m_aWeapons[i].m_Ammo = -1;
	 }
	 return;
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

void CCharacter::Die(int Killer, int Weapon)
{
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);
	CGameControllerDDRace* Controller = (CGameControllerDDRace*)GameServer()->m_pController;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	Controller->m_Teams.SetForceCharacterTeam(m_pPlayer->GetCID(), 0);

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
	MarkDestroy();
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	dbg_msg("CCHaracter::Die", "ID %d Player %d m_Core %d", GetPlayer()->GetCID() ,GetPlayer() ,&m_Core);
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());

	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	vec2 Temp = m_Core.m_Vel + Force;
	if(Temp.x > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)))
		Temp.x = 0;
	if(Temp.x < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)))
		Temp.x = 0;
	if(Temp.y < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)))
		Temp.y = 0;
	if(Temp.y > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)))
		Temp.y = 0;
	m_Core.m_Vel = Temp;
	/*
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



		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
*/
	// set attacker's face to happy (taunt!)
	/*if(g_Config.m_SvEmotionalTees)
	{
	if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
		if (pChr)
		{
			pChr->m_EmoteType = EMOTE_HAPPY;
			pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
		}
	}*///Removed you can set your emote via /emoteEMOTENAME
	//set the attacked face to pain
	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter* SnapChar = GameServer()->GetPlayerChar(SnappingClient);
	if(SnapChar && !SnapChar->m_Super && 
		GameServer()->m_apPlayers[SnappingClient]->GetTeam() != -1 &&
		!CanCollide(SnappingClient) && !GameServer()->m_apPlayers[SnappingClient]->m_IsUsingDDRaceClient) return;
	if(GetPlayer()->m_Invisible &&
		GetPlayer()->GetCID() != SnappingClient &&
		GameServer()->m_apPlayers[SnappingClient]->m_Authed < GetPlayer()->m_Authed
	)
		return;
	CNetObj_Character *Character = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		Character->m_Tick = 0;
		m_Core.Write(Character);
	}
	else
	{
		Character->m_Tick = m_ReckoningTick;
		m_SendCore.Write(Character);
	}

	if(m_DoSplash)
	{
		Character->m_Jumped = 3;
	}
	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	Character->m_Emote = m_EmoteType;

	Character->m_AmmoCount = 0;
	Character->m_Health = 0;
	Character->m_Armor = 0;

	if (m_FreezeTime > 0 || m_FreezeTime == -1)
	{
		Character->m_Emote = EMOTE_BLINK;
		Character->m_Weapon = WEAPON_NINJA;
		Character->m_AmmoCount = 0;
	}
	else
		Character->m_Weapon = m_ActiveWeapon;
	Character->m_AttackTick = m_AttackTick;

	Character->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient)
	{
		Character->m_Health = m_Health;
		Character->m_Armor = m_Armor;
		if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			Character->m_AmmoCount = (!m_FreezeTime)?m_aWeapons[m_ActiveWeapon].m_Ammo:0;
	}

	if (Character->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			Character->m_Emote = EMOTE_BLINK;
	}

	Character->m_PlayerState = m_PlayerState;
}
