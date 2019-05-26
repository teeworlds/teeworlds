#include "lualoader.h"

extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#define FMT_HEADER_ONLY
#include <infcroya/fmt/format.h>
#include <game/server/gamecontext.h>

void LuaLoader::LogError(const char* error_msg) const
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

bool LuaLoader::Load(const char* filename)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "LuaLoader::Load(%s)", filename);
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", aBuf);

	if (luaL_loadfile(L, filename) != LUA_OK) {
		LogError(lua_tostring(L, -1));
		return false;
	}
	lua_pcall(L, 0, 0, 0);
	return true;
}

void LuaLoader::Init(int num_players)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "LuaLoader::Init(%d)", num_players);
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", aBuf);

	lua_pushinteger(L, num_players);
	lua_setglobal(L, "infc_num_players");

	lua_getglobal(L, "infc_init");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
}

int LuaLoader::GetTimelimit() const
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetTimelimit()");

	lua_getglobal(L, "infc_get_timelimit");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));

	int timelimit = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return timelimit;
}

std::vector<vec2> LuaLoader::GetCirclePositions()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetCirclePositions()");

	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);

		for (size_t i = 1; i <= len; i++) {
			int NumVars = 5; // 5 variables: x, y, Radius, MinRadius, CircleShrinkSpeed
			if (i % NumVars != 0)
				continue;
			int ZeroIndex = i - NumVars;

			lua_rawgeti(L, -1, ZeroIndex + 1);
			int x = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, ZeroIndex + 2);
			int y = lua_tointeger(L, -1);
			lua_pop(L, 1);

			positions.push_back(vec2(x, y));
		}
	}
	return positions;
}

std::vector<float> LuaLoader::GetCircleRadiuses()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetCircleRadiuses()");

	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);
		for (size_t i = 1; i <= len; i++) {
			int NumVars = 5; // 5 variables: x, y, Radius, MinRadius, CircleShrinkSpeed
			if (i % NumVars != 0)
				continue;
			int ZeroIndex = i - NumVars;

			lua_rawgeti(L, -1, ZeroIndex + 3);
			float R = lua_tonumber(L, -1);
			lua_pop(L, 1);

			radiuses.push_back(R);
		}
	}
	return radiuses;
}

std::vector<float> LuaLoader::GetCircleMinRadiuses()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetCircleMinRadiuses()");

	std::vector<float> min_radiuses;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);
		for (size_t i = 1; i <= len; i++) {
			int NumVars = 5; // 5 variables: x, y, Radius, MinRadius, CircleShrinkSpeed
			if (i % NumVars != 0)
				continue;
			int ZeroIndex = i - NumVars;

			lua_rawgeti(L, -1, ZeroIndex + 4);
			float R = lua_tonumber(L, -1);
			lua_pop(L, 1);

			min_radiuses.push_back(R);
		}
	}
	return min_radiuses;
}

std::vector<float> LuaLoader::GetCircleShrinkSpeeds()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetCircleShrinkSpeeds()");

	std::vector<float> shrink_speeds;
	lua_getglobal(L, "infc_get_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
	else {
		size_t len = lua_rawlen(L, -1);
		for (size_t i = 1; i <= len; i++) {
			int NumVars = 5; // 5 variables: x, y, Radius, MinRadius, CircleShrinkSpeed
			if (i % NumVars != 0)
				continue;
			int ZeroIndex = i - NumVars;

			lua_rawgeti(L, -1, ZeroIndex + 5);
			float ShrinkSpeed = lua_tonumber(L, -1);
			lua_pop(L, 1);

			shrink_speeds.push_back(ShrinkSpeed);
		}
	}
	return shrink_speeds;
}

std::vector<vec2> LuaLoader::GetInfCirclePositions()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetInfCirclePositions()");

	std::vector<vec2> positions;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
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

std::vector<float> LuaLoader::GetInfCircleRadiuses()
{
	m_pGameServer->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua", "LuaLoader::GetInfCircleRadiuses()");

	std::vector<float> radiuses;
	lua_getglobal(L, "infc_get_inf_circle_pos");
	if (lua_pcall(L, 0, 1, 0) != LUA_OK)
		LogError(lua_tostring(L, -1));
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
