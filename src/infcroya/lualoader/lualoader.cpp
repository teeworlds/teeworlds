#include "lualoader.h"

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#define FMT_HEADER_ONLY
#include <infcroya/fmt/format.h>
#include <game/server/gamecontext.h>

void LuaLoader::log_error(const char* error_msg) const
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "Lua error: %s", error_msg);
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", aBuf);
}

LuaLoader::LuaLoader(CGameContext* pGameServer)
{
	L = luaL_newstate();
	luaL_openlibs(L);
	this->m_pGameServer = pGameServer;
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader constructed");
}

LuaLoader::~LuaLoader()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader destructed");
	lua_close(L);
}

bool LuaLoader::load(const char* filename)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "LuaLoader::load(%s)", filename);
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", aBuf);

	if (luaL_loadfile(L, filename) != LUA_OK) {
		log_error(lua_tostring(L, -1));
		return false;
	}
	lua_pcall(L, 0, 0, 0);
	return true;
}

void LuaLoader::init(int num_players)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "LuaLoader::init(%d)", num_players);
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", aBuf);

	lua_pushinteger(L, num_players);
	lua_setglobal(L, "infc_num_players");

	lua_getglobal(L, "infc_init");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));
}

int LuaLoader::get_timelimit() const
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::get_timelimit()");

	lua_getglobal(L, "infc_get_timelimit");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));

	int timelimit = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return timelimit;
}

std::vector<vec2> LuaLoader::get_circle_positions()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::get_circle_positions()");

	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);

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
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::get_circle_radiuses()");

	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);
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
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::get_inf_circle_positions()");

	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);

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
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::get_inf_circle_radiuses()");

	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		log_error(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);
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