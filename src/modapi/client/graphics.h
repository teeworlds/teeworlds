#ifndef MODAPI_CLIENT_GRAPHICS_H
#define MODAPI_CLIENT_GRAPHICS_H

#include <base/tl/array.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#include <modapi/graphics.h>
#include <modapi/client/assetsmanager.h>

class IModAPI_AssetsFile;
class CRenderTools;

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

class CModAPI_SkeletonState
{
	class CBoneState
	{
		
	};
};

class CModAPI_Client_Graphics
{
private:
	IGraphics* m_pGraphics;
	CModAPI_AssetManager* m_pAssetManager;
	
	void ApplyTeeAlign(CModAPI_Asset_Animation::CFrame& pFrame, int& Flags, int Align, vec2 Dir, vec2 Aim, vec2 Offset);
	
	void DrawLineElement_TilingNone(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time);
	void DrawLineElement_TilingStretch(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time);
	void DrawLineElement_TilingRepeated(const CModAPI_Asset_LineStyle::CElement* pElement, vec2 StartPoint, vec2 EndPoint, float Size, float Time);

public:
	CModAPI_Client_Graphics(IGraphics* pGraphics, CModAPI_AssetManager* pAssetManager);
	void Init();
	
	IGraphics *Graphics() { return m_pGraphics; };
	CModAPI_AssetManager *AssetManager() { return m_pAssetManager; };
	
	bool TextureSet(CModAPI_AssetPath AssetPath);
	
	void DrawSprite(CModAPI_AssetPath AssetPath, vec2 Pos, vec2 Size, float Angle, int FlipFlag, vec4 Color);
	void DrawSprite(CModAPI_AssetPath AssetPath, vec2 Pos, float Size, float Angle, int FlipFlag, vec4 Color);
	void DrawSprite(CModAPI_AssetPath AssetPath, vec2 Pos, float Size, float Angle, int FlipFlag);
	void DrawAnimatedSprite(CModAPI_AssetPath SpritePath, vec2 Pos, float Size, float Angle, int FlipFlag, CModAPI_AssetPath AnimationPath, float Time, vec2 Offset);
	void DrawLine(CModAPI_AssetPath LineStylePath, vec2 StartPoint, vec2 EndPoint, float Size, float Time);
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
};

#endif
