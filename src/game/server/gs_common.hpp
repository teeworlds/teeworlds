/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include "../g_game.hpp"
#include "../generated/gs_data.hpp"

extern TUNING_PARAMS tuning;

inline int cmask_all() { return -1; }
inline int cmask_one(int cid) { return 1<<cid; }
inline int cmask_all_except_one(int cid) { return 0x7fffffff^cmask_one(cid); }
inline bool cmask_is_set(int mask, int cid) { return (mask&cmask_one(cid)) != 0; }

/*
	Tick
		Game Context (GAMECONTEXT::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (PLAYER::tick)
			

	Snap
		Game Context (GAMECONTEXT::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (PLAYER::snap)

*/


#include "eventhandler.hpp"

#include "entity.hpp"


#include "gamecontroller.hpp"
#include "entities/character.hpp"
#include "player.hpp"
#include "gamecontext.hpp"

enum
{
	CHAT_ALL=-2,
	CHAT_SPEC=-1,
	CHAT_RED=0,
	CHAT_BLUE=1
};
