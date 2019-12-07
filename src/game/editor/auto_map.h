#ifndef GAME_EDITOR_AUTO_MAP_H
#define GAME_EDITOR_AUTO_MAP_H

#include <base/tl/array.h>
#include <base/vmath.h>

#include <engine/external/json-parser/json.h>

typedef struct
{
	int x, y;
	int w, h;
} RECTi;

class IAutoMapper
{
protected:
	class CEditor *m_pEditor;
	int m_Type;

public:
	enum
	{
		TYPE_TILESET,
		TYPE_DOODADS,

		MAX_RULES=256
	};

	IAutoMapper(class CEditor *pEditor, int Type) : m_pEditor(pEditor), m_Type(Type) {}
	virtual ~IAutoMapper() {};
	virtual void Load(const json_value &rElement) = 0;
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, RECTi Area) {}
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, int Ammount) {} // for convenience purposes

	virtual int RuleSetNum() = 0;
	virtual const char* GetRuleSetName(int Index) const = 0;

	int GetType() const { return m_Type; }

	static bool Random(int Value)
	{
		return (random_int() % Value) == 0;
	}

	static const char *GetTypeName(int Type)
	{
		if(Type == TYPE_TILESET)
			return "tileset";
		else if(Type == TYPE_DOODADS)
			return "doodads";
		else
			return "";
	}
};

class CTilesetMapper: public IAutoMapper
{
	struct CRuleCondition
	{
		int m_X;
		int m_Y;
		int m_Value;

		enum
		{
			EMPTY=-2,
			FULL=-1
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

	array<CRuleSet> m_aRuleSets;

public:
	CTilesetMapper(class CEditor *pEditor) : IAutoMapper(pEditor, TYPE_TILESET) { m_aRuleSets.clear(); }

	virtual void Load(const json_value &rElement);
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, RECTi Area);

	virtual int RuleSetNum() { return m_aRuleSets.size(); }
	virtual const char* GetRuleSetName(int Index) const;
};

class CDoodadsMapper: public IAutoMapper
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

		bool operator<(const CRule &Other) const
		{
			if((m_Location == CDoodadsMapper::CRule::FLOOR && Other.m_Location == CDoodadsMapper::CRule::FLOOR)
				|| (m_Location == CDoodadsMapper::CRule::CEILING && Other.m_Location == CDoodadsMapper::CRule::CEILING))
			{
				if(m_Size.x < Other.m_Size.x)
					return true;
			}
			else if(m_Location == CDoodadsMapper::CRule::WALLS && Other.m_Location == CDoodadsMapper::CRule::WALLS)
			{
				if(m_Size.y < Other.m_Size.y)
					return true;
			}

			return false;
		}
	};

	struct CRuleSet
	{
		char m_aName[128];

		array<CRule> m_aRules;
	};

	CDoodadsMapper(class CEditor *pEditor) :  IAutoMapper(pEditor, TYPE_DOODADS) { m_aRuleSets.clear(); }

	virtual void Load(const json_value &rElement);
	virtual void Proceed(class CLayerTiles *pLayer, int ConfigID, int Amount);
	void AnalyzeGameLayer();

	virtual int RuleSetNum() { return m_aRuleSets.size(); }
	virtual const char* GetRuleSetName(int Index) const;

private:
	void PlaceDoodads(CLayerTiles *pLayer, CRule *pRule, array<array<int> > *pPositions, int Amount, int LeftWall = 0);

	array<CRuleSet> m_aRuleSets;

	array<array<int> > m_FloorIDs;
	array<array<int> > m_CeilingIDs;
	array<array<int> > m_RightWallIDs;
	array<array<int> > m_LeftWallIDs;
};

#endif
