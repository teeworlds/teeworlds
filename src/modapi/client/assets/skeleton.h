#ifndef MODAPI_CLIENT_ASSETS_SKELETON_H
#define MODAPI_CLIENT_ASSETS_SKELETON_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Skeleton : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_SKELETON;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		struct CBone
		{
			int m_ParentPath;
			float m_Length;
			float m_Anchor;
			float m_TranslationX;
			float m_TranslationY;
			float m_ScaleX;
			float m_ScaleY;
			float m_Angle;
			
			char m_aName[64];
			int m_Color;
		};
		
		struct CLayer
		{
			char m_aName[64];
		};
			
		int m_ParentPath;
		int m_DefaultSkinPath;
		
		int m_NumBones;
		int m_BonesData;
		
		int m_NumLayers;
		int m_LayersData;
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
			TYPE_BONE=0, //Bone from the current skeleton
			TYPE_LAYER,   //Bone from the parent skeleton
			NUM_SOURCES,       //Asset provided by the skin
		};

	public:
		CSubPath() : CModAPI_GenericPath() { }
		CSubPath(int PathInt) : CModAPI_GenericPath(PathInt) { }
		CSubPath(int Type, int Id) : CModAPI_GenericPath(Type, 0, 0x0, Id) { }
		
		static inline CSubPath Null() { return CSubPath(CModAPI_GenericPath::UNDEFINED); }
		static inline CSubPath Bone(int Id) { return CSubPath(TYPE_BONE, Id); }
		static inline CSubPath Layer(int Id) { return CSubPath(TYPE_LAYER, Id); }
	};
	
	class CBonePath : public CModAPI_GenericPath<0, 2, 0>
	{
	public:
		enum
		{
			SRC_LOCAL=0, //Bone from the current skeleton
			SRC_PARENT,   //Bone from the parent skeleton
			NUM_SOURCES,       //Asset provided by the skin
		};

	public:
		CBonePath() : CModAPI_GenericPath() { }
		CBonePath(int PathInt) : CModAPI_GenericPath(PathInt) { }
		CBonePath(int Source, int Id) : CModAPI_GenericPath(0, Source, 0x0, Id) { }
		
		static inline CBonePath Null() { return CBonePath(CModAPI_GenericPath::UNDEFINED); }
		static inline CBonePath Local(int Id) { return CBonePath(SRC_LOCAL, Id); }
		static inline CBonePath Parent(int Id) { return CBonePath(SRC_PARENT, Id); }
	};

	class CBone
	{
	public:
		//Hierarchy
		CModAPI_Asset_Skeleton::CBonePath m_ParentPath;
		
		//Physics
		float m_Length;
		float m_Anchor;
		vec2 m_Translation;
		vec2 m_Scale;
		float m_Angle;
		
		//Gui
		char m_aName[64];
		vec4 m_Color;
		
	public:
		CBone() :
			m_ParentPath(CModAPI_Asset_Skeleton::CBonePath::Null()),
			m_Length(64.0f),
			m_Anchor(1.0f),
			m_Translation(vec2(0.0f, 0.0f)),
			m_Scale(vec2(1.0f, 1.0f)),
			m_Angle(0.0f),
			m_Color(1.0f, 0.0f, 0.0f, 1.0f)
		{
			
		}
		
		inline CBone& Parent(CModAPI_Asset_Skeleton::CBonePath v)
		{
			m_ParentPath = v;
			return *this;
		}
		
		inline CBone& Length(float v)
		{
			m_Length = v;
			return *this;
		}
		
		inline CBone& Anchor(float v)
		{
			m_Anchor = v;
			return *this;
		}
		
		inline CBone& Translation(vec2 v)
		{
			m_Translation = v;
			return *this;
		}
		
		inline CBone& Scale(vec2 v)
		{
			m_Scale = v;
			return *this;
		}
		
		inline CBone& Angle(float v)
		{
			m_Angle = v;
			return *this;
		}
		
		inline CBone& Name(const char* pText)
		{
			str_copy(m_aName, pText, sizeof(m_aName));
			return *this;
		}
		
		inline CBone& Color(vec4 v)
		{
			m_Color = v;
			return *this;
		}
	};
	
	class CLayer
	{
	public:
		char m_aName[64];
		
		inline CLayer& Name(const char* pText)
		{
			str_copy(m_aName, pText, sizeof(m_aName));
			return *this;
		}
	};

