/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/tl/array.h>

#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/demo.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/render.h>

#include "camera.h"
#include "mapimages.h"
#include "menus.h"
#include "maplayers.h"

CMapLayers::CMapLayers(int Type)
{
	m_Type = Type;
	m_pMenuMap = 0;
	m_pMenuLayers = 0;
	m_OnlineStartTime = 0;
}

void CMapLayers::OnStateChange(int NewState, int OldState)
{
	if(NewState == IClient::STATE_ONLINE)
		m_OnlineStartTime = Client()->LocalTime(); // reset time for non-scynchronized envelopes
}

void CMapLayers::LoadBackgroundMap()
{
	const char *pMenuMap = Config()->m_ClMenuMap;
	if(str_comp(pMenuMap, "auto") == 0)
	{
		switch(time_season())
		{
			case SEASON_SPRING:
				pMenuMap = "heavens";
				break;
			case SEASON_SUMMER:
				pMenuMap = "jungle";
				break;
			case SEASON_AUTUMN:
				pMenuMap = "autumn";
				break;
			case SEASON_WINTER:
				pMenuMap = "winter";
				break;
			case SEASON_NEWYEAR:
				pMenuMap = "newyear";
				break;
		}
	}

	const int HourOfTheDay = time_houroftheday();
	const bool IsDaytime = HourOfTheDay >= 6 && HourOfTheDay < 18;

	char aBuf[128];
	// check for the appropriate day/night map
	str_format(aBuf, sizeof(aBuf), "ui/themes/%s_%s.map", pMenuMap, IsDaytime ? "day" : "night");
	if(!m_pMenuMap->Load(aBuf, m_pClient->Storage()))
	{
		// fall back on generic map
		str_format(aBuf, sizeof(aBuf), "ui/themes/%s.map", pMenuMap);
		if(!m_pMenuMap->Load(aBuf, m_pClient->Storage()))
		{
			// fall back on day/night alternative map
			str_format(aBuf, sizeof(aBuf), "ui/themes/%s_%s.map", pMenuMap, IsDaytime ? "night" : "day");
			if(!m_pMenuMap->Load(aBuf, m_pClient->Storage()))
			{
				str_format(aBuf, sizeof(aBuf), "map '%s' not found", pMenuMap);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
				return;
			}
		}
	}

	str_format(aBuf, sizeof(aBuf), "loaded map '%s'", pMenuMap);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);

	m_pMenuLayers->Init(Kernel(), m_pMenuMap);
	m_pClient->m_pMapimages->OnMenuMapLoad(m_pMenuMap);
	LoadEnvPoints(m_pMenuLayers, m_lEnvPointsMenu);
}

int CMapLayers::GetInitAmount() const
{
	if(m_Type == TYPE_BACKGROUND)
		return 1 + (Config()->m_ClShowMenuMap ? 14 : 0);
	return 0;
}

void CMapLayers::OnInit()
{
	if(m_Type == TYPE_BACKGROUND)
	{
		m_pMenuLayers = new CLayers;
		m_pMenuMap = CreateEngineMap();
		m_pClient->m_pMenus->RenderLoading(1);
		if(Config()->m_ClShowMenuMap)
		{
			LoadBackgroundMap();
			m_pClient->m_pMenus->RenderLoading(14);
		}
	}

	m_pEggTiles = 0;
}

void CMapLayers::OnMapLoad()
{
	if(Layers())
	{
		LoadEnvPoints(Layers(), m_lEnvPoints);

		// easter time, place eggs
		if(m_pClient->IsEaster())
			PlaceEasterEggs(Layers());
	}
}

void CMapLayers::OnShutdown()
{
	if(m_pEggTiles)
	{
		mem_free(m_pEggTiles);
		m_pEggTiles = 0;
	}
}

