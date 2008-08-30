#include <engine/e_client_interface.h>
#include <game/generated/g_protocol.hpp>
#include <game/generated/gc_data.hpp>

#include <game/gamecore.hpp> // get_angle
#include <game/client/gameclient.hpp>
#include <game/client/ui.hpp>
#include <game/client/gc_render.hpp>
#include "emoticon.hpp"

EMOTICON::EMOTICON()
{
	on_reset();
}

void EMOTICON::con_key_emoticon(void *result, void *user_data)
{
	((EMOTICON *)user_data)->active = console_arg_int(result, 0) != 0;
}

void EMOTICON::on_init()
{
	MACRO_REGISTER_COMMAND("+emote", "", con_key_emoticon, this);
}

void EMOTICON::on_reset()
{
	was_active = false;
	active = false;
	selected_emote = -1;
}

void EMOTICON::on_message(int msgtype, void *rawmsg)
{
	if(msgtype == NETMSGTYPE_SV_EMOTICON)
	{
		NETMSG_SV_EMOTICON *msg = (NETMSG_SV_EMOTICON *)rawmsg;
		gameclient.clients[msg->cid].emoticon = msg->emoticon;
		gameclient.clients[msg->cid].emoticon_start = client_tick();
	}	
}

bool EMOTICON::on_mousemove(float x, float y)
{
	if(!active)
		return false;
	
	selector_mouse += vec2(x,y);
	return true;
}

void EMOTICON::draw_circle(float x, float y, float r, int segments)
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

	
void EMOTICON::on_render()
{
	if(!active)
	{
		if(was_active && selected_emote)
			emote(selected_emote);
		was_active = false;
		return;
	}
	
	was_active = true;
	
	int x, y;
	inp_mouse_relative(&x, &y);

	selector_mouse.x += x;
	selector_mouse.y += y;

	if (length(selector_mouse) > 140)
		selector_mouse = normalize(selector_mouse) * 140;

	float selected_angle = get_angle(selector_mouse) + 2*pi/24;
	if (selected_angle < 0)
		selected_angle += 2*pi;

	if (length(selector_mouse) > 100)
		selected_emote = (int)(selected_angle / (2*pi) * 12.0f);

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

		bool selected = selected_emote == i;

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
    gfx_quads_drawTL(selector_mouse.x+screen.w/2,selector_mouse.y+screen.h/2,24,24);
    gfx_quads_end();
}

void EMOTICON::emote(int emoticon)
{
	NETMSG_CL_EMOTICON msg;
	msg.emoticon = emoticon;
	msg.pack(MSGFLAG_VITAL);
	client_send_msg();
}
