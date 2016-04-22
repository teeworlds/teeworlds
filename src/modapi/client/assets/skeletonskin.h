#ifndef MODAPI_CLIENT_ASSETS_SKELETONSKIN_H
#define MODAPI_CLIENT_ASSETS_SKELETONSKIN_H

#include <modapi/client/assets.h>
#include <modapi/client/assets/skeleton.h>

class CModAPI_Asset_SkeletonSkin : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_SKELETONSKIN;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		struct CSprite
		{
			int m_SpritePath;
			int m_BonePath;
			int m_LayerPath;
			float m_TranslationX;
			float m_TranslationY;
			float m_ScaleX;
			float m_ScaleY;
			float m_Angle;
			float m_Anchor;
			int m_Color;
			int m_Alignment;
		};
		
		int m_SkeletonPath;
		
		int m_NumSprites;
		int m_SpritesData;
	};
	
	void InitFromAssetsFile(class CModAPI_AssetManager* pAssetManager, class IModAPI_AssetsFile* pAssetsFile, const CStorageType* pItem);
	void SaveInAssetsFile(class CDataFileWriter* pFileWriter, int Position);
	void Unload(class CModAPI_AssetManager* pAssetManager);

public:
	class CSubPath : public CModAPI_GenericPath<2, 0, 0>
	{
	public:
		enum
		{
			TYPE_SPRITE=0,
			NUM_TYPES,
		};

	public:
		CSubPath() : CModAPI_GenericPath() { }
		CSubPath(int PathInt) : CModAPI_GenericPath(PathInt) { }
		CSubPath(int Type, int Id) : CModAPI_GenericPath(Type, 0, 0x0, Id) { }
		
		static inline CSubPath Null() { return CSubPath(CModAPI_GenericPath::UNDEFINED); }
		static inline CSubPath Sprite(int Id) { return CSubPath(TYPE_SPRITE, Id); }
	};
	
	enum
	{
		ALIGNEMENT_WORLD=0,
		ALIGNEMENT_BONE,
	};

	class CSprite
	{
	public:
		CModAPI_AssetPath m_SpritePath;
		CModAPI_Asset_Skeleton::CBonePath m_BonePath;
		CModAPI_Asset_Skeleton::CBonePath m_LayerPath;
		vec2 m_Translation;
		vec2 m_Scale;
		float m_Angle;
		vec4 m_Color;
		float m_Anchor;
		int m_Alignment;
		
	public:
		CSprite() :
			m_BonePath(CModAPI_Asset_Skeleton::CBonePath::Null()),
			m_Translation(vec2(0.0f, 0.0f)),
			m_Scale(vec2(64.0f, 64.0f)),
			m_Angle(0.0f),
			m_Color(1.0f, 1.0f, 1.0f, 1.0f),
			m_Anchor(1.0f),
			m_Alignment(ALIGNEMENT_BONE)
		{
			
		}
		
		CSprite(CModAPI_AssetPath Path) :
			m_SpritePath(Path),
			m_BonePath(CModAPI_Asset_Skeleton::CBonePath::Null()),
			m_Translation(vec2(0.0f, 0.0f)),
			m_Scale(vec2(64.0f, 64.0f)),
			m_Angle(0.0f),
			m_Color(1.0f, 1.0f, 1.0f, 1.0f),
			m_Anchor(1.0f),
			m_Alignment(ALIGNEMENT_BONE)
		{
			
		}
		
		inline CSprite& Bone(CModAPI_Asset_Skeleton::CBonePath v)
		{
			m_BonePath = v;
			return *this;
		}
		
		inline CSprite& Layer(CModAPI_Asset_Skeleton::CBonePath v)
		{
			m_LayerPath = v;
			return *this;
		}
		
		inline CSprite& Translation(vec2 v)
		{
			m_Translation = v;
			return *this;
		}
		
		inline CSprite& Scale(vec2 v)
		{
			m_Scale = v;
			return *this;
		}
		
		inline CSprite& Angle(float v)
		{
			m_Angle = v;
			return *this;
		}
		
		inline CSprite& Anchor(float v)
		{
			m_Anchor = v;
			return *this;
		}
		
		inline CSprite& Color(vec4 v)
		{
			m_Color = v;
			return *this;
		}
		
		inline CSprite& Alignment(int v)
		{
			m_Alignment = v;
			return *this;
		}
	};

