#ifndef GAME_EDITOR_AUTO_MAP_H
#define GAME_EDITOR_AUTO_MAP_H

#include <base/tl/array.h>

class CAutoMapper
{
	struct CRuleCondition
	{
		int m_X;
		int m_Y;
		int m_Value;

		enum
		{
			EMPTY = -2,
			FULL = -1
		};
	};

	struct CRule
	{
		int m_Index;
		int m_HFlip;
		int m_VFlip;
		int m_Random;
		int m_Rotation;
		
		array<CRuleCondition> m_aConditions;
	};

	struct CRuleSet
	{
		char m_aName[128];
		int m_BaseTile;
		
		array<CRule> m_aRules;
	};

public:
	CAutoMapper(class CEditor *pEditor);

	void Load(const char* pTileName);
	void Proceed(class CLayerTiles *pLayer, int ConfigID);

	int RuleSetNum() { return m_aRuleSets.size(); }
	const char* GetRuleSetName(int Index);

	const bool IsLoaded() { return m_FileLoaded; }
private:
	array<CRuleSet> m_aRuleSets;
	class CEditor *m_pEditor;
	bool m_FileLoaded;
};


#endif
