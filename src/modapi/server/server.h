#ifndef MODAPI_SERVER_H
#define MODAPI_SERVER_H

class CModAPI_Server
{
private:
	char m_aModName[128];

public:
	CModAPI_Server(const char* pModName);
	
	const char* GetName() const;
	
	virtual int CreateModFile(class IStorage* pStorage, const char* pFilename) = 0;
	virtual int CreateEditorFile(class IStorage* pStorage, const char* pFilename) = 0;
};

#endif
