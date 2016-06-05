#include <modapi/client/assetsmanager.h>
#include <engine/storage.h>
#include <modapi/shared/assetsfile.h>
#include <generated/client_data.h>
#include <game/client/render.h>
#include <engine/shared/datafile.h>
#include <engine/shared/config.h>

#include <game/gamecore.h>

#include <engine/external/pnglite/pnglite.h>
	
CModAPI_AssetManager::CModAPI_AssetManager(IGraphics* pGraphics, IStorage* pStorage)
{
	m_pGraphics = pGraphics;
	m_pStorage = pStorage;
}

#define GET_ASSET_CATALOG(TypeName, CatalogName) template<>\
CModAPI_AssetCatalog<TypeName>* CModAPI_AssetManager::GetAssetCatalog<TypeName>()\
{\
	return &CatalogName;\
}

//Search Tag: TAG_NEW_ASSET
GET_ASSET_CATALOG(CModAPI_Asset_Image, m_ImagesCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_Sprite, m_SpritesCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_Skeleton, m_SkeletonsCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_SkeletonSkin, m_SkeletonSkinsCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_SkeletonAnimation, m_SkeletonAnimationsCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_Character, m_CharactersCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_CharacterPart, m_CharacterPartsCatalog)
GET_ASSET_CATALOG(CModAPI_Asset_List, m_ListsCatalog)

void CModAPI_AssetManager::Init(IStorage* pStorage)
{
	LoadInteralAssets();
	LoadSkinAssets(pStorage);
}

