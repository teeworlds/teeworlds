/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/demo.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/effects.h>

#include "items.h"


void CItems::RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID)
{
	// get positions
	float Curvature = 0;
	float Speed = 0;
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		Curvature = m_pClient->m_Tuning.m_GrenadeCurvature;
		Speed = m_pClient->m_Tuning.m_GrenadeSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_SHOTGUN)
	{
		Curvature = m_pClient->m_Tuning.m_ShotgunCurvature;
		Speed = m_pClient->m_Tuning.m_ShotgunSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_GUN)
	{
		Curvature = m_pClient->m_Tuning.m_GunCurvature;
		Speed = m_pClient->m_Tuning.m_GunSpeed;
	}

	static float s_LastGameTickTime = Client()->GameTickTime();
	if(m_pClient->m_Snap.m_pGameData && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
		s_LastGameTickTime = Client()->GameTickTime();
	float Ct = (Client()->PrevGameTick()-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
	if(Ct < 0)
		return; // projectile havn't been shot yet

	vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
	vec2 StartVel(pCurrent->m_VelX/100.0f, pCurrent->m_VelY/100.0f);
	vec2 Pos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct);
	vec2 PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct-0.001f);


	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Type, 0, NUM_WEAPONS-1)].m_pSpriteProj);
	vec2 Vel = Pos-PrevPos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), Client()->IntraGameTick());


	// add particle for this projectile
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		m_pClient->m_pEffects->SmokeTrail(Pos, Vel*-1);
		static float s_Time = 0.0f;
		static float s_LastLocalTime = Client()->LocalTime();

		if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
			if(!pInfo->m_Paused)
				s_Time += (Client()->LocalTime()-s_LastLocalTime)*pInfo->m_Speed;
		}
		else
		{
			if(m_pClient->m_Snap.m_pGameData && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
				s_Time += Client()->LocalTime()-s_LastLocalTime;
		}

		Graphics()->QuadsSetRotation(s_Time*pi*2*2 + ItemID);
		s_LastLocalTime = Client()->LocalTime();
	}
	else
	{
		m_pClient->m_pEffects->BulletTrail(Pos);

		if(length(Vel) > 0.00001f)
			Graphics()->QuadsSetRotation(angle(Vel));
		else
			Graphics()->QuadsSetRotation(0);

	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 32, 32);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

void CItems::RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	float Angle = 0.0f;
	float Size = 64.0f;
	const int c[] = {
		SPRITE_PICKUP_HEALTH,
		SPRITE_PICKUP_ARMOR,
		SPRITE_PICKUP_GRENADE,
		SPRITE_PICKUP_SHOTGUN,
		SPRITE_PICKUP_LASER,
		SPRITE_PICKUP_NINJA
		};
	RenderTools()->SelectSprite(c[pCurrent->m_Type]);

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
	}
	

	Graphics()->QuadsSetRotation(Angle);

	static float s_Time = 0.0f;
	static float s_LastLocalTime = Client()->LocalTime();
	float Offset = Pos.y/32.0f + Pos.x/32.0f;
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(!pInfo->m_Paused)
			s_Time += (Client()->LocalTime()-s_LastLocalTime)*pInfo->m_Speed;
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameData && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			s_Time += Client()->LocalTime()-s_LastLocalTime;
 	}
	Pos.x += cosf(s_Time*2.0f+Offset)*2.5f;
	Pos.y += sinf(s_Time*2.0f+Offset)*2.5f;
	s_LastLocalTime = Client()->LocalTime();
	RenderTools()->DrawSprite(Pos.x, Pos.y, Size);
	Graphics()->QuadsEnd();
}

void CItems::RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameDataFlag *pPrevGameDataFlag, const CNetObj_GameDataFlag *pCurGameDataFlag)
{
	float Angle = 0.0f;
	float Size = 42.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	if(pCurrent->m_Team == TEAM_RED)
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
	else
		RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);

	Graphics()->QuadsSetRotation(Angle);

	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());

	if(pCurGameDataFlag)
	{
		// make sure that the flag isn't interpolated between capture and return
		if(pPrevGameDataFlag &&
			((pCurrent->m_Team == TEAM_RED && pPrevGameDataFlag->m_FlagCarrierRed != pCurGameDataFlag->m_FlagCarrierRed) ||
			(pCurrent->m_Team == TEAM_BLUE && pPrevGameDataFlag->m_FlagCarrierBlue != pCurGameDataFlag->m_FlagCarrierBlue)))
			Pos = vec2(pCurrent->m_X, pCurrent->m_Y);

		// make sure to use predicted position if we are the carrier
		if(m_pClient->m_LocalClientID != -1 &&
			((pCurrent->m_Team == TEAM_RED && pCurGameDataFlag->m_FlagCarrierRed == m_pClient->m_LocalClientID) ||
			(pCurrent->m_Team == TEAM_BLUE && pCurGameDataFlag->m_FlagCarrierBlue == m_pClient->m_LocalClientID)))
			Pos = m_pClient->m_LocalCharacterPos;
	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y-Size*0.75f, Size, Size*2);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();
}


