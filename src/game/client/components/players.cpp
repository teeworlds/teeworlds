/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/skins.h>
#include <game/client/components/effects.h>
#include <game/client/components/sounds.h>
#include <game/client/components/controls.h>
#include <game/client/components/water.h>

#include "players.h"


inline float NormalizeAngular(float f)
{
	return fmod(f+pi*2, pi*2);
}

inline float AngularMixDirection (float Src, float Dst) { return sinf(Dst-Src) >0?1:-1; }
inline float AngularDistance(float Src, float Dst) { return asinf(sinf(Dst-Src)); }

inline float AngularApproach(float Src, float Dst, float Amount)
{
	float d = AngularMixDirection (Src, Dst);
	float n = Src + Amount*d;
	if(AngularMixDirection (n, Dst) != d)
		return Dst;
	return n;
}

void CPlayers::RenderHook(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CTeeRenderInfo *pRenderInfo,
	int ClientID
	) const
{
	CNetObj_Character Prev = *pPrevChar;
	CNetObj_Character Player = *pPlayerChar;
	CTeeRenderInfo RenderInfo = *pRenderInfo;

	float IntraTick = Client()->IntraGameTick();

	// set size
	RenderInfo.m_Size = 64.0f;

	if(m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(ClientID))
	{
		m_pClient->UsePredictedChar(&Prev, &Player, &IntraTick, ClientID);
	}

	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);

	// draw hook
	if(Prev.m_HookState>0 && Player.m_HookState>0)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		//Graphics()->QuadsBegin();

		vec2 Pos = Position;
		vec2 HookPos;

		if(Player.m_HookedPlayer != -1 && m_pClient->m_Snap.m_aCharacters[Player.m_HookedPlayer].m_Active)
		{
			// `HookedPlayer != -1` means that a player is being hooked
			bool Predicted = m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(Player.m_HookedPlayer);
			HookPos = m_pClient->GetCharPos(Player.m_HookedPlayer, Predicted);
		}
		else
		{
			// The hook is in the air, on a hookable tile or the hooked player is out of range
			HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Player.m_HookX, Player.m_HookY), IntraTick);
		}

		float d = distance(Pos, HookPos);
		vec2 Dir = normalize(Pos-HookPos);

		Graphics()->QuadsSetRotation(angle(Dir)+pi);

		// render head
		RenderTools()->SelectSprite(SPRITE_HOOK_HEAD);
		IGraphics::CQuadItem QuadItem(HookPos.x, HookPos.y, 24,16);
		Graphics()->QuadsDraw(&QuadItem, 1);

		// render chain
		RenderTools()->SelectSprite(SPRITE_HOOK_CHAIN);
		IGraphics::CQuadItem Array[1024];
		int i = 0;
		for(float f = 16; f < d && i < 1024; f += 16, i++)
		{
			vec2 p = HookPos + Dir*f;
			Array[i] = IGraphics::CQuadItem(p.x, p.y,16,16);
		}

		Graphics()->QuadsDraw(Array, i);
		Graphics()->QuadsSetRotation(0);
		Graphics()->QuadsEnd();

		RenderTools()->RenderTeeHand(&RenderInfo, Position, normalize(HookPos-Pos), -pi/2, vec2(20, 0));
	}
}

