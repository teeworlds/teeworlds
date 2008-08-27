#include <string.h>

#include <engine/e_client_interface.h>

extern "C" {
	#include <engine/client/ec_font.h>
};

#include <game/generated/gc_data.hpp>
#include <game/gamecore.hpp>
#include <game/version.hpp>

#include <game/layers.hpp>


#include "gameclient.hpp"
#include "components/skins.hpp"

#include "gc_client.hpp"
#include "gc_render.hpp"
#include "gc_map_image.hpp"

extern unsigned char internal_data[];

extern void menu_init();
extern bool menu_active;
extern bool menu_game_active;

static float load_total;
static float load_current;

extern "C" void modc_console_init()
{
	//client_console_init();
}

//binds_save()

static void load_sounds_thread(void *do_render)
{
	// load sounds
	for(int s = 0; s < data->num_sounds; s++)
	{
		//if(do_render) // TODO: repair me
			//render_loading(load_current/load_total);
		for(int i = 0; i < data->sounds[s].num_sounds; i++)
		{
			int id = snd_load_wv(data->sounds[s].sounds[i].filename);
			data->sounds[s].sounds[i].id = id;
		}

		if(do_render)
			load_current++;
	}
}

extern "C" void modc_init()
{
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		snap_set_staticsize(i, netobj_get_size(i));
		
	gameclient.on_init();
	
	static FONT_SET default_font;
	int64 start = time_get();
	
	int before = gfx_memory_usage();
	font_set_load(&default_font, "data/fonts/default_font%d.tfnt", "data/fonts/default_font%d.png", "data/fonts/default_font%d_b.png", 14, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 36);
	dbg_msg("font", "gfx memory used for font textures: %d", gfx_memory_usage()-before);
	
	gfx_text_set_default_font(&default_font);

	//particle_reset();
	//menu_init();
	
	// setup sound channels
	snd_set_channel(CHN_GUI, 1.0f, 0.0f);
	snd_set_channel(CHN_MUSIC, 1.0f, 0.0f);
	snd_set_channel(CHN_WORLD, 0.9f, 1.0f);
	snd_set_channel(CHN_GLOBAL, 1.0f, 0.0f);

	// load the data container
	//data = load_data_from_memory(internal_data);

	// TODO: should be removed
	snd_set_listener_pos(0.0f, 0.0f);

	// setup load amount
	load_total = data->num_images;
	load_current = 0;
	if(!config.cl_threadsoundloading)
		load_total += data->num_sounds;
	
	// load textures
	for(int i = 0; i < data->num_images; i++)
	{
		// TODO: repair me
		//render_loading(load_current/load_total);
		data->images[i].id = gfx_load_texture(data->images[i].filename, IMG_AUTO, 0);
		load_current++;
	}

	gameclient.skins->init();
	//skin_init();
	
	if(config.cl_threadsoundloading)
		thread_create(load_sounds_thread, 0);
	else
		load_sounds_thread((void*)1);
	
	int64 end = time_get();
	dbg_msg("", "%f.2ms", ((end-start)*1000)/(float)time_freq());
}

extern "C" void modc_save_config()
{
	//binds_save();
}


CHARACTER_CORE predicted_prev_char;
CHARACTER_CORE predicted_char;

extern "C" void modc_entergame() {}
extern "C" void modc_shutdown() {}
extern "C" void modc_predict() { gameclient.on_predict(); }
extern "C" void modc_newsnapshot() { gameclient.on_snapshot(); }
extern "C" int modc_snap_input(int *data) { return gameclient.on_snapinput(data); }
extern "C" void modc_statechange(int state, int old) { gameclient.on_statechange(state, old); }
extern "C" void modc_render() { gameclient.on_render(); }

extern "C" void modc_rcon_line(const char *line)
{
	//console_rcon_print(line);
}

/*
NETOBJ_PROJECTILE extraproj_projectiles[MAX_EXTRA_PROJECTILES];
int extraproj_num;

void extraproj_reset()
{
	extraproj_num = 0;
}*/

