#pragma once

#include <vector>
#include <base/vmath.h>

/**
Class that loads <mapname>.lua and returns circle positions.
todo: implement circle resizing, moving etc
*/
class LuaLoader {
private:
	struct lua_State* L;
public:
	LuaLoader();
	~LuaLoader();

	// path to file as parameter, e.g "maps/infc_normandie.lua"
	bool load(const char* filename);

	void init(int num_players);

	int get_timelimit() const;

	std::vector<vec2> get_circle_positions();
	std::vector<float> get_circle_radiuses();

	std::vector<vec2> get_inf_circle_positions();
	std::vector<float> get_inf_circle_radiuses();
};