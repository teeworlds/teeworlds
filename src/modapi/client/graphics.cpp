#include <modapi/client/graphics.h>
#include <engine/storage.h>
#include <modapi/shared/assetsfile.h>
#include <generated/client_data.h>
#include <game/client/render.h>
#include <engine/shared/datafile.h>
#include <engine/shared/config.h>

#include <game/gamecore.h>

#include <engine/external/pnglite/pnglite.h>
	
CModAPI_Client_Graphics::CModAPI_Client_Graphics(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
}

#define CREATE_INTERNAL_SPRITE(id, name, image, x, y, w, h) {\
		CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.NewAsset(CModAPI_AssetPath::Internal(id));\
		pSprite->SetName(name);\
		pSprite->Init(CModAPI_AssetPath::Internal(image), x, y, w, h);\
	}

void CModAPI_Client_Graphics::Init()
{
	//Images
	{
		CModAPI_Asset_Image* pImage = m_ImagesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_IMAGE_GAME));
		pImage->SetName("game.png");
		pImage->m_Texture = g_pData->m_aImages[IMAGE_GAME].m_Id;
		pImage->m_Width = 1024;
		pImage->m_Height = 512;
		pImage->m_GridWidth = 32;
		pImage->m_GridHeight = 16;
	}
	
	//Sprites
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_HAMMER, "hammer", MODAPI_IMAGE_GAME, 2, 1, 4, 3);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GUN, "gun", MODAPI_IMAGE_GAME, 2, 4, 4, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_SHOTGUN, "shotgun", MODAPI_IMAGE_GAME, 2, 6, 8, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GRENADE, "grenade", MODAPI_IMAGE_GAME, 2, 8, 7, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_LASER, "laserRifle", MODAPI_IMAGE_GAME, 2, 12, 7, 3);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_NINJA, "ninjaSword", MODAPI_IMAGE_GAME, 2, 10, 8, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GUN_MUZZLE1, "gunMuzzle1", MODAPI_IMAGE_GAME, 8, 4, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GUN_MUZZLE2, "gunMuzzle2", MODAPI_IMAGE_GAME, 12, 4, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GUN_MUZZLE3, "gunMuzzle3", MODAPI_IMAGE_GAME, 16, 4, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_SHOTGUN_MUZZLE1, "shotgunMuzzle1", MODAPI_IMAGE_GAME, 12, 6, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_SHOTGUN_MUZZLE2, "shotgunMuzzle2", MODAPI_IMAGE_GAME, 16, 6, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_SHOTGUN_MUZZLE3, "shotgunMuzzle3", MODAPI_IMAGE_GAME, 20, 6, 3, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_NINJA_MUZZLE1, "ninjaMuzzle1", MODAPI_IMAGE_GAME, 25, 0, 7, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_NINJA_MUZZLE2, "ninjaMuzzle2", MODAPI_IMAGE_GAME, 25, 4, 7, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_NINJA_MUZZLE3, "ninjaMuzzle3", MODAPI_IMAGE_GAME, 25, 8, 7, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_HAMMER_CURSOR, "hammerCursor", MODAPI_IMAGE_GAME, 0, 0, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GUN_CURSOR, "gunCursor", MODAPI_IMAGE_GAME, 0, 4, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_SHOTGUN_CURSOR, "shotgunCursor", MODAPI_IMAGE_GAME, 0, 6, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_GRENADE_CURSOR, "grenadeCursor", MODAPI_IMAGE_GAME, 0, 8, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_LASER_CURSOR, "laserCursor", MODAPI_IMAGE_GAME, 0, 12, 2 , 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_NINJA_CURSOR, "ninjaCursor", MODAPI_IMAGE_GAME, 0, 10 , 2, 2);
	
	//Sprites list
	{
		CModAPI_Asset_List* pList = m_SpritesCatalog.NewList(CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_GUN_MUZZLES));
		pList->SetName("gunMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_GUN_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_GUN_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_GUN_MUZZLE3));
	}
	{
		CModAPI_Asset_List* pList = m_SpritesCatalog.NewList(CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_SHOTGUN_MUZZLES));
		pList->SetName("shotgunMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_SHOTGUN_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_SHOTGUN_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_SHOTGUN_MUZZLE3));
	}
	{
		CModAPI_Asset_List* pList = m_SpritesCatalog.NewList(CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_NINJA_MUZZLES));
		pList->SetName("ninjaMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_NINJA_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_NINJA_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(MODAPI_SPRITE_NINJA_MUZZLE3));
	}

	//Animations
	vec2 BodyPos = vec2(0.0f, -4.0f);
	vec2 FootPos = vec2(0.0f, 10.0f);
	
		//Idle
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_IDLE_BACKFOOT));
		pAnim->SetName("teeIdle_backFoot");
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(-7.0f,  0.0f),  0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_IDLE_FRONTFOOT));
		pAnim->SetName("teeIdle_frontFoot");
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(7.0f,  0.0f),  0.0f, 1.f);
	}
	
		//InAir
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_INAIR_BACKFOOT));
		pAnim->SetName("teeAir_backFoot");
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(-3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_INAIR_FRONTFOOT));
		pAnim->SetName("teeAir_frontFoot");
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	}
	
		//Walk
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_BODY));
		pAnim->SetName("teeWalk_body");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.2f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.4f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.6f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.8f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(1.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_BACKFOOT));
		pAnim->SetName("teeWalk_backFoot");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(  8.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.2f, FootPos + vec2( -8.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.4f, FootPos + vec2(-10.0f, -4.0f), 0.2f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.6f, FootPos + vec2( -8.0f, -8.0f), 0.3f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.8f, FootPos + vec2(  4.0f, -4.0f), -0.2f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(1.0f, FootPos + vec2(  8.0f,  0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_FRONTFOOT));
		pAnim->SetName("teeWalk_frontFoot");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f, FootPos + vec2(-10.0f, -4.0f), 0.2f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.2f, FootPos + vec2( -8.0f, -8.0f), 0.3f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.4f, FootPos + vec2(  4.0f, -4.0f), -0.2f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.6f, FootPos + vec2(  8.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.8f, FootPos + vec2(  8.0f,  0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(1.0f, FootPos + vec2(-10.0f, -4.0f), 0.2f*pi*2.0f, 1.f);
	}
	
		//Weapons
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_HAMMERATTACK_WEAPON));
		pAnim->SetName("hammerAttack_weapon");
		pAnim->AddKeyFrame(0.0f/5.0f, vec2(0.0f, 0.0f), -pi/2-0.10f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.2f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.25f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.4f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.30f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.6f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.25f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.8f/5.0f, vec2(0.0f, 0.0f), -pi/2-0.10f*pi*2.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_WEAPON));
		pAnim->SetName("gunAttack_weapon");
		pAnim->AddKeyFrame(0.00f, vec2(32.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(32.0f - 10.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(32.0f, 0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_MUZZLE));
		pAnim->SetName("gunAttack_muzzle");
		pAnim->AddKeyFrame(0.00f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.02f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.04f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.06f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.08f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.10f, vec2(82.0f, -6.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.12f, vec2(82.0f, -6.0f), 0.0f, 0.0f, 0);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_BACKHAND));
		pAnim->SetName("gunAttack_backHand");
		pAnim->AddKeyFrame(0.00f, vec2(0.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(-10.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(0.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON));
		pAnim->SetName("shotgunAttack_weapon");
		pAnim->AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE));
		pAnim->SetName("shotgunAttack_muzzle");
		pAnim->AddKeyFrame(0.00f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.02f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.04f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.06f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.08f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.10f, vec2(94.0f, -6.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.12f, vec2(94.0f, -6.0f), 0.0f, 0.0f, 0);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND));
		pAnim->SetName("shotgunAttack_backHand");
		pAnim->AddKeyFrame(0.00f, vec2(0.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(-10.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(0.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GRENADEATTACK_WEAPON));
		pAnim->SetName("grenadeAttack_weapon");
		pAnim->AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GRENADEATTACK_BACKHAND));
		pAnim->SetName("grenadeAttack_backhand");
		pAnim->AddKeyFrame(0.00f, vec2(0.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(-10.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(0.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_LASERATTACK_WEAPON));
		pAnim->SetName("laserAttack_weapon");
		pAnim->AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
		pAnim->AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_NINJAATTACK_WEAPON));
		pAnim->SetName("ninjaAttack_weapon");
		pAnim->AddKeyFrame(0.00f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.25f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.10f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.05f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.15f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.35f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.42f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.40f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(0.50f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.35f*pi*2.0f, 1.f);
		pAnim->AddKeyFrame(1.00f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.25f*pi*2.0f, 1.f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ANIMATION_NINJAATTACK_MUZZLE));
		pAnim->SetName("ninjaAttack_muzzle");
		pAnim->AddKeyFrame(0.00f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.02f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.04f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.06f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 0);
		pAnim->AddKeyFrame(0.08f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 1);
		pAnim->AddKeyFrame(0.10f, vec2(-40.0f, -4.0f), 0.0f, 1.0f, 2);
		pAnim->AddKeyFrame(0.12f, vec2(-40.0f, -4.0f), 0.0f, 0.0f, 0);
	}
	
	//TeeAnimations
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_IDLE));
		pAnim->SetName("teeIdle");
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_IDLE_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_IDLE_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_INAIR));
		pAnim->SetName("teeAir");
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_INAIR_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_INAIR_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_WALK));
		pAnim->SetName("teeWalk");
		pAnim->m_BodyAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_BODY);
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_WALK_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_GUNATTACK));
		pAnim->SetName("teeGunAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = 4;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_SHOTGUNATTACK));
		pAnim->SetName("teeShotgunAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = -2;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_GRENADEATTACK));
		pAnim->SetName("teeGrenadeAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GRENADEATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = -2;
	}
	
	//Weapons
		//Hammer
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_HAMMER));
		pAttach->SetName("hammer");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_HAMMER_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_HAMMER),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_HAMMERATTACK_WEAPON),
			MODAPI_TEEALIGN_HORIZONTAL,
			4,
			-20
		);
	}
		//Gun
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_GUN));
		pAttach->SetName("gun");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_GUN_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_GUNATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_GUN_MUZZLES),
			64,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_MUZZLE),
			MODAPI_TEEALIGN_AIM,
			0,
			4
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_GUN),
			64,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GUNATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			4
		);
	}
		//Shotgun
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_SHOTGUN));
		pAttach->SetName("shotgun");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_SHOTGUN_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_SHOTGUNATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_SHOTGUN_MUZZLES),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_SHOTGUN),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Grenade
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_GRENADE));
		pAttach->SetName("grenade");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_GRENADE_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(MODAPI_TEEANIMATION_GRENADEATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_GRENADE),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_GRENADEATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Laser
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_LASER));
		pAttach->SetName("laser");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_LASER_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_LASER),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_LASERATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Ninja
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(MODAPI_ATTACH_NINJA));
		pAttach->SetName("ninja");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(MODAPI_SPRITE_NINJA_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::InternalList(MODAPI_SPRITELIST_NINJA_MUZZLES),
			160,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_NINJAATTACK_MUZZLE),
			MODAPI_TEEALIGN_DIRECTION,
			0,
			0
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(MODAPI_SPRITE_NINJA),
			96,
			CModAPI_AssetPath::Internal(MODAPI_ANIMATION_NINJAATTACK_WEAPON),
			MODAPI_TEEALIGN_HORIZONTAL,
			0,
			0
		);
	}
}

