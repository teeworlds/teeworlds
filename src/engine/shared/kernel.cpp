/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include "kernel.h"

CKernel::CKernel()
{
	m_NumInterfaces = 0;
}

CKernel::CInterfaceInfo *CKernel::FindInterfaceInfo(const char *pName)
{
	for(int i = 0; i < m_NumInterfaces; i++)
	{
		if(str_comp(pName, m_aInterfaces[i].m_aName) == 0)
			return &m_aInterfaces[i];
	}
	return 0;
}

bool CKernel::RegisterInterfaceImpl(const char *pName, IInterface *pInterface)
{
	// TODO: More error checks here
	if(!pInterface)
	{
		dbg_msg("kernel", "ERROR: couldn't register interface %s. null pointer given", pName);
		return false;
	}

	if(m_NumInterfaces == MAX_INTERFACES)
	{
		dbg_msg("kernel", "ERROR: couldn't register interface '%s'. maximum of interfaces reached", pName);
		return false;
	}

	if(FindInterfaceInfo(pName) != 0)
	{
		dbg_msg("kernel", "ERROR: couldn't register interface '%s'. interface already exists", pName);
		return false;
	}

	pInterface->m_pKernel = this;
	m_aInterfaces[m_NumInterfaces].m_pInterface = pInterface;
	str_copy(m_aInterfaces[m_NumInterfaces].m_aName, pName, sizeof(m_aInterfaces[m_NumInterfaces].m_aName));
	m_NumInterfaces++;

	return true;
}

bool CKernel::ReregisterInterfaceImpl(const char *pName, IInterface *pInterface)
{
	if(FindInterfaceInfo(pName) == 0)
	{
		dbg_msg("kernel", "ERROR: couldn't reregister interface '%s'. interface doesn't exist");
		return false;
	}

	pInterface->m_pKernel = this;

	return true;
}

IInterface *CKernel::RequestInterfaceImpl(const char *pName)
{
	CInterfaceInfo *pInfo = FindInterfaceInfo(pName);
	if(!pInfo)
	{
		dbg_msg("kernel", "failed to find interface with the name '%s'", pName);
		return 0;
	}
	return pInfo->m_pInterface;
}

IKernel *IKernel::Create() { return new CKernel; }
