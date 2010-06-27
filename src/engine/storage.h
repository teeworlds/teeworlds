// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_STORAGE_H
#define ENGINE_STORAGE_H

#include "kernel.h"

class IStorage : public IInterface
{
	MACRO_INTERFACE("storage", 0)
public:
	enum
	{
		TYPE_SAVE = 1,
		TYPE_CURRENT = 2,
		TYPE_DATA = 4,
		TYPE_ALL = ~0
	};
	
	virtual void ListDirectory(int Types, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser) = 0;
	virtual FILE *OpenFile(const char *pFilename, int Flags, char *pBuffer = 0, int BufferSize = 0) = 0;
};

extern IStorage *CreateStorage(const char *pApplicationName, const char *pArgv0);


#endif