CModAPI_LineStyle* CModAPI_Client_Graphics::GetLineStyle(int Id)
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_LINESTYLES)
			return 0;
		else
			return &m_InternalLineStyles[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalLineStyles.size())
			return 0;
		else
			return &m_ExternalLineStyles[Id];
	}
}

CModAPI_AssetPath CModAPI_Client_Graphics::AddImage(IStorage* pStorage, int StorageType, const char* pFilename)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
	{
		io_close(File);
	}
	else
	{
		dbg_msg("mod", "failed to open file. filename='%s'", pFilename);
		return CModAPI_AssetPath::Null();
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("mod", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
		{
			png_close_file(&Png); // ignore_convention
		}
		return CModAPI_AssetPath::Null();
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA) || Png.width > (2<<12) || Png.height > (2<<12)) // ignore_convention
	{
		dbg_msg("mod", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return CModAPI_AssetPath::Null();
	}

	pBuffer = (unsigned char *) mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	int Format;
	if(Png.color_type == PNG_TRUECOLOR) 
		Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA)
		Format = CImageInfo::FORMAT_RGBA;
	
	CModAPI_AssetPath Path;
	CModAPI_Asset_Image* pImage = m_ImagesCatalog.NewExternalAsset(&Path);
	pImage->SetName(pFilename);
	pImage->m_Width = Png.width;
	pImage->m_Height = Png.height;
	pImage->m_Format = Format;
	pImage->m_pData = pBuffer;
	pImage->m_Texture = m_pGraphics->LoadTextureRaw(Png.width, Png.height, Format, pBuffer, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

	return Path;
}

