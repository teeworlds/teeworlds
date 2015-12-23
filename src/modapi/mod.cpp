#include <modapi/mod.h>

#include <engine/storage.h>
#include <engine/shared/datafile.h>

#include <engine/external/pnglite/pnglite.h>

CModAPI_ModCreator::CModAPI_ModCreator()
{
	
}
	
int CModAPI_ModCreator::AddImage(const char* pFileName)
{
	//Load image
	
	return true;
}

int CModAPI_ModCreator::Save(class IStorage *pStorage, const char *pFileName)
{
	//Save images
	CDataFileWriter df;
	if(!df.Open(pStorage, pFileName)) return 0;
	
	df.Finish();
	
	return 1;
}
