// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include "kernel.h"

class IConfig : public IInterface
{
	MACRO_INTERFACE("config", 0)
public:
	typedef void (*SAVECALLBACKFUNC)(IConfig *pConfig, void *pUserData);

	virtual void Init() = 0;
	virtual void Reset() = 0;
	virtual void RestoreStrings() = 0;
	virtual void Save() = 0;
	
	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData) = 0;
	
	virtual void WriteLine(const char *pLine) = 0;
};

extern IConfig *CreateConfig();

#endif