void CModAPI_Client_Graphics::DeleteAsset(int Type, CModAPI_AssetPath Path)
{
	
}

int CModAPI_Client_Graphics::SaveInAssetsFile(IStorage *pStorage, const char *pFileName)
{
	CDataFileWriter df;
	if(!df.Open(pStorage, pFileName))
	{
		dbg_msg("ModAPIGraphics", "can't create the assets file %s", pFileName);
		return 0;
	}
	
	m_ImagesCatalog.SaveInAssetsFile(&df);
	m_SpritesCatalog.SaveInAssetsFile(&df);
	m_AnimationsCatalog.SaveInAssetsFile(&df);
	m_TeeAnimationsCatalog.SaveInAssetsFile(&df);
	m_AttachesCatalog.SaveInAssetsFile(&df);
	
	df.Finish();
}

int CModAPI_Client_Graphics::OnAssetsFileLoaded(IModAPI_AssetsFile* pAssetsFile)
{
	m_ImagesCatalog.LoadFromAssetsFile(this, pAssetsFile);
	m_SpritesCatalog.LoadFromAssetsFile(this, pAssetsFile);
	m_AnimationsCatalog.LoadFromAssetsFile(this, pAssetsFile);
	m_TeeAnimationsCatalog.LoadFromAssetsFile(this, pAssetsFile);
	m_AttachesCatalog.LoadFromAssetsFile(this, pAssetsFile);
	
	/*
	
	//Load line styles
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_LINESTYLE, &Start, &Num);
		
		m_ExternalLineStyles.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_LineStyle *pItem = (CModAPI_ModItem_LineStyle*) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id > Num) return 0;
			
			CModAPI_LineStyle* pLineStyle = &m_ExternalLineStyles[pItem->m_Id];
			
			pLineStyle->m_OuterWidth = static_cast<float>(pItem->m_OuterWidth);
			pLineStyle->m_OuterColor = ModAPI_IntToColor(pItem->m_OuterColor);
			pLineStyle->m_InnerWidth = static_cast<float>(pItem->m_InnerWidth);
			pLineStyle->m_InnerColor = ModAPI_IntToColor(pItem->m_InnerColor);
			
			pLineStyle->m_LineSpriteType = pItem->m_LineSpriteType;
			pLineStyle->m_LineSprite1 = pItem->m_LineSprite1;
			pLineStyle->m_LineSprite2 = pItem->m_LineSprite2;
			pLineStyle->m_LineSpriteSizeX = pItem->m_LineSpriteSizeX;
			pLineStyle->m_LineSpriteSizeY = pItem->m_LineSpriteSizeY;
			pLineStyle->m_LineSpriteOverlapping = pItem->m_LineSpriteOverlapping;
			pLineStyle->m_LineSpriteAnimationSpeed = pItem->m_LineSpriteAnimationSpeed;
			
			pLineStyle->m_StartPointSprite1 = pItem->m_StartPointSprite1;
			pLineStyle->m_StartPointSprite2 = pItem->m_StartPointSprite2;
			pLineStyle->m_StartPointSpriteX = pItem->m_StartPointSpriteX;
			pLineStyle->m_StartPointSpriteY = pItem->m_StartPointSpriteY;
			pLineStyle->m_StartPointSpriteSizeX = pItem->m_StartPointSpriteSizeX;
			pLineStyle->m_StartPointSpriteSizeY = pItem->m_StartPointSpriteSizeY;
			pLineStyle->m_StartPointSpriteAnimationSpeed = pItem->m_StartPointSpriteAnimationSpeed;
			
			pLineStyle->m_EndPointSprite1 = pItem->m_EndPointSprite1;
			pLineStyle->m_EndPointSprite2 = pItem->m_EndPointSprite2;
			pLineStyle->m_EndPointSpriteX = pItem->m_EndPointSpriteX;
			pLineStyle->m_EndPointSpriteY = pItem->m_EndPointSpriteY;
			pLineStyle->m_EndPointSpriteSizeX = pItem->m_EndPointSpriteSizeX;
			pLineStyle->m_EndPointSpriteSizeY = pItem->m_EndPointSpriteSizeY;
			pLineStyle->m_EndPointSpriteAnimationSpeed = pItem->m_EndPointSpriteAnimationSpeed;
			
			pLineStyle->m_AnimationType = pItem->m_AnimationType;
			pLineStyle->m_AnimationSpeed = static_cast<float>(pItem->m_AnimationSpeed);
		}
	}
	
	return 1;
	*/
}

