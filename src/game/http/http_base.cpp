#include "http_base.h"

void IHttpBase::AddField(CHttpField Field)
{
	if(!m_Finish)
	{
		//dbg_msg("http", "added: %s -> %s", Field.m_aKey, Field.m_aValue);
		m_Fields.add(Field);
	}
}

void IHttpBase::AddField(const char *pKey, const char *pValue)
{
	CHttpField Tmp;
	str_copy(Tmp.m_aKey, pKey, sizeof(Tmp.m_aKey));
	str_copy(Tmp.m_aValue, pValue, sizeof(Tmp.m_aValue));
	AddField(Tmp);
}

void IHttpBase::AddField(const char *pKey, int Value)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%d", Value);
	AddField(pKey, aBuf);
}

const char *IHttpBase::GetField(const char *pKey)
{
	for(int i = 0; i < m_Fields.size(); i++)
	{
		if(str_comp_nocase(m_Fields[i].m_aKey, pKey) == 0)
			return m_Fields[i].m_aValue;
	}
	return 0;
}

void IHttpBase::SetFile(IOHANDLE File, const char *pFilename)
{
	if(!m_Finish)
	{
		m_File = File;
		str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
	}
}

const char *IHttpBase::GetFilename()
{
	char *pShort = m_aFilename;
	for(char *pCur = pShort; *pCur; pCur++)
	{
		if(*pCur == '/' || *pCur == '\\')
			pShort = pCur+1;
	}
	return pShort;
}

void IHttpBase::CloseFiles()
{
	if(m_File)
		io_close(m_File);
	m_File = 0;
}
