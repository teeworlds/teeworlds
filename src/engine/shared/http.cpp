#include <base/system.h>

#include "http.h"

IHttpBase::IHttpBase() : m_Finish(false), m_FieldNum(0) { }

IHttpBase::~IHttpBase() { }

void IHttpBase::AddField(const char *pKey, const char *pValue)
{
	if(m_Finish || m_FieldNum >= HTTP_MAX_HEADER_FIELDS)
		return;
	
	str_copy(m_aFields[m_FieldNum].m_aKey, pKey, sizeof(m_aFields[m_FieldNum].m_aKey));
	str_copy(m_aFields[m_FieldNum].m_aValue, pValue, sizeof(m_aFields[m_FieldNum].m_aValue));
	dbg_msg("http/base", "added field: '%s' -> '%s' (%d)", m_aFields[m_FieldNum].m_aKey, m_aFields[m_FieldNum].m_aValue, m_FieldNum);
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
	dbg_msg("http/base", "searching key: '%s'", pKey);
	for(int i = 0; i < m_FieldNum; i++)
	{
		if(str_comp_nocase(m_aFields[i].m_aKey, pKey) == 0)
		{
			dbg_msg("http/base", "found key: '%s' -> '%s' (%d)", m_aFields[i].m_aKey, m_aFields[i].m_aValue, m_FieldNum);
			return m_aFields[i].m_aValue;
		}
	}
	return 0;
}

CHttpClient::CHttpClient()
{
	mem_zero(m_apConnections, sizeof(m_apConnections));
}

CHttpClient::~CHttpClient()
{
	for(int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
	{
		if(m_apConnections[i])
			delete m_apConnections[i];
	}
};

bool CHttpClient::Send(CHttpConnection *pCon)
{
	for(int i = 0; i < HTTP_MAX_CONNECTIONS; i++)
	{
		if(!m_apConnections[i])
		{
			m_apConnections[i] = pCon;
			pCon->Update();
			return true;
		}
	}

	pCon->SetState(CHttpConnection::STATE_ERROR, "dropping request, too many connections");
	pCon->Call();
	delete pCon;
	return false;
}

void CHttpClient::Update()
{
	// TODO: rework bandwidth limiting
	// TODO: add some priority handling?
	int Max = 3;
	int Count = 0;
	for(int i = 0; i < HTTP_MAX_CONNECTIONS && Count < Max; i++)
	{
		if(m_apConnections[i])
		{
			if(m_apConnections[i]->Update() != 0)
			{
				m_apConnections[i]->Call();
				delete m_apConnections[i];
				m_apConnections[i] = 0;
			}
			Count++;
		}
	}
}