int CModAPI_Client_Graphics::OnAssetsFileUnloaded()
{
	m_AttachesCatalog.Unload(this);
	m_TeeAnimationsCatalog.Unload(this);
	m_AnimationsCatalog.Unload(this);
	m_SpritesCatalog.Unload(this);
	m_ImagesCatalog.Unload(this);
}

bool CModAPI_Client_Graphics::TextureSet(CModAPI_AssetPath AssetPath)
{
	const CModAPI_Asset_Image* pImage = m_ImagesCatalog.GetAsset(AssetPath);
	if(pImage == 0)
		return false;
	
	m_pGraphics->TextureSet(pImage->m_Texture);
	
	return true;
}

void CModAPI_Client_Graphics::DrawSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int Flags, float Opacity)
{
	//Get sprite
	const CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.GetAsset(SpritePath);
	if(pSprite == 0)
		return;
	
	//Get texture
	const CModAPI_Asset_Image* pImage = m_ImagesCatalog.GetAsset(pSprite->m_ImagePath);
	if(pImage == 0)
		return;

	m_pGraphics->BlendNormal();
	m_pGraphics->TextureSet(pImage->m_Texture);
	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(1.0f*Opacity, 1.0f*Opacity, 1.0f*Opacity, Opacity);
	
	//Compute texture subquad
	float texX0 = pSprite->m_X/(float)pImage->m_GridWidth;
	float texX1 = (pSprite->m_X + pSprite->m_Width - 1.0f/32.0f)/(float)pImage->m_GridWidth;
	float texY0 = pSprite->m_Y/(float)pImage->m_GridHeight;
	float texY1 = (pSprite->m_Y + pSprite->m_Height - 1.0f/32.0f)/(float)pImage->m_GridHeight;
	
	float Temp = 0;
	if(Flags&MODAPI_SPRITEFLAG_FLIP_Y)
	{
		Temp = texY0;
		texY0 = texY1;
		texY1 = Temp;
	}

	if(Flags&MODAPI_SPRITEFLAG_FLIP_X)
	{
		Temp = texX0;
		texX0 = texX1;
		texX1 = Temp;
	}

	m_pGraphics->QuadsSetSubset(texX0, texY0, texX1, texY1);
	
	m_pGraphics->QuadsSetRotation(Angle);

	//Compute sprite size
	float ratio = sqrtf(pSprite->m_Width * pSprite->m_Width + pSprite->m_Height * pSprite->m_Height);
	vec2 QuadSize = vec2(Size * pSprite->m_Width/ratio, Size * pSprite->m_Height/ratio);
	
	//Draw
	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, QuadSize.x, QuadSize.y);
	m_pGraphics->QuadsDraw(&QuadItem, 1);
	
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawAnimatedSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int SpriteFlag, CModAPI_AssetPath AnimationPath, float Time, vec2 Offset)
{
	const CModAPI_Asset_Animation* pAnimation = m_AnimationsCatalog.GetAsset(AnimationPath);
	if(pAnimation == 0) return;
	
	CModAPI_Asset_Animation::CFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
		
	vec2 AnimationPos = Frame.m_Pos + rotate(Offset, Frame.m_Angle);
	
	float FrameAngle = Frame.m_Angle;
	if(SpriteFlag & MODAPI_SPRITEFLAG_FLIP_ANIM_X)
	{
		AnimationPos.x = -AnimationPos.x;
		FrameAngle = -FrameAngle;
	}
	else if(SpriteFlag & MODAPI_SPRITEFLAG_FLIP_ANIM_Y)
	{
		AnimationPos.y = -AnimationPos.y;
		FrameAngle = -FrameAngle;
	}
	
	vec2 SpritePos = Pos + rotate(AnimationPos, Angle);
	
	DrawSprite(SpritePath, SpritePos, Size, Angle + FrameAngle, SpriteFlag, Frame.m_Opacity);
}