void CModAPI_AssetManager::LoadInteralAssets()
{
	//Images
	{
		CModAPI_AssetPath ImagePath = AddImage(IStorage::TYPE_ALL, "game.png", CModAPI_AssetPath::SRC_INTERNAL);
		CModAPI_Asset_Image* pImage = GetAsset<CModAPI_Asset_Image>(ImagePath);
		if(pImage)
		{			
			pImage->m_GridWidth = 32;
			pImage->m_GridHeight = 16;
		}
	}
	{
		CModAPI_AssetPath ImagePath = AddImage(IStorage::TYPE_ALL, "particles.png", CModAPI_AssetPath::SRC_INTERNAL);
		CModAPI_Asset_Image* pImage = GetAsset<CModAPI_Asset_Image>(ImagePath);
		if(pImage)
		{			
			pImage->m_GridWidth = 8;
			pImage->m_GridHeight = 8;
		}
	}
	{
		CModAPI_AssetPath ImagePath = AddImage(IStorage::TYPE_ALL, "modapi/dummy.png", CModAPI_AssetPath::SRC_INTERNAL);
		CModAPI_Asset_Image* pImage = GetAsset<CModAPI_Asset_Image>(ImagePath);
		if(pImage)
		{			
			pImage->m_GridWidth = 16;
			pImage->m_GridHeight = 8;
		}
	}
	dbg_msg("assetsmanager", "Images initialised");
	
	#define CREATE_INTERNAL_SPRITE(id, name, image, x, y, w, h) {\
		CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, id));\
		pSprite->SetName(name);\
		pSprite->Init(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_IMAGE, image), x, y, w, h);\
	}
	
	{
		CModAPI_Asset_Sprite* pSprite = m_SpritesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_WHITESQUARE));
		pSprite->SetName("whiteSquare");
		pSprite->Init(CModAPI_AssetPath::Null(), 0, 0, 1, 1);
	}
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
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_HOOK_HEAD, "hookEndPoint", MODAPI_IMAGE_GAME, 3, 0, 2, 1);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_HOOK_CHAIN, "hookChain", MODAPI_IMAGE_GAME, 2, 0 , 1, 1);
	
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_PART_SPLAT1, "partSplat1", MODAPI_IMAGE_PARTICLES, 2, 0 , 1, 1);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_PART_SPLAT2, "partSplat2", MODAPI_IMAGE_PARTICLES, 3, 0 , 1, 1);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_PART_SPLAT3, "partSplat3", MODAPI_IMAGE_PARTICLES, 4, 0 , 1, 1);
	
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_BODYSHADOW, "dummyBodyShadow", MODAPI_IMAGE_DUMMY, 0, 0, 4, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_BODYCOLOR, "dummyBodyColor", MODAPI_IMAGE_DUMMY, 4, 0, 4, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_BODYSHADING, "dummyBodyShading", MODAPI_IMAGE_DUMMY, 0, 4, 4, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_BODYOUTLINE, "dummyBodyOutline", MODAPI_IMAGE_DUMMY, 4, 4, 4, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_MARKING, "dummyMarking", MODAPI_IMAGE_DUMMY, 8, 4, 4, 4);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_FOOTSHADOW, "dummyFootShadow", MODAPI_IMAGE_DUMMY, 10, 2, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_FOOTCOLOR, "dummyFootColor", MODAPI_IMAGE_DUMMY, 8, 2, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_HANDSHADOW, "dummyHandShadow", MODAPI_IMAGE_DUMMY, 10, 0, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_HANDCOLOR, "dummyHandColor", MODAPI_IMAGE_DUMMY, 8, 0, 2, 2);
	CREATE_INTERNAL_SPRITE(MODAPI_SPRITE_DUMMY_EYESNORMAL, "dummyEyesNormal", MODAPI_IMAGE_DUMMY, 12, 0, 2, 1);
	dbg_msg("assetsmanager", "Sprites initialised");

	//Sprites list
	{
		CModAPI_Asset_List* pList = m_ListsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LIST, MODAPI_LIST_GUN_MUZZLES));
		pList->SetName("gunMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN_MUZZLE3));
	}
	{
		CModAPI_Asset_List* pList = m_ListsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LIST, MODAPI_LIST_SHOTGUN_MUZZLES));
		pList->SetName("shotgunMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN_MUZZLE3));
	}
	{
		CModAPI_Asset_List* pList = m_ListsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LIST, MODAPI_LIST_NINJA_MUZZLES));
		pList->SetName("ninjaMuzzles");
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_NINJA_MUZZLE1));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_NINJA_MUZZLE2));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_NINJA_MUZZLE3));
	}
	{
		CModAPI_Asset_List* pList = m_ListsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LIST, MODAPI_LIST_PART_SPLATS));
		pList->SetName("partSplats");
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_PART_SPLAT1));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_PART_SPLAT2));
		pList->Add(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_PART_SPLAT3));
	}
	dbg_msg("assetsmanager", "Sprite Lists initialised");

	//Animations
	vec2 BodyPos = vec2(0.0f, -4.0f);
	vec2 FootPos = vec2(0.0f, 10.0f);
	
		//Idle
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_IDLE_BACKFOOT));
		pAnim->SetName("teeIdle_backFoot");
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(-7.0f,  0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_IDLE_FRONTFOOT));
		pAnim->SetName("teeIdle_frontFoot");
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(7.0f,  0.0f));
	}
	
		//InAir
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INAIR_BACKFOOT));
		pAnim->SetName("teeAir_backFoot");
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(-3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INAIR_FRONTFOOT));
		pAnim->SetName("teeAir_frontFoot");
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
	}
	
		//Walk
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_BODY));
		pAnim->SetName("teeWalk_body");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f).Position(BodyPos + vec2(0.0f,  0.0f));
		pAnim->AddKeyFrame(0.2f).Position(BodyPos + vec2(0.0f, -1.0f));
		pAnim->AddKeyFrame(0.4f).Position(BodyPos + vec2(0.0f,  0.0f));
		pAnim->AddKeyFrame(0.6f).Position(BodyPos + vec2(0.0f,  0.0f));
		pAnim->AddKeyFrame(0.8f).Position(BodyPos + vec2(0.0f, -1.0f));
		pAnim->AddKeyFrame(1.0f).Position(BodyPos + vec2(0.0f,  0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_BACKFOOT));
		pAnim->SetName("teeWalk_backFoot");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(  8.0f,  0.0f)).Angle(0.0f);
		pAnim->AddKeyFrame(0.2f).Position(FootPos + vec2( -8.0f,  0.0f)).Angle(0.0f);
		pAnim->AddKeyFrame(0.4f).Position(FootPos + vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
		pAnim->AddKeyFrame(0.6f).Position(FootPos + vec2( -8.0f, -8.0f)).Angle(0.3f*pi*2.0f);
		pAnim->AddKeyFrame(0.8f).Position(FootPos + vec2(  4.0f, -4.0f)).Angle(-0.2f*pi*2.0f);
		pAnim->AddKeyFrame(1.0f).Position(FootPos + vec2(  8.0f,  0.0f)).Angle(0.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_FRONTFOOT));
		pAnim->SetName("teeWalk_frontFoot");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.0f).Position(FootPos + vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
		pAnim->AddKeyFrame(0.2f).Position(FootPos + vec2( -8.0f, -8.0f)).Angle(0.3f*pi*2.0f);
		pAnim->AddKeyFrame(0.4f).Position(FootPos + vec2(  4.0f, -4.0f)).Angle(-0.2f*pi*2.0f);
		pAnim->AddKeyFrame(0.6f).Position(FootPos + vec2(  8.0f,  0.0f)).Angle(0.0f);
		pAnim->AddKeyFrame(0.8f).Position(FootPos + vec2(  8.0f,  0.0f)).Angle(0.0f);
		pAnim->AddKeyFrame(1.0f).Position(FootPos + vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
	}
	
		//Weapons
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HAMMERATTACK_WEAPON));
		pAnim->SetName("hammerAttack_weapon");
		pAnim->AddKeyFrame(0.0f/5.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2-0.10f*pi*2.0f);
		pAnim->AddKeyFrame(0.2f/5.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.25f*pi*2.0f);
		pAnim->AddKeyFrame(0.4f/5.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.30f*pi*2.0f);
		pAnim->AddKeyFrame(0.6f/5.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.25f*pi*2.0f);
		pAnim->AddKeyFrame(0.8f/5.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2-0.10f*pi*2.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_WEAPON));
		pAnim->SetName("gunAttack_weapon");
		pAnim->AddKeyFrame(0.00f).Position(vec2(32.0f, 0.0f));
		pAnim->AddKeyFrame(0.05f).Position(vec2(32.0f - 10.0f, 0.0f));
		pAnim->AddKeyFrame(0.10f).Position(vec2(32.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_MUZZLE));
		pAnim->SetName("gunAttack_muzzle");
		pAnim->AddKeyFrame(0.00f).Position(vec2(82.0f, -6.0f)).ListId(0);
		pAnim->AddKeyFrame(0.02f).Position(vec2(82.0f, -6.0f)).ListId(1);
		pAnim->AddKeyFrame(0.04f).Position(vec2(82.0f, -6.0f)).ListId(2);
		pAnim->AddKeyFrame(0.06f).Position(vec2(82.0f, -6.0f)).ListId(0);
		pAnim->AddKeyFrame(0.08f).Position(vec2(82.0f, -6.0f)).ListId(1);
		pAnim->AddKeyFrame(0.10f).Position(vec2(82.0f, -6.0f)).ListId(2);
		pAnim->AddKeyFrame(0.12f).Position(vec2(82.0f, -6.0f)).ListId(0).Opacity(0.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_BACKHAND));
		pAnim->SetName("gunAttack_backHand");
		pAnim->AddKeyFrame(0.00f).Position(vec2(0.0f - 15.0f + 32.0f, 4.0f)).Angle(-3*pi/4);
		pAnim->AddKeyFrame(0.05f).Position(vec2(-10.0f - 15.0f + 32.0f, 4.0f)).Angle(-3*pi/4);
		pAnim->AddKeyFrame(0.10f).Position(vec2(0.0f - 15.0f + 32.0f, 4.0f)).Angle(-3*pi/4);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON));
		pAnim->SetName("shotgunAttack_weapon");
		pAnim->AddKeyFrame(0.00f).Position(vec2(24.0f, 0.0f));
		pAnim->AddKeyFrame(0.05f).Position(vec2(24.0f - 10.0f, 0.0f));
		pAnim->AddKeyFrame(0.10f).Position(vec2(24.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE));
		pAnim->SetName("shotgunAttack_muzzle");
		pAnim->AddKeyFrame(0.00f).Position(vec2(94.0f, -6.0f)).ListId(0);
		pAnim->AddKeyFrame(0.02f).Position(vec2(94.0f, -6.0f)).ListId(1);
		pAnim->AddKeyFrame(0.04f).Position(vec2(94.0f, -6.0f)).ListId(2);
		pAnim->AddKeyFrame(0.06f).Position(vec2(94.0f, -6.0f)).ListId(0);
		pAnim->AddKeyFrame(0.08f).Position(vec2(94.0f, -6.0f)).ListId(1);
		pAnim->AddKeyFrame(0.10f).Position(vec2(94.0f, -6.0f)).ListId(2);
		pAnim->AddKeyFrame(0.12f).Position(vec2(94.0f, -6.0f)).ListId(0).Opacity(0.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND));
		pAnim->SetName("shotgunAttack_backHand");
		pAnim->AddKeyFrame(0.00f).Position(vec2(0.0f - 5.0f + 24.0f, 4.0f)).Angle(-pi/2);
		pAnim->AddKeyFrame(0.05f).Position(vec2(-10.0f - 5.0f + 24.0f, 4.0f)).Angle(-pi/2);
		pAnim->AddKeyFrame(0.10f).Position(vec2(0.0f - 5.0f + 24.0f, 4.0f)).Angle(-pi/2);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GRENADEATTACK_WEAPON));
		pAnim->SetName("grenadeAttack_weapon");
		pAnim->AddKeyFrame(0.00f).Position(vec2(24.0f, 0.0f));
		pAnim->AddKeyFrame(0.05f).Position(vec2(24.0f - 10.0f, 0.0f));
		pAnim->AddKeyFrame(0.10f).Position(vec2(24.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GRENADEATTACK_BACKHAND));
		pAnim->SetName("grenadeAttack_backhand");
		pAnim->AddKeyFrame(0.00f).Position(vec2(0.0f - 5.0f + 24.0f, 7.0f)).Angle(-pi/2);
		pAnim->AddKeyFrame(0.05f).Position(vec2(-10.0f - 5.0f + 24.0f, 7.0f)).Angle(-pi/2);
		pAnim->AddKeyFrame(0.10f).Position(vec2(0.0f - 5.0f + 24.0f, 7.0f)).Angle(-pi/2);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_LASERATTACK_WEAPON));
		pAnim->SetName("laserAttack_weapon");
		pAnim->AddKeyFrame(0.00f).Position(vec2(24.0f, 0.0f));
		pAnim->AddKeyFrame(0.05f).Position(vec2(24.0f - 10.0f, 0.0f));
		pAnim->AddKeyFrame(0.10f).Position(vec2(24.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_NINJAATTACK_WEAPON));
		pAnim->SetName("ninjaAttack_weapon");
		pAnim->AddKeyFrame(0.00f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2-0.25f*pi*2.0f);
		pAnim->AddKeyFrame(0.10f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2-0.05f*pi*2.0f);
		pAnim->AddKeyFrame(0.15f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.35f*pi*2.0f);
		pAnim->AddKeyFrame(0.42f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.40f*pi*2.0f);
		pAnim->AddKeyFrame(0.50f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2+0.35f*pi*2.0f);
		pAnim->AddKeyFrame(1.00f/2.0f).Position(vec2(0.0f, 0.0f)).Angle(-pi/2-0.25f*pi*2.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_NINJAATTACK_MUZZLE));
		pAnim->SetName("ninjaAttack_muzzle");
		pAnim->AddKeyFrame(0.00f).Position(vec2(-40.0f, -4.0f)).ListId(0);
		pAnim->AddKeyFrame(0.02f).Position(vec2(-40.0f, -4.0f)).ListId(1);
		pAnim->AddKeyFrame(0.04f).Position(vec2(-40.0f, -4.0f)).ListId(2);
		pAnim->AddKeyFrame(0.06f).Position(vec2(-40.0f, -4.0f)).ListId(0);
		pAnim->AddKeyFrame(0.08f).Position(vec2(-40.0f, -4.0f)).ListId(1);
		pAnim->AddKeyFrame(0.10f).Position(vec2(-40.0f, -4.0f)).ListId(2);
		pAnim->AddKeyFrame(0.12f).Position(vec2(-40.0f, -4.0f)).ListId(0).Opacity(0.0f);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HOOK_CHAIN));
		pAnim->SetName("hookChain");
		pAnim->AddKeyFrame(0.0f).Position(vec2(20.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HOOK_HEAD));
		pAnim->SetName("hookHead");
		pAnim->AddKeyFrame(0.0f).Size(vec2(0.75f, 1.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_OUTER_LASER));
		pAnim->SetName("outerLaser");
		pAnim->AddKeyFrame(0.0f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f));
		pAnim->AddKeyFrame(0.150f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f)).Size(vec2(1.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INNER_LASER));
		pAnim->SetName("innerLaser");
		pAnim->AddKeyFrame(0.0f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f));
		pAnim->AddKeyFrame(0.150f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f)).Size(vec2(1.0f, 0.0f));
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_OUTER_LASER_HEAD));
		pAnim->SetName("outerLaserHead");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.00f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f)).ListId(0);
		pAnim->AddKeyFrame(0.02f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f)).ListId(1);
		pAnim->AddKeyFrame(0.04f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f)).ListId(2);
		pAnim->AddKeyFrame(0.06f).Color(vec4(0.075f, 0.075f, 0.25f, 1.0f)).ListId(0);
	}
	{
		CModAPI_Asset_Animation* pAnim = m_AnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INNER_LASER_HEAD));
		pAnim->SetName("innerLaserHead");
		pAnim->m_CycleType = MODAPI_ANIMCYCLETYPE_LOOP;
		pAnim->AddKeyFrame(0.00f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f)).ListId(0);
		pAnim->AddKeyFrame(0.02f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f)).ListId(1);
		pAnim->AddKeyFrame(0.04f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f)).ListId(2);
		pAnim->AddKeyFrame(0.06f).Color(vec4(0.5f, 0.5f, 1.0f, 1.0f)).ListId(0);
	}
	dbg_msg("assetsmanager", "Animations initialised");
	
	//TeeAnimations
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_IDLE));
		pAnim->SetName("teeIdle");
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_IDLE_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_IDLE_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_INAIR));
		pAnim->SetName("teeAir");
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INAIR_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INAIR_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_WALK));
		pAnim->SetName("teeWalk");
		pAnim->m_BodyAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_BODY);
		pAnim->m_BackFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_BACKFOOT);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
		pAnim->m_FrontFootAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_WALK_FRONTFOOT);
		pAnim->m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_GUNATTACK));
		pAnim->SetName("teeGunAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = 4;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_SHOTGUNATTACK));
		pAnim->SetName("teeShotgunAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = -2;
	}
	{
		CModAPI_Asset_TeeAnimation* pAnim = m_TeeAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_GRENADEATTACK));
		pAnim->SetName("teeGrenadeAttack");
		pAnim->m_BackHandAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GRENADEATTACK_BACKHAND);
		pAnim->m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
		pAnim->m_BackHandOffsetY = -2;
	}
	dbg_msg("assetsmanager", "Tee Animations initialised");
	
	//Weapons
		//Hammer
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_HAMMER));
		pAttach->SetName("hammer");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_HAMMER_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_HAMMER),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HAMMERATTACK_WEAPON),
			MODAPI_TEEALIGN_HORIZONTAL,
			4,
			-20
		);
	}
		//Gun
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_GUN));
		pAttach->SetName("gun");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_GUNATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_LIST_GUN_MUZZLES),
			64,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_MUZZLE),
			MODAPI_TEEALIGN_AIM,
			0,
			4
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN),
			64,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GUNATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			4
		);
	}
		//Shotgun
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_SHOTGUN));
		pAttach->SetName("shotgun");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_SHOTGUNATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_LIST_SHOTGUN_MUZZLES),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Grenade
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_GRENADE));
		pAttach->SetName("grenade");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GRENADE_CURSOR);
		pAttach->m_TeeAnimationPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_TEEANIMATION, MODAPI_TEEANIMATION_GRENADEATTACK);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GRENADE),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_GRENADEATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Laser
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_LASER));
		pAttach->SetName("laser");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_LASER_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_LASER),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_LASERATTACK_WEAPON),
			MODAPI_TEEALIGN_AIM,
			0,
			-2
		);
	}
		//Ninja
	{
		CModAPI_Asset_Attach* pAttach = m_AttachesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ATTACH, MODAPI_ATTACH_NINJA));
		pAttach->SetName("ninja");
		pAttach->m_CursorPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_NINJA_CURSOR);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_LIST_NINJA_MUZZLES),
			160,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_NINJAATTACK_MUZZLE),
			MODAPI_TEEALIGN_DIRECTION,
			0,
			0
		);
		pAttach->AddBackElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_NINJA),
			96,
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_NINJAATTACK_WEAPON),
			MODAPI_TEEALIGN_HORIZONTAL,
			0,
			0
		);
	}
	dbg_msg("assetsmanager", "Attaches initialised");
	
	//Line styles
		//Hook
	{
		CModAPI_Asset_LineStyle* pLineStyle = m_LineStylesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LINESTYLE, MODAPI_LINESTYLE_HOOK));
		pLineStyle->SetName("hook");
		pLineStyle->AddElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_HOOK_CHAIN), 16, CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HOOK_CHAIN),
			1.0f, 0.0f,	CModAPI_Asset_LineStyle::TILINGTYPE_REPEATED, 0);
		pLineStyle->AddElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_HOOK_HEAD), 16, CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_HOOK_HEAD),
			1.0f, 1.0f,	CModAPI_Asset_LineStyle::TILINGTYPE_NONE, 0);
	}
	
		//Laser
	{
		CModAPI_Asset_LineStyle* pLineStyle = m_LineStylesCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_LINESTYLE, MODAPI_LINESTYLE_LASER));
		pLineStyle->SetName("laser");
		pLineStyle->AddElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_LIST_PART_SPLATS), 24, CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_OUTER_LASER_HEAD),
			1.0f, 1.0f, CModAPI_Asset_LineStyle::TILINGTYPE_NONE, 0);
		pLineStyle->AddElement(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_LIST_PART_SPLATS), 20, CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_ANIMATION, MODAPI_ANIMATION_INNER_LASER_HEAD),
			1.0f, 1.0f, CModAPI_Asset_LineStyle::TILINGTYPE_NONE, 0);
	}
	dbg_msg("assetsmanager", "Line Styles initialised");
	
	//Skeletons
	{
		CModAPI_Asset_Skeleton* pSkeleton = m_SkeletonsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE));
		pSkeleton->SetName("tee");
		pSkeleton->m_DefaultSkinPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_DUMMY);
		pSkeleton->AddBone().Name("body").Color(vec4(1.0f, 0.0f, 1.0f, 1.0f))
			.Length(16.0f).Translation(vec2(0.0f, -4.0f));
		pSkeleton->AddBone().Name("eyes").Color(vec4(0.0f, 1.0f, 1.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY))
			.Length(8.0f).Translation(vec2(-16.0f, -3.2f)).Angle(-pi/4.0f);
		pSkeleton->AddBone().Name("backFoot").Color(vec4(0.5f, 0.0f, 0.0f, 1.0f))
			.Length(16.0f).Translation(vec2(0.0f, 10.0f));
		pSkeleton->AddBone().Name("frontFoot").Color(vec4(1.0f, 0.0f, 0.0f, 1.0f))
			.Length(16.0f).Translation(vec2(0.0f, 10.0f));
		pSkeleton->AddBone().Name("backArm").Color(vec4(0.0f, 0.5f, 0.0f, 1.0f))
			.Length(21.0f);
		pSkeleton->AddBone().Name("backHand").Color(vec4(0.0f, 0.5f, 0.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKARM))
			.Length(8.0f);
		pSkeleton->AddBone().Name("frontArm").Color(vec4(0.0f, 1.0f, 0.0f, 1.0f))
			.Length(21.0f);
		pSkeleton->AddBone().Name("frontHand").Color(vec4(0.0f, 1.0f, 0.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTARM))
			.Length(8.0f);
	
		pSkeleton->AddLayer().Name("backHandShadow");
		pSkeleton->AddLayer().Name("frontHandShadow");
		pSkeleton->AddLayer().Name("backFootShadow");
		pSkeleton->AddLayer().Name("bodyShadow");
		pSkeleton->AddLayer().Name("frontFootShadow");
		pSkeleton->AddLayer().Name("attach");
		pSkeleton->AddLayer().Name("backHand");
		pSkeleton->AddLayer().Name("frontHand");
		pSkeleton->AddLayer().Name("backFoot");
		pSkeleton->AddLayer().Name("body");
		pSkeleton->AddLayer().Name("frontFoot");
	}
	{
		CModAPI_Asset_Skeleton* pSkeleton = m_SkeletonsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_HAMMER));
		pSkeleton->SetName("hammer");
		pSkeleton->m_DefaultSkinPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_HAMMER);
		pSkeleton->m_ParentPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		pSkeleton->AddBone().Name("attach").Color(vec4(0.0f, 1.0f, 0.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_BODY))
			.Translation(vec2(-12.0f, -16.0f))
			.Length(48.0f);
	}
	{
		CModAPI_Asset_Skeleton* pSkeleton = m_SkeletonsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_GUN));
		pSkeleton->SetName("gun");
		pSkeleton->m_DefaultSkinPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_GUN);
		pSkeleton->m_ParentPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		pSkeleton->AddBone().Name("attach").Color(vec4(0.0f, 1.0f, 0.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM))
			.Translation(vec2(0.0f, 0.0f))
			.Length(32.0f);
	}
	{
		CModAPI_Asset_Skeleton* pSkeleton = m_SkeletonsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_SHOTGUN));
		pSkeleton->SetName("shotgun");
		pSkeleton->m_DefaultSkinPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_SHOTGUN);
		pSkeleton->m_ParentPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		pSkeleton->AddBone().Name("attach").Color(vec4(0.0f, 1.0f, 0.0f, 1.0f)).Parent(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM))
			.Translation(vec2(0.0f, 0.0f))
			.Length(32.0f);
	}
	dbg_msg("assetsmanager", "Skeletons initialised");
	
	//Skeleton Skins
	{
		CModAPI_Asset_SkeletonSkin* pSkeletonSkin = m_SkeletonSkinsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_DUMMY));
		pSkeletonSkin->SetName("dummy");
		pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_BODYSHADOW),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODYSHADOW))
			.Translation(vec2(-16.0f, 0.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_HANDSHADOW),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKHAND),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW))
			.Translation(vec2(-8.0f, 0.0f)).Scale(vec2(30.0f, 30.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_HANDSHADOW),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTHAND),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW))
			.Translation(vec2(-8.0f, 0.0f)).Scale(vec2(30.0f, 30.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_FOOTSHADOW),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKFOOTSHADOW))
			.Translation(vec2(-16.0f, 0.0f)).Scale(vec2(30.476f, 30.476f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_FOOTSHADOW),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTFOOTSHADOW))
			.Translation(vec2(-16.0f, 0.0f)).Scale(vec2(30.476f, 30.476f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_BODYCOLOR),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
			.Translation(vec2(-16.0f, 0.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_MARKING),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
			.Translation(vec2(-16.0f, 0.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_BODYSHADING),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
			.Translation(vec2(-16.0f, 0.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_BODYOUTLINE),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
			.Translation(vec2(-16.0f, 0.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_FOOTCOLOR),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKFOOT))
			.Translation(vec2(-16.0f, 0.0f)).Scale(vec2(30.476f, 30.476f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_FOOTCOLOR),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTFOOT))
			.Translation(vec2(-16.0f, 0.0f)).Scale(vec2(30.476f, 30.476f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_HANDCOLOR),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKHAND),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND))
			.Translation(vec2(-8.0f, 0.0f)).Scale(vec2(30.0f, 30.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_HANDCOLOR),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTHAND),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND))
			.Translation(vec2(-8.0f, 0.0f)).Scale(vec2(30.0f, 30.0f));
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_DUMMY_EYESNORMAL),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_EYES),
			CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
			.Scale(vec2(38.4, 38.4)).Alignment(CModAPI_Asset_SkeletonSkin::ALIGNEMENT_WORLD);
	}
	{
		CModAPI_Asset_SkeletonSkin* pSkeletonSkin = m_SkeletonSkinsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_HAMMER));
		pSkeletonSkin->SetName("hammer");
		pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_HAMMER);
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_HAMMER),
			CModAPI_Asset_Skeleton::CBonePath::Local(0),
			CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_ATTACH))
			.Translation(vec2(-44.0f, 0.0f))
			.Scale(vec2(76.8f, 76.8f));
	}
	{
		CModAPI_Asset_SkeletonSkin* pSkeletonSkin = m_SkeletonSkinsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_GUN));
		pSkeletonSkin->SetName("gun");
		pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_GUN);
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_GUN),
			CModAPI_Asset_Skeleton::CBonePath::Local(0),
			CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_ATTACH))
			.Translation(vec2(-21.0f, 0.0f))
			.Scale(vec2(57.24f, 57.24f));
	}
	{
		CModAPI_Asset_SkeletonSkin* pSkeletonSkin = m_SkeletonSkinsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONSKIN, MODAPI_SKELETONSKIN_SHOTGUN));
		pSkeletonSkin->SetName("shotgun");
		pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_SHOTGUN);
		pSkeletonSkin->AddSprite(
			CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SPRITE, MODAPI_SPRITE_SHOTGUN),
			CModAPI_Asset_Skeleton::CBonePath::Local(0),
			CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_ATTACH))
			.Translation(vec2(-21.0f, 0.0f))
			.Scale(vec2(93.13f, 93.13f));
	}
	dbg_msg("assetsmanager", "Skeleton Skins initialised");
	
	//Skeleton Animations
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_TEEIDLE));
		pSkeletonAnimation->SetName("teeIdle");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),  0).Translation(vec2(-7.0f,  0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 0).Translation(vec2( 7.0f,  0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKARM), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_HOOK);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
	}
	
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_TEEAIR));
		pSkeletonAnimation->SetName("teeAir");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),  0).Translation(vec2(-3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 0).Translation(vec2( 3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKARM), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_HOOK);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
	}
	
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_TEEAIR2));
		pSkeletonAnimation->SetName("teeAir2");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),  0).Translation(vec2(-3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 0).Translation(vec2( 3.0f,  0.0f)).Angle(-0.1f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKARM), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_HOOK);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKFOOT), 0).Color(vec4(0.5f, 0.5f, 0.5f, 1.0f));
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTFOOT), 0).Color(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	}
	
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_TEEWALK));
		pSkeletonAnimation->SetName("teeWalk");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKARM), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_HOOK);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 0 ).Translation(vec2(0.0f,  0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 6 ).Translation(vec2(0.0f, -1.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 12).Translation(vec2(0.0f,  0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 18).Translation(vec2(0.0f,  0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 24).Translation(vec2(0.0f, -1.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), 30).Translation(vec2(0.0f,  0.0f));
		pSkeletonAnimation->SetCycle(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY), CModAPI_Asset_SkeletonAnimation::CYCLETYPE_LOOP);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 0 ).Translation(vec2(  8.0f,  0.0f)).Angle(0.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 6 ).Translation(vec2( -8.0f,  0.0f)).Angle(0.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 12).Translation(vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 18).Translation(vec2( -8.0f, -8.0f)).Angle(0.3f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 24).Translation(vec2(  4.0f, -4.0f)).Angle(-0.2f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), 30).Translation(vec2(  8.0f,  0.0f)).Angle(0.0f);
		pSkeletonAnimation->SetCycle(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT), CModAPI_Asset_SkeletonAnimation::CYCLETYPE_LOOP);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 0 ).Translation(vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 6 ).Translation(vec2( -8.0f, -8.0f)).Angle(0.3f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 12).Translation(vec2(  4.0f, -4.0f)).Angle(-0.2f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 18).Translation(vec2(  8.0f,  0.0f)).Angle(0.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 24).Translation(vec2(  8.0f,  0.0f)).Angle(0.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), 30).Translation(vec2(-10.0f, -4.0f)).Angle(0.2f*pi*2.0f);
		pSkeletonAnimation->SetCycle(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT), CModAPI_Asset_SkeletonAnimation::CYCLETYPE_LOOP);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
	}
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_HAMMERATTACK));
		pSkeletonAnimation->SetName("hammerAttack");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_HAMMER);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 0).Angle(-pi/2-0.10f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 2).Angle(-pi/2+0.25f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 4).Angle(-pi/2+0.30f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 6).Angle(-pi/2+0.25f*pi*2.0f);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 9).Angle(-pi/2-0.10f*pi*2.0f);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_EYES), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_HIDDEN);
	}
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_GUNATTACK));
		pSkeletonAnimation->SetName("gunAttack");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_GUN);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 0)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 2)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 4)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
			
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 0)
			.Angle(-3*pi/4)
			.Translation(vec2(0.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 2)
			.Angle(-3*pi/4)
			.Translation(vec2(-10.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 4)
			.Angle(-3*pi/4)
			.Translation(vec2(0.0f, 0.0f));
			
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 0)
			.Translation(vec2(0.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 2)
			.Translation(vec2(-10.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 4)
			.Translation(vec2(0.0f, 0.0f));
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_EYES), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_VISIBLE);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_VISIBLE);
	}
	{
		CModAPI_Asset_SkeletonAnimation* pSkeletonAnimation = m_SkeletonAnimationsCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETONANIMATION, MODAPI_SKELETONANIMATION_SHOTGUNATTACK));
		pSkeletonAnimation->SetName("shotgunAttack");
		pSkeletonAnimation->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_SHOTGUN);
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 0)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 2)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTARM), 4)
			.Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
			
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 0)
			.Angle(-pi/2)
			.Translation(vec2(0.0f, 6.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 2)
			.Angle(-pi/2)
			.Translation(vec2(-10.0f, 6.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_FRONTHAND), 4)
			.Angle(-pi/2)
			.Translation(vec2(0.0f, 6.0f));
			
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 0)
			.Translation(vec2(0.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 2)
			.Translation(vec2(-10.0f, 0.0f));
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Local(0), 4)
			.Translation(vec2(0.0f, 0.0f));
		
		pSkeletonAnimation->AddBoneKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEEBONE_EYES), 0).Alignment(CModAPI_Asset_SkeletonAnimation::BONEALIGN_AIM);
		
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHANDSHADOW), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_VISIBLE);
		pSkeletonAnimation->AddLayerKeyFrame(CModAPI_Asset_Skeleton::CBonePath::Parent(MODAPI_TEELAYER_FRONTHAND), 0).State(CModAPI_Asset_SkeletonAnimation::LAYERSTATE_VISIBLE);
	}
	dbg_msg("assetsmanager", "Skeleton Animations initialised");
	
	//Character
	{
		CModAPI_Asset_Character* pCharacter = m_CharactersCatalog.NewAsset(CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE));
		pCharacter->SetName("tee");
		
		pCharacter->AddPart().Name("body");
		pCharacter->AddPart().Name("marking");
		pCharacter->AddPart().Name("decoration");
		pCharacter->AddPart().Name("hands");
		pCharacter->AddPart().Name("feet");
		pCharacter->AddPart().Name("eyes");
	}
	dbg_msg("assetsmanager", "Characters initialised");
}

