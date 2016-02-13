#ifndef MODAPI_CLIENT_ASSETS_H
#define MODAPI_CLIENT_ASSETS_H

#include <base/tl/array.h>
#include <engine/graphics.h>

#include <modapi/graphics.h>
#include <modapi/shared/assetsfile.h>

enum
{
	MODAPI_ASSETTYPE_IMAGE = 0,
	MODAPI_ASSETTYPE_IMAGE_LIST,
	MODAPI_ASSETTYPE_SPRITE,
	MODAPI_ASSETTYPE_SPRITE_LIST,
	MODAPI_ASSETTYPE_ANIMATION,
	MODAPI_ASSETTYPE_ANIMATION_LIST,
	MODAPI_ASSETTYPE_TEEANIMATION,
	MODAPI_ASSETTYPE_TEEANIMATION_LIST,
	MODAPI_ASSETTYPE_ATTACH,
	MODAPI_ASSETTYPE_ATTACH_LIST,
	MODAPI_NUM_ASSETTYPES,
};

class CModAPI_Asset
{
public:
	struct CStorageType
	{
		int m_Name;
	};
	
public:
	char m_aName[128];

public:
	inline void SetName(const char* pName)
	{
		str_copy(m_aName, pName, sizeof(m_aName));
	}
};

class CModAPI_AssetPath
{
public:
	enum
	{
		FLAG_EXTERNAL=0x1, //Asset provided by the server
		FLAG_LIST=0x2, //List of assets
		
		SHIFT=2,
	};
	
public:
	int m_Path;

public:
	CModAPI_AssetPath() :
		m_Path(-1)
	{
		
	}
	
	CModAPI_AssetPath(int PathInt) :
		m_Path(PathInt)
	{
		
	}
	
	CModAPI_AssetPath(int Id, int Flags) :
		m_Path((Id << SHIFT) + Flags)
	{
		
	}
	
	inline bool IsList() const
	{
		return (m_Path & FLAG_LIST);
	}
	
	inline bool IsNull() const
	{
		return (m_Path == -1);
	}
	
	inline bool IsInternal() const
	{
		return !(m_Path & FLAG_EXTERNAL);
	}
	
	inline bool IsExternal() const
	{
		return (m_Path & FLAG_EXTERNAL);
	}
	
	inline int GetId() const
	{
		return (m_Path >> SHIFT);
	}
	
	inline int ToAssetsFile() const
	{
		return m_Path;
	}
	
	inline bool operator==(const CModAPI_AssetPath& path)
	{
		return path.m_Path == m_Path;
	}
	
	static inline CModAPI_AssetPath Internal(int Id)
	{
		return CModAPI_AssetPath(Id, 0x0);
	}
	
	static inline CModAPI_AssetPath External(int Id)
	{
		return CModAPI_AssetPath(Id, FLAG_EXTERNAL);
	}
	
	static inline CModAPI_AssetPath InternalList(int Id)
	{
		return CModAPI_AssetPath(Id, FLAG_LIST);
	}
	
	static inline CModAPI_AssetPath ExternalList(int Id)
	{
		return CModAPI_AssetPath(Id, FLAG_EXTERNAL | FLAG_LIST);
	}
	
	static inline CModAPI_AssetPath Null()
	{
		return CModAPI_AssetPath(-1);
	}
	
	static inline CModAPI_AssetPath FromAssetsFile(int PathInt)
	{
		return CModAPI_AssetPath(PathInt);
	}
};

#endif
