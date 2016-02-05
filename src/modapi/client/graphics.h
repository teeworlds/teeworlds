#ifndef MODAPI_CLIENT_GRAPHICS_H
#define MODAPI_CLIENT_GRAPHICS_H

#include <base/tl/array.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <modapi/graphics.h>
#include <modapi/shared/animation.h>

class IMod;
class CRenderTools;

class CModAPI_Image : public CImageInfo
{
public:
	IGraphics::CTextureHandle m_Texture;
};

struct CModAPI_Sprite
{
	int m_X;
	int m_Y;
	int m_W;
	int m_H;
	int m_ImageId;
	int m_GridX;
	int m_GridY;
	
	CModAPI_Sprite();
	CModAPI_Sprite(int ImageId, int X, int Y, int W, int H, int GridX, int GridY);
};

struct CModAPI_LineStyle
{
	//Line with quads, like laser
	int m_OuterWidth;
	vec4 m_OuterColor;
	int m_InnerWidth;
	vec4 m_InnerColor;

	//Draw line with sprites, like hook
	int m_LineSpriteType; //MODAPI_LINESTYLE_SPRITETYPE_XXXXX
	int m_LineSprite1;
	int m_LineSprite2;
	int m_LineSpriteSizeX;
	int m_LineSpriteSizeY;
	int m_LineSpriteOverlapping;
	int m_LineSpriteAnimationSpeed;

	//Start point sprite
	int m_StartPointSprite1;
	int m_StartPointSprite2;
	int m_StartPointSpriteX;
	int m_StartPointSpriteY;
	int m_StartPointSpriteSizeX;
	int m_StartPointSpriteSizeY;
	int m_StartPointSpriteAnimationSpeed;
	
	//End point prite
	int m_EndPointSprite1;
	int m_EndPointSprite2;
	int m_EndPointSpriteX;
	int m_EndPointSpriteY;
	int m_EndPointSpriteSizeX;
	int m_EndPointSpriteSizeY;
	int m_EndPointSpriteAnimationSpeed;
	
	//General information
	int m_AnimationType; //MODAPI_LINESTYLE_ANIMATION_XXXXX
	int m_AnimationSpeed;
};

struct CModAPI_TeeAnimation
{
	int m_BodyAnimation;
	int m_BackFootAnimation;
	int m_FrontFootAnimation;
	
	int m_BackHandAlignment;
	int m_BackHandAnimation;
	int m_BackHandOffsetX;
	int m_BackHandOffsetY;
	
	int m_FrontHandAlignment;
	int m_FrontHandAnimation;
	int m_FrontHandOffsetX;
	int m_FrontHandOffsetY;
	
	CModAPI_TeeAnimation();
};

struct CModAPI_TeeAnimationState
{
	vec2 m_MotionDir;
	vec2 m_AimDir;
	
	CModAPI_AnimationFrame m_Body;
	CModAPI_AnimationFrame m_BackFoot;
	CModAPI_AnimationFrame m_FrontFoot;
	
	bool m_BackHandEnabled;
	CModAPI_AnimationFrame m_BackHand;
	int m_BackHandFlags;
	
	bool m_FrontHandEnabled;
	CModAPI_AnimationFrame m_FrontHand;
	int m_FrontHandFlags;
};

struct CModAPI_WeaponAnimationState
{
	CModAPI_AnimationFrame m_Weapon;
	int m_WeaponFlags;
	
	bool m_MuzzleEnabled;
	CModAPI_AnimationFrame m_Muzzle;
	int m_MuzzleFlags;
};

struct CModAPI_Weapon
{
	int m_WeaponSprite;
	int m_WeaponSize;
	int m_WeaponAlignment;
	int m_WeaponOffsetX;
	int m_WeaponOffsetY;
	int m_WeaponAnimation;
	
	int m_NumMuzzles;
	int m_MuzzleSprite;
	int m_MuzzleSize;
	int m_MuzzleAlignment;
	int m_MuzzleOffsetX;
	int m_MuzzleOffsetY;
	int m_MuzzleAnimation;
	
	int m_TeeAnimation;
	
	CModAPI_Weapon();
};

class CModAPI_Client_Graphics
{
private:
	CModAPI_Image m_InternalImages[MODAPI_NUM_IMAGES];
	CModAPI_Animation m_InternalAnimations[MODAPI_NUM_ANIMATIONS];
	CModAPI_TeeAnimation m_InternalTeeAnimations[MODAPI_NUM_TEEANIMATIONS];
	CModAPI_Sprite m_InternalSprites[MODAPI_NUM_SPRITES];
	CModAPI_LineStyle m_InternalLineStyles[MODAPI_NUM_LINESTYLES];
	CModAPI_Weapon m_InternalWeapons[MODAPI_NUM_WEAPONS];
	
	array<CModAPI_Image> m_ExternalImages;
	array<CModAPI_Animation> m_ExternalAnimations;
	array<CModAPI_TeeAnimation> m_ExternalTeeAnimations;
	array<CModAPI_Sprite> m_ExternalSprites;
	array<CModAPI_LineStyle> m_ExternalLineStyles;
	array<CModAPI_Weapon> m_ExternalWeapons;
	
	IGraphics* m_pGraphics;
	
	void ApplyTeeAlign(CModAPI_AnimationFrame& pFrame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset);
	
public:
	CModAPI_Client_Graphics(IGraphics* pGraphics);
	void Init();
	const CModAPI_Image* GetImage(int Id) const;
	const CModAPI_Animation* GetAnimation(int Id) const;
	const CModAPI_TeeAnimation* GetTeeAnimation(int Id) const;
	const CModAPI_Sprite* GetSprite(int Id) const;
	const CModAPI_LineStyle* GetLineStyle(int Id) const;
	const CModAPI_Weapon* GetWeapon(int Id) const;
	
	int OnModLoaded(IMod* pMod);
	int OnModUnloaded();
	
	bool TextureSet(int ImageID);
	
	void DrawSprite(CRenderTools* pRenderTools, int SpriteID, vec2 Pos, float Size, float Angle, int FlipFlag, float Opacity = 1.0f);
	void DrawAnimatedSprite(CRenderTools* pRenderTools, int SpriteID, vec2 Pos, float Size, float Angle, int FlipFlag, int AnimationID, float Time, vec2 Offset);
	void DrawLine(CRenderTools* pRenderTools, int LineStyleID, vec2 StartPoint, vec2 EndPoint, float Ms);
	void DrawText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment);
	void DrawAnimatedText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment, int AnimationID, float Time, vec2 Offset);
	
	void DrawWeaponMuzzle(CRenderTools* pRenderTools, const CModAPI_WeaponAnimationState* pState, int WeaponID, vec2 Pos);
	void DrawWeapon(CRenderTools* pRenderTools, const CModAPI_WeaponAnimationState* pState, int WeaponID, vec2 Pos);
	void DrawTee(CRenderTools* pRenderTools, class CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pState, vec2 Pos, vec2 Dir, int Emote);
	
	//Tee Animation
	void InitTeeAnimationState(CModAPI_TeeAnimationState* pState, vec2 MotionDir, vec2 AimDir);
	void AddTeeAnimationState(CModAPI_TeeAnimationState* pState, int TeeAnimationID, float Time);
	
	//Weapon Animation
	void InitWeaponAnimationState(CModAPI_WeaponAnimationState* pState, vec2 MotionDir, vec2 AimDir, int WeaponID, float Time);
};

#endif