int CModAPI_AssetManager::LoadSkinAssets_BodyScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/body/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "body/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 2;
		pImage->m_GridHeight = 2;
	
		CModAPI_AssetPath ShadowPath;
		CModAPI_AssetPath ColorPath;
		CModAPI_AssetPath ShadingPath;
		CModAPI_AssetPath OutinePath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ShadowPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "body/%s/shadow", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 0, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ColorPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "body/%s/color", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 0, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ShadingPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "body/%s/shading", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 1, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&OutinePath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "body/%s/outline", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 1, 1, 1);
		}
		{
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "body/%s", aName);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODYSHADOW))
				.Anchor(0.0f);
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
				.Anchor(0.0f);
			pSkeletonSkin->AddSprite(
				ShadingPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
				.Anchor(0.0f);
			pSkeletonSkin->AddSprite(
				OutinePath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
				.Anchor(0.0f);
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			pCharacterPart->SetName(aName);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_BODY);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

int CModAPI_AssetManager::LoadSkinAssets_FootScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/feet/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "feet/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 2;
		pImage->m_GridHeight = 1;
	
		CModAPI_AssetPath ShadowPath;
		CModAPI_AssetPath ColorPath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ShadowPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "feet/%s/shadow", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 0, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ColorPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "feet/%s/color", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 0, 1, 1);
		}
		{
			str_format(aBuf, sizeof(aBuf), "feet/%s", aName);
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKFOOTSHADOW))
				.Anchor(0.0f)
				.Scale(vec2(30.476f, 30.476f));
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTFOOTSHADOW))
				.Anchor(0.0f)
				.Scale(vec2(30.476f, 30.476f));
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKFOOT),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKFOOT))
				.Anchor(0.0f)
				.Scale(vec2(30.476f, 30.476f));
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTFOOT),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTFOOT))
				.Anchor(0.0f)
				.Scale(vec2(30.476f, 30.476f));
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			pCharacterPart->SetName(aName);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_FEET);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

