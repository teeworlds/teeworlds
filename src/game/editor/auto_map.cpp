#include <stdio.h>	// sscanf

#include <engine/console.h>
#include <engine/storage.h>
#include <engine/shared/linereader.h>

#include "auto_map.h"
#include "editor.h"

#include <engine/external/tinyxml/tinyxml.h>

CTileSetMapper::CTileSetMapper(CEditor *pEditor)
{
	m_pEditor = pEditor;
}

void CTileSetMapper::Load(TiXmlElement* pElement)
{
	TiXmlElement* pConfNode = pElement->FirstChildElement();
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
					
				clamp(NewRuleSet.m_BaseTile, 0, 255);
			}

			if(str_comp(pRuleNode->Value(), "Rule"))
				continue;
			
			// create a new rule
			CRule NewRule;
			if(pRuleNode->QueryIntAttribute("index", &NewRule.m_Index) != TIXML_SUCCESS)
				NewRule.m_Index = 1;
				
			clamp(NewRule.m_Index, 0, 255);
			
			NewRule.m_Random = 0;
			
			if(pRuleNode->QueryIntAttribute("rotate", &NewRule.m_Rotation) != TIXML_SUCCESS)
				NewRule.m_Rotation = 0;
			if(NewRule.m_Rotation != 90 && NewRule.m_Rotation != 180 && NewRule.m_Rotation != 270)
				NewRule.m_Rotation = 0;
			
			if(pRuleNode->QueryIntAttribute("hflip", &NewRule.m_HFlip) != TIXML_SUCCESS)
				NewRule.m_HFlip = 0;
			if(pRuleNode->QueryIntAttribute("vflip", &NewRule.m_VFlip) != TIXML_SUCCESS)
				NewRule.m_VFlip = 0;
			
			clamp(NewRule.m_HFlip, 0, 1);
			clamp(NewRule.m_VFlip, 0, 1);
			
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
						
						Condition.m_Value = CRuleCondition::EMPTY;
						if(str_comp(pValue, "full") == 0)
							Condition.m_Value = CRuleCondition::FULL;
					}
					else
						clamp(Condition.m_Value, (int)CRuleCondition::EMPTY, 255);
					
					NewRule.m_aConditions.add(Condition);
				}
				else if(str_comp(pNode->Value(), "Random") == 0)
				{
					if(pNode->QueryIntAttribute("value", &NewRule.m_Random) != TIXML_SUCCESS)
						NewRule.m_Random = 0;
						
					clamp(NewRule.m_Random, 0, 99999);
				}
			}
			
			NewRuleSet.m_aRules.add(NewRule);
		}
		
		m_aRuleSets.add(NewRuleSet);
	}
}

const char* CTileSetMapper::GetRuleSetName(int Index)
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CTileSetMapper::Proceed(CLayerTiles *pLayer, int ConfigID)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;

	int BaseTile = pConf->m_BaseTile;

	// auto map !
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	for(int y = 0; y < pLayer->m_Height; y++)
		for(int x = 0; x < pLayer->m_Width; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			if(pTile->m_Index == 0)
				continue;

			pTile->m_Index = BaseTile;

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
	
	m_pEditor->m_Map.m_Modified = true;
}

CDoodadMapper::CDoodadMapper(CEditor *pEditor)
{
	m_pEditor = pEditor;
}

int CompareRules(const void *a, const void *b)
{
	CDoodadMapper::CRule *ra = (CDoodadMapper::CRule*)a;
	CDoodadMapper::CRule *rb = (CDoodadMapper::CRule*)b;
	
	if((ra->m_Location == CDoodadMapper::CRule::FLOOR && rb->m_Location == CDoodadMapper::CRule::FLOOR)
		|| (ra->m_Location == CDoodadMapper::CRule::CEILING && rb->m_Location  == CDoodadMapper::CRule::CEILING))
	{
		if(ra->m_Size.x < rb->m_Size.x)
        return +1;
		if(rb->m_Size.x < ra->m_Size.x)
			return -1;
	}
	else if(ra->m_Location == CDoodadMapper::CRule::WALLS && rb->m_Location == CDoodadMapper::CRule::WALLS)
	{
		if(ra->m_Size.y < rb->m_Size.y)
        return +1;
		if(rb->m_Size.y < ra->m_Size.y)
			return -1;
	}
    
    return 0;
}

