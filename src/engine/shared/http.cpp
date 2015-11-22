#include <base/system.h>

#include "http.h"

IHttpBase::IHttpBase() : m_Finalized(false), m_FieldNum(0) { }

IHttpBase::~IHttpBase() { }

void IHttpBase::AddField(const char *pKey, const char *pValue)
{
	if(m_Finalized || m_FieldNum >= HTTP_MAX_HEADER_FIELDS)
		return;
	
	str_copy(m_aFields[m_FieldNum].m_aKey, pKey, sizeof(m_aFields[m_FieldNum].m_aKey));
	str_copy(m_aFields[m_FieldNum].m_aValue, pValue, sizeof(m_aFields[m_FieldNum].m_aValue));
	//dbg_msg("http/base", "added field: '%s' -> '%s' (%d)", m_aFields[m_FieldNum].m_aKey, m_aFields[m_FieldNum].m_aValue, m_FieldNum);
	m_FieldNum++;
}

void IHttpBase::AddField(const char *pKey, int Value)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%d", Value);
	AddField(pKey, aBuf);
}

const char *IHttpBase::GetField(const char *pKey) const
{
	//dbg_msg("http/base", "searching key: '%s'", pKey);
	for(int i = 0; i < m_FieldNum; i++)
	{
		if(str_comp_nocase(m_aFields[i].m_aKey, pKey) == 0)
		{
			//dbg_msg("http/base", "found key: '%s' -> '%s' (%d)", m_aFields[i].m_aKey, m_aFields[i].m_aValue, m_FieldNum);
			return m_aFields[i].m_aValue;
		}
	}
	return 0;
}

CHttpClient::CHttpClient()
{
	for (int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
		m_aConnections[i].SetID(i);
}

CHttpClient::~CHttpClient() { }

void CHttpClient::Send(const char *pAddr, CRequest *pRequest)
{
	CRequestData *pData = new CRequestData();
	pData->m_pRequest = pRequest;
	m_pEngine->HostLookup(&pData->m_Lookup, pAddr, NETTYPE_IPV4);

	char aAddr[256];
	str_copy(aAddr, pAddr, sizeof(aAddr));

	for(int k = 0; aAddr[k]; k++)
	{
		if(aAddr[k] == ':')
		{
			aAddr[k] = 0;
			break;
		}
	}

	pRequest->AddField("Host", aAddr);

	m_lPendingRequests.add(pData);
}

void CHttpClient::Update()
{
	// TODO: rework bandwidth limiting
	// TODO: add some priority handling?
	for(int i = 0; i < m_lPendingRequests.size(); i++)
	{
		CRequestData *pData = m_lPendingRequests[i];
		if(pData->m_Lookup.m_Job.Status() != CJob::STATE_DONE)
			continue;

		if(pData->m_Lookup.m_Job.Result() != 0)
		{
			pData->m_pRequest->ExecuteCallback(0, true);
			delete pData->m_pRequest;
			delete pData;
			m_lPendingRequests.remove_index(i);
			i--;
		}
		else
		{
			if(pData->m_Lookup.m_Addr.port == 0)
				pData->m_Lookup.m_Addr.port = 80;

			for (int j = 0; j < HTTP_MAX_CONNECTIONS; j++)
			{
				CHttpConnection *pConn = &m_aConnections[j];
				if(pConn->CheckState(CHttpConnection::STATE_WAITING) && pConn->CompareAddr(pData->m_Lookup.m_Addr))
				{
					pConn->SetRequest(pData->m_pRequest);
					delete pData;
					m_lPendingRequests.remove_index(i);
					i--;
					break;
				}
				else if(pConn->CheckState(CHttpConnection::STATE_OFFLINE))
				{
					pConn->Connect(pData->m_Lookup.m_Addr);
					pConn->SetRequest(pData->m_pRequest);
					delete pData;
					m_lPendingRequests.remove_index(i);
					i--;
					break;
				}
			}
		}
	}

	for(int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
		m_aConnections[i].Update();
}