void CMapLayers::LoadEnvPoints(const CLayers *pLayers, array<CEnvPoint>& lEnvPoints)
{
	lEnvPoints.clear();

	int StartEnvPoints, NumEnvPointItems;
	pLayers->Map()->GetType(MAPITEMTYPE_ENVPOINTS, &StartEnvPoints, &NumEnvPointItems);
	if(!NumEnvPointItems) // NumEnvPointItems is 0 or 1, as all points are packed into one item
		return;

	int EnvelopeItemVersion = -1;
	int StartEnv, NumEnv;
	pLayers->Map()->GetType(MAPITEMTYPE_ENVELOPE, &StartEnv, &NumEnv);
	for(int Env = 0; Env < NumEnv; Env++)
	{
		int ItemSize;
		const CMapItemEnvelope *pItem = static_cast<CMapItemEnvelope *>(pLayers->Map()->GetItem(StartEnv + Env, 0x0, 0x0, &ItemSize));
		if(CMapItemChecker::IsValid(pItem, ItemSize))
		{
			if(EnvelopeItemVersion == -1)
				EnvelopeItemVersion = pItem->m_Version;
			else if(EnvelopeItemVersion != pItem->m_Version)
				EnvelopeItemVersion = -2; // conflicting versions found
		}
	}
	if(EnvelopeItemVersion < 0)
		return;

	int PointsItemSize;
	const CEnvPoint *pPoints = static_cast<CEnvPoint *>(pLayers->Map()->GetItem(StartEnvPoints, 0x0, 0x0, &PointsItemSize));
	int NumEnvPoints;
	if(!CMapItemChecker::IsValid(pPoints, PointsItemSize, EnvelopeItemVersion, &NumEnvPoints) || !NumEnvPoints)
		return;

	for(int Env = 0; Env < NumEnv; Env++)
	{
		int ItemSize;
		const CMapItemEnvelope *pItem = static_cast<CMapItemEnvelope *>(pLayers->Map()->GetItem(StartEnv + Env, 0x0, 0x0, &ItemSize));
		if(!CMapItemChecker::IsValid(pItem, ItemSize) || pItem->m_NumPoints <= 0 || pItem->m_NumPoints > NumEnvPoints)
			continue;

		for(int Point = 0; Point < pItem->m_NumPoints; Point++)
		{
			const int PointIndex = pItem->m_StartPoint + Point;
			if(PointIndex < 0 || PointIndex >= NumEnvPoints)
				continue;

			if(EnvelopeItemVersion >= CMapItemEnvelope_v3::CURRENT_VERSION)
			{
				lEnvPoints.add(pPoints[PointIndex]);
			}
			else
			{
				// backwards compatibility
				// convert CEnvPoint_v1 -> CEnvPoint
				const CEnvPoint_v1 *pEnvPoint_v1 = &(reinterpret_cast<const CEnvPoint_v1 *>(pPoints))[PointIndex];

				CEnvPoint EnvPoint;
				mem_zero(&EnvPoint, sizeof(CEnvPoint));
				EnvPoint.m_Time = pEnvPoint_v1->m_Time;
				EnvPoint.m_Curvetype = pEnvPoint_v1->m_Curvetype;

				for(int c = 0; c < minimum(pItem->m_Channels, (int)CEnvPoint::MAX_CHANNELS); c++)
				{
					EnvPoint.m_aValues[c] = pEnvPoint_v1->m_aValues[c];
				}

				lEnvPoints.add(EnvPoint);
			}
		}
	}
}

void CMapLayers::EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser)
{
	CMapLayers *pThis = (CMapLayers *)pUser;

	for(int c = 0; c < CEnvPoint::MAX_CHANNELS; c++)
		pChannels[c] = 0;

	CLayers *pLayers;
	CEnvPoint *pPoints;
	int NumPoints;
	if(pThis->Client()->State() == IClient::STATE_ONLINE || pThis->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		pLayers = pThis->Layers();
		pPoints = pThis->m_lEnvPoints.base_ptr();
		NumPoints = pThis->m_lEnvPoints.size();
	}
	else
	{
		pLayers = pThis->m_pMenuLayers;
		pPoints = pThis->m_lEnvPointsMenu.base_ptr();
		NumPoints = pThis->m_lEnvPointsMenu.size();
	}

	int StartEnv, NumEnv;
	pLayers->Map()->GetType(MAPITEMTYPE_ENVELOPE, &StartEnv, &NumEnv);
	if(Env >= NumEnv)
		return;

	int ItemSize;
	const CMapItemEnvelope *pItem = static_cast<CMapItemEnvelope *>(pLayers->Map()->GetItem(StartEnv + Env, 0x0, 0x0, &ItemSize));
	if(!CMapItemChecker::IsValid(pItem, ItemSize) || pItem->m_StartPoint < 0 || pItem->m_StartPoint >= NumPoints || pItem->m_NumPoints < 0 || pItem->m_StartPoint + pItem->m_NumPoints > NumPoints)
		return;

	const CEnvPoint *pItemPoints = pPoints + pItem->m_StartPoint;

	static float s_Time = 0.0f;
	const float EnvelopTicks = (pItemPoints[pItem->m_NumPoints - 1].m_Time - pItemPoints[0].m_Time) / 1000.0f * pThis->Client()->GameTickSpeed();
	if(pThis->Client()->State() == IClient::STATE_ONLINE || pThis->Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		if(pThis->m_pClient->m_Snap.m_pGameData && !pThis->m_pClient->IsWorldPaused())
		{
			if(pItem->m_Version < 2 || pItem->m_Synchronized)
			{
				float PrevAnimationTick = fmod(pThis->Client()->PrevGameTick() - pThis->m_pClient->m_Snap.m_pGameData->m_GameStartTick, EnvelopTicks);
				float CurAnimationTick = fmod(pThis->Client()->GameTick() - pThis->m_pClient->m_Snap.m_pGameData->m_GameStartTick, EnvelopTicks);
				if(PrevAnimationTick > CurAnimationTick)
					CurAnimationTick += EnvelopTicks;
				s_Time = mix(PrevAnimationTick, CurAnimationTick, pThis->Client()->IntraGameTick()) / pThis->Client()->GameTickSpeed();
			}
			else
				s_Time = pThis->Client()->LocalTime() - pThis->m_OnlineStartTime;
		}
	}
	else
	{
		s_Time = pThis->Client()->LocalTime();
	}

	CRenderTools::RenderEvalEnvelope(pItemPoints, pItem->m_NumPoints, CEnvPoint::MAX_CHANNELS, s_Time + TimeOffset, pChannels);
}

