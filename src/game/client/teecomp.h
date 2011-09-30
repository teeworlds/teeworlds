#ifndef __TEECOMP_HPP_
#define __TEECOMP_HPP_

#include <base/vmath.h>

enum {
	TC_STATS_FRAGS=1,
	TC_STATS_DEATHS=2,
	TC_STATS_SUICIDES=4,
	TC_STATS_RATIO=8,
	TC_STATS_NET=16,
	TC_STATS_FPM=32,
	TC_STATS_SPREE=64,
	TC_STATS_BESTSPREE=128,
	TC_STATS_FLAGGRABS=256,
	TC_STATS_WEAPS=512,
	TC_STATS_FLAGCAPTURES=1024,

	TC_SCORE_COUNTRY=1,
	TC_SCORE_CLAN=2,
	TC_SCORE_PING=4,
};

class CTeecompUtils
{
public:
	static vec3 GetTeamColor(int ForTeam, int LocalTeam, int Color1, int Color2, int Method);
	static bool GetForcedSkinName(int ForTeam, int LocalTeam, const char*& SkinName);
	static bool GetForceDmColors(int ForTeam, int LocalTeam);
	static void ResetConfig();
	static const char* RgbToName(int rgb);
	static const char* TeamColorToName(int rgb);
};

#endif
