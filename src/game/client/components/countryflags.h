/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_COUNTRYFLAGS_H
#define GAME_CLIENT_COMPONENTS_COUNTRYFLAGS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CCountryFlags : public CComponent
{
public:
	struct CCountryFlag
	{
		int m_CountryCode;
		int m_Texture;

		bool operator<(const CCountryFlag &Other) { return m_CountryCode < Other.m_CountryCode; }
	};

	void OnInit();

	int Num() const;
	const CCountryFlag *Get(int Index) const;
	int Find(int CountryCode) const;

private:
	sorted_array<CCountryFlag> m_aCountryFlags;

	void LoadCountryflagsIndexfile();
};
#endif