void CModAPI_Client_Graphics::DrawLine(CRenderTools* pRenderTools,int LineStyleID, vec2 StartPoint, vec2 EndPoint, float Ms)
{
	/*
	const CModAPI_LineStyle* pLineStyle = GetLineStyle(LineStyleID);
	if(pLineStyle == 0) return;
	
	vec2 Dir = normalize(EndPoint-StartPoint);
	vec2 DirOrtho = vec2(-Dir.y, Dir.x);
	float Length = distance(StartPoint, EndPoint);
	float ScaleFactor = 1.0f;
	if(pLineStyle->m_AnimationType == MODAPI_LINESTYLE_ANIMATION_SCALEDOWN)
	{
		float Speed = Ms / pLineStyle->m_AnimationSpeed;
		ScaleFactor = 1.0f - clamp(Speed, 0.0f, 1.0f);
	}

	m_pGraphics->BlendNormal();
	m_pGraphics->TextureClear();
	m_pGraphics->QuadsBegin();

	//Outer line
	if(pLineStyle->m_OuterWidth > 0)
	{
		float Width = pLineStyle->m_OuterWidth;
		const vec4& Color = pLineStyle->m_OuterColor;
		
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPoint.x-Out.x, StartPoint.y-Out.y,
				StartPoint.x+Out.x, StartPoint.y+Out.y,
				EndPoint.x-Out.x, EndPoint.y-Out.y,
				EndPoint.x+Out.x, EndPoint.y+Out.y);
		m_pGraphics->QuadsDrawFreeform(&Freeform, 1);
	}

	//Inner line
	if(pLineStyle->m_InnerWidth > 0)
	{
		float Width = pLineStyle->m_InnerWidth;
		const vec4& Color = pLineStyle->m_InnerColor;
		
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);
		vec2 Out = DirOrtho * Width * ScaleFactor;
		
		IGraphics::CFreeformItem Freeform(
				StartPoint.x-Out.x, StartPoint.y-Out.y,
				StartPoint.x+Out.x, StartPoint.y+Out.y,
				EndPoint.x-Out.x, EndPoint.y-Out.y,
				EndPoint.x+Out.x, EndPoint.y+Out.y);
		m_pGraphics->QuadsDrawFreeform(&Freeform, 1);
	}

	m_pGraphics->QuadsEnd();
	
	//Sprite for line
	if(pLineStyle->m_LineSprite1 >= 0)
	{
		int SpriteId = pLineStyle->m_LineSprite1;
		
		const CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.GetAsset(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
			
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImagePath))
			return;
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		IGraphics::CQuadItem Array[1024];
		int i = 0;
		float step = pLineStyle->m_LineSpriteSizeX - pLineStyle->m_LineSpriteOverlapping;
		for(float f = pLineStyle->m_LineSpriteSizeX/2.0f; f < Length && i < 1024; f += step, i++)
		{
			vec2 p = StartPoint + Dir*f;
			Array[i] = IGraphics::CQuadItem(p.x, p.y, pLineStyle->m_LineSpriteSizeX, pLineStyle->m_LineSpriteSizeY * ScaleFactor);
		}

		m_pGraphics->QuadsDraw(Array, i);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
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
		
		const CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.GetAsset(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImagePath))
			return;
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		vec2 NewPos = StartPoint + (Dir * pLineStyle->m_StartPointSpriteX) + (DirOrtho * pLineStyle->m_StartPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_StartPointSpriteSizeX,
			pLineStyle->m_StartPointSpriteSizeY);
		
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
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
		
		const CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.GetAsset(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImagePath))
			return;
		
		m_pGraphics->QuadsBegin();		
		m_pGraphics->QuadsSetRotation(angle(Dir));
		pRenderTools->SelectModAPISprite(pSprite);
		
		vec2 NewPos = EndPoint + (Dir * pLineStyle->m_EndPointSpriteX) + (DirOrtho * pLineStyle->m_EndPointSpriteY);
		
		IGraphics::CQuadItem QuadItem(
			NewPos.x,
			NewPos.y,
			pLineStyle->m_EndPointSpriteSizeX,
			pLineStyle->m_EndPointSpriteSizeY);
		
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->QuadsEnd();
	}
	return;
	*/
}