void CPlayers::RenderHarpoon(
	CNetObj_Character* pOwnerChar,
	CNetObj_Character* pOwnerPrev,
	CNetObj_Character* pVictimChar,
	CNetObj_Character* pVictimPrev,
	const CNetObj_HarpoonDragPlayer* DragInfo
) const
{
	float IntraTick = Client()->IntraGameTick();

	vec2 VictimPos;
	if (DragInfo->m_VictimID!=-1)
		VictimPos = m_pClient->GetCharPos(DragInfo->m_VictimID, m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(DragInfo->m_VictimID));
	else
		VictimPos = vec2(DragInfo->m_X / 100.0f, DragInfo->m_Y / 100.f);

	vec2 PlayerPos;
	if(DragInfo->m_OwnerID!=-1)
		PlayerPos = m_pClient->GetCharPos(DragInfo->m_OwnerID, m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(DragInfo->m_OwnerID));
	else
		PlayerPos = vec2(DragInfo->m_Owner_X / 100.0f, DragInfo->m_Owner_Y / 100.f);

	vec2 HarpoonPos;
	vec2 HarpoonLinePos;
	vec2 HarpoonDir = normalize(VictimPos - PlayerPos);
	float AngleDir = angle(HarpoonDir);
	HarpoonLinePos = HarpoonPos;
	HarpoonPos = VictimPos - HarpoonDir * 24;
	float AccountForAlignment = direction(AngleDir).x > 0 ? 0.10f : -0.10f; //Magic Values acquired through trial and error
	HarpoonLinePos = VictimPos - direction(AngleDir + AccountForAlignment) * 36;

	float Angle;
	if (DragInfo->m_OwnerID != -1)
		Angle = mix((float)pOwnerPrev->m_Angle, (float)pOwnerChar->m_Angle, IntraTick) / 256.0f;
	else
		Angle = 1; //doesn't matter, offscreen!
	vec2 WeaponPos = HarpoonMouthAlignment(PlayerPos, Angle);

	RenderTools()->RenderRope(WeaponPos, HarpoonLinePos, 8);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetRotation(AngleDir);
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_pSpriteProj, HarpoonDir.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
	RenderTools()->DrawSprite(HarpoonPos.x, HarpoonPos.y, g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_VisualSize * 0.67f);
	Graphics()->QuadsEnd();
}

void CPlayers::RenderEntityHarpoon(
	CNetObj_Character* pOwnerChar,
	CNetObj_Character* pOwnerPrev,
	const CNetObj_HarpoonDragPlayer* DragInfo
) const
{
	float IntraTick = Client()->IntraGameTick();

	vec2 PlayerPos;
	if (DragInfo->m_OwnerID != -1)
		PlayerPos = m_pClient->GetCharPos(DragInfo->m_OwnerID, m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(DragInfo->m_OwnerID));
	else
		PlayerPos = vec2(DragInfo->m_Owner_X / 100.0f, DragInfo->m_Owner_Y / 100.f);

	vec2 VictimEntityPos = vec2(DragInfo->m_X / 100.0f, DragInfo->m_Y / 100.0f);

	vec2 HarpoonPos;
	vec2 HarpoonLinePos;
	vec2 HarpoonDir = normalize(VictimEntityPos - PlayerPos);
	float AngleDir = angle(HarpoonDir);
	HarpoonLinePos = HarpoonPos;
	HarpoonPos = VictimEntityPos - HarpoonDir * 24;
	float AccountForAlignment = direction(AngleDir).x > 0 ? 0.10f : -0.10f; //Magic Values acquired through trial and error
	HarpoonLinePos = VictimEntityPos - direction(AngleDir + AccountForAlignment) * 36;

	float Angle;
	if (DragInfo->m_OwnerID != -1)
		Angle = mix((float)pOwnerPrev->m_Angle, (float)pOwnerChar->m_Angle, IntraTick) / 256.0f;
	else
		Angle = 1; //doesn't matter, offscreen!
	vec2 WeaponPos = HarpoonMouthAlignment(PlayerPos, Angle);

	RenderTools()->RenderRope(WeaponPos, HarpoonLinePos, 8);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetRotation(AngleDir);
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_pSpriteProj, HarpoonDir.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
	RenderTools()->DrawSprite(HarpoonPos.x, HarpoonPos.y, g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_VisualSize * 0.67f);
	Graphics()->QuadsEnd();
}
vec2 CPlayers::HarpoonMouthAlignment(vec2 PlayerPos, float Angle) const
{
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);
	float AccountForAlignment = direction(Angle).x > 0 ? -0.05f : 0.03f; //Magic Values acquired through trial and error
	return PlayerPos + direction(State.GetAttach()->m_Angle * pi * 2 + Angle + AccountForAlignment) * 52;
}

