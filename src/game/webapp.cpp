/* Webapp class by Sushi and Redix */

#include <engine/storage.h>
#include <engine/shared/config.h>
#include <base/math.h>

#include "http/request.h"
#include "http/response.h"
#include "webapp.h"

IWebapp::IWebapp(IStorage *pStorage) : m_pStorage(pStorage)
{
	char aBuf[512];
	int Port = 80;
	str_copy(aBuf, g_Config.m_WaWebappIp, sizeof(aBuf));

	for(int k = 0; aBuf[k]; k++)
	{
		if(aBuf[k] == ':')
		{
			Port = str_toint(aBuf+k+1);
			aBuf[k] = 0;
			break;
		}
	}

	if(net_host_lookup(aBuf, &m_Addr, NETTYPE_IPV4) != 0)
		net_host_lookup("localhost", &m_Addr, NETTYPE_IPV4);
	m_Addr.port = Port;
}

CRequest *IWebapp::CreateRequest(const char *pURI, int Method, bool Api)
{
	char aURI[256];
	if(Api)
	{
		str_format(aURI, sizeof(aURI), "%s/%s", g_Config.m_WaApiPath, pURI);
		pURI = aURI;
	}
	CRequest *pRequest = new CRequest(g_Config.m_WaWebappIp, pURI, Method);
	RegisterFields(pRequest, Api);
	return pRequest;
}

bool IWebapp::Send(CRequest *pRequest, CResponse *pResponse, int Type, CWebData *pUserData)
{
	CHttpConnection *pCon = new CHttpConnection();
	if(!pCon->Create(m_Addr, Type))
		return false;
	pCon->SetRequest(pRequest);
	pCon->SetResponse(pResponse);
	pCon->SetUserData(pUserData);
	m_Connections.add(pCon);
	return true;
}

bool IWebapp::SendRequest(CRequest *pRequest, int Type, CWebData *pUserData)
{
	return Send(pRequest, new CResponse(), Type, pUserData);
}

bool IWebapp::Download(const char *pFilename, const char *pURI, int Type, class CWebData *pUserData)
{
	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return false;
	CRequest *pRequest = CreateRequest(pURI, CRequest::HTTP_GET, false);
	CResponse *pResponse = new CResponse();
	pResponse->SetFile(File, pFilename);
	return Send(pRequest, pResponse, Type, pUserData);
}

bool IWebapp::Upload(const char *pFilename, const char *pURI, const char *pUploadName, int Type, class CWebData *pUserData, int64 StartTime)
{
	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return false;
	CRequest *pRequest = CreateRequest(pURI, CRequest::HTTP_POST);
	pRequest->SetFile(File, pFilename, pUploadName);
	pRequest->SetStartTime(StartTime);
	return SendRequest(pRequest, Type, pUserData);
}

void IWebapp::Update()
{
	int Max = 3;
	for(int i = 0; i < min(m_Connections.size(), Max); i++)
	{
		if(m_Connections[i]->Update() != 0)
		{
			m_Connections[i]->CloseFiles();
			OnResponse(m_Connections[i]);
			
			delete m_Connections[i];
			m_Connections.remove_index_fast(i);
		}
	}
}
