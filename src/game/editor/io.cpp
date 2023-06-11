/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/client.h>
#include <engine/console.h>
#include <engine/map.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>

#include <game/gamecore.h> // StrToInts, IntsToStr

#include "editor.h"

int CEditor::Save(const char *pFilename)
{
	return m_Map.Save(Kernel()->RequestInterface<IStorage>(), pFilename);
}

int CEditorMap::Save(class IStorage *pStorage, const char *pFileName)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "saving to '%s'...", pFileName);
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", aBuf);
	CDataFileWriter df;
	if(!df.Open(pStorage, pFileName))
	{
		str_format(aBuf, sizeof(aBuf), "failed to open file '%s'...", pFileName);
		m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", aBuf);
		return 0;
	}

	// save version
	{
		CMapItemVersion Item;
		Item.m_Version = CMapItemVersion::CURRENT_VERSION;
		df.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Item), &Item);
	}

	// save map info
	{
		CMapItemInfo Item;
		Item.m_Version = CMapItemInfo::CURRENT_VERSION;

		if(m_MapInfo.m_aAuthor[0])
			Item.m_Author = df.AddData(str_length(m_MapInfo.m_aAuthor)+1, m_MapInfo.m_aAuthor);
		else
			Item.m_Author = -1;
		if(m_MapInfo.m_aVersion[0])
			Item.m_MapVersion = df.AddData(str_length(m_MapInfo.m_aVersion)+1, m_MapInfo.m_aVersion);
		else
			Item.m_MapVersion = -1;
		if(m_MapInfo.m_aCredits[0])
			Item.m_Credits = df.AddData(str_length(m_MapInfo.m_aCredits)+1, m_MapInfo.m_aCredits);
		else
			Item.m_Credits = -1;
		if(m_MapInfo.m_aLicense[0])
			Item.m_License = df.AddData(str_length(m_MapInfo.m_aLicense)+1, m_MapInfo.m_aLicense);
		else
			Item.m_License = -1;

		df.AddItem(MAPITEMTYPE_INFO, 0, sizeof(Item), &Item);
	}

	// save images
	for(int i = 0; i < m_lImages.size(); i++)
	{
		CEditorImage *pImg = m_lImages[i];

		// analyze the image for when saving (should be done when we load the image)
		// TODO!
		pImg->AnalyzeTileFlags();

		CMapItemImage Item;
		Item.m_Version = CMapItemImage::CURRENT_VERSION;

		Item.m_Width = pImg->m_Width;
		Item.m_Height = pImg->m_Height;
		Item.m_External = pImg->m_External;
		Item.m_ImageName = df.AddData(str_length(pImg->m_aName)+1, pImg->m_aName);
		if(pImg->m_External)
			Item.m_ImageData = -1;
		else
			Item.m_ImageData = df.AddData(Item.m_Width * Item.m_Height * pImg->GetPixelSize(), pImg->m_pData);
		Item.m_Format = pImg->m_Format;
		df.AddItem(MAPITEMTYPE_IMAGE, i, sizeof(Item), &Item);
	}

	// save layers
	int LayerCount = 0, GroupCount = 0;
	for(int g = 0; g < m_lGroups.size(); g++)
	{
		CLayerGroup *pGroup = m_lGroups[g];
		if(!pGroup->m_SaveToMap)
			continue;

		CMapItemGroup GItem;
		GItem.m_Version = CMapItemGroup::CURRENT_VERSION;

		GItem.m_ParallaxX = pGroup->m_ParallaxX;
		GItem.m_ParallaxY = pGroup->m_ParallaxY;
		GItem.m_OffsetX = pGroup->m_OffsetX;
		GItem.m_OffsetY = pGroup->m_OffsetY;
		GItem.m_UseClipping = pGroup->m_UseClipping;
		GItem.m_ClipX = pGroup->m_ClipX;
		GItem.m_ClipY = pGroup->m_ClipY;
		GItem.m_ClipW = pGroup->m_ClipW;
		GItem.m_ClipH = pGroup->m_ClipH;
		GItem.m_StartLayer = LayerCount;
		GItem.m_NumLayers = 0;

		// save group name
		StrToInts(GItem.m_aName, sizeof(GItem.m_aName)/sizeof(int), pGroup->m_aName);

		for(int l = 0; l < pGroup->m_lLayers.size(); l++)
		{
			if(!pGroup->m_lLayers[l]->m_SaveToMap)
				continue;

			if(pGroup->m_lLayers[l]->m_Type == LAYERTYPE_TILES)
			{
				m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving tiles layer");
				CLayerTiles *pLayer = (CLayerTiles *)pGroup->m_lLayers[l];
				pLayer->PrepareForSave();

				CMapItemLayerTilemap Item;
				Item.m_Version = CMapItemLayerTilemap::CURRENT_VERSION;

				Item.m_Layer.m_Version = 0; // unused
				Item.m_Layer.m_Flags = pLayer->m_Flags;
				Item.m_Layer.m_Type = pLayer->m_Type;

				Item.m_Color = pLayer->m_Color;
				Item.m_ColorEnv = pLayer->m_ColorEnv;
				Item.m_ColorEnvOffset = pLayer->m_ColorEnvOffset;

				Item.m_Width = pLayer->m_Width;
				Item.m_Height = pLayer->m_Height;
				Item.m_Flags = pLayer->m_Game ? TILESLAYERFLAG_GAME : 0;
				Item.m_Image = pLayer->m_Image;
				Item.m_Data = df.AddData(pLayer->m_SaveTilesSize, pLayer->m_pSaveTiles);

				// save layer name
				StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), pLayer->m_aName);

				df.AddItem(MAPITEMTYPE_LAYER, LayerCount, sizeof(Item), &Item);

				GItem.m_NumLayers++;
				LayerCount++;
			}
			else if(pGroup->m_lLayers[l]->m_Type == LAYERTYPE_QUADS)
			{
				m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving quads layer");
				CLayerQuads *pLayer = (CLayerQuads *)pGroup->m_lLayers[l];
				if(pLayer->m_lQuads.size())
				{
					CMapItemLayerQuads Item;
					Item.m_Version = CMapItemLayerQuads::CURRENT_VERSION;
					Item.m_Layer.m_Version = 0; // unused
					Item.m_Layer.m_Flags = pLayer->m_Flags;
					Item.m_Layer.m_Type = pLayer->m_Type;
					Item.m_Image = pLayer->m_Image;

					// add the data
					Item.m_NumQuads = pLayer->m_lQuads.size();
					Item.m_Data = df.AddDataSwapped(pLayer->m_lQuads.size()*sizeof(CQuad), pLayer->m_lQuads.base_ptr());

					// save layer name
					StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), pLayer->m_aName);

					df.AddItem(MAPITEMTYPE_LAYER, LayerCount, sizeof(Item), &Item);

					GItem.m_NumLayers++;
					LayerCount++;
				}
			}
		}

		df.AddItem(MAPITEMTYPE_GROUP, GroupCount++, sizeof(GItem), &GItem);
	}

	// check for bezier curve envelopes, otherwise use older, smaller envelope points
	int Version = CMapItemEnvelope_v2::CURRENT_VERSION;
	int Size = sizeof(CEnvPoint_v1);
	for(int e = 0; e < m_lEnvelopes.size(); e++)
	{
		for(int p = 0; p < m_lEnvelopes[e]->m_lPoints.size(); p++)
		{
			if(m_lEnvelopes[e]->m_lPoints[p].m_Curvetype == CURVETYPE_BEZIER)
			{
				Version = CMapItemEnvelope::CURRENT_VERSION;
				Size = sizeof(CEnvPoint);
				break;
			}
		}
	}

	// save envelopes
	int PointCount = 0;
	for(int e = 0; e < m_lEnvelopes.size(); e++)
	{
		CMapItemEnvelope Item;
		Item.m_Version = Version;
		Item.m_Channels = m_lEnvelopes[e]->GetChannels();
		Item.m_StartPoint = PointCount;
		Item.m_NumPoints = m_lEnvelopes[e]->m_lPoints.size();
		Item.m_Synchronized = m_lEnvelopes[e]->m_Synchronized;
		StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), m_lEnvelopes[e]->m_aName);

		df.AddItem(MAPITEMTYPE_ENVELOPE, e, sizeof(Item), &Item);
		PointCount += Item.m_NumPoints;
	}

	// save points
	const int TotalSize = Size * PointCount;
	if(TotalSize)
	{
		unsigned char *pPoints = (unsigned char *)mem_alloc(TotalSize);
		int Offset = 0;
		for(int e = 0; e < m_lEnvelopes.size(); e++)
		{
			for(int p = 0; p < m_lEnvelopes[e]->m_lPoints.size(); p++)
			{
				mem_copy(pPoints + Offset, &(m_lEnvelopes[e]->m_lPoints[p]), Size);
				Offset += Size;
			}
		}

		df.AddItem(MAPITEMTYPE_ENVPOINTS, 0, TotalSize, pPoints);
		mem_free(pPoints);
	}

	// finish the data file
	df.Finish();
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving done");

	// send rcon.. if we can
	if(m_pEditor->Client()->RconAuthed())
	{
		CServerInfo CurrentServerInfo;
		m_pEditor->Client()->GetServerInfo(&CurrentServerInfo);
		char aMapName[128];
		m_pEditor->ExtractName(pFileName, aMapName, sizeof(aMapName));
		if(!str_comp(aMapName, CurrentServerInfo.m_aMap))
			m_pEditor->Client()->Rcon("reload");
	}

	return 1;
}

