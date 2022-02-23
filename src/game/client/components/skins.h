/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SKINS_H
#define GAME_CLIENT_COMPONENTS_SKINS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

// todo: fix duplicate skins (different paths)
class CSkins : public CComponent
{
public:
	enum
	{
		SKINFLAG_SPECIAL=1<<0,
		SKINFLAG_STANDARD=1<<1,

		DARKEST_COLOR_LGT=61,

		NUM_COLOR_COMPONENTS=4,

		HAT_NUM=2,
		HAT_OFFSET_SIDE=2,
	};

	struct CSkinPart
	{
		int m_Flags;
		char m_aName[MAX_SKIN_ARRAY_SIZE];
		IGraphics::CTextureHandle m_OrgTexture;
		IGraphics::CTextureHandle m_ColorTexture;
		vec3 m_BloodColor;

		bool operator<(const CSkinPart &Other) { return str_comp_nocase(m_aName, Other.m_aName) < 0; }
	};

	struct CSkin
	{
		int m_Flags;
		char m_aName[MAX_SKIN_ARRAY_SIZE];
		const CSkinPart *m_apParts[NUM_SKINPARTS];
		int m_aPartColors[NUM_SKINPARTS];
		int m_aUseCustomColors[NUM_SKINPARTS];

		bool operator<(const CSkin &Other) { return str_comp_nocase(m_aName, Other.m_aName) < 0; }
		bool operator==(const CSkin &Other) { return mem_comp(this, &Other, sizeof(CSkin)) == 0; }
	};

	static const char * const ms_apSkinPartNames[NUM_SKINPARTS];
	static const char * const ms_apColorComponents[NUM_COLOR_COMPONENTS];

	static char *ms_apSkinVariables[NUM_SKINPARTS];
	static int *ms_apUCCVariables[NUM_SKINPARTS]; // use custom color
	static int *ms_apColorVariables[NUM_SKINPARTS];
	IGraphics::CTextureHandle m_XmasHatTexture;
	IGraphics::CTextureHandle m_BotTexture;
	IGraphics::CTextureHandle m_DivingGearTexture;

	int GetInitAmount() const;
	void OnInit();

	void AddSkin(const char *pSkinName);
	void RemoveSkin(const CSkin *pSkin);

	int Num();
	int NumSkinPart(int Part);
	const CSkin *Get(int Index);
	int Find(const char *pName, bool AllowSpecialSkin);
	const CSkinPart *GetSkinPart(int Part, int Index);
	int FindSkinPart(int Part, const char *pName, bool AllowSpecialPart);
	void RandomizeSkin();

	vec3 GetColorV3(int v) const;
	vec4 GetColorV4(int v, bool UseAlpha) const;
	int GetTeamColor(int UseCustomColors, int PartColor, int Team, int Part) const;

	// returns true if everything was valid and nothing changed
	bool ValidateSkinParts(char *apPartNames[NUM_SKINPARTS], int *pUseCustomColors, int *pPartColors, int GameFlags) const;

	void SaveSkinfile(const char *pSaveSkinName);

private:
	int m_ScanningPart;
	sorted_array<CSkinPart> m_aaSkinParts[NUM_SKINPARTS];
	sorted_array<CSkin> m_aSkins;
	CSkin m_DummySkin;

	static int SkinPartScan(const char *pName, int IsDir, int DirType, void *pUser);
	static int SkinScan(const char *pName, int IsDir, int DirType, void *pUser);
};

#endif
