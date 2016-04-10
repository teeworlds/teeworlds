#ifndef MODAPI_CLIENT_ASSETS_ANIMATION_H
#define MODAPI_CLIENT_ASSETS_ANIMATION_H

#include <modapi/client/assets.h>
	
class CModAPI_Asset_Animation : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_ANIMATION;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		int m_CycleType;
		int m_NumKeyFrame;
		int m_KeyFrameData;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);
	
public:
	struct CFrame
	{
		vec2 m_Pos;
		vec2 m_Size;
		float m_Angle;
		vec4 m_Color;
		int m_ListId;
		
		CFrame() :
			m_Pos(vec2(0.0f, 0.0f)),
			m_Size(vec2(1.0f, 1.0f)),
			m_Angle(0.0f),
			m_Color(vec4(1.0f, 1.0f, 1.0f, 1.0f)),
			m_ListId(0)
		{
			
		}
		
		inline CFrame& Position(vec2 v)
		{
			m_Pos = v;
			return *this;
		}
		inline CFrame& Size(vec2 v)
		{
			m_Size = v;
			return *this;
		}
		inline CFrame& Angle(float v)
		{
			m_Angle = v;
			return *this;
		}
		inline CFrame& Color(vec4 v)
		{
			m_Color = v;
			return *this;
		}
		inline CFrame& Opacity(float v)
		{
			m_Color.a = v;
			return *this;
		}
		inline CFrame& ListId(int v)
		{
			m_ListId = v;
			return *this;
		}
	};
	
	struct CKeyFrame : public CFrame
	{
		float m_Time;
	};
	
public:	
	array<CKeyFrame> m_lKeyFrames;
	int m_CycleType; //MODAPI_ANIMCYCLETYPE_XXXXX

public:	
	CModAPI_Asset_Animation() :
		m_CycleType(MODAPI_ANIMCYCLETYPE_CLAMP)
	{
		
	}
	
	int GetKeyFrameId(float Time) const;
	void AddKeyFrame(const CKeyFrame& Frame);
	CKeyFrame& AddKeyFrame(float Time);
	void MoveUpKeyFrame(int Id);
	void MoveDownKeyFrame(int Id);
	void DuplicateKeyFrame(int Id);
	void DeleteKeyFrame(int Id);
	void GetFrame(float Time, CFrame* pFrame) const;
	float GetDuration() const;
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path) { }
	inline int AddSubItem(int SubItemType) { }
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
