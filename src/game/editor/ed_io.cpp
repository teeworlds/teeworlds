#include <string.h>
#include <stdio.h>
#include "ed_editor.hpp"

template<typename T>
static int make_version(int i, const T &v)
{ return (i<<16)+sizeof(T); }

// backwards compatiblity
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
			
			LAYER_TILES *l = new LAYER_TILES(tmap->width, tmap->height);
			
			if(tmap->main)
			{
				// move game layer to correct position
				for(int i = 0; i < map->groups[0]->layers.len()-1; i++)
				{
					if(map->groups[0]->layers[i] == editor.map.game_layer)
						map->groups[0]->swap_layers(i, i+1);
				}
				
				game_width = tmap->width;
				game_height = tmap->height;
			}

			// add new layer
			map->groups[0]->add_layer(l);

			// process the data
			unsigned char *src_data = (unsigned char *)datafile_get_data(df, tmap->data);
			TILE *dst_data = l->tiles;
			
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
			img->format = IMG_RGBA;
			
			// copy image data
			img->data = mem_alloc(img->width*img->height*4, 1);
			mem_copy(img->data, data, img->width*img->height*4);
			img->tex_id = gfx_load_texture_raw(img->width, img->height, img->format, img->data, IMG_AUTO, 0);
			map->images.add(img);
			
			// unload image
			datafile_unload_data(df, imgres->image_data);
		}
	}
	
	// load entities
	{
		LAYER_GAME *g = map->game_layer;
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
}

int EDITOR::save(const char *filename)
{
	return map.save(filename);
}

int MAP::save(const char *filename)
{
	dbg_msg("editor", "saving to '%s'...", filename);
	DATAFILE_OUT *df = datafile_create(filename);
	if(!df)
	{
		dbg_msg("editor", "failed to open file '%s'...", filename);
		return 0;
	}
		
	// save version
	{
		MAPITEM_VERSION item;
		item.version = 1;
		datafile_add_item(df, MAPITEMTYPE_VERSION, 0, sizeof(item), &item);
	}

	// save images
	for(int i = 0; i < images.len(); i++)
	{
		EDITOR_IMAGE *img = images[i];
		
		// analyse the image for when saving (should be done when we load the image)
		// TODO!
		img->analyse_tileflags();
		
		MAPITEM_IMAGE item;
		item.version = 1;
		
		item.width = img->width;
		item.height = img->height;
		item.external = img->external;
		item.image_name = datafile_add_data(df, strlen(img->name)+1, img->name);
		if(img->external)
			item.image_data = -1;
		else
			item.image_data = datafile_add_data(df, item.width*item.height*4, img->data);
		datafile_add_item(df, MAPITEMTYPE_IMAGE, i, sizeof(item), &item);
	}
	
	// save layers
	int layer_count = 0;
	for(int g = 0; g < groups.len(); g++)
	{
		LAYERGROUP *group = groups[g];
		MAPITEM_GROUP gitem;
		gitem.version = MAPITEM_GROUP::CURRENT_VERSION;
		
		gitem.parallax_x = group->parallax_x;
		gitem.parallax_y = group->parallax_y;
		gitem.offset_x = group->offset_x;
		gitem.offset_y = group->offset_y;
		gitem.use_clipping = group->use_clipping;
		gitem.clip_x = group->clip_x;
		gitem.clip_y = group->clip_y;
		gitem.clip_w = group->clip_w;
		gitem.clip_h = group->clip_h;
		gitem.start_layer = layer_count;
		gitem.num_layers = 0;
		
		for(int l = 0; l < group->layers.len(); l++)
		{
			if(group->layers[l]->type == LAYERTYPE_TILES)
			{
				dbg_msg("editor", "saving tiles layer");
				LAYER_TILES *layer = (LAYER_TILES *)group->layers[l];
				layer->prepare_for_save();
				
				MAPITEM_LAYER_TILEMAP item;
				item.version = 2;
				
				item.layer.flags = layer->flags;
				item.layer.type = layer->type;
				
				item.color.r = 255; // not in use right now
				item.color.g = 255;
				item.color.b = 255;
				item.color.a = 255;
				item.color_env = -1;
				item.color_env_offset = 0;
				
				item.width = layer->width;
				item.height = layer->height;
				item.flags = layer->game;
				item.image = layer->image;
				item.data = datafile_add_data(df, layer->width*layer->height*sizeof(TILE), layer->tiles);
				datafile_add_item(df, MAPITEMTYPE_LAYER, layer_count, sizeof(item), &item);
				
				gitem.num_layers++;
				layer_count++;
			}
			else if(group->layers[l]->type == LAYERTYPE_QUADS)
			{
				dbg_msg("editor", "saving quads layer");
				LAYER_QUADS *layer = (LAYER_QUADS *)group->layers[l];
				if(layer->quads.len())
				{
					MAPITEM_LAYER_QUADS item;
					item.version = 1;
					item.layer.flags =  layer->flags;
					item.layer.type = layer->type;
					item.image = layer->image;
					
					// add the data
					item.num_quads = layer->quads.len();
					item.data = datafile_add_data_swapped(df, layer->quads.len()*sizeof(QUAD), layer->quads.getptr());
					datafile_add_item(df, MAPITEMTYPE_LAYER, layer_count, sizeof(item), &item);
					
					// clean up
					//mem_free(quads);

					gitem.num_layers++;
					layer_count++;
				}
			}
		}
		
		datafile_add_item(df, MAPITEMTYPE_GROUP, g, sizeof(gitem), &gitem);
	}
	
	// save envelopes
	int point_count = 0;
	for(int e = 0; e < envelopes.len(); e++)
	{
		MAPITEM_ENVELOPE item;
		item.version = 1;
		item.channels = envelopes[e]->channels;
		item.start_point = point_count;
		item.num_points = envelopes[e]->points.len();
		item.name = -1;
		
		datafile_add_item(df, MAPITEMTYPE_ENVELOPE, e, sizeof(item), &item);
		point_count += item.num_points;
	}
	
	// save points
	int totalsize = sizeof(ENVPOINT) * point_count;
	ENVPOINT *points = (ENVPOINT *)mem_alloc(totalsize, 1);
	point_count = 0;
	
	for(int e = 0; e < envelopes.len(); e++)
	{
		int count = envelopes[e]->points.len();
		mem_copy(&points[point_count], envelopes[e]->points.getptr(), sizeof(ENVPOINT)*count);
		point_count += count;
	}

	datafile_add_item(df, MAPITEMTYPE_ENVPOINTS, 0, totalsize, points);
	
	// finish the data file
	datafile_finish(df);
	dbg_msg("editor", "done");
	
	// send rcon.. if we can
	if(client_rcon_authed())
	{
		client_rcon("sv_map_reload 1");
	}
	
	return 1;
}

