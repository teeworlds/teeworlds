/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LOCALIZATION_H
#define GAME_LOCALIZATION_H
#include <base/tl/string.h>
#include <base/tl/sorted_array.h>

class CLocalizationDatabase
{
	class CString
	{
	public:
		unsigned m_Hash;

		// TODO: do this as an const char * and put everything on a incremental heap
		string m_Replacement;

		bool operator <(const CString &Other) const { return m_Hash < Other.m_Hash; }
		bool operator <=(const CString &Other) const { return m_Hash <= Other.m_Hash; }
		bool operator ==(const CString &Other) const { return m_Hash == Other.m_Hash; }
	};

	sorted_array<CString> m_Strings;
	int m_VersionCounter;
	int m_CurrentVersion;

public:
	CLocalizationDatabase();

	bool Load(const char *pFilename, class IStorage *pStorage, class IConsole *pConsole);

	int Version() { return m_CurrentVersion; }

	void AddString(const char *pOrgStr, const char *pNewStr);
	const char *FindString(unsigned Hash);
};

extern CLocalizationDatabase g_Localization;

class CLocConstString
{
	const char *m_pDefaultStr;
	const char *m_pCurrentStr;
	unsigned m_Hash;
	int m_Version;
public:
	CLocConstString(const char *pStr);
	void Reload();

	inline operator const char *()
	{
		if(m_Version != g_Localization.Version())
			Reload();
		return m_pCurrentStr;
	}
};


extern const char *Localize(const char *pStr)
GNUC_ATTRIBUTE((format_arg(1)));
#endif