void CModAPI_Client_Graphics::DrawText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment)
{	
	float width = pTextRender->TextWidth(0, Size, pText, -1);
	float height = pTextRender->TextHeight(Size);
	
	switch(Alignment)
	{
		case MODAPI_TEXTALIGN_CENTER:
			Pos.x -= width/2;
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_RIGHT_BOTTOM:
			break;
		case MODAPI_TEXTALIGN_RIGHT_CENTER:
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_RIGHT_TOP:
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_CENTER_TOP:
			Pos.x -= width/2;
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_LEFT_TOP:
			Pos.x -= width;
			Pos.y -= height;
			break;
		case MODAPI_TEXTALIGN_LEFT_CENTER:
			Pos.x -= width;
			Pos.y -= height/2;
			break;
		case MODAPI_TEXTALIGN_LEFT_BOTTOM:
			Pos.x -= width;
			break;
		case MODAPI_TEXTALIGN_CENTER_BOTTOM:
			Pos.y -= height/2;
			break;
	}
	
	pTextRender->TextColor(Color.r,Color.g,Color.b,Color.a);
	pTextRender->Text(0, Pos.x, Pos.y, Size, pText, -1);
	
	//reset color
	pTextRender->TextColor(255,255,255,1);
}

void CModAPI_Client_Graphics::DrawAnimatedText(
	ITextRender* pTextRender,
	const char *pText,
	vec2 Pos,
	vec4 Color,
	float Size,
	int Alignment,
	CModAPI_AssetPath AnimationPath,
	float Time,
	vec2 Offset
)
{
	const CModAPI_Asset_Animation* pAnimation = m_AnimationsCatalog.GetAsset(AnimationPath);
	if(pAnimation == 0) return;
	
	CModAPI_Asset_Animation::CFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	vec2 TextPos = Pos + Frame.m_Pos + rotate(Offset, Frame.m_Angle);
	
	vec4 NewColor = Color;
	NewColor.a *= Frame.m_Opacity;
	
	DrawText(pTextRender, pText, TextPos, NewColor, Size, Alignment);
}

