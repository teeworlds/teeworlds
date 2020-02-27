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
		char m_aCountryCodeString[8];
		bool m_Blocked;
		IGraphics::CTextureHandle m_Texture;

		bool operator<(const CCountryFlag &Other) const { return str_comp(m_aCountryCodeString, Other.m_aCountryCodeString) < 0; }
	};

	int GetInitAmount() const;
	void OnInit();

	int Num() const;
	const CCountryFlag *GetByCountryCode(int CountryCode) const;
	const CCountryFlag *GetByIndex(int Index, bool SkipBlocked = false) const;
	void Render(int CountryCode, const vec4 *pColor, float x, float y, float w, float h, bool AllowBlocked=false);

private:
	enum
	{
		CODE_LB=-1,
		CODE_UB=999,
		CODE_RANGE=CODE_UB-CODE_LB+1,
	};
	sorted_array<CCountryFlag> m_aCountryFlags;
	int m_CodeIndexLUT[CODE_RANGE];

	void LoadCountryflagsIndexfile();
};
#endif
