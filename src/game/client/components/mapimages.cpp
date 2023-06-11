/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <game/client/component.h>
#include <game/mapitems.h>

#include "mapimages.h"

CMapImages::CMapImages()
{
	m_Info[MAP_TYPE_GAME].m_Count = 0;
	m_Info[MAP_TYPE_MENU].m_Count = 0;

	m_EasterIsLoaded = false;
}

void CMapImages::LoadMapImages(IMap *pMap, class CLayers *pLayers, int MapType)
{
	if(MapType < 0 || MapType >= NUM_MAP_TYPES)
		return;

	// unload all textures
	for(int i = 0; i < m_Info[MapType].m_Count; i++)
		Graphics()->UnloadTexture(&(m_Info[MapType].m_aTextures[i]));
	m_Info[MapType].m_Count = 0;

	int Start;
	pMap->GetType(MAPITEMTYPE_IMAGE, &Start, &m_Info[MapType].m_Count);
	m_Info[MapType].m_Count = clamp(m_Info[MapType].m_Count, 0, int(MAX_TEXTURES));

	// load new textures
	for(int i = 0; i < m_Info[MapType].m_Count; i++)
	{
		int TextureFlags = 0;
		bool FoundQuadLayer = false;
		bool FoundTileLayer = false;
		for(int k = 0; k < pLayers->NumLayers(); k++)
		{
			const CMapItemLayer *pLayer = pLayers->GetLayer(k);
			if(!pLayer)
				continue;
			if(!FoundQuadLayer && pLayer->m_Type == LAYERTYPE_QUADS && reinterpret_cast<const CMapItemLayerQuads *>(pLayer)->m_Image == i)
				FoundQuadLayer = true;
			if(!FoundTileLayer && pLayer->m_Type == LAYERTYPE_TILES && reinterpret_cast<const CMapItemLayerTilemap *>(pLayer)->m_Image == i)
				FoundTileLayer = true;
		}
		if(FoundTileLayer)
			TextureFlags = FoundQuadLayer ? IGraphics::TEXLOAD_MULTI_DIMENSION : IGraphics::TEXLOAD_ARRAY_256;

		int ItemSize;
		const CMapItemImage *pImg = static_cast<CMapItemImage *>(pMap->GetItem(Start + i, 0x0, 0x0, &ItemSize));
		if(!CMapItemChecker::IsValid(pImg, ItemSize))
			continue;

		const int Format = pImg->m_Version < CMapItemImage_v2::CURRENT_VERSION ? CImageInfo::FORMAT_RGBA : pImg->m_Format;
		if(pImg->m_External || (Format != CImageInfo::FORMAT_RGB && Format != CImageInfo::FORMAT_RGBA))
		{
			char aName[IO_MAX_PATH_LENGTH];
			if(!pMap->GetDataString(pImg->m_ImageName, aName, sizeof(aName)))
				continue;
			char aBuf[IO_MAX_PATH_LENGTH];
			str_format(aBuf, sizeof(aBuf), "mapres/%s.png", aName);
			m_Info[MapType].m_aTextures[i] = Graphics()->LoadTexture(aBuf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, TextureFlags);
		}
		else
		{
			if(pImg->m_Width * pImg->m_Height * CImageInfo::GetPixelSize(Format) != pMap->GetDataSize(pImg->m_ImageData))
				continue;
			const void *pData = pMap->GetData(pImg->m_ImageData);
			if(!pData)
				continue;
			m_Info[MapType].m_aTextures[i] = Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, Format, pData, CImageInfo::FORMAT_RGBA, TextureFlags);
			pMap->UnloadData(pImg->m_ImageData);
		}
	}

	// easter time, preload easter tileset
	if(m_pClient->IsEaster())
		GetEasterTexture();
}

int CMapImages::GetActiveMapType() const
{
	if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return MAP_TYPE_GAME;
	return MAP_TYPE_MENU;
}

void CMapImages::OnMapLoad()
{
	LoadMapImages(Kernel()->RequestInterface<IMap>(), Layers(), MAP_TYPE_GAME);
}

void CMapImages::OnMenuMapLoad(IMap *pMap)
{
	CLayers MenuLayers;
	MenuLayers.Init(Kernel(), pMap);
	LoadMapImages(pMap, &MenuLayers, MAP_TYPE_MENU);
}

IGraphics::CTextureHandle CMapImages::GetEasterTexture()
{
	if(!m_EasterIsLoaded)
	{
		m_EasterTexture = Graphics()->LoadTexture("mapres/easter.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_ARRAY_256);
		if(!m_EasterTexture.IsValid())
			Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "mapimages", "Failed to load easter.png");
		m_EasterIsLoaded = true;
	}
	return m_EasterTexture;
}

IGraphics::CTextureHandle CMapImages::Get(int Index) const
{
	const int MapType = GetActiveMapType();
	return m_Info[MapType].m_aTextures[clamp(Index, 0, m_Info[MapType].m_Count)];
}

int CMapImages::Num() const
{
	return m_Info[GetActiveMapType()].m_Count;
}
