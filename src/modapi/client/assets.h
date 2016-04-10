#ifndef MODAPI_CLIENT_ASSETS_H
#define MODAPI_CLIENT_ASSETS_H

#include <base/tl/array.h>
#include <engine/graphics.h>

#include <modapi/graphics.h>
#include <modapi/shared/assetsfile.h>
#include <modapi/client/assetpath.h>

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

public:
	enum
	{
		NAME = 0, //String
		NUM_MEMBERS,
	};
	
	template<typename T>
	T GetValue(int ValueType, int Path, T DefaultValue)
	{
		return DefaultValue;
	}
	
	template<typename T>
	bool SetValue(int ValueType, int Path, T Value)
	{
		return false;
	}
};

#endif
