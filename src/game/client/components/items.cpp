/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/demo.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/effects.h>
#include <game/client/components/water.h>
#include <game/client/components/controls.h>

#include "items.h"
//#include "players.h"


void CItems::RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID)
{
	// get positions
	float Curvature = 0;
	float Speed = 0;
	float WaterResistance = m_pClient->m_Tuning.m_LiquidProjectileResistance;
	float BulletSize = 1.0f;
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		Curvature = m_pClient->m_Tuning.m_GrenadeCurvature;
		Speed = m_pClient->m_Tuning.m_GrenadeSpeed;
		BulletSize = 7.5f;
	}
	else if(pCurrent->m_Type == WEAPON_SHOTGUN)
	{
		Curvature = m_pClient->m_Tuning.m_ShotgunCurvature;
		Speed = m_pClient->m_Tuning.m_ShotgunSpeed;
		WaterResistance = m_pClient->m_Tuning.m_ShotgunWaterResistance;
	}
	else if(pCurrent->m_Type == WEAPON_GUN)
	{
		Curvature = m_pClient->m_Tuning.m_GunCurvature;
		Speed = m_pClient->m_Tuning.m_GunSpeed;
	}
	

	static float s_LastGameTickTime = Client()->GameTickTime();
	if(!m_pClient->IsWorldPaused() && !m_pClient->IsDemoPlaybackPaused())
		s_LastGameTickTime = Client()->GameTickTime();

	float Ct;
	if(m_pClient->ShouldUsePredicted() && Config()->m_ClPredictProjectiles)
		Ct = ((float)(Client()->PredGameTick() - 1 - pCurrent->m_StartTick) + Client()->PredIntraGameTick())/(float)SERVER_TICK_SPEED;
	else
		Ct = (Client()->PrevGameTick()-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
	if(Ct < 0)
	{
		if(Ct > -s_LastGameTickTime / 2)
		{
			// Fixup the timing which might be screwed during demo playback because
			// s_LastGameTickTime depends on the system timer, while the other part
			// Client()->PrevGameTick()-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED
			// is virtually constant (for projectiles fired on the current game tick):
			// (x - (x+2)) / 50 = -0.04
			//
			// We have a strict comparison for the passed time being more than the time between ticks
			// if(CurtickStart > m_Info.m_CurrentTime) in CDemoPlayer::Update()
			// so on the practice the typical value of s_LastGameTickTime varies from 0.02386 to 0.03999
			// which leads to Ct from -0.00001 to -0.01614.
			// Round up those to 0.0 to fix missing rendering of the projectile.
			Ct = 0;
		}
		else
		{
			return; // projectile haven't been shot yet
		}
	}

	vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
	vec2 StartVel(pCurrent->m_VelX/100.0f, pCurrent->m_VelY/100.0f);

	vec2 Pos;
	vec2 PrevPos;
	if (pCurrent->m_Water)
	{
		if (pCurrent->m_Water == 1)
		{
			Pos = CalcPos(StartPos, StartVel, Curvature, Speed * WaterResistance, Ct);
			PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed * WaterResistance, Ct - 0.001f);

			//vec2 PreWaterPos = CalcPos(vec2(pPrev->m_X, pPrev->m_Y), vec2(pPrev->m_VelX/100.0f, pPrev->m_VelY/100.0f), Curvature, Speed, Ct - 0.001f);
			vec2 PreWaterPos = CalcPos(StartPos, StartVel, Curvature, Speed * WaterResistance, Ct - 0.001f);

			// Need to figure out a smart way for this. Perhaps let server help out?
			if (Collision()->IntersectWater(PreWaterPos, Pos, &PreWaterPos, vec2(1, 1)))
			{
				//m_pClient->m_pEffects->PowerupShine(PreWaterPos,vec2(1.0f,1.0f));
				//Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", "Water Surface Found");
				if (m_pClient->m_pWater->HitWater(PreWaterPos.x, PreWaterPos.y, distance(PrevPos, Pos) * BulletSize * 25))
				{
					/*char aBuf[128];
					char aBuf2[128];
					char aBuf3[128];
					str_format(aBuf, sizeof(aBuf), "%f", distance(PrevPos, Pos));
					str_format(aBuf2, sizeof(aBuf2), "PreWater: %f  %f", PrevPos.x, PrevPos.y);
					str_format(aBuf3, sizeof(aBuf3), "Pos: %f  %f", Pos.x, Pos.y);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf2);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf3);*/
				}

			}
		}
		else
		{
			Pos = CalcPos(StartPos, StartVel, Curvature * 2, Speed * 0.6f, Ct);
			PrevPos = CalcPos(StartPos, StartVel, Curvature * 2, Speed * 0.6f, Ct - 0.001f);
		}
	}
	else
	{
		Pos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct);
		PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct - 0.001f);
	}

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Type, 0, NUM_WEAPONS-1)].m_pSpriteProj);
	const vec2 Vel = Pos-PrevPos;

	// add particle for this projectile
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		m_pClient->m_pEffects->SmokeTrail(Pos, Vel*-1);
		const float Now = Client()->LocalTime();
		static float s_Time = 0.0f;
		static float s_LastLocalTime = Now;
		s_Time += (Now - s_LastLocalTime) * m_pClient->GetAnimationPlaybackSpeed();
		Graphics()->QuadsSetRotation(s_Time*pi*2*2 + ItemID);
		s_LastLocalTime = Now;
	}
	else
	{
		m_pClient->m_pEffects->BulletTrail(Pos);
		Graphics()->QuadsSetRotation(length(Vel) > 0.00001f ? angle(Vel) : 0);
	}
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 32, 32);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

