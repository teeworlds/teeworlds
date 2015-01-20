/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_MASTERSERVER_H
#define ENGINE_MASTERSERVER_H

#include "kernel.h"

class IMasterServer : public IInterface
{
	MACRO_INTERFACE("masterserver", 0)
public:

	enum
	{
		MAX_MASTERSERVERS=4
	};

	virtual void Init() = 0;
	virtual void SetDefault() = 0;
	virtual int Load() = 0;
	virtual int Save() = 0;

	virtual int RefreshAddresses(int Nettype) = 0;
	virtual void Update() = 0;
	virtual bool IsRefreshing() const = 0;
	virtual NETADDR GetAddr(int Index) const= 0;
	virtual const char *GetName(int Index) const = 0;
	virtual bool IsValid(int Index) const = 0;
};

class IEngineMasterServer : public IMasterServer
{
	MACRO_INTERFACE("enginemasterserver", 0)
public:
};

extern IEngineMasterServer *CreateEngineMasterServer();

#endif



