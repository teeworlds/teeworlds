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

bool CHttpClient::Send(const char *pAddr, CRequest *pRequest)
{
	pRequest->AddField("Host", pAddr);
	CRequestData Data;
	Data.m_pRequest = pRequest;

	char aAddr[256];
	str_copy(aAddr, pAddr, sizeof(aAddr));

	int Port = 80;
	for(int k = 0; aAddr[k]; k++)
	{
		if(aAddr[k] == ':')
		{
			Port = str_toint(aAddr + k + 1);
			aAddr[k] = 0;
			break;
		}
	}

	if(net_host_lookup(aAddr, &Data.m_Addr, NETTYPE_IPV4) != 0)
	{
		pRequest->ExecuteCallback(0, true);
		return false;
	}

	Data.m_Addr.port = Port;

	m_lPendingRequests.add(Data);
	return true;
}

void CHttpClient::Update()
{
	// TODO: rework bandwidth limiting
	// TODO: add some priority handling?
	for(int i = 0; i < m_lPendingRequests.size(); i++)
	{
		for(int j = 0; j < HTTP_MAX_CONNECTIONS; j++)
		{
			CRequestData *pData = &m_lPendingRequests[i];
			CHttpConnection *pConn = &m_aConnections[j];
			if(pConn->CheckState(CHttpConnection::STATE_WAITING) && pConn->CompareAddr(pData->m_Addr))
			{
				pConn->SetRequest(pData->m_pRequest);
				m_lPendingRequests.remove_index(i);
				i--;
				break;
			}
			else if(pConn->CheckState(CHttpConnection::STATE_OFFLINE))
			{
				pConn->Connect(pData->m_Addr);
				pConn->SetRequest(pData->m_pRequest);
				m_lPendingRequests.remove_index(i);
				i--;
				break;
			}
		}
	}

	for(int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
		m_aConnections[i].Update();
}