int CModAPI_AssetManager::LoadSkinAssets_HandsScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/hands/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "hands/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 2;
		pImage->m_GridHeight = 1;
	
		CModAPI_AssetPath ShadowPath;
		CModAPI_AssetPath ColorPath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ShadowPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "hands/%s/shadow", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 0, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ColorPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "hands/%s/color", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 0, 1, 1);
		}
		{
			str_format(aBuf, sizeof(aBuf), "hands/%s", aName);
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKHAND),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHANDSHADOW))
				.Anchor(0.0f)
				.Scale(vec2(30.0f, 30.0f));
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTHAND),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHANDSHADOW))
				.Anchor(0.0f)
				.Scale(vec2(30.0f, 30.0f));
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BACKHAND),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BACKHAND))
				.Anchor(0.0f)
				.Scale(vec2(30.0f, 30.0f));
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_FRONTHAND),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_FRONTHAND))
				.Anchor(0.0f)
				.Scale(vec2(30.0f, 30.0f));
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			pCharacterPart->SetName(aName);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_HANDS);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

int CModAPI_AssetManager::LoadSkinAssets_MarkingScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/marking/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "marking/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 1;
		pImage->m_GridHeight = 1;
	
		CModAPI_AssetPath ColorPath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ColorPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "marking/%s/color", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 0, 1, 1);
		}
		{
			str_format(aBuf, sizeof(aBuf), "marking/%s", aName);
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY))
				.Anchor(0.0f)
				.Scale(vec2(64.0f, 64.0f));
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			pCharacterPart->SetName(aName);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_MARKING);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

