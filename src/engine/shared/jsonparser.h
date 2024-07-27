/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_JSONPARSER_H
#define ENGINE_SHARED_JSONPARSER_H

#include <engine/storage.h>

#include <engine/external/json-parser/json.h>

class CJsonParser
{
	char m_aError[256];
	json_value *m_pParsedJson;

public:
	CJsonParser();
	~CJsonParser();
	json_value *ParseFile(const char *pFilename, IStorage *pStorage, int StorageType = IStorage::TYPE_ALL);
	json_value *ParseData(const void *pFileData, unsigned FileSize, const char *pContext = "rawdata");
	json_value *ParseString(const char *pString, const char *pContext = "string"); // assumes a zero-terminated string
	json_value *ParsedJson() { return m_pParsedJson; }
	const char *Error() const { return m_aError; }
};

#endif