void CMapLayers::OnRender()
{
	CLayers *pLayers = 0;
	if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pLayers = Layers();
	else if(m_pMenuMap && m_pMenuMap->IsLoaded())
		pLayers = m_pMenuLayers;

	if(!pLayers)
		return;

	CUIRect Screen;
	Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);

	vec2 Center = *m_pClient->m_pCamera->GetCenter();

	const bool DumpTileLayers = Input()->KeyIsPressed(KEY_LCTRL) && Input()->KeyIsPressed(KEY_LSHIFT) && UI()->KeyPress(KEY_KP_0);
	bool PassedGameLayer = false;

	for(int g = 0; g < pLayers->NumGroups(); g++)
	{
		const CMapItemGroup *pGroup = pLayers->GetGroup(g);
		if(!pGroup || pGroup->m_StartLayer >= pLayers->NumLayers())
			continue;

		if(!Config()->m_GfxNoclip && pGroup->m_Version >= 2 && pGroup->m_UseClipping && Layers()->GameGroup())
		{
			// set clipping
			float Points[4];
			RenderTools()->MapScreenToGroup(Center.x, Center.y, pLayers->GameGroup(), m_pClient->m_pCamera->GetZoom());
			Graphics()->GetScreen(&Points[0], &Points[1], &Points[2], &Points[3]);
			float x0 = (pGroup->m_ClipX - Points[0]) / (Points[2]-Points[0]);
			float y0 = (pGroup->m_ClipY - Points[1]) / (Points[3]-Points[1]);
			float x1 = ((pGroup->m_ClipX+pGroup->m_ClipW) - Points[0]) / (Points[2]-Points[0]);
			float y1 = ((pGroup->m_ClipY+pGroup->m_ClipH) - Points[1]) / (Points[3]-Points[1]);

			if(x1 < 0.0f || x0 > 1.0f || y1 < 0.0f || y0 > 1.0f)
				continue;

			Graphics()->ClipEnable((int)(x0*Graphics()->ScreenWidth()), (int)(y0*Graphics()->ScreenHeight()),
				(int)((x1-x0)*Graphics()->ScreenWidth()), (int)((y1-y0)*Graphics()->ScreenHeight()));
		}

		RenderTools()->MapScreenToGroup(Center.x, Center.y, pGroup, m_pClient->m_pCamera->GetZoom());

		for(int l = 0; l < minimum(pGroup->m_NumLayers, pLayers->NumLayers()); l++)
		{
			const CMapItemLayer *pLayer = pLayers->GetLayer(pGroup->m_StartLayer + l);
			if(!pLayer)
				continue;

			const bool IsGameLayer = pLayer == reinterpret_cast<const CMapItemLayer *>(pLayers->GameLayer());
			PassedGameLayer |= IsGameLayer;

			bool Render = false;
			if(m_Type == -1)
				Render = true;
			else if(m_Type == 0)
			{
				if(PassedGameLayer && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK))
					return;
				Render = true;
			}
			else if(PassedGameLayer && !IsGameLayer)
			{
				Render = true;
			}

			if(!Render)
				continue;

			// skip rendering if detail layers is not wanted
			if(!(pLayer->m_Flags&LAYERFLAG_DETAIL && !Config()->m_GfxHighDetail && !IsGameLayer && (Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)))
			{
				if(pLayer->m_Type == LAYERTYPE_TILES && DumpTileLayers)
				{
					const CMapItemLayerTilemap *pTMap = reinterpret_cast<const CMapItemLayerTilemap *>(pLayer);
					const int NumTiles = pLayers->Map()->GetDataSize(pTMap->m_Data) / sizeof(CTile);
					const CTile *pTiles = static_cast<CTile *>(pLayers->Map()->GetData(pTMap->m_Data));
					if(NumTiles != pTMap->m_Width * pTMap->m_Height || !pTiles)
						continue;
					CServerInfo CurrentServerInfo;
					Client()->GetServerInfo(&CurrentServerInfo);
					char aFilename[IO_MAX_PATH_LENGTH];
					str_format(aFilename, sizeof(aFilename), "dumps/tilelayer_dump_%s-%d-%d-%dx%d.txt", CurrentServerInfo.m_aMap, g, l, pTMap->m_Width, pTMap->m_Height);
					IOHANDLE File = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
					if(File)
					{
						for(int y = 0; y < pTMap->m_Height; y++)
						{
							for(int x = 0; x < pTMap->m_Width; x++)
								io_write(File, &(pTiles[y*pTMap->m_Width + x].m_Index), sizeof(pTiles[y*pTMap->m_Width + x].m_Index));
							io_write_newline(File);
						}
						io_close(File);
					}
				}

				if(!IsGameLayer)
				{
					if(pLayer->m_Type == LAYERTYPE_TILES)
					{
						const CMapItemLayerTilemap *pTMap = reinterpret_cast<const CMapItemLayerTilemap *>(pLayer);
						if(pTMap->m_Image < 0)
							Graphics()->TextureClear();
						else
							Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pTMap->m_Image));

						const int NumTiles = pLayers->Map()->GetDataSize(pTMap->m_Data) / sizeof(CTile);
						const CTile *pTiles = static_cast<CTile *>(pLayers->Map()->GetData(pTMap->m_Data));
						if(NumTiles != pTMap->m_Width * pTMap->m_Height || !pTiles)
							continue;

						Graphics()->BlendNone();
						const vec4 Color = vec4(pTMap->m_Color.r/255.0f, pTMap->m_Color.g/255.0f, pTMap->m_Color.b/255.0f, pTMap->m_Color.a/255.0f);
						RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE,
														EnvelopeEval, this, pTMap->m_ColorEnv, pTMap->m_ColorEnvOffset);
						Graphics()->BlendNormal();
						RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT,
														EnvelopeEval, this, pTMap->m_ColorEnv, pTMap->m_ColorEnvOffset);
					}
					else if(pLayer->m_Type == LAYERTYPE_QUADS)
					{
						const CMapItemLayerQuads *pQLayer = reinterpret_cast<const CMapItemLayerQuads *>(pLayer);
						if(pQLayer->m_Image < 0)
							Graphics()->TextureClear();
						else
							Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pQLayer->m_Image));

						const int NumQuads = pLayers->Map()->GetDataSize(pQLayer->m_Data) / sizeof(CQuad);
						const CQuad *pQuads = static_cast<CQuad *>(pLayers->Map()->GetDataSwapped(pQLayer->m_Data));
						if(NumQuads != pQLayer->m_NumQuads || !pQuads)
							continue;

						//Graphics()->BlendNone();
						//RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_OPAQUE, EnvelopeEval, this);
						Graphics()->BlendNormal();
						RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_TRANSPARENT, EnvelopeEval, this);
					}
				}
			}

			// eggs
			if(m_pClient->IsEaster())
			{
				if(m_pEggTiles && l + 1 < pGroup->m_NumLayers)
				{
					const CMapItemLayer *pNextLayer = pLayers->GetLayer(pGroup->m_StartLayer + l + 1);
					if(pNextLayer == reinterpret_cast<CMapItemLayer *>(pLayers->GameLayer()))
					{
						Graphics()->TextureSet(m_pClient->m_pMapimages->GetEasterTexture());
						Graphics()->BlendNormal();
						RenderTools()->RenderTilemap(m_pEggTiles, m_EggLayerWidth, m_EggLayerHeight, 32.0f, vec4(1,1,1,1), LAYERRENDERFLAG_TRANSPARENT, EnvelopeEval, this, -1, 0);
					}
				}
			}
		}
		if(!Config()->m_GfxNoclip)
			Graphics()->ClipDisable();
	}

	if(!Config()->m_GfxNoclip)
		Graphics()->ClipDisable();

	// reset the screen like it was before
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

