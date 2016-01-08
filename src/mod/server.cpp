#include <mod/server.h>

#include <modapi/server/modcreator.h>

CMod_Server::CMod_Server() :
	CModAPI_Server("mod") //Name of the mod
{
	
}

int CMod_Server::CreateModFile(IStorage* pStorage, const char* pFilename)
{
	CModAPI_ModCreator ModCreator;
	
	//Add some custom elements here
	
	if(ModCreator.Save(pStorage, pFilename) < 0) return false;
	else return true;
}
