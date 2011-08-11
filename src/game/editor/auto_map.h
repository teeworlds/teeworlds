#ifndef GAME_EDITOR_AUTO_MAP_H
#define GAME_EDITOR_AUTO_MAP_H

#include <base/tl/array.h>

class CAutoMapper
{
	struct CPosRule
	{
		int m_X;
		int m_Y;
		int m_Value;
		bool m_IndexValue;

		enum
		{
			EMPTY=0,
			FULL
		};
	};

	struct CIndexRule
	{
		int m_ID;
		array<CPosRule> m_aRules;
		int m_Flag;
		int m_RandomValue;
		bool m_BaseTile;
	};

	struct CConfiguration
	{
		array<CIndexRule> m_aIndexRules;
		char m_aName[128];
	};

public:
	CAutoMapper(class CEditor *pEditor);

	void Load(const char* pTileName);
	void Proceed(class CLayerTiles *pLayer, int ConfigID);

	int ConfigNamesNum() { return m_lConfigs.size(); }
	const char* GetConfigName(int Index);

	const bool IsLoaded() { return m_FileLoaded; }
private:
	array<CConfiguration> m_lConfigs;
	class CEditor *m_pEditor;
	bool m_FileLoaded;
};


#endif