void CItems::RenderLaser(const struct CNetObj_Laser *pCurrent)
{
	vec2 Pos = vec2(pCurrent->m_X, pCurrent->m_Y);
	vec2 From = vec2(pCurrent->m_FromX, pCurrent->m_FromY);
	vec2 Dir = normalize(Pos-From);

	float Ticks = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	float Ms = (Ticks/50.0f) * 1000.0f;
	float a = Ms / m_pClient->m_Tuning.m_LaserBounceDelay;
	a = clamp(a, 0.0f, 1.0f);
	float Ia = 1-a;

	vec2 Out, Border;

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	//vec4 inner_color(0.15f,0.35f,0.75f,1.0f);
	//vec4 outer_color(0.65f,0.85f,1.0f,1.0f);

	// do outline
	vec4 OuterColor(0.075f, 0.075f, 0.25f, 1.0f);
	Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (7.0f*Ia);

	IGraphics::CFreeformItem Freeform(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	// do inner
	vec4 InnerColor(0.5f, 0.5f, 1.0f, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (5.0f*Ia);
	Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f); // center

	Freeform = IGraphics::CFreeformItem(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	Graphics()->QuadsEnd();

	// render head
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
		Graphics()->QuadsBegin();

		int Sprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(Sprites[Client()->GameTick()%3]);
		Graphics()->QuadsSetRotation(Client()->GameTick());
		Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
		IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f);
		QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	Graphics()->BlendNormal();
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
			RenderProjectile((const CNetObj_Projectile *)pData, Item.m_ID);
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
		//ModAPI
		else if(Item.m_Type == NETOBJTYPE_MODAPI_SPRITE)
		{
			RenderModAPISprite((const CNetObj_ModAPI_Sprite *)pData);
		}
		else if(Item.m_Type == NETOBJTYPE_MODAPI_LINE)
		{
			RenderModAPILine((const CNetObj_ModAPI_Line *)pData);
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
			if (pPrev)
			{
				const void *pPrevGameDataFlag = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_GAMEDATAFLAG, m_pClient->m_Snap.m_GameDataFlagSnapID);
				RenderFlag(static_cast<const CNetObj_Flag *>(pPrev), static_cast<const CNetObj_Flag *>(pData),
							static_cast<const CNetObj_GameDataFlag *>(pPrevGameDataFlag), m_pClient->m_Snap.m_pGameDataFlag);
			}
		}
	}
}

void CItems::RenderModAPISprite(const CNetObj_ModAPI_Sprite *pCurrent)
{
	if(!ModAPIGraphics()) return;
	
	const CModAPI_Sprite* pSprite = ModAPIGraphics()->GetSprite(pCurrent->m_SpriteId);
	if(pSprite == 0) return;
	
	if(pSprite->m_External)
	{
		const CModAPI_Image* pImage = ModAPIGraphics()->GetImage(pSprite->m_ImageId);
		if(pImage == 0) return;
		
		Graphics()->BlendNormal();
		Graphics()->TextureSet(pImage->m_Texture);
		Graphics()->QuadsBegin();
	}
	else
	{
		int Texture;
		
		switch(pSprite->m_ImageId)
		{
			case MODAPI_INTERNALIMG_GAME:
				Texture = IMAGE_GAME;
				break;
			default:
				return;
		}
		
		Graphics()->BlendNormal();
		Graphics()->TextureSet(g_pData->m_aImages[Texture].m_Id);
		Graphics()->QuadsBegin();
	}
	
	float Angle = 2.0*pi*static_cast<float>(pCurrent->m_Angle)/360.0f;
	float Size = pCurrent->m_Size;

	RenderTools()->SelectModAPISprite(pSprite);
	
	Graphics()->QuadsSetRotation(Angle);

	vec2 Pos = vec2(pCurrent->m_X, pCurrent->m_Y);
	
	RenderTools()->DrawSprite(Pos.x, Pos.y, Size);
	Graphics()->QuadsEnd();
}