int CModAPI_AssetManager::LoadSkinAssets_DecorationScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/decoration/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "decoration/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 2;
		pImage->m_GridHeight = 1;
	
		CModAPI_AssetPath ShadowPath;
		CModAPI_AssetPath ColorPath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ShadowPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "decoration/%s/shadow", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 1, 0, 1, 1);
		}
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&ColorPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "decoration/%s/color", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 0, 1, 1);
		}
		{
			str_format(aBuf, sizeof(aBuf), "decoration/%s", aName);
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				ShadowPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODYSHADOW))
				.Anchor(0.0f)
				.Scale(vec2(64.0f, 64.0f));
			pSkeletonSkin->AddSprite(
				ColorPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_BODY),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
				.Anchor(0.0f)
				.Scale(vec2(64.0f, 64.0f));
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			pCharacterPart->SetName(aName);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_DECORATION);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

int CModAPI_AssetManager::LoadSkinAssets_EyeScan(const char *pFilename, int IsDir, int DirType, void *pUser)
{
	CModAPI_AssetManager *pSelf = (CModAPI_AssetManager *)pUser;
	int l = str_length(pFilename);
	if(l < 4 || IsDir || str_comp(pFilename+l-4, ".png") != 0)
		return 0;

	char aName[128];
	str_copy(aName, pFilename, sizeof(aName));
	aName[str_length(aName)-4] = 0;
	
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "skins/eyes/%s", pFilename);
	
	CModAPI_AssetPath ImagePath = pSelf->AddImage(IStorage::TYPE_ALL, aBuf, CModAPI_AssetPath::SRC_SKIN);
	CModAPI_Asset_Image* pImage = pSelf->GetAsset<CModAPI_Asset_Image>(ImagePath);
	if(pImage)
	{
		str_format(aBuf, sizeof(aBuf), "eyes/%s.png", aName);
		pImage->SetName(aBuf);
		
		pImage->m_GridWidth = 4;
		pImage->m_GridHeight = 4;
	
		CModAPI_AssetPath NormalPath;
		CModAPI_AssetPath SkeletonSkinPath;
		{
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&NormalPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s/normal", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 0, 2, 1);
		}
		{
			CModAPI_AssetPath SpritePath;
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&SpritePath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s/angry", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 2, 0, 2, 1);
		}
		{
			CModAPI_AssetPath SpritePath;
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&SpritePath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s/pain", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 1, 2, 1);
		}
		{
			CModAPI_AssetPath SpritePath;
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&SpritePath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s/happy", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 2, 1, 2, 1);
		}
		{
			CModAPI_AssetPath SpritePath;
			CModAPI_Asset_Sprite* pSprite = pSelf->m_SpritesCatalog.NewAsset(&SpritePath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s/fear", aName);
			pSprite->SetName(aBuf);
			pSprite->Init(ImagePath, 0, 2, 2, 1);
		}
		{
			str_format(aBuf, sizeof(aBuf), "eyes/%s", aName);
			CModAPI_Asset_SkeletonSkin* pSkeletonSkin = pSelf->m_SkeletonSkinsCatalog.NewAsset(&SkeletonSkinPath, CModAPI_AssetPath::SRC_SKIN);
			pSkeletonSkin->SetName(aBuf);
			pSkeletonSkin->m_SkeletonPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_SKELETON, MODAPI_SKELETON_TEE);
			pSkeletonSkin->AddSprite(
				NormalPath,
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEEBONE_EYES),
				CModAPI_Asset_Skeleton::CBonePath::Local(MODAPI_TEELAYER_BODY))
				.Scale(vec2(38.4, 38.4))
				.Alignment(CModAPI_Asset_SkeletonSkin::ALIGNEMENT_WORLD);
		}
		{
			CModAPI_AssetPath CharacterPartPath;
			CModAPI_Asset_CharacterPart* pCharacterPart = pSelf->m_CharacterPartsCatalog.NewAsset(&CharacterPartPath, CModAPI_AssetPath::SRC_SKIN);
			str_format(aBuf, sizeof(aBuf), "eyes/%s", aName);
			pCharacterPart->SetName(aBuf);
			pCharacterPart->m_CharacterPath = CModAPI_AssetPath::Internal(CModAPI_AssetPath::TYPE_CHARACTER, MODAPI_CHARACTER_TEE);
			pCharacterPart->m_CharacterPart = CModAPI_Asset_Character::CSubPath::Part(MODAPI_SKINPART_EYES);
			pCharacterPart->m_SkeletonSkinPath = SkeletonSkinPath;
		}
	}
	
	return 0;
}

