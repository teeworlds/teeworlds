/* copyright (c) 2007 magnus auvinen, see licence.txt for more info */
#include <engine/e_server_interface.h>
#include <game/server/entities/character.hpp>
#include <game/server/player.hpp>
#include "tdm.hpp"

GAMECONTROLLER_TDM::GAMECONTROLLER_TDM()
{
	gametype = "TDM";
	game_flags = GAMEFLAG_TEAMS;
}

int GAMECONTROLLER_TDM::on_character_death(class CHARACTER *victim, class PLAYER *killer, int weapon)
{
	GAMECONTROLLER::on_character_death(victim, killer, weapon);
	
	if(weapon >= 0)
	{
		// do team scoring
		if(killer == victim->player)
			teamscore[killer->team&1]--; // klant arschel
		else
			teamscore[killer->team&1]++; // good shit
	}
	return 0;
}

void GAMECONTROLLER_TDM::tick()
{
	do_team_score_wincheck();
	GAMECONTROLLER::tick();
}
