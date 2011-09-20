#ifndef GAME_EDITOR_AUTO_MAP_H
#define GAME_EDITOR_AUTO_MAP_H

#include <stdlib.h> // rand
#include <base/tl/array.h>
#include <base/vmath.h>

class IAutoMapper
{
public:
	virtual void Load(class TiXmlElement* pElement) = 0;
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID) {}
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, int Ammount) {} // for convenience purposes

	virtual int RuleSetNum() = 0;
	virtual const char* GetRuleSetName(int Index) = 0;
	
	virtual int GetType() = 0;
	
	static bool Random(int Value)
	{
		return ((int)((float)rand() / ((float)RAND_MAX + 1) * Value) == 1);
	}
	
	enum
	{
		TYPE_TILESET,
		TYPE_DOODADS
	};
};

class CTileSetMapper: public IAutoMapper
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
	CTileSetMapper(class CEditor *pEditor);

	virtual void Load(class TiXmlElement* pElement);
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID);

	virtual int RuleSetNum() { return m_aRuleSets.size(); }
	virtual const char* GetRuleSetName(int Index);
	
	virtual int GetType() { return IAutoMapper::TYPE_TILESET; }
	
private:
	array<CRuleSet> m_aRuleSets;
	class CEditor *m_pEditor;
};

class CDoodadMapper: public IAutoMapper
{
public:
	struct CRule
	{
		ivec2 m_Rect;
		ivec2 m_Size;
		ivec2 m_RelativePos;
		
		int m_Location;
		int m_Random;
		
		int m_HFlip;
		int m_VFlip;
		
		enum
		{
			FLOOR=0,
			CEILING,
			WALLS
		};
	};
	
	struct CRuleSet
	{
		char m_aName[128];
		
		array<CRule> m_aRules;
	};


	CDoodadMapper(class CEditor *pEditor);

	virtual void Load(class TiXmlElement* pElement);
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, int Ammount);
	void AnalyzeGameLayer();

	virtual int RuleSetNum() { return m_aRuleSets.size(); }
	virtual const char* GetRuleSetName(int Index);
	
	virtual int GetType() { return IAutoMapper::TYPE_DOODADS; }
	
private:
	array<CRuleSet> m_aRuleSets;
	
	array<array<int>> m_FloorIDs;
	array<array<int>> m_CeilingIDs;
	array<array<int>> m_RightWallIDs;
	array<array<int>> m_LeftWallIDs;
	
	class CEditor *m_pEditor;
};

#endif