void CModAPI_Client_Graphics::GetTeeAlignAxis(int Align, vec2 Dir, vec2 Aim, vec2& PartDirX, vec2& PartDirY)
{
	PartDirX = vec2(1.0f, 0.0f);
	PartDirY = vec2(0.0f, 1.0f);
	switch(Align)
	{
		case MODAPI_TEEALIGN_AIM:
			PartDirX = normalize(Aim);
			if(Aim.x >= 0.0f)
			{
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
		case MODAPI_TEEALIGN_HORIZONTAL:
			if(Aim.x < 0.0f)
			{
				PartDirX = vec2(-1.0f, 0.0f);
			}
			PartDirY = vec2(0.0f, 1.0f);
			break;
		case MODAPI_TEEALIGN_DIRECTION:
			PartDirX = normalize(Dir);
			if(Dir.x >= 0.0f)
			{
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
	}
}

void CModAPI_Client_Graphics::ApplyTeeAlign(CModAPI_Asset_Animation::CFrame& Frame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset)
{
	Flags = 0x0;
	vec2 PartDirX = vec2(1.0f, 0.0f);
	vec2 PartDirY = vec2(0.0f, 1.0f);
	switch(Align)
	{
		case MODAPI_TEEALIGN_AIM:
			PartDirX = normalize(Aim);
			if(Aim.x >= 0.0f)
			{
				Frame.m_Angle = angle(Aim) + Frame.m_Angle;
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				Frame.m_Angle = angle(Aim) - Frame.m_Angle;
				Flags = MODAPI_SPRITEFLAG_FLIP_Y;
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
			}
			break;
		case MODAPI_TEEALIGN_HORIZONTAL:
			if(Aim.x < 0.0f)
			{
				Frame.m_Angle = -Frame.m_Angle;
				Flags = MODAPI_SPRITEFLAG_FLIP_X;
				PartDirX = vec2(-1.0f, 0.0f);
			}
			PartDirY = vec2(0.0f, 1.0f);
			break;
		case MODAPI_TEEALIGN_DIRECTION:
			PartDirX = normalize(Dir);
			if(Dir.x >= 0.0f)
			{
				Frame.m_Angle = angle(Dir) + Frame.m_Angle;
				PartDirY = vec2(-PartDirX.y, PartDirX.x);
			}
			else
			{
				Frame.m_Angle = angle(Dir) - Frame.m_Angle;
				PartDirY = vec2(PartDirX.y, -PartDirX.x);
				Flags = MODAPI_SPRITEFLAG_FLIP_Y;
			}
			break;
	}
	
	Frame.m_Pos = Offset + PartDirX * Frame.m_Pos.x + PartDirY * Frame.m_Pos.y;	
}

void CModAPI_Client_Graphics::DrawAttach(CRenderTools* pRenderTools, const CModAPI_AttachAnimationState* pState, CModAPI_AssetPath AttachPath, vec2 Pos, float Scaling)
{
	const CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.GetAsset(AttachPath);
	if(pAttach == 0) return;
	
	for(int i=0; i<pAttach->m_BackElements.size(); i++)
	{
		CModAPI_AssetPath SpritePath = m_SpritesCatalog.GetFinalAssetPath(pAttach->m_BackElements[i].m_SpritePath, pState->m_Frames[i].m_ListId);
		
		DrawSprite(SpritePath,
			Pos + vec2(pState->m_Frames[i].m_Pos.x, pState->m_Frames[i].m_Pos.y) * Scaling,
			pAttach->m_BackElements[i].m_Size * Scaling,
			pState->m_Frames[i].m_Angle,
			pState->m_Flags[i],
			pState->m_Frames[i].m_Opacity
		);
	}
}

void CModAPI_Client_Graphics::DrawTee(CRenderTools* pRenderTools, CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pTeeState, vec2 Pos, vec2 Dir, int Emote)
{
	float Scaling = pInfo->m_Size/64.0f;
	
	float BodySize = pInfo->m_Size;
	float HandSize = BodySize*30.0f/64.0f;
	float FootSize = BodySize/2.1f;
	
	IGraphics::CQuadItem QuadItemBody(Pos.x + pTeeState->m_Body.m_Pos.x*Scaling, Pos.y + pTeeState->m_Body.m_Pos.y*Scaling, BodySize, BodySize);
	IGraphics::CQuadItem QuadItemBackFoot(Pos.x + pTeeState->m_BackFoot.m_Pos.x*Scaling, Pos.y + pTeeState->m_BackFoot.m_Pos.y*Scaling, FootSize, FootSize);
	IGraphics::CQuadItem QuadItemFrontFoot(Pos.x + pTeeState->m_FrontFoot.m_Pos.x*Scaling, Pos.y + pTeeState->m_FrontFoot.m_Pos.y*Scaling, FootSize, FootSize);
	IGraphics::CQuadItem QuadItemBackHand(Pos.x + pTeeState->m_BackHand.m_Pos.x*Scaling, Pos.y + pTeeState->m_BackHand.m_Pos.y*Scaling, HandSize, HandSize);
	IGraphics::CQuadItem QuadItemFrontHand(Pos.x + pTeeState->m_FrontHand.m_Pos.x*Scaling, Pos.y + pTeeState->m_FrontHand.m_Pos.y*Scaling, HandSize, HandSize);
	IGraphics::CQuadItem QuadItem;
	
	bool IndicateAirJump = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
		
	if(pTeeState->m_BackHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		vec4 Color = pInfo->m_aColors[3];
		float Opacity = pTeeState->m_BackHand.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);

		//draw hand, outline
		QuadItem = QuadItemBackHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemBackHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}
	
	if(pTeeState->m_FrontHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		
		vec4 Color = pInfo->m_aColors[3];
		float Opacity = pTeeState->m_FrontHand.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);

		//draw hand, outline
		QuadItem = QuadItemFrontHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemFrontHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4];
		float Opacity = pTeeState->m_BackFoot.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBackFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration, outline
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[2];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body, outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4];
		float Opacity = pTeeState->m_FrontFoot.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		
		QuadItem = QuadItemFrontFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4];
		float Opacity = pTeeState->m_BackFoot.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		
		QuadItem = QuadItemBackFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[2];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw body
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw marking
	if(pInfo->m_aTextures[1].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[1]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[1];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_MARKING, 0, 0, 0);
		
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body shadow and upper outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		vec4 Color = pInfo->m_aColors[0];
		float Opacity = pTeeState->m_Body.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_SHADOW, 0, 0, 0);
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_UPPER_OUTLINE, 0, 0, 0);
		QuadItem = QuadItemBody;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsEnd();
	}
	
	// draw eyes
	if(pInfo->m_aTextures[5].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[5]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(0.0f);
		vec4 Color = pInfo->m_aColors[5];
		float Opacity = Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		switch (Emote)
		{
			case EMOTE_PAIN:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_PAIN, 0, 0, 0);
				break;
			case EMOTE_HAPPY:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_HAPPY, 0, 0, 0);
				break;
			case EMOTE_SURPRISE:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_SURPRISE, 0, 0, 0);
				break;
			case EMOTE_ANGRY:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_ANGRY, 0, 0, 0);
				break;
			default:
				pRenderTools->SelectSprite(SPRITE_TEE_EYES_NORMAL, 0, 0, 0);
				break;
		}

		float EyeScale = BodySize*0.60f;
		float h = (Emote == EMOTE_BLINK) ? BodySize*0.15f/2.0f : EyeScale/2.0f;
		vec2 Offset = vec2(Dir.x*0.125f, -0.05f+Dir.y*0.10f)*BodySize;
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_Body.m_Pos.x*Scaling + Offset.x, Pos.y + pTeeState->m_Body.m_Pos.y*Scaling + Offset.y, EyeScale, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		vec4 Color = pInfo->m_aColors[4];
		float Opacity = pTeeState->m_FrontFoot.m_Opacity * Color.a;
		m_pGraphics->SetColor(Color.r*Opacity, Color.g*Opacity, Color.b*Opacity, Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		
		QuadItem = QuadItemFrontFoot;
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
}