void CMapLayers::ConchainBackgroundMap(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CMapLayers*)pUserData)->BackgroundMapUpdate();
}

void CMapLayers::OnConsoleInit()
{
	Console()->Chain("cl_menu_map", ConchainBackgroundMap, this);
	Console()->Chain("cl_show_menu_map", ConchainBackgroundMap, this);
}

void CMapLayers::BackgroundMapUpdate()
{
	if(m_Type == TYPE_BACKGROUND && m_pMenuMap)
	{
		// unload map
		m_pMenuMap->Unload();
		if(Config()->m_ClShowMenuMap)
			LoadBackgroundMap();
	}
}

static void PlaceEggDoodads(int LayerWidth, int LayerHeight, CTile *aOutTiles, const CTile *aGameLayerTiles, int ItemWidth, int ItemHeight, const int* aImageTileID, int ImageTileIDCount, int Freq)
{
	for(int y = 0; y < LayerHeight-ItemHeight; y++)
	{
		for(int x = 0; x < LayerWidth-ItemWidth; x++)
		{
			bool Overlap = false;
			bool ObstructedByWall = false;
			bool HasGround = true;

			for(int iy = 0; iy < ItemHeight; iy++)
			{
				for(int ix = 0; ix < ItemWidth; ix++)
				{
					int Tid = (y+iy) * LayerWidth + (x+ix);
					int DownTid = (y+iy+1) * LayerWidth + (x+ix);

					if(aOutTiles[Tid].m_Index != 0)
					{
						Overlap = true;
						break;
					}

					if(aGameLayerTiles[Tid].m_Index == 1)
					{
						ObstructedByWall = true;
						break;
					}

					if(iy == ItemHeight-1 && aGameLayerTiles[DownTid].m_Index != 1)
					{
						HasGround = false;
						break;
					}
				}
			}

			if(!Overlap && !ObstructedByWall && HasGround && random_int()%Freq == 0)
			{
				const int BaskerStartID = aImageTileID[random_int()%ImageTileIDCount];

				for(int iy = 0; iy < ItemHeight; iy++)
				{
					for(int ix = 0; ix < ItemWidth; ix++)
					{
						int Tid = (y+iy) * LayerWidth + (x+ix);
						aOutTiles[Tid].m_Index = BaskerStartID + iy * 16 + ix;
					}
				}
			}
		}
	}
}

