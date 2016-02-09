#include <modapi/client/graphics.h>
#include <modapi/moditem.h>
#include <modapi/shared/mod.h>
#include <generated/client_data.h>
#include <game/client/render.h>
#include <engine/shared/config.h>

#include <game/gamecore.h>

CModAPI_Sprite::CModAPI_Sprite()
{
	
}

CModAPI_Sprite::CModAPI_Sprite(int ImageId, int X, int Y, int W, int H, int GridX, int GridY) :
	m_ImageId(ImageId),
	m_X(X),
	m_Y(Y),
	m_W(W),
	m_H(H),
	m_GridX(GridX),
	m_GridY(GridY)
{
	
}

CModAPI_TeeAnimation::CModAPI_TeeAnimation() :
	m_BodyAnimation(-1),
	m_BackFootAnimation(-1),
	m_FrontFootAnimation(-1),
	
	m_BackHandAlignment(MODAPI_TEEALIGN_NONE),
	m_BackHandAnimation(-1),
	m_BackHandOffsetX(0),
	m_BackHandOffsetY(0),
	
	m_FrontHandAlignment(MODAPI_TEEALIGN_NONE),
	m_FrontHandAnimation(-1),
	m_FrontHandOffsetX(0),
	m_FrontHandOffsetY(0)
{
	
}
	
CModAPI_Weapon::CModAPI_Weapon() :
	m_WeaponSprite(-1),
	m_WeaponSize(0),
	m_WeaponAlignment(MODAPI_TEEALIGN_NONE),
	m_WeaponOffsetX(0),
	m_WeaponOffsetY(0),
	m_WeaponAnimation(-1),
	
	m_NumMuzzles(-1),
	m_MuzzleSprite(-1),
	m_MuzzleSize(0),
	m_MuzzleAlignment(MODAPI_TEEALIGN_NONE),
	m_MuzzleOffsetX(0),
	m_MuzzleOffsetY(0),
	m_MuzzleAnimation(-1),
	
	m_TeeAnimation(-1)
{
	
}
	
CModAPI_Client_Graphics::CModAPI_Client_Graphics(IGraphics* pGraphics)
{
	m_pGraphics = pGraphics;
}