void CModAPI_AssetManager::LoadSkinAssets(IStorage* pStorage)
{
	dbg_msg("assetsmanager", "load skin assets");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/body", CModAPI_AssetManager::LoadSkinAssets_BodyScan, this);
	dbg_msg("assetsmanager", "body assets loaded");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/feet", CModAPI_AssetManager::LoadSkinAssets_FootScan, this);
	dbg_msg("assetsmanager", "feet assets loaded");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/eyes", CModAPI_AssetManager::LoadSkinAssets_EyeScan, this);
	dbg_msg("assetsmanager", "eyes assets loaded");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/hands", CModAPI_AssetManager::LoadSkinAssets_HandsScan, this);
	dbg_msg("assetsmanager", "eyes assets loaded");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/marking", CModAPI_AssetManager::LoadSkinAssets_MarkingScan, this);
	dbg_msg("assetsmanager", "eyes assets loaded");
	Storage()->ListDirectory(IStorage::TYPE_ALL, "skins/decoration", CModAPI_AssetManager::LoadSkinAssets_DecorationScan, this);
	dbg_msg("assetsmanager", "eyes assets loaded");
}

CModAPI_AssetPath CModAPI_AssetManager::FindSkinPart(int p, const char* pName)
{
	char aAssetName[256];
	str_format(aAssetName, sizeof(aAssetName), "%s", pName);
	
	for(int i=0; i<m_SkeletonSkinsCatalog.m_Assets[CModAPI_AssetPath::SRC_SKIN].size(); i++)
	{
		if(str_comp(m_SkeletonSkinsCatalog.m_Assets[CModAPI_AssetPath::SRC_SKIN][i].m_aName, aAssetName) == 0)
			return CModAPI_AssetPath::Skin(CModAPI_AssetPath::TYPE_CHARACTERPART, i);
	}
	
	return CModAPI_AssetPath::Null();
}