void CItems::RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent)
{
	if (pCurrent->m_DisturbedTick)
	{
		int DisturbedTick = pCurrent->m_DisturbedTick;
		int DisturbedMax = m_pClient->m_Tuning.m_PickUpDisturbedTime * m_pClient->Client()->GameTickSpeed();

		if (DisturbedMax * 0.2 >= DisturbedTick) //20% and less, flicker quicker
		{
			if ((m_pClient->Client()->GameTick() % 10) / 5)
				return;
		}
		else if (DisturbedMax * 0.5 >= DisturbedTick) //we are halfway, do not render every 10 ticks
		{
			if ((m_pClient->Client()->GameTick() % 20) / 10)
				return;
		}
	}


	if (pCurrent->m_Type == PICKUP_DIVING)
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DIVING_GEAR].m_Id);
	else
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	float Size = 64.0f;
	const int aSprites[] = {
		SPRITE_PICKUP_HEALTH,
		SPRITE_PICKUP_ARMOR,
		SPRITE_PICKUP_GRENADE,
		SPRITE_PICKUP_SHOTGUN,
		SPRITE_PICKUP_LASER,
		SPRITE_PICKUP_NINJA,
		SPRITE_PICKUP_GUN,
		SPRITE_PICKUP_HAMMER,
		SPRITE_PICKUP_HARPOON,
		SPRITE_DIVING_GEAR,
		};

	RenderTools()->SelectSprite(aSprites[pCurrent->m_Type]);

	switch(pCurrent->m_Type)
	{
	case PICKUP_GRENADE:
		Size = g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_VisualSize;
		break;
	case PICKUP_SHOTGUN:
		Size = g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_VisualSize;
		break;
	case PICKUP_LASER:
		Size = g_pData->m_Weapons.m_aId[WEAPON_LASER].m_VisualSize;
		break;
	case PICKUP_NINJA:
		m_pClient->m_pEffects->PowerupShine(Pos, vec2(96,18));
		Size *= 2.0f;
		Pos.x -= 10.0f;
		break;
	case PICKUP_GUN:
		Size = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_VisualSize;
		break;
	case PICKUP_HAMMER:
		Size = g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_VisualSize;
		break;
	case PICKUP_DIVING:
		Size = 88.0f;
		break;
	case PICKUP_HARPOON:
		Size = g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_VisualSize;
		break;
	}

	const float Now = Client()->LocalTime();
	static float s_Time = 0.0f;
	static float s_LastLocalTime = Now;
	s_Time += (Now - s_LastLocalTime) * m_pClient->GetAnimationPlaybackSpeed();
	const float Offset = Pos.y/32.0f + Pos.x/32.0f;
	Pos.x += cosf(s_Time*2.0f+Offset)*2.5f;
	Pos.y += sinf(s_Time*2.0f+Offset)*2.5f;
	s_LastLocalTime = Now;

	Graphics()->QuadsSetRotation(0.0f);
	RenderTools()->DrawSprite(Pos.x, Pos.y, Size);
	Graphics()->QuadsEnd();
}

