/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <game/g_math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
	#include <engine/e_config.h> // TODO: this shouldn't be here
	#include <engine/client/ec_font.h> // TODO: this shouldn't be here
	#include <engine/e_client_interface.h>
};

#include "../g_game.h"
#include "../g_version.h"
#include "../g_layers.h"
#include "../g_math.h"
#include "gc_map_image.h"
#include "../generated/gc_data.h"
#include "gc_menu.h"
#include "gc_skin.h"
#include "gc_ui.h"
#include "gc_client.h"
#include "gc_render.h"
#include "gc_anim.h"
#include "gc_console.h"

struct data_container *data = 0;
int64 debug_firedelay = 0;

NETOBJ_PLAYER_INPUT input_data = {0};
int input_target_lock = 0;

int chat_mode = CHATMODE_NONE;
bool menu_active = false;
bool menu_game_active = false;
int emoticon_selector_active = 0;
int scoreboard_active = 0;
static int emoticon_selected_emote = -1;

tuning_params tuning;

vec2 mouse_pos;
vec2 local_character_pos;
vec2 local_target_pos;

/*
const NETOBJ_PLAYER_CHARACTER *local_character = 0;
const NETOBJ_PLAYER_CHARACTER *local_prev_character = 0;
const NETOBJ_PLAYER_INFO *local_info = 0;
const NETOBJ_FLAG *flags[2] = {0,0};
const NETOBJ_GAME *gameobj = 0;
*/

snapstate netobjects;

int picked_up_weapon = -1;

client_data client_datas[MAX_CLIENTS];
void client_data::update_render_info()
{
	render_info = skin_info;

	// force team colors
	if(netobjects.gameobj && netobjects.gameobj->gametype != GAMETYPE_DM)
	{
		const int team_colors[2] = {65387, 10223467};
		if(team >= 0 || team <= 1)
		{
			render_info.texture = skin_get(skin_id)->color_texture;
			render_info.color_body = skin_get_color(team_colors[team]);
			render_info.color_feet = skin_get_color(team_colors[team]);
		}
	}		
}


void snd_play_random(int chn, int setid, float vol, vec2 pos)
{
	soundset *set = &data->sounds[setid];

	if(!set->num_sounds)
		return;

	if(set->num_sounds == 1)
	{
		snd_play_at(chn, set->sounds[0].id, 0, pos.x, pos.y);
		return;
	}

	// play a random one
	int id;
	do {
		id = rand() % set->num_sounds;
	} while(id == set->last);
	snd_play_at(chn, set->sounds[id].id, 0, pos.x, pos.y);
	set->last = id;
}


void send_switch_team(int team)
{
	msg_pack_start(MSG_SETTEAM, MSGFLAG_VITAL);
	msg_pack_int(team);
	msg_pack_end();
	client_send_msg();	
}

class damage_indicators
{
public:
	int64 lastupdate;
	struct item
	{
		vec2 pos;
		vec2 dir;
		float life;
		float startangle;
	};

	enum
	{
		MAX_ITEMS=64,
	};

	damage_indicators()
	{
		lastupdate = 0;
		num_items = 0;
	}

	item items[MAX_ITEMS];
	int num_items;

	item *create_i()
	{
		if (num_items < MAX_ITEMS)
		{
			item *p = &items[num_items];
			num_items++;
			return p;
		}
		return 0;
	}

	void destroy_i(item *i)
	{
		num_items--;
		*i = items[num_items];
	}

	void create(vec2 pos, vec2 dir)
	{
		item *i = create_i();
		if (i)
		{
			i->pos = pos;
			i->life = 0.75f;
			i->dir = dir*-1;
			i->startangle = (( (float)rand()/(float)RAND_MAX) - 1.0f) * 2.0f * pi;
		}
	}

	void render()
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		for(int i = 0; i < num_items;)
		{
			vec2 pos = mix(items[i].pos+items[i].dir*75.0f, items[i].pos, clamp((items[i].life-0.60f)/0.15f, 0.0f, 1.0f));

			items[i].life -= client_frametime();
			if(items[i].life < 0.0f)
				destroy_i(&items[i]);
			else
			{
				gfx_setcolor(1.0f,1.0f,1.0f, items[i].life/0.1f);
				gfx_quads_setrotation(items[i].startangle + items[i].life * 2.0f);
				select_sprite(SPRITE_STAR1);
				draw_sprite(pos.x, pos.y, 48.0f);
				i++;
			}
		}
		gfx_quads_end();
	}

};

static damage_indicators dmgind;

void effect_damage_indicator(vec2 pos, vec2 dir)
{
	dmgind.create(pos, dir);
}

void render_damage_indicators()
{
	dmgind.render();
}

static char chat_input[512];
static unsigned chat_input_len;
static const int chat_max_lines = 10;

struct chatline
{
	int tick;
	int client_id;
	int team;
	int name_color;
	char name[64];
	char text[512];
};

chatline chat_lines[chat_max_lines];
static int chat_current_line = 0;

void chat_reset()
{
	for(int i = 0; i < chat_max_lines; i++)
		chat_lines[i].tick = -1000000;
	chat_current_line = 0;
}

void chat_add_line(int client_id, int team, const char *line)
{
	chat_current_line = (chat_current_line+1)%chat_max_lines;
	chat_lines[chat_current_line].tick = client_tick();
	chat_lines[chat_current_line].client_id = client_id;
	chat_lines[chat_current_line].team = team;
	chat_lines[chat_current_line].name_color = -2;

	if(client_id == -1) // server message
	{
		str_copy(chat_lines[chat_current_line].name, "*** ", sizeof(chat_lines[chat_current_line].name));
		str_format(chat_lines[chat_current_line].text, sizeof(chat_lines[chat_current_line].text), "%s", line);
	}
	else
	{
		if(client_datas[client_id].team == -1)
			chat_lines[chat_current_line].name_color = -1;

		if(netobjects.gameobj && netobjects.gameobj->gametype != GAMETYPE_DM)
		{
			if(client_datas[client_id].team == 0)
				chat_lines[chat_current_line].name_color = 0;
			else if(client_datas[client_id].team == 1)
				chat_lines[chat_current_line].name_color = 1;
		}
		
		str_copy(chat_lines[chat_current_line].name, client_datas[client_id].name, sizeof(chat_lines[chat_current_line].name));
		str_format(chat_lines[chat_current_line].text, sizeof(chat_lines[chat_current_line].text), ": %s", line);
	}
}