void CModAPI_Client_Graphics::InitTeeAnimationState(CModAPI_TeeAnimationState* pState, vec2 MotionDir, vec2 AimDir)
{
	pState->m_MotionDir = MotionDir;
	pState->m_AimDir = AimDir;
	
	pState->m_Body.m_Pos.x = 0.0f;
	pState->m_Body.m_Pos.y = -4.0f;
	pState->m_Body.m_Angle = 0.0f;
	pState->m_Body.m_Opacity = 1.0f;
	
	pState->m_BackFoot.m_Pos.x = 0.0f;
	pState->m_BackFoot.m_Pos.y = 10.0f;
	pState->m_BackFoot.m_Angle = 0.0f;
	pState->m_BackFoot.m_Opacity = 1.0f;
	
	pState->m_FrontFoot.m_Pos.x = 0.0f;
	pState->m_FrontFoot.m_Pos.y = 10.0f;
	pState->m_FrontFoot.m_Angle = 0.0f;
	pState->m_FrontFoot.m_Opacity = 1.0f;
	
	pState->m_BackHandEnabled = false;
	pState->m_BackHand.m_Pos.x = 0.0f;
	pState->m_BackHand.m_Pos.y = 0.0f;
	pState->m_BackHand.m_Angle = 0.0f;
	pState->m_BackHand.m_Opacity = 1.0f;
	
	pState->m_FrontHandEnabled = false;
	pState->m_FrontHand.m_Pos.x = 0.0f;
	pState->m_FrontHand.m_Pos.y = 0.0f;
	pState->m_FrontHand.m_Angle = 0.0f;
	pState->m_FrontHand.m_Opacity = 1.0f;
}

void CModAPI_Client_Graphics::AddTeeAnimationState(CModAPI_TeeAnimationState* pState, CModAPI_AssetPath TeeAnimationPath, float Time)
{
	const CModAPI_Asset_TeeAnimation* pTeeAnim = m_TeeAnimationsCatalog.GetAsset(TeeAnimationPath);
	if(pTeeAnim == 0) return;
	
	if(!pTeeAnim->m_BodyAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBodyAnim = m_AnimationsCatalog.GetAsset(pTeeAnim->m_BodyAnimationPath);
		if(pBodyAnim)
		{
			pBodyAnim->GetFrame(Time, &pState->m_Body);
		}
	}
	
	if(!pTeeAnim->m_BackFootAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBackFootAnim = m_AnimationsCatalog.GetAsset(pTeeAnim->m_BackFootAnimationPath);
		if(pBackFootAnim)
		{
			pBackFootAnim->GetFrame(Time, &pState->m_BackFoot);
		}
	}
	
	if(!pTeeAnim->m_FrontFootAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pFrontFootAnim = m_AnimationsCatalog.GetAsset(pTeeAnim->m_FrontFootAnimationPath);
		if(pFrontFootAnim)
		{
			pFrontFootAnim->GetFrame(Time, &pState->m_FrontFoot);
		}
	}
	
	if(!pTeeAnim->m_BackHandAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pBackHandAnim = m_AnimationsCatalog.GetAsset(pTeeAnim->m_BackHandAnimationPath);
		if(pBackHandAnim)
		{
			pState->m_BackHandEnabled = true;
			
			pBackHandAnim->GetFrame(Time, &pState->m_BackHand);
			
			ApplyTeeAlign(pState->m_BackHand, pState->m_BackHandFlags, pTeeAnim->m_BackHandAlignment, pState->m_MotionDir, pState->m_AimDir, vec2(pTeeAnim->m_BackHandOffsetX, pTeeAnim->m_BackHandOffsetY));
		}
	}
	
	if(!pTeeAnim->m_FrontHandAnimationPath.IsNull())
	{
		const CModAPI_Asset_Animation* pFrontHandAnim = m_AnimationsCatalog.GetAsset(pTeeAnim->m_FrontHandAnimationPath);
		if(pFrontHandAnim)
		{
			pState->m_FrontHandEnabled = true;
			pFrontHandAnim->GetFrame(Time, &pState->m_FrontHand);
		}
	}
}

void CModAPI_Client_Graphics::InitAttachAnimationState(CModAPI_AttachAnimationState* pState, vec2 MotionDir, vec2 AimDir, CModAPI_AssetPath AttachPath, float Time)
{
	
	const CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.GetAsset(AttachPath);
	if(pAttach)
	{
		pState->m_Frames.set_size(pAttach->m_BackElements.size());
		pState->m_Flags.set_size(pAttach->m_BackElements.size());
		
		for(int i=0; i<pAttach->m_BackElements.size(); i++)
		{
			pState->m_Frames[i].m_Pos.x = 0.0f;
			pState->m_Frames[i].m_Pos.y = 0.0f;
			pState->m_Frames[i].m_Angle = 0.0f;
			pState->m_Frames[i].m_Opacity = 1.0f;
			pState->m_Frames[i].m_ListId = 0;
			pState->m_Flags[i] = 0x0;
			
			const CModAPI_Asset_Animation* pAnimation = m_AnimationsCatalog.GetAsset(pAttach->m_BackElements[i].m_AnimationPath);
			if(pAnimation)
			{
				pAnimation->GetFrame(Time, &pState->m_Frames[i]);
			}
			
			ApplyTeeAlign(
				pState->m_Frames[i],
				pState->m_Flags[i],
				pAttach->m_BackElements[i].m_Alignment,
				MotionDir,
				AimDir,
				vec2(pAttach->m_BackElements[i].m_OffsetX, pAttach->m_BackElements[i].m_OffsetY)
			);
		}
	}
}

template<>
CModAPI_Asset_Image* CModAPI_Client_Graphics::GetAsset<CModAPI_Asset_Image>(CModAPI_AssetPath AssetPath)
{
	return m_ImagesCatalog.GetAsset(AssetPath);
}

