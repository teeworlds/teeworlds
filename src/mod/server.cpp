#include <mod/server.h>

#include <modapi/mapitem.h>
#include <modapi/server/editorcreator.h>

CMod_Server::CMod_Server() :
	CModAPI_Server("mod") //Name of the mod
{
	
}

int CMod_Server::CreateEditorFile(IStorage* pStorage, const char* pFilename)
{
	CModAPI_EditorRessourcesCreator Creator("mod");
	
	//Add some custom elements here
	
	if(Creator.Save(pStorage, pFilename) < 0) return false;
	else return true;
}
