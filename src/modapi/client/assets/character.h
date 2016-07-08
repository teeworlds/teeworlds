#ifndef MODAPI_CLIENT_ASSETS_CHARACTER_H
#define MODAPI_CLIENT_ASSETS_CHARACTER_H

#include <modapi/client/assets.h>

class CModAPI_Asset_Character : public CModAPI_Asset
{
public:
	static const int TypeId = CModAPI_AssetPath::TYPE_CHARACTER;

	struct CStorageType : public CModAPI_Asset::CStorageType
	{
		struct CPart
		{
			char m_aName[128];
		};
		
		int m_NumParts;
		int m_PartsData;
		
		int m_IdlePath;
		int m_WalkPath;
		int m_ControlledJumpPath;
		int m_UncontrolledJumpPath;
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
			TYPE_PART=0,
			NUM_TYPES,
		};

	public:
		CSubPath() : CModAPI_GenericPath() { }
		CSubPath(int PathInt) : CModAPI_GenericPath(PathInt) { }
		CSubPath(int Type, int Id) : CModAPI_GenericPath(Type, 0, 0x0, Id) { }
		
		static inline CSubPath Null() { return CSubPath(CModAPI_GenericPath::UNDEFINED); }
		static inline CSubPath Part(int Id) { return CSubPath(TYPE_PART, Id); }
	};
	
	class CPart
	{
	public:
		char m_aName[128];
	
	public:
		inline CPart& Name(const char* pText)
		{
			str_copy(m_aName, pText, sizeof(m_aName));
			return *this;
		}
	};
	
	array<CPart> m_Parts;
	
	CModAPI_AssetPath m_IdlePath;
	CModAPI_AssetPath m_WalkPath;
	CModAPI_AssetPath m_ControlledJumpPath;
	CModAPI_AssetPath m_UncontrolledJumpPath;

public:
	CModAPI_Asset_Character()
	{
		
	}

	CPart& AddPart();
	
	//Getter/Setter for AssetsEdtiro
public:
	enum
	{
		PART_NAME = CModAPI_Asset::NUM_MEMBERS, //Int
		IDLEPATH, //Path
		WALKPATH, //Path
		CONTROLLEDJUMPPATH, //Path
		UNCONTROLLEDJUMPPATH, //Path
	};
	
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
	
	inline void OnAssetDeleted(const CModAPI_AssetPath& Path) { }
	
	inline int AddSubItem(int SubItemType)
	{
		switch(SubItemType)
		{
			case CSubPath::TYPE_PART:
				AddPart();
				return CSubPath::Part(m_Parts.size()-1).ConvertToInteger();
		}
	}
	
	inline bool DeleteSubItem(int SubItemPath) { return false; }
	
	inline void OnSubItemDeleted(const CModAPI_AssetPath& Path, int SubItemPath) { }
};

#endif