void CDoodadMapper::Load(TiXmlElement* pElement)
{
	char aBuf[256];
	
	TiXmlElement* pConfNode = pElement->FirstChildElement();
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
			if(str_comp(pRuleNode->Value(), "Rule"))
				continue;
			
			// create a new rule
			CRule NewRule;
			if(pRuleNode->QueryIntAttribute("startid", &NewRule.m_Rect.x) != TIXML_SUCCESS)
				NewRule.m_Rect.x = 1;
			if(pRuleNode->QueryIntAttribute("endid", &NewRule.m_Rect.y) != TIXML_SUCCESS)
				NewRule.m_Rect.y = 1;
				
			if(pRuleNode->QueryIntAttribute("rx", &NewRule.m_RelativePos.x) != TIXML_SUCCESS)
				NewRule.m_RelativePos.x = 0;
			if(pRuleNode->QueryIntAttribute("ry", &NewRule.m_RelativePos.y) != TIXML_SUCCESS)
				NewRule.m_RelativePos.y = 0;
				
			clamp(NewRule.m_Rect.x, 0, 255);
			clamp(NewRule.m_Rect.y, 0, 255);
			
			// broken, skip
			if(NewRule.m_Rect.x > NewRule.m_Rect.y)
				continue;
			
			// width
			NewRule.m_Size.x = abs(NewRule.m_Rect.x-NewRule.m_Rect.y)%16+1;
			// height
			NewRule.m_Size.y = floor((float)abs(NewRule.m_Rect.x-NewRule.m_Rect.y)/16)+1;
			
			NewRule.m_Random = 1;
			
			if(pRuleNode->QueryIntAttribute("hflip", &NewRule.m_HFlip) != TIXML_SUCCESS)
				NewRule.m_HFlip = 0;
			if(pRuleNode->QueryIntAttribute("vflip", &NewRule.m_VFlip) != TIXML_SUCCESS)
				NewRule.m_VFlip = 0;
			
			clamp(NewRule.m_HFlip, 0, 1);
			clamp(NewRule.m_VFlip, 0, 1);
			
			// get rule's content
			TiXmlElement* pNode = pRuleNode->FirstChildElement();
			for(pNode; pNode; pNode = pNode->NextSiblingElement())
			{
				if(str_comp(pNode->Value(), "Location") == 0)
				{
					// the value is not an index, check if it's full or empty
					const char* pValue = pNode->Attribute("value");
					
					NewRule.m_Location = CRule::FLOOR;
					if(str_comp(pValue, "floor") == 0)
						NewRule.m_Location = CRule::FLOOR;
					if(str_comp(pValue, "ceiling") == 0)
						NewRule.m_Location = CRule::CEILING;
					if(str_comp(pValue, "walls") == 0)
						NewRule.m_Location = CRule::WALLS;
				}
				else if(str_comp(pNode->Value(), "Random") == 0)
				{
					if(pNode->QueryIntAttribute("value", &NewRule.m_Random) != TIXML_SUCCESS)
						NewRule.m_Random = 1;
						
					clamp(NewRule.m_Random, 1, 9999);
				}
			}
			
			NewRuleSet.m_aRules.add(NewRule);
		}
		
		m_aRuleSets.add(NewRuleSet);
	}
	
	// sort
	for(int i = 0; i < m_aRuleSets.size(); i++)
		qsort(m_aRuleSets[i].m_aRules.base_ptr(), m_aRuleSets[i].m_aRules.size(), sizeof(m_aRuleSets[i].m_aRules[0]), CompareRules);
	
	str_format(aBuf, sizeof(aBuf),"Frist size: %d", m_aRuleSets[0].m_aRules[0].m_Size.x);
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);
}