void CModAPI_Client_Graphics::Init()
{
	//Images
	m_InternalImages[MODAPI_IMAGE_GAME].m_Texture = g_pData->m_aImages[IMAGE_GAME].m_Id;
	
	//Sprites
	m_InternalSprites[MODAPI_SPRITE_HAMMER] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 1, 4, 3, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_GUN] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 4, 4, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_SHOTGUN] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 6, 8, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_GRENADE] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 8, 7, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_LASER] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 12, 7, 3, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_NINJA] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 2, 10, 8, 2, 32, 16);
	
	m_InternalSprites[MODAPI_SPRITE_GUN_MUZZLE1] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 8, 4, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_GUN_MUZZLE2] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 12, 4, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_GUN_MUZZLE3] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 16, 4, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_SHOTGUN_MUZZLE1] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 12, 6, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_SHOTGUN_MUZZLE2] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 16, 6, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_SHOTGUN_MUZZLE3] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 20, 6, 3, 2, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_NINJA_MUZZLE1] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 25, 0, 7, 4, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_NINJA_MUZZLE2] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 25, 4, 7, 4, 32, 16);
	m_InternalSprites[MODAPI_SPRITE_NINJA_MUZZLE3] = CModAPI_Sprite(MODAPI_INTERNAL_ID(MODAPI_IMAGE_GAME), 25, 8, 7, 4, 32, 16);

	//Animations
	vec2 BodyPos = vec2(0.0f, -4.0f);
	vec2 FootPos = vec2(0.0f, 10.0f);
	
		//Idle
	m_InternalAnimations[MODAPI_ANIMATION_IDLE_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(-7.0f,  0.0f),  0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_IDLE_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(7.0f,  0.0f),  0.0f, 1.f);
	
		//InAir
	m_InternalAnimations[MODAPI_ANIMATION_INAIR_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(-3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_INAIR_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(3.0f,  0.0f),  -0.1f*pi*2.0f, 1.f);
	
		//Walk
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.2f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.4f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.6f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(0.8f, BodyPos + vec2(0.0f, -1.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BODY].AddKeyFrame(1.0f, BodyPos + vec2(0.0f,  0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.0f, FootPos + vec2(  8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.2f, FootPos + vec2( -8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.4f, FootPos + vec2(-10.0f, -4.0f),  0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.6f, FootPos + vec2( -8.0f, -8.0f),  0.3f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(0.8f, FootPos + vec2(  4.0f, -4.0f), -0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_BACKFOOT].AddKeyFrame(1.0f, FootPos + vec2(  8.0f,  0.0f),  0.0f*pi*2.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.0f, FootPos + vec2(-10.0f, -4.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.2f, FootPos + vec2( -8.0f, -8.0f),  0.0f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.4f, FootPos + vec2(  4.0f, -4.0f),  0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.6f, FootPos + vec2(  8.0f,  0.0f),  0.3f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(0.8f, FootPos + vec2(  8.0f,  0.0f), -0.2f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_WALK_FRONTFOOT].AddKeyFrame(1.0f, FootPos + vec2(-10.0f, -4.0f),  0.0f*pi*2.0f, 1.f);
	
		//Weapons	
	m_InternalAnimations[MODAPI_ANIMATION_HAMMERATTACK_WEAPON].AddKeyFrame(0.0f/5.0f, vec2(0.0f, 0.0f), -pi/2-0.10f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_HAMMERATTACK_WEAPON].AddKeyFrame(0.2f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.25f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_HAMMERATTACK_WEAPON].AddKeyFrame(0.4f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.30f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_HAMMERATTACK_WEAPON].AddKeyFrame(0.6f/5.0f, vec2(0.0f, 0.0f), -pi/2+0.25f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_HAMMERATTACK_WEAPON].AddKeyFrame(0.8f/5.0f, vec2(0.0f, 0.0f), -pi/2-0.10f*pi*2.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_WEAPON].AddKeyFrame(0.00f, vec2(32.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_WEAPON].AddKeyFrame(0.05f, vec2(32.0f - 10.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_WEAPON].AddKeyFrame(0.10f, vec2(32.0f, 0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_MUZZLE].AddKeyFrame(0.00f, vec2(82.0f, -6.0f), 0.0f, 1.0f);
	
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_BACKHAND].AddKeyFrame(0.00f, vec2(0.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_BACKHAND].AddKeyFrame(0.05f, vec2(-10.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GUNATTACK_BACKHAND].AddKeyFrame(0.10f, vec2(0.0f - 15.0f + 32.0f, 4.0f), -3*pi/4, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON].AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON].AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON].AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE].AddKeyFrame(0.00f, vec2(94.0f, -6.0f), 0.0f, 1.0f);
	
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND].AddKeyFrame(0.00f, vec2(0.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND].AddKeyFrame(0.05f, vec2(-10.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND].AddKeyFrame(0.10f, vec2(0.0f - 5.0f + 24.0f, 4.0f), -pi/2, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_WEAPON].AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_WEAPON].AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_WEAPON].AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_BACKHAND].AddKeyFrame(0.00f, vec2(0.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_BACKHAND].AddKeyFrame(0.05f, vec2(-10.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_GRENADEATTACK_BACKHAND].AddKeyFrame(0.10f, vec2(0.0f - 5.0f + 24.0f, 7.0f), -pi/2, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_LASERATTACK_WEAPON].AddKeyFrame(0.00f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_LASERATTACK_WEAPON].AddKeyFrame(0.05f, vec2(24.0f - 10.0f, 0.0f), 0.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_LASERATTACK_WEAPON].AddKeyFrame(0.10f, vec2(24.0f, 0.0f), 0.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(0.00f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.25f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(0.10f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.05f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(0.15f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.35f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(0.42f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.40f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(0.50f/2.0f, vec2(0.0f, 0.0f), -pi/2+0.35f*pi*2.0f, 1.f);
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_WEAPON].AddKeyFrame(1.00f/2.0f, vec2(0.0f, 0.0f), -pi/2-0.25f*pi*2.0f, 1.f);
	
	m_InternalAnimations[MODAPI_ANIMATION_NINJAATTACK_MUZZLE].AddKeyFrame(0.00f, vec2(-40.0f, -4.0f), 0.0f, 1.0f);
	
	//TeeAnimations
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_IDLE_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_IDLE_FRONTFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_IDLE].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_INAIR_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_INAIR_FRONTFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_INAIR].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_BodyAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_BODY);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_BackFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_BACKFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_FrontFootAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_WALK_FRONTFOOT);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_BackHandAlignment = MODAPI_TEEALIGN_NONE;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_WALK].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GUNATTACK].m_BackHandAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_GUNATTACK_BACKHAND);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GUNATTACK].m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GUNATTACK].m_BackHandOffsetY = 4;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GUNATTACK].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_SHOTGUNATTACK].m_BackHandAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_SHOTGUNATTACK_BACKHAND);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_SHOTGUNATTACK].m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_SHOTGUNATTACK].m_BackHandOffsetY = -2;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_SHOTGUNATTACK].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GRENADEATTACK].m_BackHandAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_GRENADEATTACK_BACKHAND);
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GRENADEATTACK].m_BackHandAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GRENADEATTACK].m_BackHandOffsetY = -2;
	m_InternalTeeAnimations[MODAPI_TEEANIMATION_GRENADEATTACK].m_FrontHandAlignment = MODAPI_TEEALIGN_NONE;
	
	//Weapons
		//Hammer
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_HAMMER);
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponAlignment = MODAPI_TEEALIGN_HORIZONTAL;
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponOffsetX = 4;
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponOffsetY = -20;
	m_InternalWeapons[MODAPI_WEAPON_HAMMER].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_HAMMERATTACK_WEAPON);
	
		//Gun
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_GUN);
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponSize = 64;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponOffsetY = 4;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_GUNATTACK_WEAPON);
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_NumMuzzles = 3;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_GUN_MUZZLE1);
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleSize = 64;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleOffsetY = 4;
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_MuzzleAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_GUNATTACK_MUZZLE);
	m_InternalWeapons[MODAPI_WEAPON_GUN].m_TeeAnimation = MODAPI_INTERNAL_ID(MODAPI_TEEANIMATION_GUNATTACK);
	
		//Shotgun
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_SHOTGUN);
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponOffsetY = -2;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_SHOTGUNATTACK_WEAPON);
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_NumMuzzles = 3;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_SHOTGUN_MUZZLE1);
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleOffsetY = -2;
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_MuzzleAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_SHOTGUNATTACK_MUZZLE);
	m_InternalWeapons[MODAPI_WEAPON_SHOTGUN].m_TeeAnimation = MODAPI_INTERNAL_ID(MODAPI_TEEANIMATION_SHOTGUNATTACK);
	
		//Grenade
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_GRENADE);
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponOffsetY = -2;
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_GRENADEATTACK_WEAPON);
	m_InternalWeapons[MODAPI_WEAPON_GRENADE].m_TeeAnimation = MODAPI_INTERNAL_ID(MODAPI_TEEANIMATION_GRENADEATTACK);
	
		//Grenade
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_LASER);
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponAlignment = MODAPI_TEEALIGN_AIM;
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponOffsetY = -2;
	m_InternalWeapons[MODAPI_WEAPON_LASER].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_LASERATTACK_WEAPON);
	
		//Ninja
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_NINJA);
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponSize = 96;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponAlignment = MODAPI_TEEALIGN_HORIZONTAL;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponOffsetY = 0;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_WeaponAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_NINJAATTACK_WEAPON);
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_NumMuzzles = 3;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleSprite = MODAPI_INTERNAL_ID(MODAPI_SPRITE_NINJA_MUZZLE1);
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleSize = 160;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleAlignment = MODAPI_TEEALIGN_DIRECTION;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleOffsetX = 0;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleOffsetY = 0;
	m_InternalWeapons[MODAPI_WEAPON_NINJA].m_MuzzleAnimation = MODAPI_INTERNAL_ID(MODAPI_ANIMATION_NINJAATTACK_MUZZLE);
}

