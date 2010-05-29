#ifndef GAME_CLIENT_COMPONENTS_SKINS_H
#define GAME_CLIENT_COMPONENTS_SKINS_H
#include <base/vmath.h>
#include <game/client/component.h>

class CSkins : public CComponent
{
public:
	// do this better and nicer
	struct CSkin
	{
		int m_OrgTexture;
		int m_ColorTexture;
		char m_aName[31];
		char m_aTerm[1];
		vec3 m_BloodColor;
	} ;

	CSkins();
	
	void Init();
	
	vec4 GetColor(int v);
	int Num();
	const CSkin *Get(int Index);
	int Find(const char *pName);
	
private:
	enum
	{
		MAX_SKINS=256,
	};

	CSkin m_aSkins[MAX_SKINS];
	int m_NumSkins;

	static void SkinScan(const char *pName, int IsDir, void *pUser);
};
#endif