CModAPI_AssetPath CModAPI_AssetManager::AddImage(int StorageType, const char* pFilename, int Source)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = Storage()->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
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
	CModAPI_Asset_Image* pImage = m_ImagesCatalog.NewAsset(&Path, Source);
	pImage->SetName(pFilename);
	pImage->m_Width = Png.width;
	pImage->m_Height = Png.height;
	pImage->m_Format = Format;
	pImage->m_pData = pBuffer;
	pImage->m_Texture = Graphics()->LoadTextureRaw(Png.width, Png.height, Format, pBuffer, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

	return Path;
}

int CModAPI_AssetManager::SaveInAssetsFile(const char *pFileName, int Source)
{
	CDataFileWriter df;
	if(!df.Open(Storage(), pFileName))
	{
		dbg_msg("ModAPIGraphics", "can't create the assets file %s", pFileName);
		return 0;
	}
	
	{
		CStorageType Item;
		Item.m_Version = 0;
		Item.m_Source = Source;
		
		df.AddItem(CModAPI_Asset_Image::TypeId, 0, sizeof(CStorageType), &Item);
	}
	
	//Search Tag: TAG_NEW_ASSET
	m_ImagesCatalog.SaveInAssetsFile(&df, Source);
	m_SpritesCatalog.SaveInAssetsFile(&df, Source);
	m_SkeletonsCatalog.SaveInAssetsFile(&df, Source);
	m_SkeletonSkinsCatalog.SaveInAssetsFile(&df, Source);
	m_SkeletonAnimationsCatalog.SaveInAssetsFile(&df, Source);
	m_CharactersCatalog.SaveInAssetsFile(&df, Source);
	m_CharacterPartsCatalog.SaveInAssetsFile(&df, Source);
	
	df.Finish();
	
	return 1;
}

