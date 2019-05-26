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
	class CGameContext* m_pGameServer;
	void LogError(const char* error_msg) const;
public:
	LuaLoader(class CGameContext* pGameServer);
	~LuaLoader();

	// path to file as parameter, e.g "maps/infc_normandie.lua"
	bool Load(const char* filename);

	void Init(int num_players);

	int GetTimelimit() const;

	std::vector<vec2> GetCirclePositions();
	std::vector<float> GetCircleRadiuses();
	std::vector<float> GetCircleMinRadiuses();
	std::vector<float> GetCircleShrinkSpeeds();

	std::vector<vec2> GetInfCirclePositions();
	std::vector<float> GetInfCircleRadiuses();
};