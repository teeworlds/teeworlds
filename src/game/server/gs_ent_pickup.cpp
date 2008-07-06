#include <engine/e_server_interface.h>
#include "gs_common.hpp"

//////////////////////////////////////////////////
// powerup
//////////////////////////////////////////////////
PICKUP::PICKUP(int _type, int _subtype)
: ENTITY(NETOBJTYPE_PICKUP)
{
	type = _type;
	subtype = _subtype;
	proximity_radius = phys_size;

	reset();

	// TODO: should this be done here?
	game.world.insert_entity(this);
}

void PICKUP::reset()
{
	if (data->pickups[type].spawndelay > 0)
		spawntick = server_tick() + server_tickspeed() * data->pickups[type].spawndelay;
	else
		spawntick = -1;
}

void PICKUP::tick()
{
	// wait for respawn
	if(spawntick > 0)
	{
		if(server_tick() > spawntick)
		{
			// respawn
			spawntick = -1;

			if(type == POWERUP_WEAPON)
				game.create_sound(pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CHARACTER *chr = game.world.closest_character(pos, 20.0f, 0);
	if(chr)
	{
		// player picked us up, is someone was hooking us, let them go
		int respawntime = -1;
		switch (type)
		{
		case POWERUP_HEALTH:
			if(chr->increase_health(1))
			{
				game.create_sound(pos, SOUND_PICKUP_HEALTH);
				respawntime = data->pickups[type].respawntime;
			}
			break;
		case POWERUP_ARMOR:
			if(chr->increase_armor(1))
			{
				game.create_sound(pos, SOUND_PICKUP_ARMOR);
				respawntime = data->pickups[type].respawntime;
			}
			break;

		case POWERUP_WEAPON:
			if(subtype >= 0 && subtype < NUM_WEAPONS)
			{
				if(chr->weapons[subtype].ammo < data->weapons.id[subtype].maxammo || !chr->weapons[subtype].got)
				{
					chr->weapons[subtype].got = true;
					chr->weapons[subtype].ammo = min(data->weapons.id[subtype].maxammo, chr->weapons[subtype].ammo + 10);
					respawntime = data->pickups[type].respawntime;

					// TODO: data compiler should take care of stuff like this
					if(subtype == WEAPON_GRENADE)
						game.create_sound(pos, SOUND_PICKUP_GRENADE);
					else if(subtype == WEAPON_SHOTGUN)
						game.create_sound(pos, SOUND_PICKUP_SHOTGUN);
					else if(subtype == WEAPON_RIFLE)
						game.create_sound(pos, SOUND_PICKUP_SHOTGUN);

					if(chr->player)
                    	game.send_weapon_pickup(chr->player->client_id, subtype);
				}
			}
			break;
		case POWERUP_NINJA:
			{
				// activate ninja on target player
				chr->ninja.activationtick = server_tick();
				chr->weapons[WEAPON_NINJA].got = true;
				chr->last_weapon = chr->active_weapon;
				chr->active_weapon = WEAPON_NINJA;
				respawntime = data->pickups[type].respawntime;
				game.create_sound(pos, SOUND_PICKUP_NINJA);

				// loop through all players, setting their emotes
				ENTITY *ents[64];
				int num = game.world.find_entities(vec2(0, 0), 1000000, ents, 64, NETOBJTYPE_CHARACTER);
				for (int i = 0; i < num; i++)
				{
					CHARACTER *c = (CHARACTER *)ents[i];
					if (c != chr)
					{
						c->emote_type = EMOTE_SURPRISE;
						c->emote_stop = server_tick() + server_tickspeed();
					}
				}

				chr->emote_type = EMOTE_ANGRY;
				chr->emote_stop = server_tick() + 1200 * server_tickspeed() / 1000;
				
				break;
			}
		default:
			break;
		};

		if(respawntime >= 0)
		{
			dbg_msg("game", "pickup player='%d:%s' item=%d/%d",
				chr->player->client_id, server_clientname(chr->player->client_id), type, subtype);
			spawntick = server_tick() + server_tickspeed() * respawntime;
		}
	}
}

void PICKUP::snap(int snapping_client)
{
	if(spawntick != -1)
		return;

	NETOBJ_PICKUP *up = (NETOBJ_PICKUP *)snap_new_item(NETOBJTYPE_PICKUP, id, sizeof(NETOBJ_PICKUP));
	up->x = (int)pos.x;
	up->y = (int)pos.y;
	up->type = type; // TODO: two diffrent types? what gives?
	up->subtype = subtype;
}