void CMapLayers::PlaceEasterEggs(const CLayers *pLayers)
{
	if(m_pEggTiles)
	{
		mem_free(m_pEggTiles);
		m_pEggTiles = 0;
	}

	const CMapItemLayerTilemap *pGameLayer = pLayers->GameLayer();
	if(!pGameLayer)
		return;

	m_EggLayerWidth = pGameLayer->m_Width;
	m_EggLayerHeight = pGameLayer->m_Height;
	const int DataSize = pLayers->Map()->GetDataSize(pGameLayer->m_Data);
	const CTile *pGameLayerTiles = static_cast<CTile *>(pLayers->Map()->GetData(pGameLayer->m_Data));
	if(DataSize / (int)sizeof(CTile) != m_EggLayerWidth * m_EggLayerHeight || !pGameLayerTiles)
		return;

	m_pEggTiles = static_cast<CTile *>(mem_alloc(DataSize));
	mem_zero(m_pEggTiles, DataSize);

	// first pass: baskets
	static const int s_aBasketIDs[] = {
		38,
		86
	};

	static const int s_BasketCount = sizeof(s_aBasketIDs)/sizeof(s_aBasketIDs[0]);
	PlaceEggDoodads(m_EggLayerWidth, m_EggLayerHeight, m_pEggTiles, pGameLayerTiles, 3, 2, s_aBasketIDs, s_BasketCount, 250);

	// second pass: double eggs
	static const int s_aDoubleEggIDs[] = {
		9,
		25,
		41,
		57,
		73,
		89
	};

	static const int s_DoubleEggCount = sizeof(s_aDoubleEggIDs)/sizeof(s_aDoubleEggIDs[0]);
	PlaceEggDoodads(m_EggLayerWidth, m_EggLayerHeight, m_pEggTiles, pGameLayerTiles, 2, 1, s_aDoubleEggIDs, s_DoubleEggCount, 100);

	// third pass: eggs
	static const int s_aEggIDs[] = {
		1, 2, 3, 4, 5,
		17, 18, 19, 20,
		33, 34, 35, 36,
		49, 50,     52,
		65, 66,
			82,
			98
	};

	static const int s_EggCount = sizeof(s_aEggIDs)/sizeof(s_aEggIDs[0]);
	PlaceEggDoodads(m_EggLayerWidth, m_EggLayerHeight, m_pEggTiles, pGameLayerTiles, 1, 1, s_aEggIDs, s_EggCount, 30);
}
