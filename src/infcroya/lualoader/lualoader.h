#pragma once

#include <functional>

class LuaLoader {
private:
	struct lua_State* L;
public:
	LuaLoader();
	~LuaLoader();

	bool load(const char* filename);
	bool call(const char* function_name, std::function<int()> push_variables);
};