extern "C" void modc_message(int msgtype)
{
	// special messages
	if(msgtype == NETMSGTYPE_SV_EXTRAPROJECTILE)
	{
		/*
		int num = msg_unpack_int();
		
		for(int k = 0; k < num; k++)
		{
			NETOBJ_PROJECTILE proj;
			for(unsigned i = 0; i < sizeof(NETOBJ_PROJECTILE)/sizeof(int); i++)
				((int *)&proj)[i] = msg_unpack_int();
				
			if(msg_unpack_error())
				return;
				
			if(extraproj_num != MAX_EXTRA_PROJECTILES)
			{
				extraproj_projectiles[extraproj_num] = proj;
				extraproj_num++;
			}
		}
		
		return;*/
	}
	else if(msgtype == NETMSGTYPE_SV_TUNEPARAMS)
	{
		// unpack the new tuning
		TUNING_PARAMS new_tuning;
		int *params = (int *)&new_tuning;
		for(unsigned i = 0; i < sizeof(TUNING_PARAMS)/sizeof(int); i++)
			params[i] = msg_unpack_int();

		// check for unpacking errors
		if(msg_unpack_error())
			return;
			
		// apply new tuning
		tuning = new_tuning;
		return;
	}

	gameclient.on_message(msgtype);

	// normal 
	void *rawmsg = netmsg_secure_unpack(msgtype);
	if(!rawmsg)
	{
		dbg_msg("client", "dropped weird message '%s' (%d), failed on '%s'", netmsg_get_name(msgtype), msgtype, netmsg_failed_on());
		return;
	}
	
	
	if(msgtype == NETMSGTYPE_SV_CHAT)
	{

	}
	else if(msgtype == NETMSGTYPE_SV_BROADCAST)
	{
		/*
		NETMSG_SV_BROADCAST *msg = (NETMSG_SV_BROADCAST *)rawmsg;
		str_copy(broadcast_text, msg->message, sizeof(broadcast_text));
		broadcast_time = time_get()+time_freq()*10;
		*/
	}
	else if(msgtype == NETMSGTYPE_SV_MOTD)
	{
		/*
		NETMSG_SV_MOTD *msg = (NETMSG_SV_MOTD *)rawmsg;

		// process escaping			
		str_copy(server_motd, msg->message, sizeof(server_motd));
		for(int i = 0; server_motd[i]; i++)
		{
			if(server_motd[i] == '\\')
			{
				if(server_motd[i+1] == 'n')
				{
					server_motd[i] = ' ';
					server_motd[i+1] = '\n';
					i++;
				}
			}
		}

		if(server_motd[0] && config.cl_motd_time)
			server_motd_time = time_get()+time_freq()*config.cl_motd_time;
		else
			server_motd_time = 0;
			*/
	}
	else if(msgtype == NETMSGTYPE_SV_SETINFO)
	{
		NETMSG_SV_SETINFO *msg = (NETMSG_SV_SETINFO *)rawmsg;
		
		str_copy(gameclient.clients[msg->cid].name, msg->name, 64);
		str_copy(gameclient.clients[msg->cid].skin_name, msg->skin, 64);
		
		// make sure that we don't set a special skin on the client
		if(gameclient.clients[msg->cid].skin_name[0] == 'x' || gameclient.clients[msg->cid].skin_name[1] == '_')
			str_copy(gameclient.clients[msg->cid].skin_name, "default", 64);
		
		gameclient.clients[msg->cid].skin_info.color_body = gameclient.skins->get_color(msg->color_body);
		gameclient.clients[msg->cid].skin_info.color_feet = gameclient.skins->get_color(msg->color_feet);
		gameclient.clients[msg->cid].skin_info.size = 64;
		
		// find new skin
		gameclient.clients[msg->cid].skin_id = gameclient.skins->find(gameclient.clients[msg->cid].skin_name);
		if(gameclient.clients[msg->cid].skin_id < 0)
			gameclient.clients[msg->cid].skin_id = 0;
		
		if(msg->use_custom_color)
			gameclient.clients[msg->cid].skin_info.texture = gameclient.skins->get(gameclient.clients[msg->cid].skin_id)->color_texture;
		else
		{
			gameclient.clients[msg->cid].skin_info.texture = gameclient.skins->get(gameclient.clients[msg->cid].skin_id)->org_texture;
			gameclient.clients[msg->cid].skin_info.color_body = vec4(1,1,1,1);
			gameclient.clients[msg->cid].skin_info.color_feet = vec4(1,1,1,1);
		}

		gameclient.clients[msg->cid].update_render_info();
	}
    else if(msgtype == NETMSGTYPE_SV_WEAPONPICKUP)
    {
    	// TODO: repair me
    	/*NETMSG_SV_WEAPONPICKUP *msg = (NETMSG_SV_WEAPONPICKUP *)rawmsg;
        if(config.cl_autoswitch_weapons)
        	input_data.wanted_weapon = msg->weapon+1;*/
    }
	else if(msgtype == NETMSGTYPE_SV_READYTOENTER)
	{
		client_entergame();
	}
	else if(msgtype == NETMSGTYPE_SV_KILLMSG)
	{
		/*
		NETMSG_SV_KILLMSG *msg = (NETMSG_SV_KILLMSG *)rawmsg;
		
		gameclient.killmsgs.handle_message((NETMSG_SV_KILLMSG *)rawmsg);
		
		// unpack messages
		KILLMSG kill;
		kill.killer = msg->killer;
		kill.victim = msg->victim;
		kill.weapon = msg->weapon;
		kill.mode_special = msg->mode_special;
		kill.tick = client_tick();

		// add the message
		killmsg_current = (killmsg_current+1)%killmsg_max;
		killmsgs[killmsg_current] = kill;*/
	}
	else if (msgtype == NETMSGTYPE_SV_EMOTICON)
	{
		NETMSG_SV_EMOTICON *msg = (NETMSG_SV_EMOTICON *)rawmsg;

		// apply
		gameclient.clients[msg->cid].emoticon = msg->emoticon;
		gameclient.clients[msg->cid].emoticon_start = client_tick();
	}
	else if(msgtype == NETMSGTYPE_SV_SOUNDGLOBAL)
	{
		NETMSG_SV_SOUNDGLOBAL *msg = (NETMSG_SV_SOUNDGLOBAL *)rawmsg;
		snd_play_random(CHN_GLOBAL, msg->soundid, 1.0f, vec2(0,0));
	}
}

extern "C" void modc_connected()
{
	// init some stuff
	layers_init();
	col_init();
	img_init();
	//flow_init();
	
	render_tilemap_generate_skip();
	
	gameclient.on_connected();
	//tilemap_init();
	//particle_reset();
	//extraproj_reset();
	
	//last_new_predicted_tick = -1;
}

extern "C" const char *modc_net_version() { return GAME_NETVERSION; }
extern "C" const char *modc_getitemname(int type) { return netobj_get_name(type); }
