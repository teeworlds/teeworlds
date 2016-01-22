#ifndef MOD_DEFINES_H
#define MOD_DEFINES_H

enum
{
	MOD_WORLD_DEFAULT = 0, // Don't rename, world assignated to a new player
	
	//Add new worlds here
	
	MOD_NUM_WORLDS, // Don't rename, number of world to allocate
	MOD_WORLD_ALL = -1, // Don't rename, special name to designate all worlds between 0 and (MOD_NUM_WORLDS-1)
	MOD_WORLD_DEMO = MOD_WORLD_DEFAULT, // Don't rename, default world used to record demo
};

enum
{
	MOD_ENTTYPE_PROJECTILE = 0,
	MOD_ENTTYPE_LASER,
	MOD_ENTTYPE_PICKUP,
	MOD_ENTTYPE_FLAG,
	MOD_ENTTYPE_CHARACTER,
	MOD_NUM_ENTTYPES
};

enum
{
	MOD_EDITORRESOURCE_TW07 = 0,
};

#endif
