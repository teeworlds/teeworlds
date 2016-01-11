#include <base/system.h>

#include "http.h"

IHttpBase::IHttpBase() : m_Finalized(false), m_FieldNum(0) { }

IHttpBase::~IHttpBase() { }

void IHttpBase::AddField(CHttpField Field)
{
	if(m_Finalized || m_FieldNum >= HTTP_MAX_HEADER_FIELDS)
		return;
	m_aFields[m_FieldNum] = Field;
	m_FieldNum++;
}

void IHttpBase::AddField(const char *pKey, const char *pValue)
{
	if(m_Finalized || m_FieldNum >= HTTP_MAX_HEADER_FIELDS)
		return;
	str_copy(m_aFields[m_FieldNum].m_aKey, pKey, sizeof(m_aFields[m_FieldNum].m_aKey));
	str_copy(m_aFields[m_FieldNum].m_aValue, pValue, sizeof(m_aFields[m_FieldNum].m_aValue));
	m_FieldNum++;
}

void IHttpBase::AddField(const char *pKey, int Value)
{
	if(m_Finalized || m_FieldNum >= HTTP_MAX_HEADER_FIELDS)
		return;
	str_copy(m_aFields[m_FieldNum].m_aKey, pKey, sizeof(m_aFields[m_FieldNum].m_aKey));
	str_format(m_aFields[m_FieldNum].m_aValue, sizeof(m_aFields[m_FieldNum].m_aValue), "%d", Value);
	m_FieldNum++;
}

const char *IHttpBase::GetField(const char *pKey) const
{
	for(int i = 0; i < m_FieldNum; i++)
	{
		if(str_comp_nocase(m_aFields[i].m_aKey, pKey) == 0)
			return m_aFields[i].m_aValue;
	}
	return 0;
}

CRequestInfo::CRequestInfo(const char *pAddr) : m_pRequest(0), m_pResponse(new CBufferResponse()), m_pfnCallback(0), m_pUserData(0)
{
	str_copy(m_aAddr, pAddr, sizeof(m_aAddr));
}

CRequestInfo::CRequestInfo(const char *pAddr, IOHANDLE File) : m_pRequest(0), m_pResponse(new CFileResponse(File)), m_pfnCallback(0), m_pUserData(0)
{
	str_copy(m_aAddr, pAddr, sizeof(m_aAddr));
}

CRequestInfo::~CRequestInfo()
{
	if(m_pRequest)
		delete m_pRequest;
	if(m_pResponse)
		delete m_pResponse;
}

void CRequestInfo::SetCallback(FHttpCallback pfnCallback, void *pUserData)
{
	m_pfnCallback = pfnCallback;
	m_pUserData = pUserData;
}

void CRequestInfo::ExecuteCallback(IResponse *pResponse, bool Error)
{
	if(m_pfnCallback)
		m_pfnCallback(pResponse, Error, m_pUserData);
}

CHttpClient::CHttpClient()
{
	for (int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
		m_aConnections[i].SetID(i);
}

CHttpClient::~CHttpClient() { }

void CHttpClient::Send(CRequestInfo *pInfo, CRequest *pRequest)
{
	pInfo->m_pRequest = pRequest;
	m_pEngine->HostLookup(&pInfo->m_Lookup, pInfo->m_aAddr, NETTYPE_IPV4);

	for(int k = 0; pInfo->m_aAddr[k]; k++)
	{
		if(pInfo->m_aAddr[k] == ':')
		{
			pInfo->m_aAddr[k] = 0;
			break;
		}
	}

	pRequest->AddField("Host", pInfo->m_aAddr);
	m_lPendingRequests.add(pInfo);
}

CHttpConnection *CHttpClient::GetConnection(NETADDR Addr)
{
	for(int j = 0; j < HTTP_MAX_CONNECTIONS; j++)
	{
		CHttpConnection *pConn = &m_aConnections[j];
		if(pConn->State() == CHttpConnection::STATE_WAITING && pConn->CompareAddr(Addr))
			return pConn;
	}

	for (int j = 0; j < HTTP_MAX_CONNECTIONS; j++)
	{
		CHttpConnection *pConn = &m_aConnections[j];
		if(pConn->State() == CHttpConnection::STATE_OFFLINE)
			return pConn;
	}

	return 0;
}

void CHttpClient::Update()
{
	// TODO: rework bandwidth limiting
	// TODO: add some priority handling?
	for(int i = 0; i < m_lPendingRequests.size(); i++)
	{
		CRequestInfo *pInfo = m_lPendingRequests[i];
		if(pInfo->m_Lookup.m_Job.Status() != CJob::STATE_DONE)
			continue;

		if(pInfo->m_Lookup.m_Job.Result() != 0)
		{
			pInfo->ExecuteCallback(0, true);
			delete pInfo;
			m_lPendingRequests.remove_index(i);
			i--;
		}
		else
		{
			if(pInfo->m_Lookup.m_Addr.port == 0)
				pInfo->m_Lookup.m_Addr.port = 80;

			CHttpConnection *pConn = GetConnection(pInfo->m_Lookup.m_Addr);
			if(pConn)
			{
				if(pConn->State() == CHttpConnection::STATE_OFFLINE)
					pConn->Connect(pInfo->m_Lookup.m_Addr);
				pConn->SetRequest(pInfo);
				m_lPendingRequests.remove_index(i);
				i--;
			}
		}
	}

	for(int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
		m_aConnections[i].Update();
}
