/* Teerace class by Sushi and Redix */

#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/http.h>

#include "teerace.h"

const char *ITeerace::Host()
{
	return g_Config.m_WaWebappIp;
}

CRequest *ITeerace::CreateApiRequest(int Method, const char *pURI)
{
	char aURI[256];
	str_format(aURI, sizeof(aURI), "%s%s%s", g_Config.m_WaApiPath, pURI[0] == '/' ? "" : "/", pURI);
	return new CRequest(Method, aURI);
}