public:
	CModAPI_AssetPath m_SkeletonPath;
	array<CSprite> m_Sprites;

public:
	CModAPI_Asset_SkeletonSkin()
	{
		
	}
	
	void AddSprite(const CSprite& Sprite);
	CSprite& AddSprite(CModAPI_AssetPath SkeletonPath, CModAPI_Asset_Skeleton::CBonePath BonePath, CModAPI_Asset_Skeleton::CBonePath LayerPath);

	//Getter/Setter for AssetsEdtiro
public:
	enum
	{
		SUBITEM_SPRITE=0
	};

	enum
	{
		SKELETONPATH = CModAPI_Asset::NUM_MEMBERS, //AssetPath
		SPRITE_PATH, //AssetPath
		SPRITE_BONE, //Int
		SPRITE_LAYER, //Int
		SPRITE_TRANSLATION_X, //Float
		SPRITE_TRANSLATION_Y, //Float
		SPRITE_SCALE_X, //Float
		SPRITE_SCALE_Y, //Float
		SPRITE_ANGLE, //Float
		SPRITE_ANCHOR, //Float
		SPRITE_ALIGNMENT, //Int
	};
	
	inline int GetSpritePath(int i)
	{
		return i;
	}
	
	template<typename T>
	T GetValue(int ValueType, int Path, T DefaultValue)
	{
		return CModAPI_Asset::GetValue<T>(ValueType, Path, DefaultValue);
	}
	
	template<typename T>
	bool SetValue(int ValueType, int Path, T Value)
	{
		return CModAPI_Asset::SetValue<T>(ValueType, Path, Value);
	}
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path)
	{
		m_SkeletonPath.OnIdDeleted(Path);
		for(int i=0; i<m_Sprites.size(); i++)
		{
			m_Sprites[i].m_SpritePath.OnIdDeleted(Path);
		}
	}
	
	inline int AddSubItem(int SubItemType)
	{
		switch(SubItemType)
		{
			case CSubPath::TYPE_SPRITE:
				m_Sprites.add(CSprite());
				return CSubPath::Sprite(m_Sprites.size()-1).ConvertToInteger();
		}
	}
	
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath)
	{
		if(Path.GetType() == CModAPI_AssetPath::TYPE_SKELETON)
		{
			if(!(Path == m_SkeletonPath))
				return;
			
			CModAPI_Asset_Skeleton::CSubPath SubPath(SubItemPath);
			
			if(SubPath.GetType() == CModAPI_Asset_Skeleton::SUBITEM_BONE)
			{
				for(int i=0; i<m_Sprites.size(); i++)
				{
					if(m_Sprites[i].m_BonePath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_LOCAL)
					{
						int CurrentId = m_Sprites[i].m_BonePath.GetId();
						if(CurrentId >=  SubPath.GetId())
							m_Sprites[i].m_BonePath.SetId(CurrentId-1);
					}
				}
			}
			else if(SubPath.GetType() == CModAPI_Asset_Skeleton::SUBITEM_LAYER)
			{
				for(int i=0; i<m_Sprites.size(); i++)
				{
					if(m_Sprites[i].m_LayerPath.GetSource() == CModAPI_Asset_Skeleton::CBonePath::SRC_LOCAL)
					{
						int CurrentId = m_Sprites[i].m_LayerPath.GetId();
						if(CurrentId >=  SubPath.GetId())
							m_Sprites[i].m_BonePath.SetId(CurrentId-1);
					}
				}
			}
		}
	}
};

#endif
