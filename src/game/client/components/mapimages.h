/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#define GAME_CLIENT_COMPONENTS_MAPIMAGES_H
#include <game/client/component.h>

class CMapImages : public CComponent
{
	enum
	{
		MAX_TEXTURES=64,

		MAP_TYPE_GAME=0,
		MAP_TYPE_MENU,
		NUM_MAP_TYPES
	};
	struct
	{
		IGraphics::CTextureHandle m_aTextures[MAX_TEXTURES];
		int m_Count;
	} m_Info[NUM_MAP_TYPES];

	IGraphics::CTextureHandle m_EasterTexture;
	IGraphics::CTextureHandle m_EntitiesTexture;
	IGraphics::CTextureHandle m_FrontTexture;
	IGraphics::CTextureHandle m_TeleTexture;
	IGraphics::CTextureHandle m_SpeedupTexture;
	IGraphics::CTextureHandle m_SwitchTexture;
	bool m_EasterIsLoaded;
	bool m_EntitiesIsLoaded;
	bool m_FrontIsLoaded;
	bool m_TeleIsLoaded;
	bool m_SpeedupIsLoaded;
	bool m_SwitchIsLoaded;

	void LoadMapImages(class IMap *pMap, class CLayers *pLayers, int MapType);

public:
	CMapImages();

	IGraphics::CTextureHandle Get(int Index) const;
	int Num() const;

	virtual void OnMapLoad();
	void OnMenuMapLoad(class IMap *pMap);
	
	IGraphics::CTextureHandle GetEasterTexture();

	// DDRace

	IGraphics::CTextureHandle GetEntitiesTexture();
	IGraphics::CTextureHandle GetFrontTexture();
	IGraphics::CTextureHandle GetTeleTexture();
	IGraphics::CTextureHandle GetSpeedupTexture();
	IGraphics::CTextureHandle GetSwitchTexture();
};

#endif