int CEditor::Load(const char *pFileName, int StorageType)
{
	Reset();
	return m_Map.Load(Kernel()->RequestInterface<IStorage>(), pFileName, StorageType);
}

void CEditor::LoadCurrentMap()
{
	CallbackOpenMap(m_pClient->GetCurrentMapPath(), IStorage::TYPE_ALL, this);
}

int CEditorMap::Load(class IStorage *pStorage, const char *pFileName, int StorageType)
{
	IEngineMap *pMap = CreateEngineMap();
	if(!pMap->Load(pFileName, pStorage, StorageType))
	{
		delete pMap;
		return 0;
	}

	Clean();

	// load map info
	{
		int ItemSize;
		const CMapItemInfo *pItem = static_cast<CMapItemInfo *>(pMap->FindItem(MAPITEMTYPE_INFO, 0, 0x0, &ItemSize));
		if(CMapItemChecker::IsValid(pItem, ItemSize))
		{
			if(pItem->m_Author > -1)
				pMap->GetDataString(pItem->m_Author, m_MapInfo.m_aAuthor, sizeof(m_MapInfo.m_aAuthor));
			if(pItem->m_MapVersion > -1)
				pMap->GetDataString(pItem->m_MapVersion, m_MapInfo.m_aVersion, sizeof(m_MapInfo.m_aVersion));
			if(pItem->m_Credits > -1)
				pMap->GetDataString(pItem->m_Credits, m_MapInfo.m_aCredits, sizeof(m_MapInfo.m_aCredits));
			if(pItem->m_License > -1)
				pMap->GetDataString(pItem->m_License, m_MapInfo.m_aLicense, sizeof(m_MapInfo.m_aLicense));
		}
	}

	// load images
	{
		int Start, Num;
		pMap->GetType(MAPITEMTYPE_IMAGE, &Start, &Num);
		for(int i = 0; i < Num; i++)
		{
			int ItemSize;
			const CMapItemImage *pItem = static_cast<CMapItemImage *>(pMap->GetItem(Start + i, 0x0, 0x0, &ItemSize));
			if(!CMapItemChecker::IsValid(pItem, ItemSize))
				continue;

			// copy base info
			CEditorImage *pImg = new CEditorImage(m_pEditor);
			pImg->m_External = pItem->m_External != 0;
			pMap->GetDataString(pItem->m_ImageName, pImg->m_aName, sizeof(pImg->m_aName));

			const int Format = pItem->m_Version < CMapItemImage_v2::CURRENT_VERSION ? CImageInfo::FORMAT_RGBA : pItem->m_Format;
			if(pImg->m_External || (Format != CImageInfo::FORMAT_RGB && Format != CImageInfo::FORMAT_RGBA))
			{
				// load external
				char aBuf[IO_MAX_PATH_LENGTH];
				str_format(aBuf, sizeof(aBuf), "mapres/%s.png", pImg->m_aName);
				if(m_pEditor->Graphics()->LoadPNG(pImg, aBuf, IStorage::TYPE_ALL))
				{
					pImg->m_Texture = m_pEditor->Graphics()->LoadTextureRaw(
						pImg->m_Width, pImg->m_Height, pImg->m_Format, pImg->m_pData,
						CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);
					pImg->m_External = 1;
				}
			}
			else
			{
				pImg->m_Width = pItem->m_Width;
				pImg->m_Height = pItem->m_Height;
				pImg->m_Format = Format;

				// copy image data
				const int DataSize = pMap->GetDataSize(pItem->m_ImageData);
				if(!DataSize || DataSize != pImg->m_Width * pImg->m_Height * pImg->GetPixelSize())
				{
					delete pImg;
					continue;
				}
				const void *pData = pMap->GetData(pItem->m_ImageData);
				if(!pData)
				{
					delete pImg;
					continue;
				}
				pImg->m_pData = mem_alloc(DataSize);
				mem_copy(pImg->m_pData, pData, DataSize);
				pImg->m_Texture = m_pEditor->Graphics()->LoadTextureRaw(
					pImg->m_Width, pImg->m_Height, pImg->m_Format, pImg->m_pData,
					CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_MULTI_DIMENSION);
				pMap->UnloadData(pItem->m_ImageData);
			}

			pImg->LoadAutoMapper();

			m_lImages.add(pImg);
		}
	}

	// load groups
	{
		int LayersStart, LayersNum;
		pMap->GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
		int GroupsStart, GroupsNum;
		pMap->GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);

		for(int g = 0; g < GroupsNum; g++)
		{
			int GroupItemSize;
			const CMapItemGroup *pGItem = static_cast<CMapItemGroup *>(pMap->GetItem(GroupsStart + g, 0x0, 0x0, &GroupItemSize));
			if(!CMapItemChecker::IsValid(pGItem, GroupItemSize))
				continue;

			CLayerGroup *pGroup = NewGroup();
			pGroup->m_ParallaxX = clamp<int>(pGItem->m_ParallaxX, CProperty::MIN, CProperty::MAX);
			pGroup->m_ParallaxY = clamp<int>(pGItem->m_ParallaxY, CProperty::MIN, CProperty::MAX);
			pGroup->m_OffsetX = clamp<int>(pGItem->m_OffsetX, CProperty::MIN, CProperty::MAX);
			pGroup->m_OffsetY = clamp<int>(pGItem->m_OffsetY, CProperty::MIN, CProperty::MAX);

			if(pGItem->m_Version >= CMapItemGroup_v2::CURRENT_VERSION)
			{
				pGroup->m_UseClipping = pGItem->m_UseClipping != 0;
				pGroup->m_ClipX = clamp<int>(pGItem->m_ClipX, CProperty::MIN, CProperty::MAX);
				pGroup->m_ClipY = clamp<int>(pGItem->m_ClipY, CProperty::MIN, CProperty::MAX);
				pGroup->m_ClipW = clamp<int>(pGItem->m_ClipW, CProperty::MIN, CProperty::MAX);
				pGroup->m_ClipH = clamp<int>(pGItem->m_ClipH, CProperty::MIN, CProperty::MAX);
			}

			// load group name
			if(pGItem->m_Version >= CMapItemGroup_v3::CURRENT_VERSION)
				IntsToStr(pGItem->m_aName, sizeof(pGroup->m_aName) / sizeof(int), pGroup->m_aName);

			for(int l = 0; l < minimum(pGItem->m_NumLayers, LayersNum); l++)
			{
				if(pGItem->m_StartLayer >= LayersNum)
					continue;
				const int LayerIndex = pGItem->m_StartLayer + l;
				if(LayerIndex < 0 || LayerIndex >= LayersNum)
					continue;
				int LayerItemSize;
				const CMapItemLayer *pLayerItem = static_cast<CMapItemLayer *>(pMap->GetItem(LayersStart + LayerIndex, 0x0, 0x0, &LayerItemSize));
				if(!CMapItemChecker::IsValid(pLayerItem, LayerItemSize))
					continue;

				if(pLayerItem->m_Type == LAYERTYPE_TILES)
				{
					const CMapItemLayerTilemap *pTilemapItem = reinterpret_cast<const CMapItemLayerTilemap *>(pLayerItem);

					CLayerTiles *pTiles = 0;
					if(pTilemapItem->m_Flags & TILESLAYERFLAG_GAME)
					{
						pTiles = new CLayerGame(pTilemapItem->m_Width, pTilemapItem->m_Height);
						MakeGameLayer(pTiles);
						MakeGameGroup(pGroup);
					}
					else
					{
						pTiles = new CLayerTiles(pTilemapItem->m_Width, pTilemapItem->m_Height);
						pTiles->m_pEditor = m_pEditor;
						pTiles->m_Color = pTilemapItem->m_Color;
						pTiles->m_ColorEnv = pTilemapItem->m_ColorEnv;
						pTiles->m_ColorEnvOffset = pTilemapItem->m_ColorEnvOffset;
					}

					pTiles->m_Flags = pLayerItem->m_Flags;
					pGroup->AddLayer(pTiles);

					pTiles->m_Image = pTilemapItem->m_Image;
					pTiles->m_Game = pTilemapItem->m_Flags & TILESLAYERFLAG_GAME;

					// load layer name
					if(pTilemapItem->m_Version >= CMapItemLayerTilemap_v3::CURRENT_VERSION)
						IntsToStr(pTilemapItem->m_aName, sizeof(pTiles->m_aName) / sizeof(int), pTiles->m_aName);

					// get tile data
					const int DataSize = minimum(pMap->GetDataSize(pTilemapItem->m_Data), pTiles->m_Width * pTiles->m_Height * (int)sizeof(CTile));
					const CTile *pData = static_cast<CTile *>(pMap->GetData(pTilemapItem->m_Data));
					if(DataSize && pData)
					{
						mem_copy(pTiles->m_pTiles, pData, DataSize);
						pMap->UnloadData(pTilemapItem->m_Data);
					}
				}
				else if(pLayerItem->m_Type == LAYERTYPE_QUADS)
				{
					const CMapItemLayerQuads *pQuadsItem = reinterpret_cast<const CMapItemLayerQuads *>(pLayerItem);

					CLayerQuads *pQuads = new CLayerQuads;
					pQuads->m_pEditor = m_pEditor;
					pQuads->m_Flags = pLayerItem->m_Flags;
					pQuads->m_Image = pQuadsItem->m_Image;
					if(pQuads->m_Image < -1 || pQuads->m_Image >= m_lImages.size())
						pQuads->m_Image = -1;

					// load layer name
					if(pQuadsItem->m_Version >= CMapItemLayerQuads_v2::CURRENT_VERSION)
						IntsToStr(pQuadsItem->m_aName, sizeof(pQuads->m_aName) / sizeof(int), pQuads->m_aName);

					pGroup->AddLayer(pQuads);
					pQuads->m_lQuads.set_size(pQuadsItem->m_NumQuads);
					const int ExpectedDataSize = pQuadsItem->m_NumQuads * sizeof(CQuad);
					mem_zero(pQuads->m_lQuads.base_ptr(), ExpectedDataSize);

					const int DataSize = minimum(pMap->GetDataSize(pQuadsItem->m_Data), ExpectedDataSize);
					const void *pData = pMap->GetDataSwapped(pQuadsItem->m_Data);
					if(DataSize && pData)
					{
						mem_copy(pQuads->m_lQuads.base_ptr(), pData, DataSize);
						pMap->UnloadData(pQuadsItem->m_Data);
					}
				}
			}
		}
	}

	// load envelopes
	do
	{
		int StartEnvPoints, NumEnvPointItems;
		pMap->GetType(MAPITEMTYPE_ENVPOINTS, &StartEnvPoints, &NumEnvPointItems);
		if(!NumEnvPointItems) // NumEnvPointItems is 0 or 1, as all points are packed into one item
			break;

		int EnvelopeItemVersion = -1;
		int StartEnv, NumEnv;
		pMap->GetType(MAPITEMTYPE_ENVELOPE, &StartEnv, &NumEnv);
		for(int Env = 0; Env < NumEnv; Env++)
		{
			int ItemSize;
			const CMapItemEnvelope *pItem = static_cast<CMapItemEnvelope *>(pMap->GetItem(StartEnv + Env, 0x0, 0x0, &ItemSize));
			if(CMapItemChecker::IsValid(pItem, ItemSize))
			{
				if(EnvelopeItemVersion == -1)
					EnvelopeItemVersion = pItem->m_Version;
				else if(EnvelopeItemVersion != pItem->m_Version)
					EnvelopeItemVersion = -2; // conflicting versions found
			}
		}
		if(EnvelopeItemVersion < 0)
			break;

		int PointsItemSize;
		const CEnvPoint *pPoints = static_cast<CEnvPoint *>(pMap->GetItem(StartEnvPoints, 0x0, 0x0, &PointsItemSize));
		int NumEnvPoints;
		if(!CMapItemChecker::IsValid(pPoints, PointsItemSize, EnvelopeItemVersion, &NumEnvPoints) || !NumEnvPoints)
			break;

		for(int Env = 0; Env < NumEnv; Env++)
		{
			int ItemSize;
			const CMapItemEnvelope *pItem = static_cast<CMapItemEnvelope *>(pMap->GetItem(StartEnv + Env, 0x0, 0x0, &ItemSize));
			if(!CMapItemChecker::IsValid(pItem, ItemSize) || pItem->m_NumPoints < 0 || pItem->m_NumPoints > NumEnvPoints)
				continue;

			CEnvelope *pEnv = new CEnvelope(pItem->m_Channels);
			pEnv->m_lPoints.set_size(pItem->m_NumPoints);
			mem_zero(pEnv->m_lPoints.base_ptr(), pItem->m_NumPoints * sizeof(CEnvPoint));

			for(int Point = 0; Point < pItem->m_NumPoints; Point++)
			{
				const int PointIndex = pItem->m_StartPoint + Point;
				if(PointIndex < 0 || PointIndex >= NumEnvPoints)
					continue;

				if(EnvelopeItemVersion >= CMapItemEnvelope_v3::CURRENT_VERSION)
				{
					pEnv->m_lPoints[Point] = pPoints[PointIndex];
				}
				else
				{
					// backwards compatibility
					const CEnvPoint_v1 *pEnvPoint_v1 = &(reinterpret_cast<const CEnvPoint_v1 *>(pPoints))[PointIndex];

					pEnv->m_lPoints[Point].m_Time = pEnvPoint_v1->m_Time;
					pEnv->m_lPoints[Point].m_Curvetype = pEnvPoint_v1->m_Curvetype;

					for(int c = 0; c < minimum(pItem->m_Channels, pEnv->GetChannels()); c++)
					{
						pEnv->m_lPoints[Point].m_aValues[c] = pEnvPoint_v1->m_aValues[c];
					}
				}
			}

			if(pItem->m_aName[0] != -1)	// compatibility with old maps
				IntsToStr(pItem->m_aName, sizeof(pItem->m_aName) / sizeof(int), pEnv->m_aName);

			if(pItem->m_Version >= CMapItemEnvelope_v2::CURRENT_VERSION)
				pEnv->m_Synchronized = pItem->m_Synchronized;

			m_lEnvelopes.add(pEnv);
		}
	} while(false);

	delete pMap;
	return 1;
}

