#include <stdio.h>	// sscanf

#include <engine/console.h>
#include <engine/storage.h>
#include <engine/shared/linereader.h>

#include "auto_map.h"
#include "editor.h"

#include <engine/external/tinyxml/tinyxml.h>

CAutoMapper::CAutoMapper(CEditor *pEditor)
{
	m_pEditor = pEditor;
	m_FileLoaded = false;
}

void CAutoMapper::Load(const char* pTileName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "data/editor/automap/%s.xml", pTileName);
	
	char aBuf[256];
	
	TiXmlDocument File(aPath);
	m_FileLoaded = File.LoadFile();
	
	if(!m_FileLoaded)
	{
		str_format(aBuf, sizeof(aBuf),"failed to load %s.xml", pTileName);
		m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);
		return;
	}
	
	// get configurations
	TiXmlHandle hFile(&File);
	TiXmlElement* pConfNode = hFile.FirstChildElement().Element();
	for(pConfNode; pConfNode; pConfNode = pConfNode->NextSiblingElement())
	{
		// new rule set
		CRuleSet NewRuleSet;
		const char* pConfName = pConfNode->Value();
		str_copy(NewRuleSet.m_aName, pConfName, sizeof(NewRuleSet.m_aName));

		// get rules
		TiXmlElement* pRuleNode = pConfNode->FirstChildElement();
		for(pRuleNode; pRuleNode; pRuleNode = pRuleNode->NextSiblingElement())
		{
			if(str_comp(pRuleNode->Value(), "BaseTile") == 0)
			{
				if(pRuleNode->QueryIntAttribute("index", &NewRuleSet.m_BaseTile) != TIXML_SUCCESS)
					NewRuleSet.m_BaseTile = 1;
			}

			if(str_comp(pRuleNode->Value(), "Rule"))
				continue;
			
			// create a new rule
			CRule NewRule;
			if(pRuleNode->QueryIntAttribute("index", &NewRule.m_Index) != TIXML_SUCCESS)
				NewRule.m_Index = 1;
			
			NewRule.m_Random = 0;
			
			if(pRuleNode->QueryIntAttribute("rotate", &NewRule.m_Rotation) != TIXML_SUCCESS)
				NewRule.m_Rotation = 0;
			if(NewRule.m_Rotation != 90 && NewRule.m_Rotation != 180 && NewRule.m_Rotation != 270)
				NewRule.m_Rotation = 0;
			
			if(pRuleNode->QueryIntAttribute("hflip", &NewRule.m_HFlip) != TIXML_SUCCESS)
				NewRule.m_HFlip = 0;
			if(pRuleNode->QueryIntAttribute("vflip", &NewRule.m_VFlip) != TIXML_SUCCESS)
				NewRule.m_VFlip = 0;
			
			// get rule's content
			TiXmlElement* pNode = pRuleNode->FirstChildElement();
			for(pNode; pNode; pNode = pNode->NextSiblingElement())
			{
				if(str_comp(pNode->Value(), "Condition") == 0)
				{
					CRuleCondition Condition;
					
					if(pNode->QueryIntAttribute("x", &Condition.m_X) != TIXML_SUCCESS)
						Condition.m_X = 0;
					if(pNode->QueryIntAttribute("y", &Condition.m_Y) != TIXML_SUCCESS)
						Condition.m_Y = 0;
					if(pNode->QueryIntAttribute("value", &Condition.m_Value) == TIXML_WRONG_TYPE)
					{
						// the value is not an index, check if it's full or empty
						const char* pValue = pNode->Attribute("value");
						
						if(str_comp(pValue, "full") == 0)
							Condition.m_Value = CRuleCondition::FULL;
						else if(str_comp(pValue, "empty") == 0)
							Condition.m_Value = CRuleCondition::EMPTY;
						
						NewRule.m_aConditions.add(Condition);
					}
				}
				else if(str_comp(pNode->Value(), "Random") == 0)
				{
					if(pNode->QueryIntAttribute("value", &NewRule.m_Random) != TIXML_SUCCESS)
						NewRule.m_Random = 0;
				}
			}
			
			NewRuleSet.m_aRules.add(NewRule);
		}
		
		m_aRuleSets.add(NewRuleSet);
	}
	
	str_format(aBuf, sizeof(aBuf),"loaded %s.xml", pTileName);
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);
}

const char* CAutoMapper::GetRuleSetName(int Index)
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CAutoMapper::Proceed(CLayerTiles *pLayer, int ConfigID)
{
	if(!m_FileLoaded || pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;

	char aBuf[256];
	int BaseTile = pConf->m_BaseTile;
	str_format(aBuf, sizeof(aBuf),"BaseTile: %d", ConfigID);
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);

	// auto map !
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	for(int y = 0; y < pLayer->m_Height; y++)
		for(int x = 0; x < pLayer->m_Width; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			if(pTile->m_Index == 0)
				continue;

			pTile->m_Index = BaseTile;
			m_pEditor->m_Map.m_Modified = true;

			if(y == 0 || y == pLayer->m_Height-1 || x == 0 || x == pLayer->m_Width-1)
				continue;

			for(int i = 0; i < pConf->m_aRules.size(); ++i)
			{
				bool RespectRules = true;
				for(int j = 0; j < pConf->m_aRules[i].m_aConditions.size() && RespectRules; ++j)
				{
					CRuleCondition *pCondition = &pConf->m_aRules[i].m_aConditions[j];
					int CheckIndex = (y+pCondition->m_Y)*pLayer->m_Width+(x+pCondition->m_X);

					if(CheckIndex < 0 || CheckIndex >= MaxIndex)
						RespectRules = false;
					else
					{
 						if(pCondition->m_Value == CRuleCondition::EMPTY || pCondition->m_Value == CRuleCondition::FULL)
						{
							if(pLayer->m_pTiles[CheckIndex].m_Index > 0 && pCondition->m_Value == CRuleCondition::EMPTY)
								RespectRules = false;

							if(pLayer->m_pTiles[CheckIndex].m_Index == 0 && pCondition->m_Value == CRuleCondition::FULL)
								RespectRules = false;
						}
						else
						{
							if(pLayer->m_pTiles[CheckIndex].m_Index != pCondition->m_Value)
								RespectRules = false;
						}
					}
				}

				if(RespectRules &&
					(pConf->m_aRules[i].m_Random <= 1 || (int)((float)rand() / ((float)RAND_MAX + 1) * pConf->m_aRules[i].m_Random) == 1))
				{
					pTile->m_Index = pConf->m_aRules[i].m_Index;
					
					// rotate
					if(pConf->m_aRules[i].m_Rotation == 90)
						pTile->m_Flags ^= TILEFLAG_ROTATE;
					else if(pConf->m_aRules[i].m_Rotation == 180)
						pTile->m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP);
					else if(pConf->m_aRules[i].m_Rotation == 270)
						pTile->m_Flags ^= (TILEFLAG_HFLIP|TILEFLAG_VFLIP|TILEFLAG_ROTATE);
					
					// flip
					if(pConf->m_aRules[i].m_HFlip)
						pTile->m_Flags ^= pTile->m_Flags&TILEFLAG_ROTATE ? TILEFLAG_HFLIP : TILEFLAG_VFLIP;
					if(pConf->m_aRules[i].m_VFlip)
						pTile->m_Flags ^= pTile->m_Flags&TILEFLAG_ROTATE ? TILEFLAG_VFLIP : TILEFLAG_HFLIP;
				}
			}
		}
}
