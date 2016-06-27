#ifndef MODAPI_CLIENT_SKELETONRENDERER_H
#define MODAPI_CLIENT_SKELETONRENDERER_H

#include <base/tl/array.h>
#include <modapi/client/assetsmanager.h>

class CModAPI_SkeletonRenderer
{
public:
	enum
	{
		ADDDEFAULTSKIN_NO=0,
		ADDDEFAULTSKIN_ONLYPARENT,
		ADDDEFAULTSKIN_YES,
	};
	
protected:
	class CBoneState
	{
	public:
		int m_ParentSkeleton;
		int m_ParentBone;
		float m_Length;
		float m_Anchor;
		matrix2x2 m_Transform;
		vec2 m_StartPoint;
		vec2 m_EndPoint;
		vec4 m_Color;
		int m_Alignment;
		
		bool m_Finalized;
	};
	
	class CLayerState
	{
	public:
		int m_State;
		vec4 m_Color;
	};
	
	class CSkeletonState
	{
	public:
		CModAPI_AssetPath m_Path;
		int m_Parent;
		array<CBoneState> m_Bones;
		array<CLayerState> m_Layers;
	};
	
	class CSkinState
	{
	public:
		CModAPI_AssetPath m_Path;
		vec4 m_Color;
	};

protected:
	class IGraphics* m_pGraphics;
	CModAPI_AssetManager* m_pAssetManager;
	
	array<CSkeletonState> m_Skeletons;
	array<CSkinState> m_Skins;
	int m_NumLayers;
	
	vec2 m_Aim;
	vec2 m_Motion;
	vec2 m_Hook;
	
public:
	CModAPI_SkeletonRenderer(class IGraphics* pGraphics, CModAPI_AssetManager* pAssetManager) :
		m_pGraphics(pGraphics),
		m_pAssetManager(pAssetManager),
		m_NumLayers(0)
	{
		m_Aim = vec2(1.0f, 0.0f);
		m_Motion = vec2(1.0f, 0.0f);
		m_Hook = vec2(1.0f, 0.0f);
	}
	
	void SetAim(vec2 Aim) { m_Aim = Aim; }
	void SetMotion(vec2 Motion) { m_Motion = Motion; }
	void SetHook(vec2 Hook) { m_Hook = Hook; }
	
	void AddSkeleton(CModAPI_AssetPath SkeletonPath);
	void AddSkeletonWithParents(CModAPI_AssetPath SkeletonPath, int AddDefaultSkins = ADDDEFAULTSKIN_NO);
	void ApplyAnimation(CModAPI_AssetPath SkeletonAnimationPath, float Time);
	void AddSkin(CModAPI_AssetPath SkeletonSkinPath, vec4 Color);
	void AddSkinWithSkeleton(CModAPI_AssetPath SkeletonSkinPath, vec4 Color);
	void FinalizeBone(int s, int b);
	void Finalize();
	void RenderBone(vec2 Position, float Size, CModAPI_AssetPath SkeletonPath, CModAPI_Asset_Skeleton::CSubPath BonePath);
	void RenderBoneOutline(vec2 Position, float Size, CModAPI_AssetPath SkeletonPath, CModAPI_Asset_Skeleton::CSubPath BonePath);
	void RenderBones(vec2 Position, float Size);
	void RenderSkinsLayer(vec2 Position, float Size, int LayerSkeletonId, int LayerId);
	void RenderSkins(vec2 Position, float Size);
	bool BonePicking(vec2 Position, float Size, vec2 Point, CModAPI_AssetPath& SkeletonPath, CModAPI_Asset_Skeleton::CSubPath& BonePath);
	bool GetLocalAxis(vec2 Position, float Size, CModAPI_AssetPath SkeletonPath, CModAPI_Asset_Skeleton::CSubPath BonePath, vec2& Origin, vec2& AxisX, vec2& AxisY);
	bool GetParentAxis(vec2 Position, float Size, CModAPI_AssetPath SkeletonPath, CModAPI_Asset_Skeleton::CSubPath BonePath, vec2& Origin, vec2& AxisX, vec2& AxisY);
	
	class IGraphics* Graphics() { return m_pGraphics; }
	CModAPI_AssetManager* AssetManager() { return m_pAssetManager; }
};

#endif