const CModAPI_Image* CModAPI_Client_Graphics::GetImage(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_IMAGES)
			return 0;
		else
			return &m_InternalImages[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalImages.size())
			return 0;
		else
			return &m_ExternalImages[Id];
	}
}

const CModAPI_Animation* CModAPI_Client_Graphics::GetAnimation(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_ANIMATIONS)
			return 0;
		else
			return &m_InternalAnimations[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalAnimations.size())
			return 0;
		else
			return &m_ExternalAnimations[Id];
	}
}

const CModAPI_TeeAnimation* CModAPI_Client_Graphics::GetTeeAnimation(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_TEEANIMATIONS)
			return 0;
		else
			return &m_InternalTeeAnimations[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalTeeAnimations.size())
			return 0;
		else
			return &m_ExternalTeeAnimations[Id];
	}
}

const CModAPI_Sprite* CModAPI_Client_Graphics::GetSprite(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_SPRITES)
			return 0;
		else
			return &m_InternalSprites[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalSprites.size())
			return 0;
		else
			return &m_ExternalSprites[Id];
	}
}

const CModAPI_LineStyle* CModAPI_Client_Graphics::GetLineStyle(int Id) const
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

const CModAPI_Weapon* CModAPI_Client_Graphics::GetWeapon(int Id) const
{
	if(MODAPI_IS_INTERNAL_ID(Id))
	{
		Id = MODAPI_GET_INTERNAL_ID(Id);
		
		if(Id < 0 || Id >= MODAPI_NUM_WEAPONS)
			return 0;
		else
			return &m_InternalWeapons[Id];
	}
	else
	{
		Id = MODAPI_GET_EXTERNAL_ID(Id);
		
		if(Id < 0 || Id >= m_ExternalWeapons.size())
			return 0;
		else
			return &m_ExternalWeapons[Id];
	}
}

