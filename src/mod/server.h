#ifndef MOD_SERVER_H
#define MOD_SERVER_H

#include <modapi/server/server.h>

class CMod_Server : public CModAPI_Server
{
public:
	CMod_Server();
	
	virtual int CreateModFile(class IStorage* pStorage, const char* pFilename);
	virtual int CreateEditorFile(class IStorage* pStorage, const char* pFilename);
};

#endif
