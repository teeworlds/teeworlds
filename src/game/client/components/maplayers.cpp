#include <stdio.h>
#include <engine/graphics.h>
#include <engine/keys.h> //temp
#include <engine/shared/config.h>

#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/render.h>

#include <game/client/components/camera.h>
#include <game/client/components/mapimages.h>


#include "maplayers.h"

CMapLayers::CMapLayers(int t)
{
	m_Type = t;
	m_pLayers = 0;
}

void CMapLayers::OnInit()
{
	m_pLayers = Layers();
}


void CMapLayers::MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup)
{
	float Points[4];
	RenderTools()->MapscreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), 1.0f, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

void CMapLayers::EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser)
{
	CMapLayers *pThis = (CMapLayers *)pUser;
	pChannels[0] = 0;
	pChannels[1] = 0;
	pChannels[2] = 0;
	pChannels[3] = 0;

	CEnvPoint *pPoints = 0;

	{
		int Start, Num;
		pThis->m_pLayers->Map()->GetType(MAPITEMTYPE_ENVPOINTS, &Start, &Num);
		if(Num)
			pPoints = (CEnvPoint *)pThis->m_pLayers->Map()->GetItem(Start, 0, 0);
	}
	
	int Start, Num;
	pThis->m_pLayers->Map()->GetType(MAPITEMTYPE_ENVELOPE, &Start, &Num);
	
	if(Env >= Num)
		return;
	
	CMapItemEnvelope *pItem = (CMapItemEnvelope *)pThis->m_pLayers->Map()->GetItem(Start+Env, 0, 0);
	pThis->RenderTools()->RenderEvalEnvelope(pPoints+pItem->m_StartPoint, pItem->m_NumPoints, 4, pThis->Client()->LocalTime()+TimeOffset, pChannels);
}

void CMapLayers::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	
	CUIRect Screen;
	Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);
	
	vec2 Center = m_pClient->m_pCamera->m_Center;
	//float center_x = gameclient.camera->center.x;
	//float center_y = gameclient.camera->center.y;
	
	bool PassedGameLayer = false;
	
	for(int g = 0; g < m_pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = m_pLayers->GetGroup(g);
		
		if(!g_Config.m_GfxNoclip && pGroup->m_Version >= 2 && pGroup->m_UseClipping)
		{
			// set clipping
			float Points[4];
			MapScreenToGroup(Center.x, Center.y, m_pLayers->GameGroup());
			Graphics()->GetScreen(&Points[0], &Points[1], &Points[2], &Points[3]);
			float x0 = (pGroup->m_ClipX - Points[0]) / (Points[2]-Points[0]);
			float y0 = (pGroup->m_ClipY - Points[1]) / (Points[3]-Points[1]);
			float x1 = ((pGroup->m_ClipX+pGroup->m_ClipW) - Points[0]) / (Points[2]-Points[0]);
			float y1 = ((pGroup->m_ClipY+pGroup->m_ClipH) - Points[1]) / (Points[3]-Points[1]);
			
			Graphics()->ClipEnable((int)(x0*Graphics()->ScreenWidth()), (int)(y0*Graphics()->ScreenHeight()),
				(int)((x1-x0)*Graphics()->ScreenWidth()), (int)((y1-y0)*Graphics()->ScreenHeight()));
		}		
		
		MapScreenToGroup(Center.x, Center.y, pGroup);
		
		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = m_pLayers->GetLayer(pGroup->m_StartLayer+l);
			bool Render = false;
			bool IsGameLayer = false;
			
			if(pLayer == (CMapItemLayer*)m_pLayers->GameLayer())
			{
				IsGameLayer = true;
				PassedGameLayer = 1;
			}
			
			// skip rendering if detail layers if not wanted
			if(pLayer->m_Flags&LAYERFLAG_DETAIL && !g_Config.m_GfxHighDetail && !IsGameLayer)
				continue;
				
			if(m_Type == -1)
				Render = true;
			else if(m_Type == 0)
			{
				if(PassedGameLayer)
					return;
				Render = true;
			}
			else
			{
				if(PassedGameLayer && !IsGameLayer)
					Render = true;
			}
			
			if(pLayer->m_Type == LAYERTYPE_TILES && Input()->KeyPressed(KEY_KP0))
			{
				CMapItemLayerTilemap *pTMap = (CMapItemLayerTilemap *)pLayer;
				CTile *pTiles = (CTile *)m_pLayers->Map()->GetData(pTMap->m_Data);
				char buf[256];
				str_format(buf, sizeof(buf), "%d%d_%dx%d", g, l, pTMap->m_Width, pTMap->m_Height);
				FILE *f = fopen(buf, "w");
				for(int y = 0; y < pTMap->m_Height; y++)
				{
					for(int x = 0; x < pTMap->m_Width; x++)
						fprintf(f, "%d,", pTiles[y*pTMap->m_Width + x].m_Index);
					fprintf(f, "\n");
				}
				fclose(f);
			}			
			
			if(Render && !IsGameLayer)
			{
				//layershot_begin();
				
				if(pLayer->m_Type == LAYERTYPE_TILES)
				{
					CMapItemLayerTilemap *pTMap = (CMapItemLayerTilemap *)pLayer;
					if(pTMap->m_Image == -1)
						Graphics()->TextureSet(-1);
					else
						Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pTMap->m_Image));
						
					CTile *pTiles = (CTile *)m_pLayers->Map()->GetData(pTMap->m_Data);
					Graphics()->BlendNone();
					RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, vec4(1,1,1,1), TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE);
					Graphics()->BlendNormal();
					RenderTools()->RenderTilemap(pTiles, pTMap->m_Width, pTMap->m_Height, 32.0f, vec4(1,1,1,1), TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT);
				}
				else if(pLayer->m_Type == LAYERTYPE_QUADS)
				{
					CMapItemLayerQuads *pQLayer = (CMapItemLayerQuads *)pLayer;
					if(pQLayer->m_Image == -1)
						Graphics()->TextureSet(-1);
					else
						Graphics()->TextureSet(m_pClient->m_pMapimages->Get(pQLayer->m_Image));

					CQuad *pQuads = (CQuad *)m_pLayers->Map()->GetDataSwapped(pQLayer->m_Data);
					
					Graphics()->BlendNone();
					RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_OPAQUE, EnvelopeEval, this);
					Graphics()->BlendNormal();
					RenderTools()->RenderQuads(pQuads, pQLayer->m_NumQuads, LAYERRENDERFLAG_TRANSPARENT, EnvelopeEval, this);
				}
				
				//layershot_end();	
			}
		}
		if(!g_Config.m_GfxNoclip)
			Graphics()->ClipDisable();
	}
	
	if(!g_Config.m_GfxNoclip)
		Graphics()->ClipDisable();
	
	// reset the screen like it was before
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