int CModAPI_AssetManager::OnAssetsFileLoaded(IModAPI_AssetsFile* pAssetsFile)
{
	int Start, Num;
	pAssetsFile->GetType(0, &Start, &Num);
	
	int Source = CModAPI_AssetPath::SRC_EXTERNAL;
	if(Num > 0)
	{
		CStorageType* pItem = (CStorageType*) pAssetsFile->GetItem(Start, 0, 0);
		Source = pItem->m_Source % CModAPI_AssetPath::NUM_SOURCES;
	}
	
	//Search Tag: TAG_NEW_ASSET
	m_ImagesCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_SpritesCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_SkeletonsCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_SkeletonSkinsCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_SkeletonAnimationsCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_CharactersCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	m_CharacterPartsCatalog.LoadFromAssetsFile(this, pAssetsFile, Source);
	
	return 1;
}

int CModAPI_AssetManager::OnAssetsFileUnloaded()
{
	//Search Tag: TAG_NEW_ASSET
	m_CharacterPartsCatalog.Unload(this);
	m_CharactersCatalog.Unload(this);
	m_SkeletonAnimationsCatalog.Unload(this);
	m_SkeletonSkinsCatalog.Unload(this);
	m_SkeletonsCatalog.Unload(this);
	m_SpritesCatalog.Unload(this);
	m_ImagesCatalog.Unload(this);
}
	
void CModAPI_AssetManager::DeleteAsset(const CModAPI_AssetPath& Path)
{
	//Search Tag: TAG_NEW_ASSET
	m_ImagesCatalog.DeleteAsset(Path);
	m_SpritesCatalog.DeleteAsset(Path);
	m_SkeletonsCatalog.DeleteAsset(Path);
	m_SkeletonSkinsCatalog.DeleteAsset(Path);
	m_SkeletonAnimationsCatalog.DeleteAsset(Path);
	m_CharactersCatalog.DeleteAsset(Path);
	m_CharacterPartsCatalog.DeleteAsset(Path);
	
	//Search Tag: TAG_NEW_ASSET
	m_ImagesCatalog.OnAssetDeleted(Path);
	m_SpritesCatalog.OnAssetDeleted(Path);
	m_SkeletonsCatalog.OnAssetDeleted(Path);
	m_SkeletonSkinsCatalog.OnAssetDeleted(Path);
	m_SkeletonAnimationsCatalog.OnAssetDeleted(Path);
	m_CharactersCatalog.OnAssetDeleted(Path);
	m_CharacterPartsCatalog.OnAssetDeleted(Path);
}
	
int CModAPI_AssetManager::AddSubItem(CModAPI_AssetPath AssetPath, int SubItemType)
{
	#define ADD_SUB_ITEM(TypeName) case TypeName::TypeId:\
	{\
		TypeName* pAsset = GetAsset<TypeName>(AssetPath);\
		if(pAsset)\
			return pAsset->AddSubItem(SubItemType);\
		else\
			return -1;\
	}

	switch(AssetPath.GetType())
	{
		//Search Tag: TAG_NEW_ASSET
		ADD_SUB_ITEM(CModAPI_Asset_Image);
		ADD_SUB_ITEM(CModAPI_Asset_Sprite);
		ADD_SUB_ITEM(CModAPI_Asset_Skeleton);
		ADD_SUB_ITEM(CModAPI_Asset_SkeletonSkin);
		ADD_SUB_ITEM(CModAPI_Asset_SkeletonAnimation);
		ADD_SUB_ITEM(CModAPI_Asset_Character);
		ADD_SUB_ITEM(CModAPI_Asset_CharacterPart);
		ADD_SUB_ITEM(CModAPI_Asset_List);
	}
	
	return -1;
}
	
void CModAPI_AssetManager::DeleteSubItem(CModAPI_AssetPath AssetPath, int SubPath)
{
	#define DELETE_SUB_ITEM(TypeName) case TypeName::TypeId:\
	{\
		TypeName* pAsset = GetAsset<TypeName>(AssetPath);\
		if(pAsset)\
			pAsset->DeleteSubItem(SubPath);\
		break;\
	}

	switch(AssetPath.GetType())
	{
		//Search Tag: TAG_NEW_ASSET
		DELETE_SUB_ITEM(CModAPI_Asset_Image);
		DELETE_SUB_ITEM(CModAPI_Asset_Sprite);
		DELETE_SUB_ITEM(CModAPI_Asset_Skeleton);
		DELETE_SUB_ITEM(CModAPI_Asset_SkeletonSkin);
		DELETE_SUB_ITEM(CModAPI_Asset_SkeletonAnimation);
		DELETE_SUB_ITEM(CModAPI_Asset_Character);
		DELETE_SUB_ITEM(CModAPI_Asset_CharacterPart);
		DELETE_SUB_ITEM(CModAPI_Asset_List);
	}
	
	//Search Tag: TAG_NEW_ASSET
	m_ImagesCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_SpritesCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_SkeletonsCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_SkeletonSkinsCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_SkeletonAnimationsCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_CharactersCatalog.OnSubItemDeleted(AssetPath, SubPath);
	m_ListsCatalog.OnSubItemDeleted(AssetPath, SubPath);
}