void CItems::RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameDataFlag *pPrevGameDataFlag, const CNetObj_GameDataFlag *pCurGameDataFlag)
{
	const float Size = 42.0f;
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());

	if(pCurGameDataFlag)
	{
		// make sure that the flag isn't interpolated between capture and return
		if(pPrevGameDataFlag &&
			((pCurrent->m_Team == TEAM_RED && pPrevGameDataFlag->m_FlagCarrierRed != pCurGameDataFlag->m_FlagCarrierRed) ||
			(pCurrent->m_Team == TEAM_BLUE && pPrevGameDataFlag->m_FlagCarrierBlue != pCurGameDataFlag->m_FlagCarrierBlue)))
			Pos = vec2(pCurrent->m_X, pCurrent->m_Y);

		int FlagCarrier = -1;
		if(pCurrent->m_Team == TEAM_RED && pCurGameDataFlag->m_FlagCarrierRed >= 0)
			FlagCarrier = pCurGameDataFlag->m_FlagCarrierRed;
		else if(pCurrent->m_Team == TEAM_BLUE && pCurGameDataFlag->m_FlagCarrierBlue >= 0)
			FlagCarrier = pCurGameDataFlag->m_FlagCarrierBlue;

		// make sure to use predicted position
		if(FlagCarrier >= 0 && FlagCarrier < MAX_CLIENTS && m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(FlagCarrier))
			Pos = m_pClient->GetCharPos(FlagCarrier, true);
	}

	Graphics()->BlendNormal();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(pCurrent->m_Team == TEAM_RED ? SPRITE_FLAG_RED : SPRITE_FLAG_BLUE);
	Graphics()->QuadsSetRotation(0.0f);
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y-Size*0.75f, Size, Size*2);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CItems::RenderLaser(const struct CNetObj_Laser *pCurrent)
{
	const vec2 Pos = vec2(pCurrent->m_X, pCurrent->m_Y);
	const vec2 From = vec2(pCurrent->m_FromX, pCurrent->m_FromY);
	const vec2 Dir = normalize(Pos-From);

	static int s_LastGameTick = Client()->GameTick();
	static float s_LastIntraTick = Client()->IntraGameTick();
	if(!m_pClient->IsWorldPaused())
	{
		s_LastGameTick = Client()->GameTick();
		s_LastIntraTick = Client()->IntraGameTick();
	}

	// This is not using s_LastGameTick because m_StartTick is synchronized by the server
	const float LifetimeMillis = 1000.0f * (Client()->GameTick() - pCurrent->m_StartTick + s_LastIntraTick) / Client()->GameTickSpeed();
	const float RemainingRelativeLifetime = 1.0f - clamp(LifetimeMillis / m_pClient->m_Tuning.m_LaserBounceDelay, 0.0f, 1.0f);

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	// do outline
	const vec4 OuterColor(0.075f, 0.075f, 0.25f, 1.0f);
	const vec2 Outer = vec2(Dir.y, -Dir.x) * (7.0f*RemainingRelativeLifetime);
	Graphics()->SetColor(OuterColor);
	IGraphics::CFreeformItem Freeform(
			From.x-Outer.x, From.y-Outer.y,
			From.x+Outer.x, From.y+Outer.y,
			Pos.x-Outer.x, Pos.y-Outer.y,
			Pos.x+Outer.x, Pos.y+Outer.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	// do inner
	const vec4 InnerColor(0.5f, 0.5f, 1.0f, 1.0f);
	const vec2 Inner = vec2(Dir.y, -Dir.x) * (5.0f*RemainingRelativeLifetime);
	Graphics()->SetColor(InnerColor);
	Freeform = IGraphics::CFreeformItem(
			From.x-Inner.x, From.y-Inner.y,
			From.x+Inner.x, From.y+Inner.y,
			Pos.x-Inner.x, Pos.y-Inner.y,
			Pos.x+Inner.x, Pos.y+Inner.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	Graphics()->QuadsEnd();

	// render head
	Graphics()->BlendNormal();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();

	const int aSprites[] = { SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03 };
	RenderTools()->SelectSprite(aSprites[s_LastGameTick%3]);
	Graphics()->QuadsSetRotation(s_LastGameTick);
	Graphics()->SetColor(OuterColor);
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->SetColor(InnerColor);
	QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();

	Graphics()->BlendNormal();
}

void CItems::RenderPlayer(const CNetObj_Character* pCurrent)
{
	if (!(random_int() % 20))
	{
		if(pCurrent->m_BreathBubbles!=-1)
			m_pClient->m_pEffects->WaterBubble(vec2(pCurrent->m_X,pCurrent->m_Y));
	}
}
void CItems::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PROJECTILE)
		{
			//const void* pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			RenderProjectile((const CNetObj_Projectile*)pData, Item.m_ID);
			//RenderProjectile((const CNetObj_Projectile *)pData, Item.m_ID);
		}
		else if(Item.m_Type == NETOBJTYPE_PICKUP)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if(pPrev)
				RenderPickup((const CNetObj_Pickup *)pPrev, (const CNetObj_Pickup *)pData);
		}
		else if(Item.m_Type == NETOBJTYPE_LASER)
		{
			RenderLaser((const CNetObj_Laser *)pData);
		}
		else if (Item.m_Type == NETOBJTYPE_CHARACTER)
		{
			
			RenderPlayer((const CNetObj_Character*)pData);
		}
		else if (Item.m_Type == NETOBJTYPE_HARPOON)
		{
			RenderHarpoon((const CNetObj_Harpoon*)pData);
		}
	}

	// render flag
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_FLAG)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if(pPrev)
			{
				const void *pPrevGameDataFlag = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_GAMEDATAFLAG, m_pClient->m_Snap.m_GameDataFlagSnapID);
				RenderFlag(static_cast<const CNetObj_Flag *>(pPrev), static_cast<const CNetObj_Flag *>(pData),
							static_cast<const CNetObj_GameDataFlag *>(pPrevGameDataFlag), m_pClient->m_Snap.m_pGameDataFlag);
			}
		}
	}
}