int EDITOR::load(const char *filename)
{
	reset();
	return map.load(filename);
}

int MAP::load(const char *filename)
{
	DATAFILE *df = datafile_load(filename);
	if(!df)
		return 0;
		
	clean();

	// check version
	MAPITEM_VERSION *item = (MAPITEM_VERSION *)datafile_find_item(df, MAPITEMTYPE_VERSION, 0);
	if(!item)
	{
		// import old map
		MAP old_mapstuff;
		editor.reset();
		editor_load_old(df, this);
	}
	else if(item->version == 1)
	{
		//editor.reset(false);
		
		// load images
		{
			int start, num;
			datafile_get_type(df, MAPITEMTYPE_IMAGE, &start, &num);
			for(int i = 0; i < num; i++)
			{
				MAPITEM_IMAGE *item = (MAPITEM_IMAGE *)datafile_get_item(df, start+i, 0, 0);
				char *name = (char *)datafile_get_data(df, item->image_name);

				// copy base info				
				EDITOR_IMAGE *img = new EDITOR_IMAGE;
				img->external = item->external;

				if(item->external)
				{
					char buf[256];
					sprintf(buf, "mapres/%s.png", name);
					
					// load external
					EDITOR_IMAGE imginfo;
					if(gfx_load_png(&imginfo, buf))
					{
						*img = imginfo;
						img->tex_id = gfx_load_texture_raw(imginfo.width, imginfo.height, imginfo.format, imginfo.data, IMG_AUTO, 0);
						img->external = 1;
					}
				}
				else
				{
					img->width = item->width;
					img->height = item->height;
					img->format = IMG_RGBA;
					
					// copy image data
					void *data = datafile_get_data(df, item->image_data);
					img->data = mem_alloc(img->width*img->height*4, 1);
					mem_copy(img->data, data, img->width*img->height*4);
					img->tex_id = gfx_load_texture_raw(img->width, img->height, img->format, img->data, IMG_AUTO, 0);
				}

				// copy image name
				if(name)
					strncpy(img->name, name, 128);

				images.add(img);
				
				// unload image
				datafile_unload_data(df, item->image_data);
				datafile_unload_data(df, item->image_name);
			}
		}
		
		// load groups
		{
			int layers_start, layers_num;
			datafile_get_type(df, MAPITEMTYPE_LAYER, &layers_start, &layers_num);
			
			int start, num;
			datafile_get_type(df, MAPITEMTYPE_GROUP, &start, &num);
			for(int g = 0; g < num; g++)
			{
				MAPITEM_GROUP *gitem = (MAPITEM_GROUP *)datafile_get_item(df, start+g, 0, 0);
				
				if(gitem->version < 1 || gitem->version > MAPITEM_GROUP::CURRENT_VERSION)
					continue;
				
				LAYERGROUP *group = new_group();
				group->parallax_x = gitem->parallax_x;
				group->parallax_y = gitem->parallax_y;
				group->offset_x = gitem->offset_x;
				group->offset_y = gitem->offset_y;
				
				if(gitem->version >= 2)
				{
					group->use_clipping = gitem->use_clipping;
					group->clip_x = gitem->clip_x;
					group->clip_y = gitem->clip_y;
					group->clip_w = gitem->clip_w;
					group->clip_h = gitem->clip_h;
				}
				
				for(int l = 0; l < gitem->num_layers; l++)
				{
					LAYER *layer = 0;
					MAPITEM_LAYER *layer_item = (MAPITEM_LAYER *)datafile_get_item(df, layers_start+gitem->start_layer+l, 0, 0);
					if(!layer_item)
						continue;
						
					if(layer_item->type == LAYERTYPE_TILES)
					{
						MAPITEM_LAYER_TILEMAP *tilemap_item = (MAPITEM_LAYER_TILEMAP *)layer_item;
						LAYER_TILES *tiles = 0;
						
						if(tilemap_item->flags&1)
						{
							tiles = new LAYER_GAME(tilemap_item->width, tilemap_item->height);
							make_game_layer(tiles);
							make_game_group(group);
						}
						else
							tiles = new LAYER_TILES(tilemap_item->width, tilemap_item->height);

						layer = tiles;
						
						group->add_layer(tiles);
						void *data = datafile_get_data(df, tilemap_item->data);
						tiles->image = tilemap_item->image;
						tiles->game = tilemap_item->flags&1;
						
						mem_copy(tiles->tiles, data, tiles->width*tiles->height*sizeof(TILE));
						
						if(tiles->game && tilemap_item->version == make_version(1, *tilemap_item))
						{
							for(int i = 0; i < tiles->width*tiles->height; i++)
							{
								if(tiles->tiles[i].index)
									tiles->tiles[i].index += ENTITY_OFFSET;
							}
						}
						
						datafile_unload_data(df, tilemap_item->data);
					}
					else if(layer_item->type == LAYERTYPE_QUADS)
					{
						MAPITEM_LAYER_QUADS *quads_item = (MAPITEM_LAYER_QUADS *)layer_item;
						LAYER_QUADS *quads = new LAYER_QUADS;
						layer = quads;
						quads->image = quads_item->image;
						if(quads->image < -1 || quads->image >= images.len())
							quads->image = -1;
						void *data = datafile_get_data_swapped(df, quads_item->data);
						group->add_layer(quads);
						quads->quads.setsize(quads_item->num_quads);
						mem_copy(quads->quads.getptr(), data, sizeof(QUAD)*quads_item->num_quads);
						datafile_unload_data(df, quads_item->data);
					}
					
					if(layer)
						layer->flags = layer_item->flags;
				}
			}
		}
		
		// load envelopes
		{
			ENVPOINT *points = 0;
			
			{
				int start, num;
				datafile_get_type(df, MAPITEMTYPE_ENVPOINTS, &start, &num);
				if(num)
					points = (ENVPOINT *)datafile_get_item(df, start, 0, 0);
			}
			
			int start, num;
			datafile_get_type(df, MAPITEMTYPE_ENVELOPE, &start, &num);
			for(int e = 0; e < num; e++)
			{
				MAPITEM_ENVELOPE *item = (MAPITEM_ENVELOPE *)datafile_get_item(df, start+e, 0, 0);
				ENVELOPE *env = new ENVELOPE(item->channels);
				env->points.setsize(item->num_points);
				mem_copy(env->points.getptr(), &points[item->start_point], sizeof(ENVPOINT)*item->num_points);
				envelopes.add(env);
			}
		}
	}
	
	datafile_unload(df);
	
	return 0;
}

static int modify_add_amount = 0;
static void modify_add(int *index)
{
	if(*index >= 0)
		*index += modify_add_amount;
}

int EDITOR::append(const char *filename)
{
	MAP new_map;
	int err;
	err = new_map.load(filename);
	if(err)
		return err;

	// modify indecies	
	modify_add_amount = map.images.len();
	new_map.modify_image_index(modify_add);
	
	modify_add_amount = map.envelopes.len();
	new_map.modify_envelope_index(modify_add);
	
	// transfer images
	for(int i = 0; i < new_map.images.len(); i++)
		map.images.add(new_map.images[i]);
	new_map.images.clear();
	
	// transfer envelopes
	for(int i = 0; i < new_map.envelopes.len(); i++)
		map.envelopes.add(new_map.envelopes[i]);
	new_map.envelopes.clear();

	// transfer groups
	
	for(int i = 0; i < new_map.groups.len(); i++)
	{
		if(new_map.groups[i] == new_map.game_group)
			delete new_map.groups[i];
		else
			map.groups.add(new_map.groups[i]);
	}
	new_map.groups.clear();
	
	// all done \o/
	return 0;
}