const char* CDoodadMapper::GetRuleSetName(int Index)
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CDoodadMapper::AnalyzeGameLayer()
{
	// the purpose of this is to detect game layer collision's edges to place doodads according to them
	
	// clear existing edges
	m_FloorIDs.clear();
	m_CeilingIDs.clear();
	m_RightWallIDs.clear();
	m_LeftWallIDs.clear();
	
	CLayerGame *pLayer = m_pEditor->m_Map.m_pGameLayer;
	
	bool FloorKeepChaining = false;
	bool CeilingKeepChaining = false;
	int FloorChainID = 0;
	int CeilingChainID = 0;
	
	// floors and ceilings
	// browse up to down
	for(int y = 1; y < pLayer->m_Height-1; y++)
	{
		FloorKeepChaining = false;
		CeilingKeepChaining = false;
		
		for(int x = 1; x < pLayer->m_Width-1; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			
			// empty, skip
			if(pTile->m_Index == 0)
			{
				FloorKeepChaining = false;
				CeilingKeepChaining = false;
				continue;
			}
			
			// check up
			int CheckIndex = (y-1)*pLayer->m_Width+x;
			
			// add a floor part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!FloorKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					FloorChainID = m_FloorIDs.add(aChain);
					FloorKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_FloorIDs[FloorChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				FloorKeepChaining = false;
			
			// check down
			CheckIndex = (y+1)*pLayer->m_Width+x;
			
			// add a ceiling part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!CeilingKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					CeilingChainID = m_CeilingIDs.add(aChain);
					CeilingKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_CeilingIDs[CeilingChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				CeilingKeepChaining = false;
		}
	}
		
	
	bool RWallKeepChaining = false;
	bool LWallKeepChaining = false;
	int RWallChainID = 0;
	int LWallChainID = 0;
	
	// walls
	// browse left to right
	for(int x = 1; x < pLayer->m_Width-1; x++)
	{
		RWallKeepChaining = false;
		LWallKeepChaining = false;
	
		for(int y = 1; y < pLayer->m_Height-1; y++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			
			if(pTile->m_Index == 0)
			{
				RWallKeepChaining = false;
				LWallKeepChaining = false;
				continue;
			}
			
			// check right
			int CheckIndex = y*pLayer->m_Width+(x+1);
			
			// add a right wall part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!RWallKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					RWallChainID = m_RightWallIDs.add(aChain);
					RWallKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_RightWallIDs[RWallChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				RWallKeepChaining = false;
			
			// check left
			CheckIndex = y*pLayer->m_Width+(x-1);
			
			// add a left wall part
			if(pTile->m_Index == 1 && (pLayer->m_pTiles[CheckIndex].m_Index == 0 || pLayer->m_pTiles[CheckIndex].m_Index > ENTITY_OFFSET))
			{
				// create an new chain
				if(!LWallKeepChaining)
				{
					array<int> aChain;
					aChain.add(y*pLayer->m_Width+x);
					LWallChainID = m_LeftWallIDs.add(aChain);
					LWallKeepChaining = true;
				}
				else
				{
					// keep chaining
					m_LeftWallIDs[LWallChainID].add(y*pLayer->m_Width+x);
				}
			}
			else
				LWallKeepChaining = false;
		}
	}
	
	char aBuf[256];
	
	// clean up too small chains
	for(int i = 0; i < m_FloorIDs.size(); i++)
	{
		if(m_FloorIDs[i].size() < 3)
		{
			m_FloorIDs.remove_index_fast(i);
			i--;
		}
	}
	
	for(int i = 0; i < m_CeilingIDs.size(); i++)
	{
		if(m_CeilingIDs[i].size() < 3)
		{
			m_CeilingIDs.remove_index_fast(i);
			i--;
		}
	}
	
	for(int i = 0; i < m_RightWallIDs.size(); i++)
	{
		if(m_RightWallIDs[i].size() < 3)
		{
			m_RightWallIDs.remove_index_fast(i);
			i--;
		}
	}
	
	for(int i = 0; i < m_LeftWallIDs.size(); i++)
	{
		if(m_LeftWallIDs[i].size() < 3)
		{
			m_LeftWallIDs.remove_index_fast(i);
			i--;
		}
	}
}

void CDoodadMapper::PlaceDoodads(CLayerTiles *pLayer, CRule *pRule, array<array<int>> *pPositions, int Amount, int LeftWall)
{
	if(pRule->m_Location == CRule::CEILING)
		pRule->m_RelativePos.y++;
	else if(pRule->m_Location == CRule::WALLS)
		pRule->m_RelativePos.x++;
		
	// left walls
	if(LeftWall)
	{
		pRule->m_HFlip ^= LeftWall;
		pRule->m_RelativePos.x--;
		pRule->m_RelativePos.x = -pRule->m_RelativePos.x;
		pRule->m_RelativePos.x -= pRule->m_Size.x-1;
	}
	
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	int RandomValue = pRule->m_Random*((101.f-Amount)/100.0f);
	
	if(pRule->m_Random == 1)
		RandomValue = 1;
	
	// allow diversity with high Amount
	if(pRule->m_Random > 1 && RandomValue <= 1)
		RandomValue = 2;
	
	for(int f = 0; f < pPositions->size(); f++)
		for(int c = 0; c < (*pPositions)[f].size(); c += pRule->m_Size.x)
		{
			if((pRule->m_Location == CRule::FLOOR || pRule->m_Location == CRule::CEILING)
				&& (*pPositions)[f].size()-c < pRule->m_Size.x)
				break;
				
			if(pRule->m_Location == CRule::WALLS && (*pPositions)[f].size()-c < pRule->m_Size.y)
				break;
				
			if(RandomValue > 1 && !IAutoMapper::Random(RandomValue))
				continue;
			
			// where to put it
			int ID = (*pPositions)[f][c];
			
			// relative position
			ID += pRule->m_RelativePos.x;
			ID += pRule->m_RelativePos.y*pLayer->m_Width;
	
			for(int y = 0; y < pRule->m_Size.y; y++)
				for(int x = 0; x < pRule->m_Size.x; x++)
				{
					int Index = y*pLayer->m_Width+x+ID;
					if(Index <= 0 || Index >= MaxIndex)
						continue;
					
					pLayer->m_pTiles[Index].m_Index = pRule->m_Rect.x+y*16+x;
				}

			// hflip
			if(pRule->m_HFlip)
			{
				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x/2; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;
						
						int CheckIndex = Index+pRule->m_Size.x-1-x*2;
						
						if(CheckIndex <= 0 || CheckIndex >= MaxIndex)
							continue;
							
						CTile Tmp = pLayer->m_pTiles[Index];
						pLayer->m_pTiles[Index] = pLayer->m_pTiles[CheckIndex];
						pLayer->m_pTiles[CheckIndex] = Tmp;
					}
				
				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;
							
						pLayer->m_pTiles[Index].m_Flags ^= TILEFLAG_VFLIP;
					}
			}
			
			// vflip
			if(pRule->m_VFlip)
			{
				for(int y = 0; y < pRule->m_Size.y/2; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;
						
						int CheckIndex = Index+(pRule->m_Size.y-1-y*2)*pLayer->m_Width;
						
						if(CheckIndex <= 0 || CheckIndex >= MaxIndex)
							continue;
							
						CTile Tmp = pLayer->m_pTiles[Index];
						pLayer->m_pTiles[Index] = pLayer->m_pTiles[CheckIndex];
						pLayer->m_pTiles[CheckIndex] = Tmp;
					}
				
				for(int y = 0; y < pRule->m_Size.y; y++)
					for(int x = 0; x < pRule->m_Size.x; x++)
					{
						int Index = y*pLayer->m_Width+x+ID;
						if(Index <= 0 || Index >= MaxIndex)
							continue;
							
						pLayer->m_pTiles[Index].m_Flags ^= TILEFLAG_HFLIP;
					}
			}
			
			// make the place occupied
			if(RandomValue > 1)
			{
				array<int> aChainBefore;
				array<int> aChainAfter;
				
				for(int j = 0; j < c; j++)
					aChainBefore.add((*pPositions)[f][j]);
				
				int Size = pRule->m_Size.x;
				if(pRule->m_Location == CRule::WALLS)
					Size = pRule->m_Size.y;
				
				for(int j = c+Size; j < (*pPositions)[f].size(); j++)
					aChainAfter.add((*pPositions)[f][j]);
				
				pPositions->remove_index(f);
				
				// f changes, reset c
				c = -1;
				
				if(aChainBefore.size() > 1)
					pPositions->add(aChainBefore);
				if(aChainAfter.size() > 1)
					pPositions->add(aChainAfter);
			}
		}
}

void CDoodadMapper::Proceed(CLayerTiles *pLayer, int ConfigID, int Amount)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;
	
	AnalyzeGameLayer();

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;
	
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	
	// clear tiles
	for(int i = 0 ; i < MaxIndex; i++)
	{
		pLayer->m_pTiles[i].m_Index = 0;
		pLayer->m_pTiles[i].m_Flags = 0;
	}
	
	// place doodads
	for(int i = 0 ; i < pConf->m_aRules.size(); i++)
	{
		CRule *pRule = &pConf->m_aRules[i];
		
		// floors
		if(pRule->m_Location == CRule::FLOOR && m_FloorIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_FloorIDs, Amount);
		}
		
		// ceilings
		if(pRule->m_Location == CRule::CEILING && m_CeilingIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_CeilingIDs, Amount);
		}
		
		// right walls
		if(pRule->m_Location == CRule::WALLS && m_RightWallIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_RightWallIDs, Amount);
		}
		
		// left walls
		if(pRule->m_Location == CRule::WALLS && m_LeftWallIDs.size() > 0)
		{
			PlaceDoodads(pLayer, pRule, &m_LeftWallIDs, Amount, 1);
		}
	}
	
	m_pEditor->m_Map.m_Modified = true;
}