int CModAPI_Client_Graphics::OnModLoaded(IMod* pMod)
{
	//Load images
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_IMAGE, &Start, &Num);
		
		m_ExternalImages.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Image *pItem = (CModAPI_ModItem_Image*) pMod->GetItem(Start+i, 0, 0);
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			CModAPI_Image* pImage = &m_ExternalImages[pItem->m_Id];
			
			// copy base info
			pImage->m_Width = pItem->m_Width;
			pImage->m_Height = pItem->m_Height;
			pImage->m_Format = pItem->m_Format;
			int PixelSize = pImage->m_Format == CImageInfo::FORMAT_RGB ? 3 : 4;

			// copy image data
			void *pData = pMod->GetData(pItem->m_ImageData);
			pImage->m_pData = mem_alloc(pImage->m_Width*pImage->m_Height*PixelSize, 1);
			mem_copy(pImage->m_pData, pData, pImage->m_Width*pImage->m_Height*PixelSize);
			pImage->m_Texture = m_pGraphics->LoadTextureRaw(pImage->m_Width, pImage->m_Height, pImage->m_Format, pImage->m_pData, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);

			// unload image
			pMod->UnloadData(pItem->m_ImageData);
		}
	}
	
	//Load animations
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_ANIMATION, &Start, &Num);
		
		m_ExternalAnimations.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Animation *pItem = (CModAPI_ModItem_Animation *) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			const CModAPI_AnimationKeyFrame* pFrames = static_cast<CModAPI_AnimationKeyFrame*>(pMod->GetData(pItem->m_KeyFrameData));
			
			CModAPI_Animation* Animation = &m_ExternalAnimations[pItem->m_Id];
			for(int f=0; f<pItem->m_NumKeyFrame; f++)
			{
				Animation->AddKeyFrame(pFrames[f].m_Time, pFrames[f].m_Pos, pFrames[f].m_Angle, pFrames[f].m_Opacity);
			}
		}
	}
	
	//Load sprites
	{
		int Start, Num;
		pMod->GetType(MODAPI_MODITEMTYPE_SPRITE, &Start, &Num);
		
		m_ExternalSprites.set_size(Num);
		
		for(int i = 0; i < Num; i++)
		{
			CModAPI_ModItem_Sprite *pItem = (CModAPI_ModItem_Sprite *) pMod->GetItem(Start+i, 0, 0);
			
			int Id = MODAPI_GET_EXTERNAL_ID(pItem->m_Id);
			if(Id >= Num) return 0;
			
			CModAPI_Sprite* sprite = &m_ExternalSprites[pItem->m_Id];
			sprite->m_X = pItem->m_X;
			sprite->m_Y = pItem->m_Y;
			sprite->m_W = pItem->m_W;
			sprite->m_H = pItem->m_H;
			sprite->m_ImageId = pItem->m_ImageId;
			sprite->m_GridX = pItem->m_GridX;
			sprite->m_GridY = pItem->m_GridY;
		}
	}
	
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
}

