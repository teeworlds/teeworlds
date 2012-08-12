/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#define GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#include <game/client/component.h>

class CMapImages : public CComponent
{
	IGraphics::CTextureHandle m_aTextures[64];
	IGraphics::CTextureHandle m_aMenuTextures[64];
	int m_Count;
	int m_MenuCount;
public:
	CMapImages();

	IGraphics::CTextureHandle Get(int Index) const;
	int Num() const;

	virtual void OnMapLoad();
	void OnMenuMapLoad(class IMap *pMap);
};

#endif