killmsg killmsgs[killmsg_max];
int killmsg_current = 0;

//bool add_trail = false;

extern int render_popup(const char *caption, const char *text, const char *button_text);

void process_events(int snaptype)
{
	int num = snap_num_items(snaptype);
	for(int index = 0; index < num; index++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(snaptype, index, &item);

		if(item.type == NETEVENTTYPE_DAMAGEIND)
		{
			NETEVENT_DAMAGEIND *ev = (NETEVENT_DAMAGEIND *)data;
			effect_damage_indicator(vec2(ev->x, ev->y), get_direction(ev->angle));
		}
		else if(item.type == NETEVENTTYPE_AIR_JUMP)
		{
			NETEVENT_COMMON *ev = (NETEVENT_COMMON *)data;
			effect_air_jump(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_EXPLOSION)
		{
			NETEVENT_EXPLOSION *ev = (NETEVENT_EXPLOSION *)data;
			effect_explosion(vec2(ev->x, ev->y));
		}
		/*else if(item.type == EVENT_SMOKE)
		{
			EV_EXPLOSION *ev = (EV_EXPLOSION *)data;
			vec2 p(ev->x, ev->y);
		}*/
		else if(item.type == NETEVENTTYPE_SPAWN)
		{
			NETEVENT_SPAWN *ev = (NETEVENT_SPAWN *)data;
			effect_playerspawn(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_DEATH)
		{
			NETEVENT_DEATH *ev = (NETEVENT_DEATH *)data;
			effect_playerdeath(vec2(ev->x, ev->y));
		}
		else if(item.type == NETEVENTTYPE_SOUND_WORLD)
		{
			NETEVENT_SOUND_WORLD *ev = (NETEVENT_SOUND_WORLD *)data;
			snd_play_random(CHN_WORLD, ev->soundid, 1.0f, vec2(ev->x, ev->y));
		}
	}
}

void clear_object_pointers()
{
	// clear out the invalid pointers
	mem_zero(&netobjects, sizeof(netobjects));
}

void send_info(bool start)
{
	if(start)
		msg_pack_start(MSG_STARTINFO, MSGFLAG_VITAL);
	else
		msg_pack_start(MSG_CHANGEINFO, MSGFLAG_VITAL);
	msg_pack_string(config.player_name, 64);
	msg_pack_string(config.player_skin, 64);
	msg_pack_int(config.player_use_custom_color);
	msg_pack_int(config.player_color_body);
	msg_pack_int(config.player_color_feet);
	msg_pack_end();
	client_send_msg();
}

void send_emoticon(int emoticon)
{
	msg_pack_start(MSG_EMOTICON, MSGFLAG_VITAL);
	msg_pack_int(emoticon);
	msg_pack_end();
	client_send_msg();
}

void send_kill(int client_id)
{
	msg_pack_start(MSG_KILL, MSGFLAG_VITAL);
	msg_pack_int(client_id);
	msg_pack_end();
	client_send_msg();
}

void anim_seq_eval(sequence *seq, float time, keyframe *frame)
{
	if(seq->num_frames == 0)
	{
		frame->time = 0;
		frame->x = 0;
		frame->y = 0;
		frame->angle = 0;
	}
	else if(seq->num_frames == 1)
	{
		*frame = seq->frames[0];
	}
	else
	{
		//time = max(0.0f, min(1.0f, time / duration)); // TODO: use clamp
		keyframe *frame1 = 0;
		keyframe *frame2 = 0;
		float blend = 0.0f;

		// TODO: make this smarter.. binary search
		for (int i = 1; i < seq->num_frames; i++)
		{
			if (seq->frames[i-1].time <= time && seq->frames[i].time >= time)
			{
				frame1 = &seq->frames[i-1];
				frame2 = &seq->frames[i];
				blend = (time - frame1->time) / (frame2->time - frame1->time);
				break;
			}
		}

		if (frame1 && frame2)
		{
			frame->time = time;
			frame->x = mix(frame1->x, frame2->x, blend);
			frame->y = mix(frame1->y, frame2->y, blend);
			frame->angle = mix(frame1->angle, frame2->angle, blend);
		}
	}
}

void anim_eval(animation *anim, float time, animstate *state)
{
	anim_seq_eval(&anim->body, time, &state->body);
	anim_seq_eval(&anim->back_foot, time, &state->back_foot);
	anim_seq_eval(&anim->front_foot, time, &state->front_foot);
	anim_seq_eval(&anim->attach, time, &state->attach);
}

void anim_add_keyframe(keyframe *seq, keyframe *added, float amount)
{
	seq->x += added->x*amount;
	seq->y += added->y*amount;
	seq->angle += added->angle*amount;
}

void anim_add(animstate *state, animstate *added, float amount)
{
	anim_add_keyframe(&state->body, &added->body, amount);
	anim_add_keyframe(&state->back_foot, &added->back_foot, amount);
	anim_add_keyframe(&state->front_foot, &added->front_foot, amount);
	anim_add_keyframe(&state->attach, &added->attach, amount);
}

void anim_eval_add(animstate *state, animation *anim, float time, float amount)
{
	animstate add;
	anim_eval(anim, time, &add);
	anim_add(state, &add, amount);
}

static void draw_circle(float x, float y, float r, int segments)
{
	float f_segments = (float)segments;
	for(int i = 0; i < segments; i+=2)
	{
		float a1 = i/f_segments * 2*pi;
		float a2 = (i+1)/f_segments * 2*pi;
		float a3 = (i+2)/f_segments * 2*pi;
		float ca1 = cosf(a1);
		float ca2 = cosf(a2);
		float ca3 = cosf(a3);
		float sa1 = sinf(a1);
		float sa2 = sinf(a2);
		float sa3 = sinf(a3);

		gfx_quads_draw_freeform(
			x, y,
			x+ca1*r, y+sa1*r,
			x+ca3*r, y+sa3*r,
			x+ca2*r, y+sa2*r);
	}
}

static vec2 emoticon_selector_mouse;

void emoticon_selector_render()
{
	int x, y;
	inp_mouse_relative(&x, &y);

	emoticon_selector_mouse.x += x;
	emoticon_selector_mouse.y += y;

	if (length(emoticon_selector_mouse) > 140)
		emoticon_selector_mouse = normalize(emoticon_selector_mouse) * 140;

	float selected_angle = get_angle(emoticon_selector_mouse) + 2*pi/24;
	if (selected_angle < 0)
		selected_angle += 2*pi;

	if (length(emoticon_selector_mouse) > 100)
		emoticon_selected_emote = (int)(selected_angle / (2*pi) * 12.0f);

    RECT screen = *ui_screen();

	gfx_mapscreen(screen.x, screen.y, screen.w, screen.h);

	gfx_blend_normal();

	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.3f);
	draw_circle(screen.w/2, screen.h/2, 160, 64);
	gfx_quads_end();

	gfx_texture_set(data->images[IMAGE_EMOTICONS].id);
	gfx_quads_begin();

	for (int i = 0; i < 12; i++)
	{
		float angle = 2*pi*i/12.0;
		if (angle > pi)
			angle -= 2*pi;

		bool selected = emoticon_selected_emote == i;

		float size = selected ? 96 : 64;

		float nudge_x = 120 * cos(angle);
		float nudge_y = 120 * sin(angle);
		select_sprite(SPRITE_OOP + i);
		gfx_quads_draw(screen.w/2 + nudge_x, screen.h/2 + nudge_y, size, size);
	}

	gfx_quads_end();

    gfx_texture_set(data->images[IMAGE_CURSOR].id);
    gfx_quads_begin();
    gfx_setcolor(1,1,1,1);
    gfx_quads_drawTL(emoticon_selector_mouse.x+screen.w/2,emoticon_selector_mouse.y+screen.h/2,24,24);
    gfx_quads_end();
}

void render_goals(float x, float y, float w)
{
	float h = 50.0f;

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 10.0f);
	gfx_quads_end();

	// render goals
	//y = ystart+h-54;
	if(netobjects.gameobj && netobjects.gameobj->time_limit)
	{
		char buf[64];
		str_format(buf, sizeof(buf), "Time Limit: %d min", netobjects.gameobj->time_limit);
		gfx_text(0, x+w/2, y, 24.0f, buf, -1);
	}
	if(netobjects.gameobj && netobjects.gameobj->score_limit)
	{
		char buf[64];
		str_format(buf, sizeof(buf), "Score Limit: %d", netobjects.gameobj->score_limit);
		gfx_text(0, x+40, y, 24.0f, buf, -1);
	}
}

