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
	lua_close(L);
}

bool LuaLoader::load(const char* filename)
{
	if (luaL_loadfile(L, filename) != LUA_OK) {
		fmt::print("{}\n", lua_tostring(L, -1));
		return false;
	}
	return true;
}

bool LuaLoader::call(const char* function_name, std::function<int()> push_variables)
{
	lua_getglobal(L, function_name);
	int num_args = push_variables();
	if (lua_pcall(L, num_args, 0, 0) != LUA_OK) {
		fmt::print("{}\n", lua_tostring(L, -1));
		return false;
	}
	return true;
}