void CItems::RenderModAPILine(const struct CNetObj_ModAPI_Line *pCurrent)
{
	//Get line style
	const CModAPI_LineStyle* pLineStyle = ModAPIGraphics()->GetLineStyle(pCurrent->m_LineStyleId);
	if(pLineStyle == 0) return;
	
	//Geometry
	vec2 StartPos = vec2(pCurrent->m_StartX, pCurrent->m_StartY);
	vec2 EndPos = vec2(pCurrent->m_EndX, pCurrent->m_EndY);
	vec2 Dir = normalize(EndPos-StartPos);
	vec2 DirOrtho = vec2(-Dir.y, Dir.x);
	float Length = distance(StartPos, EndPos);

	float ScaleFactor = 1.0f;
	
	float Ticks = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	float Ms = (Ticks/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	if(pLineStyle->m_AnimationType == MODAPI_LINESTYLE_ANIMATION_SCALEDOWN)
	{
		float Speed = Ms / pLineStyle->m_AnimationSpeed;
		ScaleFactor = 1.0f - clamp(Speed, 0.0f, 1.0f);
	}

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	//Outer line
	if(pLineStyle->m_OuterWidth > 0)
	{
		float Width = pLineStyle->m_OuterWidth;
		const vec4& Color = pLineStyle->m_OuterColor;
		
		Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPos.x-Out.x, StartPos.y-Out.y,
				StartPos.x+Out.x, StartPos.y+Out.y,
				EndPos.x-Out.x, EndPos.y-Out.y,
				EndPos.x+Out.x, EndPos.y+Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}

	//Inner line
	if(pLineStyle->m_InnerWidth > 0)
	{
		float Width = pLineStyle->m_InnerWidth;
		const vec4& Color = pLineStyle->m_InnerColor;
		
		Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPos.x-Out.x, StartPos.y-Out.y,
				StartPos.x+Out.x, StartPos.y+Out.y,
				EndPos.x-Out.x, EndPos.y-Out.y,
				EndPos.x+Out.x, EndPos.y+Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}

	Graphics()->QuadsEnd();
	
	//Sprite for line
	if(pLineStyle->m_LineSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_LineSprite1;
		
		const CModAPI_Sprite* pSprite = ModAPIGraphics()->GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		Graphics()->BlendNormal();
			
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = ModAPIGraphics()->GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			Graphics()->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			Graphics()->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		Graphics()->QuadsBegin();		
		Graphics()->QuadsSetRotation(angle(Dir));
		RenderTools()->SelectModAPISprite(pSprite);
		
		IGraphics::CQuadItem Array[1024];
		int i = 0;
		float step = pLineStyle->m_LineSpriteSizeX - pLineStyle->m_LineSpriteOverlapping;
		for(float f = pLineStyle->m_LineSpriteSizeX/2.0f; f < Length && i < 1024; f += step, i++)
		{
			vec2 p = StartPos + Dir*f;
			Array[i] = IGraphics::CQuadItem(p.x, p.y, pLineStyle->m_LineSpriteSizeX, pLineStyle->m_LineSpriteSizeY * ScaleFactor);
		}

		Graphics()->QuadsDraw(Array, i);
		Graphics()->QuadsSetRotation(0.0f);
		Graphics()->QuadsEnd();
	}
	
	//Sprite for start point
	if(pLineStyle->m_StartPointSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_StartPointSprite1;
		
		if(pLineStyle->m_StartPointSprite2 > SpriteId) //Animation
		{
			int NbFrame = 1 + pLineStyle->m_StartPointSprite2 - pLineStyle->m_StartPointSprite1;
			SpriteId = pLineStyle->m_StartPointSprite1 + (static_cast<int>(Ms)/pLineStyle->m_StartPointSpriteAnimationSpeed)%NbFrame;
		}
		
		const CModAPI_Sprite* pSprite = ModAPIGraphics()->GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		Graphics()->BlendNormal();
		
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = ModAPIGraphics()->GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			Graphics()->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			Graphics()->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		Graphics()->QuadsBegin();		
		Graphics()->QuadsSetRotation(angle(Dir));
		RenderTools()->SelectModAPISprite(pSprite);
		
		vec2 NewPos = StartPos + (Dir * pLineStyle->m_StartPointSpriteX) + (DirOrtho * pLineStyle->m_StartPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_StartPointSpriteSizeX,
			pLineStyle->m_StartPointSpriteSizeY);
		
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsSetRotation(0.0f);
		Graphics()->QuadsEnd();
	}
	
	//Sprite for end point
	if(pLineStyle->m_EndPointSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_EndPointSprite1;
		
		if(pLineStyle->m_EndPointSprite2 > SpriteId) //Animation
		{
			int NbFrame = 1 + pLineStyle->m_EndPointSprite2 - pLineStyle->m_EndPointSprite1;
			SpriteId = pLineStyle->m_EndPointSprite1 + (static_cast<int>(Ms)/pLineStyle->m_EndPointSpriteAnimationSpeed)%NbFrame;
		}
		
		const CModAPI_Sprite* pSprite = ModAPIGraphics()->GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		Graphics()->BlendNormal();
		
		//Define sprite texture
		if(pSprite->m_External)
		{
			const CModAPI_Image* pImage = ModAPIGraphics()->GetImage(pSprite->m_ImageId);
			if(pImage == 0) return;
			
			Graphics()->TextureSet(pImage->m_Texture);
		}
		else
		{
			int Texture = 0;
			switch(pSprite->m_ImageId)
			{
				case MODAPI_INTERNALIMG_GAME:
					Texture = IMAGE_GAME;
					break;
				default:
					return;
			}
			
			Graphics()->TextureSet(g_pData->m_aImages[Texture].m_Id);
		}
		
		Graphics()->QuadsBegin();		
		Graphics()->QuadsSetRotation(angle(Dir));
		RenderTools()->SelectModAPISprite(pSprite);
		
		vec2 NewPos = EndPos + (Dir * pLineStyle->m_EndPointSpriteX) + (DirOrtho * pLineStyle->m_EndPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_EndPointSpriteSizeX,
			pLineStyle->m_EndPointSpriteSizeY);
		
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsSetRotation(0.0f);
		Graphics()->QuadsEnd();
	}
}