void CItems::RenderHarpoon(const CNetObj_Harpoon* pCurrent)
{
	static float s_LastGameTickTime = Client()->GameTickTime();
	if (!m_pClient->IsWorldPaused())
		s_LastGameTickTime = Client()->GameTickTime();
	
	float Ct;
	if (m_pClient->ShouldUsePredicted() && Config()->m_ClPredictProjectiles)
		Ct = ((float)(Client()->PredGameTick() - 1 - pCurrent->m_SpawnTick) + Client()->PredIntraGameTick()) / (float)SERVER_TICK_SPEED;
	else
		Ct = (Client()->PrevGameTick() - pCurrent->m_SpawnTick) / (float)SERVER_TICK_SPEED + s_LastGameTickTime;
	if (Ct < 0)
		return; // projectile haven't been shot yet

	vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
	vec2 StartVel(pCurrent->m_Dir_X / 100.0f, pCurrent->m_Dir_Y / 100.0f);

	vec2 Pos;
	vec2 PrevPos;
	Pos = CalcPos(StartPos, StartVel, m_pClient->m_Tuning.m_HarpoonCurvature, m_pClient->m_Tuning.m_HarpoonSpeed, Ct);
	PrevPos = CalcPos(StartPos, StartVel, m_pClient->m_Tuning.m_HarpoonCurvature, m_pClient->m_Tuning.m_HarpoonSpeed, Ct - 0.001f);


	const vec2 Vel = Pos - PrevPos;

	float Angle;
	vec2 PlayerPos;
	if (pCurrent->m_OwnerID != -1)
	{
		Angle = mix((float)m_pClient->m_Snap.m_aCharacters[pCurrent->m_OwnerID].m_Prev.m_Angle, (float)m_pClient->m_Snap.m_aCharacters[pCurrent->m_OwnerID].m_Cur.m_Angle, Client()->IntraGameTick()) / 256.0f;
		PlayerPos = m_pClient->GetCharPos(pCurrent->m_OwnerID, m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(pCurrent->m_OwnerID));
	}
	else
	{
		Angle = 1.0f; //doesn't matter, offscreen!
		PlayerPos = vec2(pCurrent->m_Owner_X / 100.0f, pCurrent->m_Owner_Y / 100.0f);
	}
	
	
	
	if (m_pClient->m_LocalClientID == pCurrent->m_OwnerID && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		// just use the direct input if it's local player we are rendering
		Angle = angle(m_pClient->m_pControls->m_MousePos);
	}
	vec2 WeaponPos = m_pClient->GetHarpoonAlignment(PlayerPos, Angle);

	float HarpoonAngle = angle(Vel);

	HarpoonAngle += direction(Angle).x > 0 ? 0.23f : -0.23f;

	vec2 HarpoonPos = Pos + direction(HarpoonAngle) * -15;//Config()->m_GfxLineMultiplier / 100.0f;
	RenderTools()->RenderRope(WeaponPos, HarpoonPos, 8);

	HarpoonAngle = angle(Vel);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	//float Size = g_pData->m_Weapons.m_aId[;
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[WEAPON_HARPOON].m_pSpriteProj, Vel.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
	//, 
	
	
	Graphics()->QuadsSetRotation(length(direction(HarpoonAngle)) > 0.00001f ? angle(direction(HarpoonAngle)) : 0);

	//RenderTools()->DrawSprite(Pos.x, Pos.y, 64.0f);
	//Graphics()->QuadsDraw(&QuadItem, 1);
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 48, 32);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}