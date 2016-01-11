#ifndef MODAPI_SERVER_EDITORCREATOR_H
#define MODAPI_SERVER_EDITORCREATOR_H

#include <base/tl/array.h>
#include <base/vmath.h>
#include <modapi/editoritem.h>

class IStorage;

class CModAPI_EditorRessourcesCreator
{
public:
	char m_aName[128];
		
	class CEntityPointTypeCreator : public CModAPI_EditorItem_EntityPointType
	{
	public:
		char m_aNameData[128];
	};
	
private:
	array<void*> m_ImagesData;
	array<CModAPI_EditorItem_Image> m_Images;
	array<CEntityPointTypeCreator> m_EntityPoints;
	
public:
	CModAPI_EditorRessourcesCreator(const char* pName);
	
	int AddImage(IStorage* pStorage, const char* Filename);
	CEntityPointTypeCreator& AddEntityPointType(int ID, const char* pName);
	
	int Save(class IStorage *pStorage, const char *pFileName);
};

#endif
