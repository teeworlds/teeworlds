/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "jsonparser.h"

CJsonParser::CJsonParser() : m_pParsedJson(0x0)
{
	m_aError[0] = '\0';
}

CJsonParser::~CJsonParser()
{
	json_value_free(m_pParsedJson);
}

json_value *CJsonParser::ParseFile(const char *pFilename, IStorage *pStorage, int StorageType)
{
	dbg_assert(!m_pParsedJson && !m_aError[0], "already parsed");

	void *pFileData;
	unsigned FileSize;
	if(!pStorage->ReadFile(pFilename, StorageType, &pFileData, &FileSize))
	{
		str_format(m_aError, sizeof(m_aError), "Failed to read '%s'", pFilename);
		return 0x0;
	}

	ParseData(pFileData, FileSize, pFilename);
	mem_free(pFileData);
	return m_pParsedJson;
}

json_value *CJsonParser::ParseData(const void *pFileData, unsigned FileSize, const char *pContext)
{
	dbg_assert(!m_pParsedJson && !m_aError[0], "already parsed");

	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aJsonError[json_error_max];
	m_pParsedJson = json_parse_ex(&JsonSettings, static_cast<const json_char *>(pFileData), FileSize, aJsonError);

	if(!m_pParsedJson)
		str_format(m_aError, sizeof(m_aError), "Failed to parse '%s': %s", pContext, aJsonError);

	return m_pParsedJson;
}

json_value *CJsonParser::ParseString(const char *pString, const char *pContext)
{
	return ParseData(pString, str_length(pString), pContext);
}