int CModAPI_Client_Graphics::OnModUnloaded()
{
	//Unload images
	for(int i = 0; i < m_ExternalImages.size(); i++)
	{
		m_pGraphics->UnloadTexture(m_ExternalImages[i].m_Texture);
		if(m_ExternalImages[i].m_pData)
		{
			mem_free(m_ExternalImages[i].m_pData);
			m_ExternalImages[i].m_pData = 0;
		}
	}
	
	m_ExternalImages.clear();
	
	//Unload sprites
	m_ExternalSprites.clear();
	
	//Unload line styles
	m_ExternalLineStyles.clear();
	
	return 1;
}

bool CModAPI_Client_Graphics::TextureSet(int ImageID)
{
	const CModAPI_Image* pImage = GetImage(ImageID);
	if(pImage == 0)
		return false;
	
	m_pGraphics->BlendNormal();
	m_pGraphics->TextureSet(pImage->m_Texture);
	
	return true;
}

void CModAPI_Client_Graphics::DrawSprite(CRenderTools* pRenderTools,int SpriteID, vec2 Pos, float Size, float Angle, int SpriteFlag, float Opacity)
{
	const CModAPI_Sprite* pSprite = GetSprite(SpriteID);
	if(pSprite == 0) return;
	
	if(!TextureSet(pSprite->m_ImageId))
		return;

	m_pGraphics->QuadsBegin();
	m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, Opacity);
	pRenderTools->SelectModAPISprite(pSprite, SpriteFlag);
	
	m_pGraphics->QuadsSetRotation(Angle);

	pRenderTools->DrawSprite(Pos.x, Pos.y, Size);
	m_pGraphics->QuadsEnd();
}

void CModAPI_Client_Graphics::DrawAnimatedSprite(CRenderTools* pRenderTools, int SpriteID, vec2 Pos, float Size, float Angle, int SpriteFlag, int AnimationID, float Time, vec2 Offset)
{
	const CModAPI_Animation* pAnimation = GetAnimation(AnimationID);
	if(pAnimation == 0) return;
	
	CModAPI_AnimationFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	float X = Pos.x;
	float Y = Pos.y;
	float animX = Frame.m_Pos.x + Offset.x * cos(Frame.m_Angle) - Offset.y * sin(Frame.m_Angle);
	float animY = Frame.m_Pos.y + Offset.x * sin(Frame.m_Angle) + Offset.y * cos(Frame.m_Angle);
	float FrameAngle = Frame.m_Angle;
	
	if(SpriteFlag & MODAPI_SPRITEFLAG_FLIP_ANIM_X)
	{
		animX = -animX;
		FrameAngle = -FrameAngle;
	}
	else if(SpriteFlag & MODAPI_SPRITEFLAG_FLIP_ANIM_Y)
	{
		animY = -animY;
		FrameAngle = -FrameAngle;
	}
	
	X += (animX * cos(Angle) - animY * sin(Angle));
	Y += (animX * sin(Angle) + animY * cos(Angle));
	
	DrawSprite(pRenderTools, SpriteID, vec2(X, Y), Size, Angle + FrameAngle, SpriteFlag);
}

void CModAPI_Client_Graphics::DrawLine(CRenderTools* pRenderTools,int LineStyleID, vec2 StartPoint, vec2 EndPoint, float Ms)
{
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
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
			
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImageId))
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
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImageId))
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
		
		const CModAPI_Sprite* pSprite = GetSprite(SpriteId);
		if(pSprite == 0) return;
		
		m_pGraphics->BlendNormal();
		
		//Define sprite texture
		if(!TextureSet(pSprite->m_ImageId))
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

void CModAPI_Client_Graphics::DrawAnimatedText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment, int AnimationID, float Time, vec2 Offset)
{
	const CModAPI_Animation* pAnimation = GetAnimation(AnimationID);
	if(pAnimation == 0) return;
	
	CModAPI_AnimationFrame Frame;
	pAnimation->GetFrame(Time, &Frame);
	
	float animX = Frame.m_Pos.x + - Offset.x * cos(Frame.m_Angle) - Offset.y * sin(Frame.m_Angle);
	float animY = Frame.m_Pos.y + - Offset.x * sin(Frame.m_Angle) + Offset.y * cos(Frame.m_Angle);
	
	float X = Pos.x + animX;
	float Y = Pos.y + animY;
	
	vec4 NewColor = Color;
	NewColor.a *= Frame.m_Opacity;
	
	DrawText(pTextRender, pText, vec2(X, Y), NewColor, Size, Alignment);
}

