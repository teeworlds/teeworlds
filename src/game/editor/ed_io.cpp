/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/client.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <game/gamecore.h>
#include "ed_editor.h"

template<typename T>
static int MakeVersion(int i, const T &v)
{ return (i<<16)+sizeof(T); }

// backwards compatiblity
/*
void editor_load_old(DATAFILE *df, MAP *map)
{
	class mapres_image
	{
	public:
		int width;
		int height;
		int image_data;
	};

	struct mapres_tilemap
	{
		int image;
		int width;
		int height;
		int x, y;
		int scale;
		int data;
		int main;
	};

	struct mapres_entity
	{
		int x, y;
		int data[1];
	};

	struct mapres_spawnpoint
	{
		int x, y;
	};

	struct mapres_item
	{
		int x, y;
		int type;
	};

	struct mapres_flagstand
	{
		int x, y;
	};

	enum
	{
		MAPRES_ENTS_START=1,
		MAPRES_SPAWNPOINT=1,
		MAPRES_ITEM=2,
		MAPRES_SPAWNPOINT_RED=3,
		MAPRES_SPAWNPOINT_BLUE=4,
		MAPRES_FLAGSTAND_RED=5,
		MAPRES_FLAGSTAND_BLUE=6,
		MAPRES_ENTS_END,
		
		ITEM_NULL=0,
		ITEM_WEAPON_GUN=0x00010001,
		ITEM_WEAPON_SHOTGUN=0x00010002,
		ITEM_WEAPON_ROCKET=0x00010003,
		ITEM_WEAPON_SNIPER=0x00010004,
		ITEM_WEAPON_HAMMER=0x00010005,
		ITEM_HEALTH =0x00020001,
		ITEM_ARMOR=0x00030001,
		ITEM_NINJA=0x00040001,
	};
	
	enum
	{
		MAPRES_REGISTERED=0x8000,
		MAPRES_IMAGE=0x8001,
		MAPRES_TILEMAP=0x8002,
		MAPRES_COLLISIONMAP=0x8003,
		MAPRES_TEMP_THEME=0x8fff,
	};

	// load tilemaps
	int game_width = 0;
	int game_height = 0;
	{
		int start, num;
		datafile_get_type(df, MAPRES_TILEMAP, &start, &num);
		for(int t = 0; t < num; t++)
		{
			mapres_tilemap *tmap = (mapres_tilemap *)datafile_get_item(df, start+t,0,0);
			
			CLayerTiles *l = new CLayerTiles(tmap->width, tmap->height);
			
			if(tmap->main)
			{
				// move game layer to correct position
				for(int i = 0; i < map->groups[0]->layers.len()-1; i++)
				{
					if(map->groups[0]->layers[i] == pEditor->map.game_layer)
						map->groups[0]->swap_layers(i, i+1);
				}
				
				game_width = tmap->width;
				game_height = tmap->height;
			}

			// add new layer
			map->groups[0]->add_layer(l);

			// process the data
			unsigned char *src_data = (unsigned char *)datafile_get_data(df, tmap->data);
			CTile *dst_data = l->tiles;
			
			for(int y = 0; y < tmap->height; y++)
				for(int x = 0; x < tmap->width; x++, dst_data++, src_data+=2)
				{
					dst_data->index = src_data[0];
					dst_data->flags = src_data[1];
				}
				
			l->image = tmap->image;
		}
	}
	
	// load images
	{
		int start, count;
		datafile_get_type(df, MAPRES_IMAGE, &start, &count);
		for(int i = 0; i < count; i++)
		{
			mapres_image *imgres = (mapres_image *)datafile_get_item(df, start+i, 0, 0);
			void *data = datafile_get_data(df, imgres->image_data);

			EDITOR_IMAGE *img = new EDITOR_IMAGE;
			img->width = imgres->width;
			img->height = imgres->height;
			img->format = CImageInfo::FORMAT_RGBA;
			
			// copy image data
			img->data = mem_alloc(img->width*img->height*4, 1);
			mem_copy(img->data, data, img->width*img->height*4);
			img->tex_id = Graphics()->LoadTextureRaw(img->width, img->height, img->format, img->data, CImageInfo::FORMAT_AUTO, 0);
			map->images.add(img);
			
			// unload image
			datafile_unload_data(df, imgres->image_data);
		}
	}
	
	// load entities
	{
		CLayerGame *g = map->game_layer;
		g->resize(game_width, game_height);
		for(int t = MAPRES_ENTS_START; t < MAPRES_ENTS_END; t++)
		{
			// fetch entities of this class
			int start, num;
			datafile_get_type(df, t, &start, &num);


			for(int i = 0; i < num; i++)
			{
				mapres_entity *e = (mapres_entity *)datafile_get_item(df, start+i,0,0);
				int x = e->x/32;
				int y = e->y/32;
				int id = -1;
					
				if(t == MAPRES_SPAWNPOINT) id = ENTITY_SPAWN;
				else if(t == MAPRES_SPAWNPOINT_RED) id = ENTITY_SPAWN_RED;
				else if(t == MAPRES_SPAWNPOINT_BLUE) id = ENTITY_SPAWN_BLUE;
				else if(t == MAPRES_FLAGSTAND_RED) id = ENTITY_FLAGSTAND_RED;
				else if(t == MAPRES_FLAGSTAND_BLUE) id = ENTITY_FLAGSTAND_BLUE;
				else if(t == MAPRES_ITEM)
				{
					if(e->data[0] == ITEM_WEAPON_SHOTGUN) id = ENTITY_WEAPON_SHOTGUN;
					else if(e->data[0] == ITEM_WEAPON_ROCKET) id = ENTITY_WEAPON_GRENADE;
					else if(e->data[0] == ITEM_NINJA) id = ENTITY_POWERUP_NINJA;
					else if(e->data[0] == ITEM_ARMOR) id = ENTITY_ARMOR_1;
					else if(e->data[0] == ITEM_HEALTH) id = ENTITY_HEALTH_1;
				}
						
				if(id > 0 && x >= 0 && x < g->width && y >= 0 && y < g->height)
					g->tiles[y*g->width+x].index = id+ENTITY_OFFSET;
			}
		}
	}
}*/

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
		Item.m_Version = 1;
		df.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Item), &Item);
	}

	// save images
	for(int i = 0; i < m_lImages.size(); i++)
	{
		CEditorImage *pImg = m_lImages[i];
		
		// analyse the image for when saving (should be done when we load the image)
		// TODO!
		pImg->AnalyseTileFlags();
		
		CMapItemImage Item;
		Item.m_Version = 1;
		
		Item.m_Width = pImg->m_Width;
		Item.m_Height = pImg->m_Height;
		Item.m_External = pImg->m_External;
		Item.m_ImageName = df.AddData(str_length(pImg->m_aName)+1, pImg->m_aName);
		if(pImg->m_External)
			Item.m_ImageData = -1;
		else
			Item.m_ImageData = df.AddData(Item.m_Width*Item.m_Height*4, pImg->m_pData);
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
				Item.m_Version = 2;
				
				Item.m_Layer.m_Flags = pLayer->m_Flags;
				Item.m_Layer.m_Type = pLayer->m_Type;
				
				Item.m_Color.r = 255; // not in use right now
				Item.m_Color.g = 255;
				Item.m_Color.b = 255;
				Item.m_Color.a = 255;
				Item.m_ColorEnv = -1;
				Item.m_ColorEnvOffset = 0;
				
				Item.m_Width = pLayer->m_Width;
				Item.m_Height = pLayer->m_Height;
				if(pLayer->m_Tele)
					Item.m_Flags = 2;
				else if(pLayer->m_Speedup)
					Item.m_Flags = 4;
				else if(pLayer->m_Front)
					Item.m_Flags = 8;
				else if(pLayer->m_Switch)
					Item.m_Flags = 16;
				else
					Item.m_Flags = pLayer->m_Game;
				Item.m_Image = pLayer->m_Image;
				if(pLayer->m_Tele)
				{
					CTile *Tiles = new CTile[pLayer->m_Width*pLayer->m_Height];
					mem_zero(Tiles, pLayer->m_Width*pLayer->m_Height*sizeof(CTile));
					Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), Tiles);
					Item.m_Tele = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTeleTile), ((CLayerTele *)pLayer)->m_pTeleTile);
					delete[] Tiles;
				}
				else if(pLayer->m_Speedup)
				{
					CTile *Tiles = new CTile[pLayer->m_Width*pLayer->m_Height];
					mem_zero(Tiles, pLayer->m_Width*pLayer->m_Height*sizeof(CTile));
					Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), Tiles);
					Item.m_Speedup = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CSpeedupTile), ((CLayerSpeedup *)pLayer)->m_pSpeedupTile);
					delete[] Tiles;
				}
				else if(pLayer->m_Front)
				{
					CTile *Tiles = new CTile[pLayer->m_Width*pLayer->m_Height];
					mem_zero(Tiles, pLayer->m_Width*pLayer->m_Height*sizeof(CTile));
					Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), Tiles);
					Item.m_Front = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), pLayer->m_pTiles);
					delete[] Tiles;
				}
				else if(pLayer->m_Switch)
				{
					CTile *Tiles = new CTile[pLayer->m_Width*pLayer->m_Height];
					mem_zero(Tiles, pLayer->m_Width*pLayer->m_Height*sizeof(CTile));
					Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), Tiles);
					Item.m_Switch = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CSwitchTile), ((CLayerSwitch *)pLayer)->m_pSwitchTile);
					delete[] Tiles;
				}
				else
					Item.m_Data = df.AddData(pLayer->m_Width*pLayer->m_Height*sizeof(CTile), pLayer->m_pTiles);
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
					Item.m_Version = 1;
					Item.m_Layer.m_Flags =  pLayer->m_Flags;
					Item.m_Layer.m_Type = pLayer->m_Type;
					Item.m_Image = pLayer->m_Image;
					
					// add the data
					Item.m_NumQuads = pLayer->m_lQuads.size();
					Item.m_Data = df.AddDataSwapped(pLayer->m_lQuads.size()*sizeof(CQuad), pLayer->m_lQuads.base_ptr());
					df.AddItem(MAPITEMTYPE_LAYER, LayerCount, sizeof(Item), &Item);
					
					// clean up
					//mem_free(quads);

					GItem.m_NumLayers++;
					LayerCount++;
				}
			}
		}
		
		df.AddItem(MAPITEMTYPE_GROUP, GroupCount++, sizeof(GItem), &GItem);
	}
	
	// save envelopes
	int PointCount = 0;
	for(int e = 0; e < m_lEnvelopes.size(); e++)
	{
		CMapItemEnvelope Item;
		Item.m_Version = 1;
		Item.m_Channels = m_lEnvelopes[e]->m_Channels;
		Item.m_StartPoint = PointCount;
		Item.m_NumPoints = m_lEnvelopes[e]->m_lPoints.size();
		StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), m_lEnvelopes[e]->m_aName);
		
		df.AddItem(MAPITEMTYPE_ENVELOPE, e, sizeof(Item), &Item);
		PointCount += Item.m_NumPoints;
	}
	
	// save points
	int TotalSize = sizeof(CEnvPoint) * PointCount;
	CEnvPoint *pPoints = (CEnvPoint *)mem_alloc(TotalSize, 1);
	PointCount = 0;
	
	for(int e = 0; e < m_lEnvelopes.size(); e++)
	{
		int Count = m_lEnvelopes[e]->m_lPoints.size();
		mem_copy(&pPoints[PointCount], m_lEnvelopes[e]->m_lPoints.base_ptr(), sizeof(CEnvPoint)*Count);
		PointCount += Count;
	}

	df.AddItem(MAPITEMTYPE_ENVPOINTS, 0, TotalSize, pPoints);
	
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

