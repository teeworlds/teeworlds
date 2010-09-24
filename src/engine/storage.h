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
	virtual const char *GetDirectory(int Type) const = 0;
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, char *pBuffer = 0, int BufferSize = 0) = 0;
	virtual bool RemoveFile(const char *pFilename) = 0;
	virtual const char* SavePath(const char *pFilename, char *pBuffer, int Max) = 0;
};

extern IStorage *CreateStorage(const char *pApplicationName, int NumArgs, const char **ppArguments);


#endif