void CModAPI_Client_Graphics::ApplyTeeAlign(CModAPI_AnimationFrame& Frame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset)
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

void CModAPI_Client_Graphics::DrawWeaponMuzzle(CRenderTools* pRenderTools, const CModAPI_WeaponAnimationState* pState, int WeaponID, vec2 Pos)
{
	if(!pState->m_MuzzleEnabled)
		return;
	
	const CModAPI_Weapon* pWeapon = GetWeapon(WeaponID);
	if(pWeapon == 0) return;
	
	int MuzzleSpriteId;
	if(MODAPI_IS_INTERNAL_ID(pWeapon->m_MuzzleSprite))
	{
		MuzzleSpriteId = MODAPI_INTERNAL_ID(MODAPI_GET_INTERNAL_ID(pWeapon->m_MuzzleSprite)+rand()%pWeapon->m_NumMuzzles);
	}
	else
	{
		MuzzleSpriteId = MODAPI_EXTERNAL_ID(MODAPI_GET_EXTERNAL_ID(pWeapon->m_MuzzleSprite)+rand()%pWeapon->m_NumMuzzles);
	}
	
	DrawSprite(pRenderTools, MuzzleSpriteId,
		Pos + vec2(pState->m_Muzzle.m_Pos.x, pState->m_Muzzle.m_Pos.y),
		pWeapon->m_MuzzleSize,
		pState->m_Muzzle.m_Angle,
		pState->m_MuzzleFlags,
		pState->m_Muzzle.m_Opacity		
	);
}

void CModAPI_Client_Graphics::DrawWeapon(CRenderTools* pRenderTools, const CModAPI_WeaponAnimationState* pState, int WeaponID, vec2 Pos)
{
	const CModAPI_Weapon* pWeapon = GetWeapon(WeaponID);
	if(pWeapon == 0) return;
	
	DrawSprite(pRenderTools, pWeapon->m_WeaponSprite,
		Pos + vec2(pState->m_Weapon.m_Pos.x, pState->m_Weapon.m_Pos.y),
		pWeapon->m_WeaponSize,
		pState->m_Weapon.m_Angle,
		pState->m_WeaponFlags
	);
}

