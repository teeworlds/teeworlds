#include "lualoader.h"

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#define FMT_HEADER_ONLY
#include <infcroya/fmt/format.h>

LuaLoader::LuaLoader()
{
	L = luaL_newstate();
	luaL_openlibs(L);
}

LuaLoader::~LuaLoader()
{
	fmt::print("LuaLoader destroyed!\n");
	lua_close(L);
}

bool LuaLoader::load(const char* filename)
{
	if (luaL_loadfile(L, filename) != LUA_OK) {
		fmt::print("{}\n", lua_tostring(L, -1));
		return false;
	}
	lua_pcall(L, 0, 0, 0);
	return true;
}

void LuaLoader::init(int num_players)
{
	lua_pushnumber(L, 228);
	lua_setglobal(L, "inf_num_players");

	lua_getglobal(L, "infc_init");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		fmt::print("{}\n", lua_tostring(L, -1));
}

std::vector<vec2> LuaLoader::get_circle_positions()
{
	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		fmt::print("{}\n", lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, 1);

		for (size_t i = 1; i <= len; i++) {
			if (i % 3 != 0) // 3 variables: x, y, Radius
				continue;

			lua_rawgeti(L, -1, i - 2);
			int x = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, i - 1);
			int y = lua_tointeger(L, -1);
			lua_pop(L, 1);

			positions.push_back(vec2(x, y));
		}
	}
	return positions;
}

std::vector<float> LuaLoader::get_circle_radiuses()
{
	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		fmt::print("{}\n", lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, 1);
		for (size_t i = 1; i <= len; i++) {
			if (i % 3 != 0) // 3 variables: x, y, Radius
				continue;

			lua_rawgeti(L, -1, i);
			int R = lua_tointeger(L, -1);
			lua_pop(L, 1);

			radiuses.push_back(R);
		}
	}
	return radiuses;
}

std::vector<vec2> LuaLoader::get_inf_circle_positions()
{
	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		fmt::print("{}\n", lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, 1);

		for (size_t i = 1; i <= len; i++) {
			if (i % 3 != 0) // 3 variables: x, y, Radius
				continue;

			lua_rawgeti(L, -1, i - 2);
			int x = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, i - 1);
			int y = lua_tointeger(L, -1);
			lua_pop(L, 1);

			positions.push_back(vec2(x, y));
		}
	}
	return positions;
}

std::vector<float> LuaLoader::get_inf_circle_radiuses()
{
	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		fmt::print("{}\n", lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, 1);
		for (size_t i = 1; i <= len; i++) {
			if (i % 3 != 0) // 3 variables: x, y, Radius
				continue;

			lua_rawgeti(L, -1, i);
			int R = lua_tointeger(L, -1);
			lua_pop(L, 1);

			radiuses.push_back(R);
		}
	}
	return radiuses;
}