int CEditorMap::Load(class IStorage *pStorage, const char *pFileName, int StorageType)
{
	CDataFileReader DataFile;
	//DATAFILE *df = datafile_load(filename);
	if(!DataFile.Open(pStorage, pFileName, StorageType))
		return 0;
		
	Clean();

	// check version
	CMapItemVersion *pItem = (CMapItemVersion *)DataFile.FindItem(MAPITEMTYPE_VERSION, 0);
	if(!pItem)
	{
		// import old map
		/*MAP old_mapstuff;
		editor->reset();
		editor_load_old(df, this);
		*/
	}
	else if(pItem->m_Version == 1)
	{
		//editor.reset(false);
		
		// load images
		{
			int Start, Num;
			DataFile.GetType( MAPITEMTYPE_IMAGE, &Start, &Num);
			for(int i = 0; i < Num; i++)
			{
				CMapItemImage *pItem = (CMapItemImage *)DataFile.GetItem(Start+i, 0, 0);
				char *pName = (char *)DataFile.GetData(pItem->m_ImageName);

				// copy base info				
				CEditorImage *pImg = new CEditorImage(m_pEditor);
				pImg->m_External = pItem->m_External;

				if(pItem->m_External)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf),"mapres/%s.png", pName);
					
					// load external
					CEditorImage ImgInfo(m_pEditor);
					if(m_pEditor->Graphics()->LoadPNG(&ImgInfo, aBuf, IStorage::TYPE_ALL))
					{
						*pImg = ImgInfo;
						pImg->m_TexId = m_pEditor->Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format, ImgInfo.m_pData, CImageInfo::FORMAT_AUTO, 0);
						pImg->m_External = 1;
					}
				}
				else
				{
					pImg->m_Width = pItem->m_Width;
					pImg->m_Height = pItem->m_Height;
					pImg->m_Format = CImageInfo::FORMAT_RGBA;
					
					// copy image data
					void *pData = DataFile.GetData(pItem->m_ImageData);
					pImg->m_pData = mem_alloc(pImg->m_Width*pImg->m_Height*4, 1);
					mem_copy(pImg->m_pData, pData, pImg->m_Width*pImg->m_Height*4);
					pImg->m_TexId = m_pEditor->Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, pImg->m_Format, pImg->m_pData, CImageInfo::FORMAT_AUTO, 0);
				}

				// copy image name
				if(pName)
					str_copy(pImg->m_aName, pName, 128);

				m_lImages.add(pImg);
				
				// unload image
				DataFile.UnloadData(pItem->m_ImageData);
				DataFile.UnloadData(pItem->m_ImageName);
			}
		}
		
		// load groups
		{
			int LayersStart, LayersNum;
			DataFile.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);
			
			int Start, Num;
			DataFile.GetType(MAPITEMTYPE_GROUP, &Start, &Num);
			for(int g = 0; g < Num; g++)
			{
				CMapItemGroup *pGItem = (CMapItemGroup *)DataFile.GetItem(Start+g, 0, 0);
				
				if(pGItem->m_Version < 1 || pGItem->m_Version > CMapItemGroup::CURRENT_VERSION)
					continue;
				
				CLayerGroup *pGroup = NewGroup();
				pGroup->m_ParallaxX = pGItem->m_ParallaxX;
				pGroup->m_ParallaxY = pGItem->m_ParallaxY;
				pGroup->m_OffsetX = pGItem->m_OffsetX;
				pGroup->m_OffsetY = pGItem->m_OffsetY;
				
				if(pGItem->m_Version >= 2)
				{
					pGroup->m_UseClipping = pGItem->m_UseClipping;
					pGroup->m_ClipX = pGItem->m_ClipX;
					pGroup->m_ClipY = pGItem->m_ClipY;
					pGroup->m_ClipW = pGItem->m_ClipW;
					pGroup->m_ClipH = pGItem->m_ClipH;
				}
				
				for(int l = 0; l < pGItem->m_NumLayers; l++)
				{
					CLayer *pLayer = 0;
					CMapItemLayer *pLayerItem = (CMapItemLayer *)DataFile.GetItem(LayersStart+pGItem->m_StartLayer+l, 0, 0);
					if(!pLayerItem)
						continue;
						
					if(pLayerItem->m_Type == LAYERTYPE_TILES)
					{
						CMapItemLayerTilemap *pTilemapItem = (CMapItemLayerTilemap *)pLayerItem;
						CLayerTiles *pTiles = 0;
						CLayerTiles *pTilesTemp = 0;
						
						if(pTilemapItem->m_Flags&1)
						{
							pTiles = new CLayerGame(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeGameLayer(pTiles);
							MakeGameGroup(pGroup);
						}
						else if(pTilemapItem->m_Flags&2)
						{
							pTiles = new CLayerTele(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeTeleLayer(pTiles);
						}
						else if(pTilemapItem->m_Flags&4)
						{
							pTiles = new CLayerSpeedup(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeSpeedupLayer(pTiles);
						}
						else if(pTilemapItem->m_Flags&8)
						{
							pTiles = new CLayerFront(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeFrontLayer(pTiles);
						}
						else if(pTilemapItem->m_Flags&16)
						{
							pTiles = new CLayerSwitch(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeSwitchLayer(pTiles);
							pTilesTemp = new CLayerSwitchOlder(pTilemapItem->m_Width, pTilemapItem->m_Height);
							MakeSwitchLayerOlder(pTilesTemp);
						}
						else
						{
							pTiles = new CLayerTiles(pTilemapItem->m_Width, pTilemapItem->m_Height);
							pTiles->m_pEditor = m_pEditor;
						}

						pLayer = pTiles;
						
						pGroup->AddLayer(pTiles);
						void *pData = DataFile.GetData(pTilemapItem->m_Data);
						pTiles->m_Image = pTilemapItem->m_Image;
						pTiles->m_Game = pTilemapItem->m_Flags&1;
						
						mem_copy(pTiles->m_pTiles, pData, pTiles->m_Width*pTiles->m_Height*sizeof(CTile));
						
						if(pTiles->m_Game && pTilemapItem->m_Version == MakeVersion(1, *pTilemapItem))
						{
							for(int i = 0; i < pTiles->m_Width*pTiles->m_Height; i++)
							{
								if(pTiles->m_pTiles[i].m_Index)
									pTiles->m_pTiles[i].m_Index += ENTITY_OFFSET;
							}
						}
						
						DataFile.UnloadData(pTilemapItem->m_Data);
						
						if(pTiles->m_Tele)
						{
							void *pTeleData = DataFile.GetData(pTilemapItem->m_Tele);
							mem_copy(((CLayerTele*)pTiles)->m_pTeleTile, pTeleData, pTiles->m_Width*pTiles->m_Height*sizeof(CTeleTile));
							
							for(int i = 0; i < pTiles->m_Width*pTiles->m_Height; i++)
							{
								if(((CLayerTele*)pTiles)->m_pTeleTile[i].m_Type == TILE_TELEIN)
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_TELEIN;
								else if(((CLayerTele*)pTiles)->m_pTeleTile[i].m_Type == TILE_TELEINEVIL)
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_TELEINEVIL;
								else if(((CLayerTele*)pTiles)->m_pTeleTile[i].m_Type == TILE_TELEOUT)
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_TELEOUT;
								/*else
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = 0;*/
							}
							DataFile.UnloadData(pTilemapItem->m_Tele);
						}
						else if(pTiles->m_Speedup)
						{
							void *pSpeedupData = DataFile.GetData(pTilemapItem->m_Speedup);
							mem_copy(((CLayerSpeedup*)pTiles)->m_pSpeedupTile, pSpeedupData, pTiles->m_Width*pTiles->m_Height*sizeof(CSpeedupTile));
							
							for(int i = 0; i < pTiles->m_Width*pTiles->m_Height; i++)
							{
								if(((CLayerSpeedup*)pTiles)->m_pSpeedupTile[i].m_Force > 0)
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_BOOST;
									((CLayerSpeedup*)pTiles)->m_pSpeedupTile[i].m_Type = TILE_BOOST;
								}
								else
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = 0;
									((CLayerSpeedup*)pTiles)->m_pSpeedupTile[i].m_Type = 0;
								}
							}
							
							DataFile.UnloadData(pTilemapItem->m_Speedup);
						}
						else if(pTiles->m_Front)
						{
							void *pFrontData = DataFile.GetData(pTilemapItem->m_Front);
							mem_copy(((CLayerFront*)pTiles)->m_pTiles, pFrontData, pTiles->m_Width*pTiles->m_Height*sizeof(CTile));

							DataFile.UnloadData(pTilemapItem->m_Front);
						}
						else if(pTiles->m_Switch)
						{
							void *pSwitchData = DataFile.GetData(pTilemapItem->m_Switch);
							mem_copy(((CLayerSwitchOlder*)pTilesTemp)->m_pSwitchTile, pSwitchData, pTiles->m_Width*pTiles->m_Height*sizeof(CTeleTile));
							for(int i = 0; i < pTiles->m_Width*pTiles->m_Height; i++)
							{
								if(((CLayerSwitchOlder*)pTilesTemp)->m_pSwitchTile[i].m_Type)
								{
									if(((CLayerSwitchOlder*)pTilesTemp)->m_pSwitchTile[i].m_Type == ENTITY_DOOR+1+ENTITY_OFFSET)
										((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type = TILE_SWITCHTIMEDCLOSE;
									else
										((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type = ((CLayerSwitchOlder*)pTilesTemp)->m_pSwitchTile[i].m_Type;
									((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Number = ((CLayerSwitchOlder*)pTilesTemp)->m_pSwitchTile[i].m_Number;
								}
								else
								{
									((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type = 0;
									((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Number = 0;
								}
								((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags = 0;
								((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Delay = 0;
							}

							for(int i = 0; i < pTiles->m_Width*pTiles->m_Height; i++)
							{
								if(((((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type > (ENTITY_CRAZY_SHOTGUN + ENTITY_OFFSET) && ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type < (ENTITY_DRAGGER_WEAK + ENTITY_OFFSET)) || ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type == (ENTITY_LASER_O_FAST + 1 + ENTITY_OFFSET)))
									continue;
								if(((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type == TILE_SWITCHTIMEDOPEN)
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_SWITCHTIMEDOPEN;
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Flags = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags;
								}
								else if(((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type == TILE_SWITCHTIMEDCLOSE)
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_SWITCHTIMEDCLOSE;
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Flags = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags;
								}
								else if(((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type == TILE_SWITCHOPEN)
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_SWITCHOPEN;
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Flags = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags;
								}
								else if(((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type == TILE_SWITCHCLOSE)
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = TILE_SWITCHCLOSE;
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Flags = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags;
								}
								else if(((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type >= (ENTITY_ARMOR_1 + ENTITY_OFFSET) && ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type <= (ENTITY_DOOR + ENTITY_OFFSET))
								{
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Index = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Type;
									((CLayerTiles*)pTiles)->m_pTiles[i].m_Flags = ((CLayerSwitch*)pTiles)->m_pSwitchTile[i].m_Flags;
								}

							}
							DataFile.UnloadData(pTilemapItem->m_Switch);
						}
					}
					else if(pLayerItem->m_Type == LAYERTYPE_QUADS)
					{
						CMapItemLayerQuads *pQuadsItem = (CMapItemLayerQuads *)pLayerItem;
						CLayerQuads *pQuads = new CLayerQuads;
						pQuads->m_pEditor = m_pEditor;
						pLayer = pQuads;
						pQuads->m_Image = pQuadsItem->m_Image;
						if(pQuads->m_Image < -1 || pQuads->m_Image >= m_lImages.size())
							pQuads->m_Image = -1;
						void *pData = DataFile.GetDataSwapped(pQuadsItem->m_Data);
						pGroup->AddLayer(pQuads);
						pQuads->m_lQuads.set_size(pQuadsItem->m_NumQuads);
						mem_copy(pQuads->m_lQuads.base_ptr(), pData, sizeof(CQuad)*pQuadsItem->m_NumQuads);
						DataFile.UnloadData(pQuadsItem->m_Data);
					}
					
					if(pLayer)
						pLayer->m_Flags = pLayerItem->m_Flags;
				}
			}
		}
		
		// load envelopes
		{
			CEnvPoint *pPoints = 0;
			
			{
				int Start, Num;
				DataFile.GetType(MAPITEMTYPE_ENVPOINTS, &Start, &Num);
				if(Num)
					pPoints = (CEnvPoint *)DataFile.GetItem(Start, 0, 0);
			}
			
			int Start, Num;
			DataFile.GetType(MAPITEMTYPE_ENVELOPE, &Start, &Num);
			for(int e = 0; e < Num; e++)
			{
				CMapItemEnvelope *pItem = (CMapItemEnvelope *)DataFile.GetItem(Start+e, 0, 0);
				CEnvelope *pEnv = new CEnvelope(pItem->m_Channels);
				pEnv->m_lPoints.set_size(pItem->m_NumPoints);
				mem_copy(pEnv->m_lPoints.base_ptr(), &pPoints[pItem->m_StartPoint], sizeof(CEnvPoint)*pItem->m_NumPoints);
				if(pItem->m_aName[0] != -1)	// compatibility with old maps
					IntsToStr(pItem->m_aName, sizeof(pItem->m_aName)/sizeof(int), pEnv->m_aName);
				m_lEnvelopes.add(pEnv);
			}
		}
	}
	
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