static int gs_ModifyAddAmount = 0;
static void ModifyAdd(int *pIndex)
{
	if(*pIndex >= 0)
		*pIndex += gs_ModifyAddAmount;
}

int CEditor::Append(const char *pFileName, int StorageType)
{
	CEditorMap NewMap;
	NewMap.m_pEditor = this;

	int Err;
	Err = NewMap.Load(Kernel()->RequestInterface<IStorage>(), pFileName, StorageType);
	if(!Err)
		return Err;

	// modify indecies
	gs_ModifyAddAmount = m_Map.m_lImages.size();
	NewMap.ModifyImageIndex(ModifyAdd);

	gs_ModifyAddAmount = m_Map.m_lEnvelopes.size();
	NewMap.ModifyEnvelopeIndex(ModifyAdd);

	// transfer images
	for(int i = 0; i < NewMap.m_lImages.size(); i++)
		m_Map.m_lImages.add(NewMap.m_lImages[i]);
	NewMap.m_lImages.clear();

	// transfer envelopes
	for(int i = 0; i < NewMap.m_lEnvelopes.size(); i++)
		m_Map.m_lEnvelopes.add(NewMap.m_lEnvelopes[i]);
	NewMap.m_lEnvelopes.clear();

	// transfer groups

	for(int i = 0; i < NewMap.m_lGroups.size(); i++)
	{
		if(NewMap.m_lGroups[i] == NewMap.m_pGameGroup)
			delete NewMap.m_lGroups[i];
		else
		{
			NewMap.m_lGroups[i]->m_pMap = &m_Map;
			m_Map.m_lGroups.add(NewMap.m_lGroups[i]);
		}
	}
	NewMap.m_lGroups.clear();

	// all done \o/
	return 0;
}