public:
	CModAPI_AssetPath m_ParentPath;
	CModAPI_AssetPath m_DefaultSkinPath;
	array<CBone> m_Bones;
	array<CLayer> m_Layers;

public:
	CModAPI_Asset_Skeleton() :
		m_ParentPath(CModAPI_AssetPath::Null())
	{
		
	}
	
	void AddBone(const CBone& Bone);
	CBone& AddBone();
	CLayer& AddLayer();

	//Getter/Setter for AssetsEditor
public:
	enum
	{
		SUBITEM_BONE=0,
		SUBITEM_LAYER,
	};

	enum
	{
		PARENTPATH = CModAPI_Asset::NUM_MEMBERS, //Path
		SKINPATH, //Path
		BONE_PARENT, //Int
		BONE_LENGTH, //Float
		BONE_TRANSLATION_X, //Float
		BONE_TRANSLATION_Y, //Float
		BONE_SCALE_X, //Float
		BONE_SCALE_Y, //Float
		BONE_ANGLE, //Float
		BONE_NAME, //String
		BONE_COLOR, //vec4
		BONE_ANCHOR, //Float
		LAYER_NAME, //String
	};
	
	inline int GetBonePath(int i)
	{
		return CSubPath::Bone(i).ConvertToInteger();
	}
	
	inline int GetLayerPath(int i)
	{
		return CSubPath::Layer(i).ConvertToInteger();
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
		m_ParentPath.OnIdDeleted(Path);
		m_DefaultSkinPath.OnIdDeleted(Path);
	}
	
	inline int AddSubItem(int SubItemType)
	{
		switch(SubItemType)
		{
			case SUBITEM_BONE:
				AddBone();
				return CSubPath::Bone(m_Bones.size()-1).ConvertToInteger();
			case SUBITEM_LAYER:
				AddLayer();
				return CSubPath::Layer(m_Layers.size()-1).ConvertToInteger();
		}
	}
	
	inline bool DeleteSubItem(CSubPath SubItemPath)
	{
		switch(SubItemPath.GetType())
		{
			case SUBITEM_BONE:
				m_Bones.remove_index(SubItemPath.GetId());
				for(int i=0; i<m_Bones.size(); i++)
				{
					if(m_Bones[i].m_ParentPath.GetSource() == CBonePath::SRC_LOCAL)
					{
						int CurrentId = m_Bones[i].m_ParentPath.GetId();
						int DeletedId = SubItemPath.GetId();
						if(CurrentId == DeletedId)
							m_Bones[i].m_ParentPath = CBonePath::Null();
						else if(CurrentId > DeletedId)
							m_Bones[i].m_ParentPath.SetId(CurrentId-1);
					}
				}
				return true;
			case SUBITEM_LAYER:
				m_Layers.remove_index(SubItemPath.GetId());
				return true;
		}
		
		return false;
	}
	
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath)
	{
		if(Path.GetType() == CModAPI_AssetPath::TYPE_SKELETON)
		{
			if(!(Path == m_ParentPath))
				return;
			
			CSubPath SubPath(SubItemPath);
			if(SubPath.GetType() != SUBITEM_BONE)
				return;
			
			for(int i=0; i<m_Bones.size(); i++)
			{
				if(m_Bones[i].m_ParentPath.GetSource() == CBonePath::SRC_PARENT)
				{
					int CurrentId = m_Bones[i].m_ParentPath.GetId();
					int DeletedId = SubPath.GetId();
					if(CurrentId == DeletedId)
						m_Bones[i].m_ParentPath = CBonePath::Null();
					else if(CurrentId > DeletedId)
						m_Bones[i].m_ParentPath.SetId(CurrentId-1);
				}
			}
		}
	}
};

#endif
