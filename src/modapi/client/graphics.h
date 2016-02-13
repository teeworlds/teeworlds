#ifndef MODAPI_CLIENT_GRAPHICS_H
#define MODAPI_CLIENT_GRAPHICS_H

#include <base/tl/array.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <modapi/graphics.h>
#include <modapi/client/assets.h>
#include <modapi/client/assetscatalog.h>
#include <modapi/client/assets/image.h>
#include <modapi/client/assets/sprite.h>
#include <modapi/client/assets/animation.h>
#include <modapi/client/assets/teeanimation.h>
#include <modapi/client/assets/attach.h>

class IModAPI_AssetsFile;
class CRenderTools;

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

struct CModAPI_TeeAnimationState
{
	vec2 m_MotionDir;
	vec2 m_AimDir;
	
	CModAPI_Asset_Animation::CFrame m_Body;
	CModAPI_Asset_Animation::CFrame m_BackFoot;
	CModAPI_Asset_Animation::CFrame m_FrontFoot;
	
	bool m_BackHandEnabled;
	CModAPI_Asset_Animation::CFrame m_BackHand;
	int m_BackHandFlags;
	
	bool m_FrontHandEnabled;
	CModAPI_Asset_Animation::CFrame m_FrontHand;
	int m_FrontHandFlags;
};

struct CModAPI_AttachAnimationState
{
	array<CModAPI_Asset_Animation::CFrame> m_Frames;
	array<int> m_Flags;
};

class CModAPI_Client_Graphics
{
private:
	IGraphics* m_pGraphics;
	
	void ApplyTeeAlign(CModAPI_Asset_Animation::CFrame& pFrame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset);
	
public:
	CModAPI_LineStyle m_InternalLineStyles[MODAPI_NUM_LINESTYLES];
	
	array<CModAPI_LineStyle> m_ExternalLineStyles;
	
	CModAPI_AssetCatalog<CModAPI_Asset_Image> m_ImagesCatalog;
	CModAPI_AssetCatalog<CModAPI_Asset_Sprite> m_SpritesCatalog;
	CModAPI_AssetCatalog<CModAPI_Asset_Animation> m_AnimationsCatalog;
	CModAPI_AssetCatalog<CModAPI_Asset_TeeAnimation> m_TeeAnimationsCatalog;
	CModAPI_AssetCatalog<CModAPI_Asset_Attach> m_AttachesCatalog;
	CModAPI_AssetCatalog<CModAPI_LineStyle> m_LineStylesCatalog;

public:
	CModAPI_Client_Graphics(IGraphics* pGraphics);
	void Init();
	
	IGraphics *Graphics() { return m_pGraphics; };
	
	CModAPI_LineStyle* GetLineStyle(int Id);
	
	CModAPI_AssetPath AddImage(class IStorage* pStorage, int StorageType, const char* pFilename);
	void DeleteAsset(int Type, CModAPI_AssetPath Path);
	
	int SaveInAssetsFile(class IStorage *pStorage, const char *pFileName);
	int OnAssetsFileLoaded(IModAPI_AssetsFile* pAssetsFile);
	int OnAssetsFileUnloaded();
	
	bool TextureSet(CModAPI_AssetPath AssetPath);
	
	void DrawSprite(CModAPI_AssetPath AssetPath, vec2 Pos, float Size, float Angle, int FlipFlag, float Opacity = 1.0f);
	void DrawAnimatedSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int FlipFlag, CModAPI_AssetPath AnimationPath, float Time, vec2 Offset);
	void DrawLine(CRenderTools* pRenderTools, int LineStyleID, vec2 StartPoint, vec2 EndPoint, float Ms);
	void DrawText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment);
	void DrawAnimatedText(ITextRender* pTextRender, const char *pText, vec2 Pos, vec4 Color, float Size, int Alignment, CModAPI_AssetPath AnimationPath, float Time, vec2 Offset);
	
	void DrawAttach(CRenderTools* pRenderTools, const CModAPI_AttachAnimationState* pState, CModAPI_AssetPath AttachPath, vec2 Pos, float Scaling);
	void DrawTee(CRenderTools* pRenderTools, class CTeeRenderInfo* pInfo, const CModAPI_TeeAnimationState* pState, vec2 Pos, vec2 Dir, int Emote);
	
	void GetTeeAlignAxis(int Align, vec2 Dir, vec2 Aim, vec2& PartDirX, vec2& PartDirY);
	
	//Tee Animation
	void InitTeeAnimationState(CModAPI_TeeAnimationState* pState, vec2 MotionDir, vec2 AimDir);
	void AddTeeAnimationState(CModAPI_TeeAnimationState* pState, CModAPI_AssetPath TeeAnimationPath, float Time);
	
	//Attach Animation
	void InitAttachAnimationState(CModAPI_AttachAnimationState* pState, vec2 MotionDir, vec2 AimDir, CModAPI_AssetPath AttachPath, float Time);
	
	template<class ASSETTYPE>
	ASSETTYPE* GetAsset(CModAPI_AssetPath AssetPath)
	{
		return 0;
	}
};

#endif
