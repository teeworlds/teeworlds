/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#define GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#include <game/client/component.h>

class CMapImages : public CComponent
{
	enum
	{
		MAX_TEXTURES=64
	};
	IGraphics::CTextureHandle m_aTextures[MAX_TEXTURES];
	IGraphics::CTextureHandle m_aMenuTextures[MAX_TEXTURES];
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