void CModAPI_Client_Graphics::DrawTee(CRenderTools* pRenderTools, CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pTeeState, vec2 Pos, vec2 Dir, int Emote)
{
	if(pTeeState->m_BackHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		float HandSize = 15.0f;
		IGraphics::CQuadItem QuadItemHand(Pos.x + pTeeState->m_BackHand.m_Pos.x, Pos.y + pTeeState->m_BackHand.m_Pos.y, 2.0f*HandSize, 2.0f*HandSize);
		IGraphics::CQuadItem QuadItem;
		
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		vec4 Color = pInfo->m_aColors[3];
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);

		//draw hand, outline
		QuadItem = QuadItemHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_BackHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_BackHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}
	
	if(pTeeState->m_FrontHandEnabled && pInfo->m_aTextures[3].IsValid())
	{
		float HandSize = 15.0f;
		IGraphics::CQuadItem QuadItemHand(Pos.x + pTeeState->m_FrontHand.m_Pos.x, Pos.y + pTeeState->m_FrontHand.m_Pos.y, 2.0f*HandSize, 2.0f*HandSize);
		IGraphics::CQuadItem QuadItem;
		
		m_pGraphics->TextureSet(pInfo->m_aTextures[3]);
		m_pGraphics->QuadsBegin();
		vec4 Color = pInfo->m_aColors[3];
		m_pGraphics->SetColor(Color.r, Color.g, Color.b, Color.a);

		//draw hand, outline
		QuadItem = QuadItemHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND_OUTLINE, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		//draw hand
		QuadItem = QuadItemHand;
		pRenderTools->SelectSprite(SPRITE_TEE_HAND, pTeeState->m_FrontHandFlags, 0, 0);
		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontHand.m_Angle);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		
		m_pGraphics->QuadsSetRotation(0);
		m_pGraphics->QuadsEnd();
	}
	
	float Size = pInfo->m_Size;
	bool IndicateAirJump = !pInfo->m_GotAirJump && g_Config.m_ClAirjumpindicator;
	
	IGraphics::CQuadItem BodyItem(Pos.x + pTeeState->m_Body.m_Pos.x, Pos.y + pTeeState->m_Body.m_Pos.y, Size, Size);
	IGraphics::CQuadItem Item;

	// draw back feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, pTeeState->m_BackFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_BackFoot.m_Pos.x, Pos.y + pTeeState->m_BackFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration, outline
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[2].r, pInfo->m_aColors[2].g, pInfo->m_aColors[2].b, pInfo->m_aColors[2].a);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION_OUTLINE, 0, 0, 0);
		
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body, outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_OUTLINE, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet, outline
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, pTeeState->m_FrontFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT_OUTLINE, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_FrontFoot.m_Pos.x, Pos.y + pTeeState->m_FrontFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw back feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;
		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_BackFoot.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[4].r*cs, pInfo->m_aColors[4].g*cs, pInfo->m_aColors[4].b*cs, pInfo->m_aColors[4].a * pTeeState->m_BackFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_BackFoot.m_Pos.x, Pos.y + pTeeState->m_BackFoot.m_Pos.y, w, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw decoration
	if(pInfo->m_aTextures[2].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[2]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[2].r, pInfo->m_aColors[2].g, pInfo->m_aColors[2].b, pInfo->m_aColors[2].a);
		pRenderTools->SelectSprite(SPRITE_TEE_DECORATION, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw body
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[0].r, pInfo->m_aColors[0].g, pInfo->m_aColors[0].b, pInfo->m_aColors[0].a);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw marking
	if(pInfo->m_aTextures[1].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[1]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[1].r, pInfo->m_aColors[1].g, pInfo->m_aColors[1].b, pInfo->m_aColors[1].a);
		pRenderTools->SelectSprite(SPRITE_TEE_MARKING, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw body shadow and upper outline
	if(pInfo->m_aTextures[0].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[0]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(pTeeState->m_Body.m_Angle);
		m_pGraphics->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_SHADOW, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		pRenderTools->SelectSprite(SPRITE_TEE_BODY_UPPER_OUTLINE, 0, 0, 0);
		Item = BodyItem;
		m_pGraphics->QuadsDraw(&Item, 1);
		m_pGraphics->QuadsEnd();
	}
	
	// draw eyes
	if(pInfo->m_aTextures[5].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[5]);
		m_pGraphics->QuadsBegin();
		m_pGraphics->QuadsSetRotation(0.0f);
		m_pGraphics->SetColor(pInfo->m_aColors[5].r, pInfo->m_aColors[5].g, pInfo->m_aColors[5].b, pInfo->m_aColors[5].a);
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

		float EyeScale = Size*0.60f;
		float h = (Emote == EMOTE_BLINK) ? Size*0.15f/2.0f : EyeScale/2.0f;
		vec2 Offset = vec2(Dir.x*0.125f, -0.05f+Dir.y*0.10f)*Size;
		IGraphics::CQuadItem QuadItem(Pos.x+Offset.x, Pos.y+Offset.y, EyeScale, h);
		m_pGraphics->QuadsDraw(&QuadItem, 1);
		m_pGraphics->QuadsEnd();
	}

	// draw front feet
	if(pInfo->m_aTextures[4].IsValid())
	{
		m_pGraphics->TextureSet(pInfo->m_aTextures[4]);
		m_pGraphics->QuadsBegin();

		float w = Size/2.1f;
		float h = w;
		float cs = (IndicateAirJump ? 0.5f : 1.0f);

		m_pGraphics->QuadsSetRotation(pTeeState->m_FrontFoot.m_Angle);
		m_pGraphics->SetColor(pInfo->m_aColors[4].r*cs, pInfo->m_aColors[4].g*cs, pInfo->m_aColors[4].b*cs, pInfo->m_aColors[4].a * pTeeState->m_FrontFoot.m_Opacity);
		pRenderTools->SelectSprite(SPRITE_TEE_FOOT, 0, 0, 0);
		IGraphics::CQuadItem QuadItem(Pos.x + pTeeState->m_FrontFoot.m_Pos.x, Pos.y + pTeeState->m_FrontFoot.m_Pos.y, w, h);
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

void CModAPI_Client_Graphics::AddTeeAnimationState(CModAPI_TeeAnimationState* pState, int TeeAnimationID, float Time)
{
	const CModAPI_TeeAnimation* pTeeAnim = GetTeeAnimation(TeeAnimationID);
	if(pTeeAnim == 0) return;
	
	if(pTeeAnim->m_BodyAnimation >= 0)
	{
		const CModAPI_Animation* pBodyAnim = GetAnimation(pTeeAnim->m_BodyAnimation);
		if(pBodyAnim)
		{
			pBodyAnim->GetFrame(Time, &pState->m_Body);
		}
	}
	
	if(pTeeAnim->m_BackFootAnimation >= 0)
	{
		const CModAPI_Animation* pBackFootAnim = GetAnimation(pTeeAnim->m_BackFootAnimation);
		if(pBackFootAnim)
		{
			pBackFootAnim->GetFrame(Time, &pState->m_BackFoot);
		}
	}
	
	if(pTeeAnim->m_FrontFootAnimation >= 0)
	{
		const CModAPI_Animation* pFrontFootAnim = GetAnimation(pTeeAnim->m_FrontFootAnimation);
		if(pFrontFootAnim)
		{
			pFrontFootAnim->GetFrame(Time, &pState->m_FrontFoot);
		}
	}
	
	if(pTeeAnim->m_BackHandAnimation >= 0)
	{
		const CModAPI_Animation* pBackHandAnim = GetAnimation(pTeeAnim->m_BackHandAnimation);
		if(pBackHandAnim)
		{
			pState->m_BackHandEnabled = true;
			
			pBackHandAnim->GetFrame(Time, &pState->m_BackHand);
			
			ApplyTeeAlign(pState->m_BackHand, pState->m_BackHandFlags, pTeeAnim->m_BackHandAlignment, pState->m_MotionDir, pState->m_AimDir, vec2(pTeeAnim->m_BackHandOffsetX, pTeeAnim->m_BackHandOffsetY));
		}
	}
	
	if(pTeeAnim->m_FrontHandAnimation >= 0)
	{
		const CModAPI_Animation* pFrontHandAnim = GetAnimation(pTeeAnim->m_FrontHandAnimation);
		if(pFrontHandAnim)
		{
			pState->m_FrontHandEnabled = true;
			pFrontHandAnim->GetFrame(Time, &pState->m_FrontHand);
		}
	}
}

void CModAPI_Client_Graphics::InitWeaponAnimationState(CModAPI_WeaponAnimationState* pState, vec2 MotionDir, vec2 AimDir, int WeaponID, float Time)
{
	pState->m_Weapon.m_Pos.x = 0.0f;
	pState->m_Weapon.m_Pos.y = 0.0f;
	pState->m_Weapon.m_Angle = 0.0f;
	pState->m_Weapon.m_Opacity = 1.0f;
	pState->m_WeaponFlags = 0x0;
	
	pState->m_MuzzleEnabled = false;
	pState->m_Muzzle.m_Pos.x = 0.0f;
	pState->m_Muzzle.m_Pos.y = 0.0f;
	pState->m_Muzzle.m_Angle = 0.0f;
	pState->m_Muzzle.m_Opacity = 1.0f;
	pState->m_MuzzleFlags = 0x0;
	
	const CModAPI_Weapon* pWeapon = GetWeapon(WeaponID);
	if(pWeapon)
	{		
		//Compute weapon position
		const CModAPI_Animation* pWeaponAnimation = GetAnimation(pWeapon->m_WeaponAnimation);
		if(pWeaponAnimation)
		{
			pWeaponAnimation->GetFrame(Time, &pState->m_Weapon);
		}
		
		ApplyTeeAlign(pState->m_Weapon, pState->m_WeaponFlags, pWeapon->m_WeaponAlignment, MotionDir, AimDir, vec2(pWeapon->m_WeaponOffsetX, pWeapon->m_WeaponOffsetY));
		
		//Compute muzzle position
		if(Time <= 0.1666f && pWeapon->m_MuzzleSprite >= 0)
		{
			pState->m_MuzzleEnabled = true;
			
			const CModAPI_Animation* pMuzzleAnimation = GetAnimation(pWeapon->m_MuzzleAnimation);
			if(pMuzzleAnimation)
			{
				pMuzzleAnimation->GetFrame(Time, &pState->m_Muzzle);
			}
			
			ApplyTeeAlign(pState->m_Muzzle, pState->m_MuzzleFlags, pWeapon->m_MuzzleAlignment, MotionDir, AimDir, vec2(pWeapon->m_MuzzleOffsetX, pWeapon->m_MuzzleOffsetY));
		}
	}
}

