#ifndef GAME_CLIENT_COMPONENTS_SKINS_H
#define GAME_CLIENT_COMPONENTS_SKINS_H
#include <base/vmath.h>
#include <base/tl/array.h>
#include <game/client/component.h>

class CSkins : public CComponent
{
public:
	// do this better and nicer
	struct CSkin
	{
		int m_OrgTexture;
		int m_ColorTexture;
		char m_aName[32];
		vec3 m_BloodColor;
	};
	
	void Init();
	
	vec4 GetColor(int v);
	int Num();
	const CSkin *Get(int Index);
	int Find(const char *pName);
	
private:
	array<CSkin> m_aSkins;

	static void SkinScan(const char *pName, int IsDir, void *pUser);
};
#endif
