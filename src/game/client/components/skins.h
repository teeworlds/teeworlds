/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SKINS_H
#define GAME_CLIENT_COMPONENTS_SKINS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

enum
{
	DARKEST_COLOR_LGT=61
};

enum
{
	SKINPART_BODY=0,
	SKINPART_TATTOO,
	SKINPART_DECORATION,
	SKINPART_HANDS,
	SKINPART_FEET,
	SKINPART_EYES,
	NUM_SKINPARTS
};

class CSkins : public CComponent
{
public:
	struct CSkinPart
	{
		char m_aName[24];
		int m_OrgTexture;
		int m_ColorTexture;
		vec3 m_BloodColor;

		bool operator<(const CSkinPart &Other) { return str_comp_nocase(m_aName, Other.m_aName) < 0; }
	};

	struct CSkin
	{
		char m_aName[24];
		const CSkinPart *m_apParts[NUM_SKINPARTS];
		int m_aPartColors[NUM_SKINPARTS];
		int m_aUseCustomColors[NUM_SKINPARTS];

		bool operator<(const CSkin &Other) { return str_comp_nocase(m_aName, Other.m_aName) < 0; }
	};

	void OnInit();

	int Num();
	int NumSkinPart(int Part);
	const CSkin *Get(int Index);
	int Find(const char *pName);
	const CSkinPart *GetSkinPart(int Part, int Index);
	int FindSkinPart(int Part, const char *pName);

	vec3 GetColorV3(int v) const;
	vec4 GetColorV4(int v, bool UseAlpha) const;
	int GetTeamColor(int UseCustomColors, int PartColor, int Team, int Part) const;

private:
	int m_ScanningPart;
	sorted_array<CSkinPart> m_aaSkinParts[NUM_SKINPARTS];
	sorted_array<CSkin> m_aSkins;
	CSkin m_DummySkin;

	static int SkinPartScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int SkinScan(const char *pName, int IsDir, int DirType, void *pUser);
};

extern char *const gs_apSkinVariables[NUM_SKINPARTS];
extern int *const gs_apUCCVariables[NUM_SKINPARTS]; // use custom color
extern int *const gs_apColorVariables[NUM_SKINPARTS];

#endif
