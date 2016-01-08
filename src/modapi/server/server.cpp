#include <base/system.h>

#include <modapi/server/server.h>

CModAPI_Server::CModAPI_Server(const char* pModName)
{
	str_copy(m_aModName, pModName, sizeof(m_aModName));
}

const char* CModAPI_Server::GetName() const
{
	return m_aModName;
}
