/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#ifndef GAME_CLIENT_RENDER_H
#define GAME_CLIENT_RENDER_H

#include "../g_vmath.h"
#include "../g_mapitems.h"

struct tee_render_info
{
	int texture;
	vec4 color_body;
	vec4 color_feet;
	float size;
};

// sprite renderings
enum
{
	SPRITE_FLAG_FLIP_Y=1,
	SPRITE_FLAG_FLIP_X=2,
};

typedef struct sprite;

void select_sprite(sprite *spr, int flags=0, int sx=0, int sy=0);
void select_sprite(int id, int flags=0, int sx=0, int sy=0);

void draw_sprite(float x, float y, float size);

// rects
void draw_round_rect(float x, float y, float w, float h, float r);
void draw_round_rect_ext(float x, float y, float w, float h, float r, int corners);

// larger rendering methods
void menu_render();
void render_game();
void render_world(float center_x, float center_y, float zoom);
void render_loading(float percent);

void render_damage_indicators();

// object render methods (gc_render_obj.cpp)
void render_tee(class animstate *anim, tee_render_info *info, int emote, vec2 dir, vec2 pos);
void render_flag(const struct obj_flag *prev, const struct obj_flag *current);
void render_powerup(const struct obj_powerup *prev, const struct obj_powerup *current);
void render_projectile(const struct obj_projectile *current, int itemid);
void render_player(
	const struct obj_player_character *prev_char, const struct obj_player_character *player_char,
	const struct obj_player_info *prev_info, const struct obj_player_info *player_info);
	
// map render methods (gc_render_map.cpp)
void render_quads(QUAD *quads, int num_quads);
void render_tilemap(TILE *tiles, int w, int h, float scale, int flags);

// helpers
void mapscreen_to_world(float center_x, float center_y, float parallax_x, float parallax_y,
	float offset_x, float offset_y, float aspect, float zoom, float *points);


#endif