void CPlayers::RenderPlayer(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CNetObj_PlayerInfo *pPlayerInfo,
	const CTeeRenderInfo *pRenderInfo,
	int ClientID
	) const
{
	CNetObj_Character Prev = *pPrevChar;
	CNetObj_Character Player = *pPlayerChar;
	CTeeRenderInfo RenderInfo = *pRenderInfo;

	// set size
	RenderInfo.m_Size = 64.0f;

	if (!(random_int() % 20))
	{
		if (pPlayerChar->m_BreathBubbles != -1)
			m_pClient->m_pEffects->WaterBubble(vec2(pPlayerChar->m_X, pPlayerChar->m_Y));
	}

	float IntraTick = Client()->IntraGameTick();

	if(Prev.m_Angle < pi*-128 && Player.m_Angle > pi*128)
		Prev.m_Angle += 2*pi*256;
	else if(Prev.m_Angle > pi*128 && Player.m_Angle < pi*-128)
		Player.m_Angle += 2*pi*256;
	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, IntraTick)/256.0f;

	//float angle = 0;

	if(m_pClient->m_LocalClientID == ClientID && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		// just use the direct input if it's local player we are rendering
		Angle = angle(m_pClient->m_pControls->m_MousePos);
	}
	else
	{
		/*
		float mixspeed = Client()->FrameTime()*2.5f;
		if(player.attacktick != prev.attacktick) // shooting boosts the mixing speed
			mixspeed *= 15.0f;

		// move the delta on a constant speed on a x^2 curve
		float current = g_GameClient.m_aClients[info.cid].angle;
		float target = player.angle/256.0f;
		float delta = angular_distance(current, target);
		float sign = delta < 0 ? -1 : 1;
		float new_delta = delta - 2*mixspeed*sqrt(delta*sign)*sign + mixspeed*mixspeed;

		// make sure that it doesn't vibrate when it's still
		if(fabs(delta) < 2/256.0f)
			angle = target;
		else
			angle = angular_approach(current, target, fabs(delta-new_delta));

		g_GameClient.m_aClients[info.cid].angle = angle;*/
	}

	if(m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(ClientID))
	{
		m_pClient->UsePredictedChar(&Prev, &Player, &IntraTick, ClientID);
	}
	const bool Paused = m_pClient->IsWorldPaused() || m_pClient->IsDemoPlaybackPaused();

	vec2 Direction = direction(Angle);
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);
	vec2 Vel = mix(vec2(Prev.m_VelX/256.0f, Prev.m_VelY/256.0f), vec2(Player.m_VelX/256.0f, Player.m_VelY/256.0f), IntraTick);

	m_pClient->m_pFlow->Add(Position, Vel*100.0f, 10.0f);

	RenderInfo.m_GotAirJump = Player.m_Jumped&2?0:1;
	// && Player.m_VelY<0;
	bool Swimming = Collision()->CheckPoint(Player.m_X, Player.m_Y, 8); //is in water and has floating tendency
	bool Stationary = Player.m_VelX <= 1 && Player.m_VelX >= -1;
	bool InAir = !Collision()->CheckPoint(Player.m_X, Player.m_Y+16);
	bool WantOtherDir = (Player.m_Direction == -1 && Vel.x > 0) || (Player.m_Direction == 1 && Vel.x < 0);
	//bool DivingGear = Player.m_DivingGear;

	if (Player.m_DivingGear)
	{
		if (m_pClient->Config()->m_ClShowDivingGear)
		{
			RenderInfo.m_DivingGearTexture = m_pClient->m_pSkins->m_DivingGearTexture;
		}
		else if (Collision()->CheckPoint(Player.m_X, Player.m_Y, 8))
			RenderInfo.m_DivingGearTexture = m_pClient->m_pSkins->m_DivingGearTexture;
		else
			RenderInfo.m_DivingGearTexture.Invalidate();
	}
	else
		RenderInfo.m_DivingGearTexture.Invalidate();
	// evaluate animation
	const float WalkTimeMagic = 100.0f;
	float WalkTime =
		((Position.x >= 0)
			? fmod(Position.x, WalkTimeMagic)
			: WalkTimeMagic - fmod(-Position.x, WalkTimeMagic))
		/ WalkTimeMagic;
	float FloatTime =
		((Position.y >= 0)
			? fmod(Position.y, WalkTimeMagic)
			: WalkTimeMagic - fmod(-Position.y, WalkTimeMagic))
		/ WalkTimeMagic;
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);

	if (Swimming)
	{
		State.Add(&g_pData->m_aAnimations[ANIM_SWIM], FloatTime, 1.0f);
		float HorizontalSpeed = clamp(Vel.x, -10.0f, 10.0f);
		//float FeetAngle = State.GetBackFoot()->m_Angle + ;
		float Scale = Vel.x < 0 ? HorizontalSpeed / 10.0f : HorizontalSpeed / 10.0f * 3.0f;
		State.GetBackFoot()->m_Angle += 0.05f;
		State.GetFrontFoot()->m_Angle += 0.05f;
		State.GetBackFoot()->m_Angle += 0.05f * Scale;
		State.GetBackFoot()->m_X -= 5.0f * Scale / 3.0f;
		State.GetFrontFoot()->m_Angle += 0.05f * Scale;
		State.GetFrontFoot()->m_X -= 5.0f * Scale / 1.5f;
	}
	else if(InAir)
		State.Add(&g_pData->m_aAnimations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(Stationary)
		State.Add(&g_pData->m_aAnimations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else if(!WantOtherDir)
		State.Add(&g_pData->m_aAnimations[ANIM_WALK], WalkTime, 1.0f);

	static float s_LastGameTickTime = Client()->GameTickTime();
	static float s_LastIntraTick = IntraTick;
	static float s_TimeUntilAnimationFrame = 1.0f;
	bool UpdateSingleAnimationFrage = false;
	if(!Paused)
	{
		s_LastGameTickTime = Client()->GameTickTime();
		s_LastIntraTick = IntraTick;
		s_TimeUntilAnimationFrame -= m_pClient->GetAnimationPlaybackSpeed();
		if(s_TimeUntilAnimationFrame <= 0.0f)
		{
			s_TimeUntilAnimationFrame += 1.0f;
			UpdateSingleAnimationFrage = true;
		}
	}

	if (Player.m_Weapon == WEAPON_HAMMER)
	{
		float ct = (Client()->PrevGameTick() - Player.m_AttackTick) / (float)SERVER_TICK_SPEED + s_LastGameTickTime;
		State.Add(&g_pData->m_aAnimations[ANIM_HAMMER_SWING], clamp(ct * 5.0f, 0.0f, 1.0f), 1.0f);
	}
	else if (Player.m_Weapon == WEAPON_NINJA)
	{
		float ct = (Client()->PrevGameTick()-Player.m_AttackTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
		State.Add(&g_pData->m_aAnimations[ANIM_NINJA_SWING], clamp(ct*2.0f,0.0f,1.0f), 1.0f);
	}

	// do skidding
	if(!InAir && WantOtherDir && length(Vel*50) > 500.0f)
	{
		static int64 SkidSoundTime = 0;
		if(time_get()-SkidSoundTime > time_freq()/10)
		{
			m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_SKID, 0.25f, Position);
			SkidSoundTime = time_get();
		}

		m_pClient->m_pEffects->SkidTrail(
			Position+vec2(-Player.m_Direction*6,12),
			vec2(-Player.m_Direction*100*length(Vel),-50)
		);
	}

	vec2 PreWaterPos = vec2(pPrevChar->m_X, pPrevChar->m_Y);
	if (Collision()->TestBox(PreWaterPos, vec2(32, 32), CCollision::COLFLAG_WATER) ^ Collision()->TestBox(Position, vec2(32, 32), CCollision::COLFLAG_WATER))
		if (Collision()->IntersectWater(PreWaterPos, Position, &PreWaterPos, vec2(32, 32)))
			m_pClient->m_pWater->HitWater(PreWaterPos.x, PreWaterPos.y, abs(pPrevChar->m_VelY) / Config()->m_GfxWaveDivider); //trial and erorr

	if (Collision()->TestBox(vec2(Position.x, Position.y), vec2(32, 32 + 1.0f) * (2.0f / 3.0f), CCollision::COLFLAG_WATER) && (!Collision()->TestBox(vec2(Position.x, Position.y - 8.0f), vec2(32, 32) * (2.0f / 3.0f), CCollision::COLFLAG_WATER)))
	{
		float Chance = random_float();
		if(Chance<0.05)
			m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_WATER_MOVEMENT, 0.1f, Position);

	}

	if (Collision()->TestBox(vec2(Position.x, Position.y), vec2(32, 64), CCollision::COLFLAG_WATER) && (!Collision()->TestBox(vec2(Position.x, Position.y - 8.0f), vec2(32, 32) * (2.0f / 3.0f), CCollision::COLFLAG_WATER)))
	{
		m_pClient->m_pWater->CreateWave(Position.x, Position.y, Player.m_VelX);
	}




	// draw gun
	if(Player.m_Weapon >= 0)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation(State.GetAttach()->m_Angle*pi*2+Angle);
		vec2 Dir = Direction;
		float Recoil = 0.0f;
		vec2 p;
		int iw = clamp(Player.m_Weapon, 0, NUM_WEAPONS - 1);
		if (Player.m_Weapon == WEAPON_HARPOON&&Player.m_AmmoCount)
		{
			p = Position + Dir * g_pData->m_Weapons.m_aId[iw].m_Offsetx - Dir*Recoil*10.0f;
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_pSpriteProj, Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
			float OffsetY = -g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsety;
			if (Direction.x < 0)
				OffsetY = -OffsetY;

			vec2 DirY(-Dir.y, Dir.x);
			vec2 MuzzlePos = p + Dir * (g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsetx + 20.0f) + DirY * OffsetY;

			RenderTools()->DrawSprite(MuzzlePos.x, MuzzlePos.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize*0.67f);
		}
		// normal weapons
		
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_pSpriteBody, Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		if (Player.m_Weapon == WEAPON_HAMMER)
		{
			// Static position for hammer
			p = Position + vec2(State.GetAttach()->m_X, State.GetAttach()->m_Y);
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
			// if attack is under way, bash stuffs
			if(Direction.x < 0)
			{
				Graphics()->QuadsSetRotation(-pi/2-State.GetAttach()->m_Angle*pi*2);
				p.x -= g_pData->m_Weapons.m_aId[iw].m_Offsetx;
			}
			else
			{
				Graphics()->QuadsSetRotation(-pi/2+State.GetAttach()->m_Angle*pi*2);
			}
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
		}
		else if(Player.m_Weapon == WEAPON_NINJA)
		{
			p = Position;
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;

			if(Direction.x < 0)
			{
				Graphics()->QuadsSetRotation(-pi/2-State.GetAttach()->m_Angle*pi*2);
				p.x -= g_pData->m_Weapons.m_aId[iw].m_Offsetx;
				m_pClient->m_pEffects->PowerupShine(p+vec2(32,0), vec2(32,12));
			}
			else
			{
				Graphics()->QuadsSetRotation(-pi/2+State.GetAttach()->m_Angle*pi*2);
				m_pClient->m_pEffects->PowerupShine(p-vec2(32,0), vec2(32,12));
			}
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);

			// HADOKEN
			if((Client()->GameTick()-Player.m_AttackTick) <= (SERVER_TICK_SPEED / 6) && g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles)
			{
				const int IteX = random_int() % g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles;
				static int s_LastIteX = IteX;
				if(UpdateSingleAnimationFrage)
					s_LastIteX = IteX;

				if(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[s_LastIteX])
				{
					const vec2 Dir = normalize(vec2(pPlayerChar->m_X,pPlayerChar->m_Y) - vec2(pPrevChar->m_X, pPrevChar->m_Y));
					p = Position - Dir * g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsetx;
					Graphics()->QuadsSetRotation(angle(Dir));
					RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[s_LastIteX], 0);
					RenderTools()->DrawSprite(p.x, p.y, 160.0f);
				}
			}
		}
		else
		{
			// TODO: should be an animation
			const float RecoilTick = (Client()->GameTick() - Player.m_AttackTick + s_LastIntraTick)/5.0f;
			const float Recoil = RecoilTick < 1.0f ? sinf(RecoilTick*pi) : 0.0f;
			p = Position + Dir * (g_pData->m_Weapons.m_aId[iw].m_Offsetx - Recoil*10.0f);
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
		}
		

		if(Player.m_Weapon == WEAPON_GUN || Player.m_Weapon == WEAPON_SHOTGUN)
		{
			// check if we're firing stuff
			if(g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles)
			{
				const float MuzzleTick = Client()->GameTick() - Player.m_AttackTick + s_LastIntraTick;
				const int IteX = random_int() % g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles;
				static int s_LastIteX = IteX;
				if(UpdateSingleAnimationFrage)
					s_LastIteX = IteX;

				if(MuzzleTick < g_pData->m_Weapons.m_aId[iw].m_Muzzleduration && g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[s_LastIteX])
				{
					const bool FlipY = Direction.x < 0.0f;
					const float OffsetY = g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsety * (FlipY ? 1 : -1);
					const vec2 MuzzlePos = p + Dir * g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsetx + vec2(-Dir.y, Dir.x) * OffsetY;
					RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[s_LastIteX], FlipY ? SPRITE_FLAG_FLIP_Y : 0);
					RenderTools()->DrawSprite(MuzzlePos.x, MuzzlePos.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
				}
			}
		}
		Graphics()->QuadsEnd();

		switch (Player.m_Weapon)
		{
			case WEAPON_GUN: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -3*pi/4, vec2(-15, 4)); break;
			case WEAPON_SHOTGUN: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -pi/2, vec2(-5, 4)); break;
			case WEAPON_GRENADE: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -pi/2, vec2(-4, 7)); break;
		}

	}

	// render the "shadow" tee
	if(m_pClient->m_LocalClientID == ClientID && Config()->m_Debug)
	{
		vec2 GhostPosition = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());
		CTeeRenderInfo Ghost = RenderInfo;
		for(int p = 0; p < NUM_SKINPARTS; p++)
			Ghost.m_aColors[p].a *= 0.5f;
		RenderTools()->RenderTee(&State, &Ghost, Player.m_Emote, Direction, GhostPosition); // render ghost
	}

	RenderTools()->RenderTee(&State, &RenderInfo, Player.m_Emote, Direction, Position);

	if(pPlayerInfo->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_DOTDOT);
		IGraphics::CQuadItem QuadItem(Position.x + 24, Position.y - 40, 64,64);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	CGameClient::CClientData *pClientData = &m_pClient->m_aClients[ClientID];
	if(pClientData->m_EmoticonStart != -1)
	{
		// adjust start tick if world paused; not if demo paused because ticks are synchronized with demo
		static int s_LastGameTick = Client()->GameTick();
		if(m_pClient->IsWorldPaused())
			pClientData->m_EmoticonStart += Client()->GameTick() - s_LastGameTick;
		s_LastGameTick = Client()->GameTick();

		const float TotalEmoteLifespan = 2 * Client()->GameTickSpeed();
		const float SinceStart = (Client()->GameTick() - pClientData->m_EmoticonStart) / (float)Client()->GameTickSpeed();
		const float FromEnd = (pClientData->m_EmoticonStart + TotalEmoteLifespan - Client()->GameTick()) / (float)Client()->GameTickSpeed();
		if(SinceStart > 0.0f && FromEnd > 0.0f)
		{
			const float Size = 64.0f;
			const float Alpha = FromEnd < 0.2f ? FromEnd / 0.2f : 1.0f;
			const float HeightFactor = SinceStart < 0.1f ? SinceStart / 0.1f : 1.0f;
			const float Wiggle = SinceStart < 0.2f ? SinceStart / 0.2f : 0.0f;

			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
			Graphics()->QuadsBegin();
			Graphics()->QuadsSetRotation(pi/6*sinf(5*Wiggle));
			Graphics()->SetColor(Alpha, Alpha, Alpha, Alpha);
			RenderTools()->SelectSprite(SPRITE_OOP + pClientData->m_Emoticon); // pClientData->m_Emoticon is an offset from the first emoticon
			IGraphics::CQuadItem QuadItem(Position.x, Position.y - 23 - Size * HeightFactor / 2.0f, Size, Size * HeightFactor);
			Graphics()->QuadsDraw(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
	}
}

void CPlayers::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	static const CNetObj_PlayerInfo *s_apInfo[MAX_CLIENTS];
	static CTeeRenderInfo s_aRenderInfo[MAX_CLIENTS];

	// update RenderInfo for ninja
	bool IsTeamplay = (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) != 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		s_apInfo[i] = (const CNetObj_PlayerInfo *)Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);
		s_aRenderInfo[i] = m_pClient->m_aClients[i].m_RenderInfo;

		if(m_pClient->m_Snap.m_aCharacters[i].m_Cur.m_Weapon == WEAPON_NINJA)
		{
			// change the skin for the player to the ninja
			int Skin = m_pClient->m_pSkins->Find("x_ninja", true);
			if(Skin != -1)
			{
				const CSkins::CSkin *pNinja = m_pClient->m_pSkins->Get(Skin);
				for(int p = 0; p < NUM_SKINPARTS; p++)
				{
					if(IsTeamplay)
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_ColorTexture;
						int ColorVal = m_pClient->m_pSkins->GetTeamColor(true, pNinja->m_aPartColors[p], m_pClient->m_aClients[i].m_Team, p);
						s_aRenderInfo[i].m_aColors[p] = m_pClient->m_pSkins->GetColorV4(ColorVal, p==SKINPART_MARKING);
					}
					else if(pNinja->m_aUseCustomColors[p])
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_ColorTexture;
						s_aRenderInfo[i].m_aColors[p] = m_pClient->m_pSkins->GetColorV4(pNinja->m_aPartColors[p], p==SKINPART_MARKING);
					}
					else
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_OrgTexture;
						s_aRenderInfo[i].m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
					}
				}
			}
		}
	}

	for (int i = 0; i < m_pClient->m_Snap.HarpoonDragCount;i++)
	{
		if (m_pClient->m_Snap.m_paHarpoonDragData[i]->m_Type == HARPOON_IN_CHARACTER)
		{
			const CNetObj_HarpoonDragPlayer* HarpoonDragInfo = m_pClient->m_Snap.m_paHarpoonDragData[i];
			CNetObj_Character* pOwnerChar;
			CNetObj_Character* pPrevOwnerChar;
			CNetObj_Character* pVictimChar;
			CNetObj_Character* pPrevVictimChar;
			if (HarpoonDragInfo->m_VictimID != -1)
			{
				pVictimChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_VictimID].m_Cur;
				pPrevVictimChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_VictimID].m_Prev;
			}
			else
			{
				pVictimChar = 0x0; // Don't worry.
				pPrevVictimChar = 0x0;
			}

			if (HarpoonDragInfo->m_OwnerID != -1)
			{
				pOwnerChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_OwnerID].m_Cur;
				pPrevOwnerChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_OwnerID].m_Prev;
			}
			else
			{
				pOwnerChar = 0x0; // Don't worry.
				pPrevOwnerChar = 0x0;
			}

			RenderHarpoon(
				pOwnerChar,
				pPrevOwnerChar,
				pVictimChar,
				pPrevVictimChar,
				HarpoonDragInfo
			);
		}
		else //if(m_pClient->m_Snap.m_paHarpoonPrevDragData[i]) //we will need prev snapshot data
		{
			const CNetObj_HarpoonDragPlayer* HarpoonDragInfo = m_pClient->m_Snap.m_paHarpoonDragData[i];
			CNetObj_Character* pOwnerChar;
			CNetObj_Character* pPrevOwnerChar;
			//const CNetObj_HarpoonDragPlayer* HarpoonPrevDragInfo = m_pClient->m_Snap.m_paHarpoonPrevDragData[i];
			if (HarpoonDragInfo->m_OwnerID != -1)
			{
				pOwnerChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_OwnerID].m_Cur;
				pPrevOwnerChar = &m_pClient->m_Snap.m_aCharacters[HarpoonDragInfo->m_OwnerID].m_Prev;
			}
			else
			{
				pOwnerChar = 0x0; // Don't worry.
				pPrevOwnerChar = 0x0;
			}


			RenderEntityHarpoon(
				pOwnerChar,
				pPrevOwnerChar,
				HarpoonDragInfo
			);
		}
	}
	// render other players in two passes, first pass we render the other, second pass we render our self
	for(int p = 0; p < 4; p++)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			// only render active characters
			if(!m_pClient->m_Snap.m_aCharacters[i].m_Active || !s_apInfo[i])
				continue;

			//
			bool Local = m_pClient->m_LocalClientID == i;
			if((p % 2) == 0 && Local) continue;
			if((p % 2) == 1 && !Local) continue;

			CNetObj_Character *pPrevChar = &m_pClient->m_Snap.m_aCharacters[i].m_Prev;
			CNetObj_Character *pCurChar = &m_pClient->m_Snap.m_aCharacters[i].m_Cur;

			if(p<2)
			{
				RenderHook(
						pPrevChar,
						pCurChar,
						&s_aRenderInfo[i],
						i
					);
			}
			else
			{
				RenderPlayer(
						pPrevChar,
						pCurChar,
						s_apInfo[i],
						&s_aRenderInfo[i],
						i
					);
			}
		}
	}
}