void render_spectators(float x, float y, float w)
{
	char buffer[1024*4];
	int count = 0;
	float h = 120.0f;
	
	str_copy(buffer, "Spectators: ", sizeof(buffer));

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 10.0f);
	gfx_quads_end();
	
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_INFO)
		{
			const NETOBJ_PLAYER_INFO *info = (const NETOBJ_PLAYER_INFO *)data;
			if(info->team == -1)
			{
				if(count)
					strcat(buffer, ", ");
				strcat(buffer, client_datas[info->cid].name);
				count++;
			}
		}
	}
	
	gfx_text(0, x+10, y, 32, buffer, (int)w-20);
}

void render_scoreboard(float x, float y, float w, int team, const char *title)
{
	animstate idlestate;
	anim_eval(&data->animations[ANIM_BASE], 0, &idlestate);
	anim_eval_add(&idlestate, &data->animations[ANIM_IDLE], 0, 1.0f);

	//float ystart = y;
	float h = 750.0f;

	gfx_blend_normal();
	gfx_texture_set(-1);
	gfx_quads_begin();
	gfx_setcolor(0,0,0,0.5f);
	draw_round_rect(x-10.f, y-10.f, w, h, 40.0f);
	gfx_quads_end();

	// render title
	if(!title)
	{
		if(netobjects.gameobj->game_over)
			title = "Game Over";
		else
			title = "Score Board";
	}

	float tw = gfx_text_width(0, 48, title, -1);

	if(team == -1)
	{
		gfx_text(0, x+w/2-tw/2, y, 48, title, -1);
	}
	else
	{
		gfx_text(0, x+10, y, 48, title, -1);

		if(netobjects.gameobj)
		{
			char buf[128];
			int score = team ? netobjects.gameobj->teamscore_blue : netobjects.gameobj->teamscore_red;
			str_format(buf, sizeof(buf), "%d", score);
			tw = gfx_text_width(0, 48, buf, -1);
			gfx_text(0, x+w-tw-30, y, 48, buf, -1);
		}
	}

	y += 54.0f;

	// find players
	const NETOBJ_PLAYER_INFO *players[MAX_CLIENTS] = {0};
	int num_players = 0;
	for(int i = 0; i < snap_num_items(SNAP_CURRENT); i++)
	{
		SNAP_ITEM item;
		const void *data = snap_get_item(SNAP_CURRENT, i, &item);

		if(item.type == NETOBJTYPE_PLAYER_INFO)
		{
			players[num_players] = (const NETOBJ_PLAYER_INFO *)data;
			num_players++;
		}
	}

	// sort players
	for(int k = 0; k < num_players; k++) // ffs, bubblesort
	{
		for(int i = 0; i < num_players-k-1; i++)
		{
			if(players[i]->score < players[i+1]->score)
			{
				const NETOBJ_PLAYER_INFO *tmp = players[i];
				players[i] = players[i+1];
				players[i+1] = tmp;
			}
		}
	}

	// render headlines
	gfx_text(0, x+10, y, 24.0f, "Score", -1);
	gfx_text(0, x+125, y, 24.0f, "Name", -1);
	gfx_text(0, x+w-70, y, 24.0f, "Ping", -1);
	y += 29.0f;

	// render player scores
	for(int i = 0; i < num_players; i++)
	{
		const NETOBJ_PLAYER_INFO *info = players[i];

		// make sure that we render the correct team
		if(team == -1 || info->team != team)
			continue;

		char buf[128];
		float font_size = 35.0f;
		if(info->local)
		{
			// background so it's easy to find the local player
			gfx_texture_set(-1);
			gfx_quads_begin();
			gfx_setcolor(1,1,1,0.25f);
			draw_round_rect(x, y, w-20, 48, 20.0f);
			gfx_quads_end();
		}

		str_format(buf, sizeof(buf), "%4d", info->score);
		gfx_text(0, x+60-gfx_text_width(0, font_size,buf,-1), y, font_size, buf, -1);
		
		gfx_text(0, x+128, y, font_size, client_datas[info->cid].name, -1);

		str_format(buf, sizeof(buf), "%4d", info->latency);
		float tw = gfx_text_width(0, font_size, buf, -1);
		gfx_text(0, x+w-tw-35, y, font_size, buf, -1);

		// render avatar
		if((netobjects.flags[0] && netobjects.flags[0]->carried_by == info->cid) ||
			(netobjects.flags[1] && netobjects.flags[1]->carried_by == info->cid))
		{
			gfx_blend_normal();
			gfx_texture_set(data->images[IMAGE_GAME].id);
			gfx_quads_begin();

			if(info->team == 0) select_sprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
			else select_sprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
			
			float size = 64.0f;
			gfx_quads_drawTL(x+55, y-15, size/2, size);
			gfx_quads_end();
		}
		
		render_tee(&idlestate, &client_datas[info->cid].render_info, EMOTE_NORMAL, vec2(1,0), vec2(x+90, y+28));

		
		y += 50.0f;
	}
}
/*
static int do_input(int *v, int key)
{
	*v += inp_key_presses(key) + inp_key_releases(key);
	if((*v&1) != inp_key_state(key))
		(*v)++;
	*v &= INPUT_STATE_MASK;
	
	return (*v&1);
}*/

