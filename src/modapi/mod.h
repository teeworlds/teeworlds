#ifndef MODAPI_MOD_H
#define MODAPI_MOD_H

#include <base/system.h>

class IStorage;

struct CModAPI_ModItem_Image
{
	int m_Width;
	int m_Height;
	int m_ImageData;
};

class CModAPI_ModCreator
{	
public:
	CModAPI_ModCreator();
	
	int AddImage(const char* pFileName);
	
	int Save(class IStorage *pStorage, const char *pFileName);
};

#endif
