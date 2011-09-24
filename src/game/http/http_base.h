/* Webapp Class by Sushi and Redix */
#ifndef GAME_HTTP_HTTP_BASE_H
#define GAME_HTTP_HTTP_BASE_H

#include <base/tl/array.h>

class IHttpBase
{
protected:
	IOHANDLE m_File;
	char m_aFilename[512];
	bool m_Finish;

	class CHttpField
	{
	public:
		char m_aKey[128];
		char m_aValue[128];
	};

	array<CHttpField> m_Fields;
	
public:
	IHttpBase() : m_File(0), m_Finish(false) {}
	virtual ~IHttpBase() { CloseFiles(); }

	virtual bool Finish() = 0;

	void AddField(CHttpField Field);
	void AddField(const char *pKey, const char *pValue);
	void AddField(const char *pKey, int Value);
	const char *GetField(const char *pKey);

	void SetFile(IOHANDLE File, const char *pFilename);
	const char *GetPath() { return m_aFilename; }
	const char *GetFilename();
	void CloseFiles();
	bool IsFile() { return m_File; }
};

#endif