void chat_say(int team, const char *line)
{
	// send chat message
	msg_pack_start(MSG_SAY, MSGFLAG_VITAL);
	msg_pack_int(team);
	msg_pack_string(line, 512);
	msg_pack_end();
	client_send_msg();
}

void chat_enable_mode(int team)
{
	if(team)
		chat_mode = CHATMODE_TEAM;
	else
		chat_mode = CHATMODE_ALL;
		
	mem_zero(chat_input, sizeof(chat_input));
	chat_input_len = 0;
}

void render_game()
{
	// update the effects
	effects_update();
	particle_update(client_frametime());

	
	float width = 400*3.0f*gfx_screenaspect();
	float height = 400*3.0f;

	bool spectate = false;

	if(config.cl_predict)
	{
		if(!netobjects.local_character || (netobjects.local_character->health < 0) || (netobjects.gameobj && netobjects.gameobj->game_over))
		{
			// don't use predicted
		}
		else
			local_character_pos = mix(predicted_prev_player.pos, predicted_player.pos, client_predintratick());
	}
	else if(netobjects.local_character && netobjects.local_prev_character)
	{
		local_character_pos = mix(
			vec2(netobjects.local_prev_character->x, netobjects.local_prev_character->y),
			vec2(netobjects.local_character->x, netobjects.local_character->y), client_intratick());
	}
	
	if(netobjects.local_info && netobjects.local_info->team == -1)
		spectate = true;

	animstate idlestate;
	anim_eval(&data->animations[ANIM_BASE], 0, &idlestate);
	anim_eval_add(&idlestate, &data->animations[ANIM_IDLE], 0, 1.0f);

	if (inp_key_down(KEY_ESC))
	{
		if (chat_mode)
			chat_mode = CHATMODE_NONE;
		else
		{
			menu_active = !menu_active;
			if(menu_active)
				menu_game_active = true;
		}
	}

	// make sure to send our info again if the menu closes	
	static bool menu_was_active = false;
	if(menu_active)
		menu_was_active = true;
	else if(menu_was_active)
	{
		send_info(false);
		menu_was_active = false;
	}

	// handle chat input
	if (!menu_active)
	{
		if(chat_mode != CHATMODE_NONE)
		{
			if(inp_key_down(KEY_ENTER) || inp_key_down(KEY_KP_ENTER))
			{
				// send message
				if(chat_input_len)
					chat_say(chat_mode == CHATMODE_ALL ? 0 : 1, chat_input);

				chat_mode = CHATMODE_NONE;
			}

			for(int i = 0; i < inp_num_events(); i++)
			{
				INPUT_EVENT e = inp_get_event(i);

				if (!(e.ch >= 0 && e.ch < 32))
				{
					if (chat_input_len < sizeof(chat_input) - 1)
					{
						chat_input[chat_input_len] = e.ch;
						chat_input[chat_input_len+1] = 0;
						chat_input_len++;
					}
				}

				if(e.key == KEY_BACKSPACE)
				{
					if(chat_input_len > 0)
					{
						chat_input[chat_input_len-1] = 0;
						chat_input_len--;
					}
				}
			}
		}
	}

	if (!menu_active)
		inp_clear_events();


	//
	float camera_max_distance = 250.0f;
	float deadzone = config.cl_mouse_deadzone;
	float follow_factor = config.cl_mouse_followfactor/100.0f;
	float mouse_max = min(camera_max_distance/follow_factor + deadzone, (float)config.cl_mouse_max_distance);
	vec2 camera_offset(0, 0);

	// fetch new input
	if(!menu_active && !emoticon_selector_active)
	{
		int x, y;
		inp_mouse_relative(&x, &y);
		mouse_pos += vec2(x, y);
		if(spectate)
		{
			if(mouse_pos.x < 200.0f) mouse_pos.x = 200.0f;
			if(mouse_pos.y < 200.0f) mouse_pos.y = 200.0f;
			if(mouse_pos.x > col_width()*32-200.0f) mouse_pos.x = col_width()*32-200.0f;
			if(mouse_pos.y > col_height()*32-200.0f) mouse_pos.y = col_height()*32-200.0f;
		}
		else
		{
			float l = length(mouse_pos);
			if(l > mouse_max)
			{
				mouse_pos = normalize(mouse_pos)*mouse_max;
				l = mouse_max;
			}

			float offset_amount = max(l-deadzone, 0) * follow_factor;
			camera_offset = normalize(mouse_pos)*offset_amount;
		}
	}

	// set listner pos
	if(spectate)
	{
		local_target_pos = mouse_pos;
		snd_set_listener_pos(mouse_pos.x, mouse_pos.y);
	}
	else
	{
		local_target_pos = local_character_pos + mouse_pos;
		snd_set_listener_pos(local_character_pos.x, local_character_pos.y);
	}

	// center at char but can be moved when mouse is far away
	/*
	float offx = 0, offy = 0;
	if (config.cl_dynamic_camera)
	{
		if(mouse_pos.x > deadzone) offx = mouse_pos.x-deadzone;
		if(mouse_pos.x <-deadzone) offx = mouse_pos.x+deadzone;
		if(mouse_pos.y > deadzone) offy = mouse_pos.y-deadzone;
		if(mouse_pos.y <-deadzone) offy = mouse_pos.y+deadzone;
		offx = offx*2/3;
		offy = offy*2/3;
	}*/

	// render the world
	float zoom = 1.0f;
	if(inp_key_pressed('E'))
		zoom = 0.5f;
	
	gfx_clear(0.65f,0.78f,0.9f);
	if(spectate)
		render_world(mouse_pos.x, mouse_pos.y, zoom);
	else
	{
		render_world(local_character_pos.x+camera_offset.x, local_character_pos.y+camera_offset.y, zoom);

		// draw screen box
		if(0)
		{
			gfx_texture_set(-1);
			gfx_blend_normal();
			gfx_lines_begin();
				float cx = local_character_pos.x+camera_offset.x;
				float cy = local_character_pos.y+camera_offset.y;
				float w = 400*3/2;
				float h = 300*3/2;
				gfx_lines_draw(cx-w,cy-h,cx+w,cy-h);
				gfx_lines_draw(cx+w,cy-h,cx+w,cy+h);
				gfx_lines_draw(cx+w,cy+h,cx-w,cy+h);
				gfx_lines_draw(cx-w,cy+h,cx-w,cy-h);
			gfx_lines_end();
		}
	}


	// pseudo format
	// ZOOM ZOOM
	/*
	float zoom = 3.0;

	// DEBUG TESTING
	if(zoom > 3.01f)
	{
		gfx_clear_mask(0);

		gfx_texture_set(-1);
		gfx_blend_normal();

		gfx_mask_op(MASK_NONE, 1);

		gfx_quads_begin();
		gfx_setcolor(0.65f,0.78f,0.9f,1.0f);

		float fov;
		if (zoom > 3.01f)
			fov = pi * (zoom - 3.0f) / 6.0f;
		else
			fov = pi / 6.0f;

		float fade = 0.7f;


		float a = get_angle(normalize(vec2(mouse_pos.x, mouse_pos.y)));
		vec2 d = get_dir(a);
		vec2 d0 = get_dir(a-fov/2.0f);
		vec2 d1 = get_dir(a+fov/2.0f);

		vec2 cd0 = get_dir(a-(fov*fade)/2.0f); // center direction
		vec2 cd1 = get_dir(a+(fov*fade)/2.0f);

		vec2 p0n = local_character_pos + d0*32.0f;
		vec2 p1n = local_character_pos + d1*32.0f;
		vec2 p0f = local_character_pos + d0*1000.0f;
		vec2 p1f = local_character_pos + d1*1000.0f;

		vec2 cn = local_character_pos + d*32.0f;
		vec2 cf = local_character_pos + d*1000.0f;

		vec2 cp0n = local_character_pos + cd0*32.0f;
		vec2 cp0f = local_character_pos + cd0*1000.0f;
		vec2 cp1n = local_character_pos + cd1*32.0f;
		vec2 cp1f = local_character_pos + cd1*1000.0f;

		gfx_quads_draw_freeform(
			p0n.x,p0n.y,
			p1n.x,p1n.y,
			p0f.x,p0f.y,
			p1f.x,p1f.y);
		gfx_quads_end();

		gfx_mask_op(MASK_SET, 0);

		render_world(local_character_pos.x+offx, local_character_pos.y+offy, 2.0f);

		gfx_mask_op(MASK_NONE, 0);

		mapscreen_to_world(local_character_pos.x+offx, local_character_pos.y+offy, 1.0f);

		gfx_texture_set(-1);
		gfx_blend_normal();
		gfx_quads_begin();
		gfx_setcolor(0.5f,0.9f,0.5f,0.25f);
		float r=0.5f, g=1.0f, b=0.5f;
		float r2=r*0.25f, g2=g*0.25f, b2=b*0.25f;

		gfx_setcolor(r,g,b,0.2f);
		gfx_quads_draw_freeform(
			cn.x,cn.y,
			cn.x,cn.y,
			cp0f.x,cp0f.y,
			cp1f.x,cp1f.y);

		gfx_setcolorvertex(0, r, g, b, 0.2f);
		gfx_setcolorvertex(1, r2, g2, b2, 0.9f);
		gfx_setcolorvertex(2, r, g, b, 0.2f);
		gfx_setcolorvertex(3, r2, g2, b2, 0.9f);
		gfx_quads_draw_freeform(
			cn.x,cn.y,
			p0n.x,p0n.y,
			cp0f.x,cp0f.y,
			p0f.x,p0f.y);

		gfx_quads_draw_freeform(
			cn.x,cn.y,
			p1n.x,p1n.y,
			cp1f.x,cp1f.y,
			p1f.x,p1f.y);

		gfx_quads_end();
	}*/

	if(netobjects.local_character && !spectate && !(netobjects.gameobj && netobjects.gameobj->game_over))
	{
		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();

		// render cursor
		if (!menu_active && !emoticon_selector_active)
		{
			select_sprite(data->weapons[netobjects.local_character->weapon%data->num_weapons].sprite_cursor);
			float cursorsize = 64;
			draw_sprite(local_target_pos.x, local_target_pos.y, cursorsize);
		}
		
		float x = 5;
		float y = 5;

		// render ammo count
		// render gui stuff
		gfx_quads_end();
		gfx_quads_begin();
		gfx_mapscreen(0,0,300*gfx_screenaspect(),300);
		
		// if weaponstage is active, put a "glow" around the stage ammo
		select_sprite(SPRITE_TEE_BODY);
		for (int i = 0; i < netobjects.local_character->weaponstage; i++)
			gfx_quads_drawTL(x+netobjects.local_character->ammocount * 12 -i*12, y+22, 11, 11);
		select_sprite(data->weapons[netobjects.local_character->weapon%data->num_weapons].sprite_proj);
		for (int i = 0; i < min(netobjects.local_character->ammocount, 10); i++)
			gfx_quads_drawTL(x+i*12,y+24,10,10);

		gfx_quads_end();

		gfx_texture_set(data->images[IMAGE_GAME].id);
		gfx_quads_begin();
		int h = 0;

		// render health
		select_sprite(SPRITE_HEALTH_FULL);
		for(; h < netobjects.local_character->health; h++)
			gfx_quads_drawTL(x+h*12,y,10,10);

		select_sprite(SPRITE_HEALTH_EMPTY);
		for(; h < 10; h++)
			gfx_quads_drawTL(x+h*12,y,10,10);

		// render armor meter
		h = 0;
		select_sprite(SPRITE_ARMOR_FULL);
		for(; h < netobjects.local_character->armor; h++)
			gfx_quads_drawTL(x+h*12,y+12,10,10);

		select_sprite(SPRITE_ARMOR_EMPTY);
		for(; h < 10; h++)
			gfx_quads_drawTL(x+h*12,y+12,10,10);
		gfx_quads_end();
	}

	// render kill messages
	{
		gfx_mapscreen(0, 0, width*1.5f, height*1.5f);
		float startx = width*1.5f-10.0f;
		float y = 20.0f;

		for(int i = 0; i < killmsg_max; i++)
		{

			int r = (killmsg_current+i+1)%killmsg_max;
			if(client_tick() > killmsgs[r].tick+50*10)
				continue;

			float font_size = 48.0f;
			float killername_w = gfx_text_width(0, font_size, client_datas[killmsgs[r].killer].name, -1);
			float victimname_w = gfx_text_width(0, font_size, client_datas[killmsgs[r].victim].name, -1);

			float x = startx;

			// render victim name
			x -= victimname_w;
			gfx_text(0, x, y, font_size, client_datas[killmsgs[r].victim].name, -1);

			// render victim tee
			x -= 24.0f;
			
			if(netobjects.gameobj && netobjects.gameobj->gametype == GAMETYPE_CTF)
			{
				if(killmsgs[r].mode_special&1)
				{
					gfx_blend_normal();
					gfx_texture_set(data->images[IMAGE_GAME].id);
					gfx_quads_begin();

					if(client_datas[killmsgs[r].victim].team == 0) select_sprite(SPRITE_FLAG_BLUE);
					else select_sprite(SPRITE_FLAG_RED);
					
					float size = 56.0f;
					gfx_quads_drawTL(x, y-16, size/2, size);
					gfx_quads_end();					
				}
			}
			
			render_tee(&idlestate, &client_datas[killmsgs[r].victim].render_info, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28));
			x -= 32.0f;
			
			// render weapon
			x -= 44.0f;
			if (killmsgs[r].weapon >= 0)
			{
				gfx_texture_set(data->images[IMAGE_GAME].id);
				gfx_quads_begin();
				select_sprite(data->weapons[killmsgs[r].weapon].sprite_body);
				draw_sprite(x, y+28, 96);
				gfx_quads_end();
			}
			x -= 52.0f;

			if(killmsgs[r].victim != killmsgs[r].killer)
			{
				if(netobjects.gameobj && netobjects.gameobj->gametype == GAMETYPE_CTF)
				{
					if(killmsgs[r].mode_special&2)
					{
						gfx_blend_normal();
						gfx_texture_set(data->images[IMAGE_GAME].id);
						gfx_quads_begin();

						if(client_datas[killmsgs[r].killer].team == 0) select_sprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
						else select_sprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);
						
						float size = 56.0f;
						gfx_quads_drawTL(x-56, y-16, size/2, size);
						gfx_quads_end();				
					}
				}				
				
				// render killer tee
				x -= 24.0f;
				render_tee(&idlestate, &client_datas[killmsgs[r].killer].render_info, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28));
				x -= 32.0f;

				// render killer name
				x -= killername_w;
				gfx_text(0, x, y, font_size, client_datas[killmsgs[r].killer].name, -1);
			}

			y += 44;
		}
	}

	// render chat
	{
		gfx_mapscreen(0,0,300*gfx_screenaspect(),300);
		float x = 10.0f;
		float y = 300.0f-50.0f;
		float starty = -1;
		if(chat_mode != CHATMODE_NONE)
		{
			// render chat input
			char buf[sizeof(chat_input)+16];
			if(chat_mode == CHATMODE_ALL)
				str_format(buf, sizeof(buf), "All: %s_", chat_input);
			else if(chat_mode == CHATMODE_TEAM)
				str_format(buf, sizeof(buf), "Team: %s_", chat_input);
			else
				str_format(buf, sizeof(buf), "Chat: %s_", chat_input);
			gfx_text(0, x, y, 8.0f, buf, 380);
			starty = y;
		}

		y -= 8;

		int i;
		for(i = 0; i < chat_max_lines; i++)
		{
			int r = ((chat_current_line-i)+chat_max_lines)%chat_max_lines;
			if(client_tick() > chat_lines[r].tick+50*15)
				break;

			float begin = x;

			// render name
			gfx_text_color(0.8f,0.8f,0.8f,1);
			if(chat_lines[r].client_id == -1)
				gfx_text_color(1,1,0.5f,1); // system
			else if(chat_lines[r].team)
				gfx_text_color(0.45f,0.9f,0.45f,1); // team message
			else if(chat_lines[r].name_color == 0)
				gfx_text_color(1.0f,0.5f,0.5f,1); // red
			else if(chat_lines[r].name_color == 1)
				gfx_text_color(0.7f,0.7f,1.0f,1); // blue
			else if(chat_lines[r].name_color == -1)
				gfx_text_color(0.75f,0.5f,0.75f, 1); // spectator
				
			// render line
			int lines = int(gfx_text_width(0, 10, chat_lines[r].text, -1)) / 300 + 1;
			
			gfx_text(0, begin, y - 8 * (lines - 1), 8, chat_lines[r].name, -1);
			begin += gfx_text_width(0, 10, chat_lines[r].name, -1);

			gfx_text_color(1,1,1,1);
			if(chat_lines[r].client_id == -1)
				gfx_text_color(1,1,0.5f,1); // system
			else if(chat_lines[r].team)
				gfx_text_color(0.65f,1,0.65f,1); // team message

			gfx_text(0, begin, y - 8 * (lines - 1), 8, chat_lines[r].text, 300);
			y -= 6 * lines;
		}

		gfx_text_color(1,1,1,1);
	}

	// render goals
	if(netobjects.gameobj)
	{
		int gametype = netobjects.gameobj->gametype;
		
		float whole = 300*gfx_screenaspect();
		float half = whole/2.0f;
		
		gfx_mapscreen(0,0,300*gfx_screenaspect(),300);
		if(!netobjects.gameobj->sudden_death)
		{
			char buf[32];
			int time = 0;
			if(netobjects.gameobj->time_limit)
			{
				time = netobjects.gameobj->time_limit*60 - ((client_tick()-netobjects.gameobj->round_start_tick)/client_tickspeed());

				if(netobjects.gameobj->game_over)
					time  = 0;
			}
			else
				time = (client_tick()-netobjects.gameobj->round_start_tick)/client_tickspeed();

			str_format(buf, sizeof(buf), "%d:%02d", time /60, time %60);
			float w = gfx_text_width(0, 16, buf, -1);
			gfx_text(0, half-w/2, 2, 16, buf, -1);
		}

		if(netobjects.gameobj->sudden_death)
		{
			const char *text = "Sudden Death";
			float w = gfx_text_width(0, 16, text, -1);
			gfx_text(0, half-w/2, 2, 16, text, -1);
		}

		// render small score hud
		if(!(netobjects.gameobj && netobjects.gameobj->game_over) && (gametype == GAMETYPE_TDM || gametype == GAMETYPE_CTF))
		{
			for(int t = 0; t < 2; t++)
			{
				gfx_blend_normal();
				gfx_texture_set(-1);
				gfx_quads_begin();
				if(t == 0)
					gfx_setcolor(1,0,0,0.25f);
				else
					gfx_setcolor(0,0,1,0.25f);
				draw_round_rect(whole-40, 300-40-15+t*20, 50, 18, 5.0f);
				gfx_quads_end();

				char buf[32];
				str_format(buf, sizeof(buf), "%d", t?netobjects.gameobj->teamscore_blue:netobjects.gameobj->teamscore_red);
				float w = gfx_text_width(0, 14, buf, -1);
				
				if(gametype == GAMETYPE_CTF)
				{
					gfx_text(0, whole-20-w/2+5, 300-40-15+t*20+2, 14, buf, -1);
					if(netobjects.flags[t])
					{
 						if(netobjects.flags[t]->carried_by == -2 || (netobjects.flags[t]->carried_by == -1 && ((client_tick()/10)&1)))
 						{
							gfx_blend_normal();
							gfx_texture_set(data->images[IMAGE_GAME].id);
							gfx_quads_begin();

							if(t == 0) select_sprite(SPRITE_FLAG_RED);
							else select_sprite(SPRITE_FLAG_BLUE);
							
							float size = 16;					
							gfx_quads_drawTL(whole-40+5, 300-40-15+t*20+1, size/2, size);
							gfx_quads_end();
						}
						else if(netobjects.flags[t]->carried_by >= 0)
						{
							int id = netobjects.flags[t]->carried_by%MAX_CLIENTS;
							const char *name = client_datas[id].name;
							float w = gfx_text_width(0, 10, name, -1);
							gfx_text(0, whole-40-5-w, 300-40-15+t*20+2, 10, name, -1);
							tee_render_info info = client_datas[id].render_info;
							info.size = 18.0f;
							
							render_tee(&idlestate, &info, EMOTE_NORMAL, vec2(1,0),
								vec2(whole-40+10, 300-40-15+9+t*20+1));
						}
					}
				}
				else
					gfx_text(0, whole-20-w/2, 300-40-15+t*20+2, 14, buf, -1);
			}
		}

		// render warmup timer
		if(netobjects.gameobj->warmup)
		{
			char buf[256];
			float w = gfx_text_width(0, 24, "Warmup", -1);
			gfx_text(0, 150*gfx_screenaspect()+-w/2, 50, 24, "Warmup", -1);

			int seconds = netobjects.gameobj->warmup/SERVER_TICK_SPEED;
			if(seconds < 5)
				str_format(buf, sizeof(buf), "%d.%d", seconds, (netobjects.gameobj->warmup*10/SERVER_TICK_SPEED)%10);
			else
				str_format(buf, sizeof(buf), "%d", seconds);
			w = gfx_text_width(0, 24, buf, -1);
			gfx_text(0, 150*gfx_screenaspect()+-w/2, 75, 24, buf, -1);
		}
	}

	if (menu_active)
	{
		menu_render();
		return;
	}

	// do emoticon
	if(emoticon_selector_active)
		emoticon_selector_render();
	else
	{
		emoticon_selector_mouse = vec2(0,0);	
	
		if(emoticon_selected_emote != -1)
		{
			send_emoticon(emoticon_selected_emote);
			emoticon_selected_emote = -1;
		}
	}
	
	// render debug stuff
	
	{
		float w = 300*gfx_screenaspect();
		gfx_mapscreen(0, 0, w, 300);

		char buf[512];
		if(config.cl_showfps)
		{
			str_format(buf, sizeof(buf), "%d", (int)(1.0f/client_frametime()));
			gfx_text(0, w-10-gfx_text_width(0,12,buf,-1), 10, 12, buf, -1);
		}
	}
	
	if(config.debug && netobjects.local_character && netobjects.local_prev_character)
	{
		gfx_mapscreen(0, 0, 300*gfx_screenaspect(), 300);
		
		float speed = distance(vec2(netobjects.local_prev_character->x, netobjects.local_prev_character->y),
			vec2(netobjects.local_character->x, netobjects.local_character->y));
		
		char buf[512];
		str_format(buf, sizeof(buf), "%.2f %d", speed/2, netobj_num_corrections());
		gfx_text(0, 150, 50, 12, buf, -1);
	}

	// render score board
	if(scoreboard_active || // user requested
		(!spectate && (!netobjects.local_character || netobjects.local_character->health < 0)) || // not spectating and is dead
		(netobjects.gameobj && netobjects.gameobj->game_over) // game over
		)
	{
		gfx_mapscreen(0, 0, width, height);

		float w = 650.0f;

		if(netobjects.gameobj && netobjects.gameobj->gametype == GAMETYPE_DM)
		{
			render_scoreboard(width/2-w/2, 150.0f, w, 0, 0);
			//render_scoreboard(gameobj, 0, 0, -1, 0);
		}
		else
		{
				
			if(netobjects.gameobj && netobjects.gameobj->game_over)
			{
				const char *text = "DRAW!";
				if(netobjects.gameobj->teamscore_red > netobjects.gameobj->teamscore_blue)
					text = "Red Team Wins!";
				else if(netobjects.gameobj->teamscore_blue > netobjects.gameobj->teamscore_red)
					text = "Blue Team Wins!";
					
				float w = gfx_text_width(0, 92.0f, text, -1);
				gfx_text(0, width/2-w/2, 45, 92.0f, text, -1);
			}
			
			render_scoreboard(width/2-w-20, 150.0f, w, 0, "Red Team");
			render_scoreboard(width/2 + 20, 150.0f, w, 1, "Blue Team");
		}

		render_goals(width/2-w/2, 150+750+25, w);
		render_spectators(width/2-w/2, 150+750+25+50+25, w);
	}
	
	
	
	{
		gfx_mapscreen(0, 0, 300*gfx_screenaspect(), 300);

		if(client_connection_problems())
		{
			const char *text = "Connection Problems...";
			float w = gfx_text_width(0, 24, text, -1);
			gfx_text(0, 150*gfx_screenaspect()-w/2, 50, 24, text, -1);
		}
		
		tuning_params standard_tuning;

		// render warning about non standard tuning
		bool flash = time_get()/(time_freq()/2)%2 == 0;
		if(config.cl_warning_tuning && memcmp(&standard_tuning, &tuning, sizeof(tuning_params)) != 0)
		{
			const char *text = "Warning! Server is running non-standard tuning.";
			if(flash)
				gfx_text_color(1,0.4f,0.4f,1.0f);
			else
				gfx_text_color(0.75f,0.2f,0.2f,1.0f);
			gfx_text(0x0, 5, 40, 6, text, -1);
			gfx_text_color(1,1,1,1);
		}
		
		// render tuning debugging
		if(config.dbg_tuning)
		{
			float y = 50.0f;
			int count = 0;
			for(int i = 0; i < tuning.num(); i++)
			{
				char buf[128];
				float current, standard;
				tuning.get(i, &current);
				standard_tuning.get(i, &standard);
				
				if(standard == current)
					gfx_text_color(1,1,1,1.0f);
				else
					gfx_text_color(1,0.25f,0.25f,1.0f);
		
				float w;
				float x = 5.0f;
				
				str_format(buf, sizeof(buf), "%.2f", standard);
				x += 20.0f;
				w = gfx_text_width(0, 5, buf, -1);
				gfx_text(0x0, x-w, y+count*6, 5, buf, -1);

				str_format(buf, sizeof(buf), "%.2f", current);
				x += 20.0f;
				w = gfx_text_width(0, 5, buf, -1);
				gfx_text(0x0, x-w, y+count*6, 5, buf, -1);

				x += 5.0f;
				gfx_text(0x0, x, y+count*6, 5, tuning.names[i], -1);
				
				count++;
			}
		}
		
		gfx_text_color(1,1,1,1);
	}
	
}

