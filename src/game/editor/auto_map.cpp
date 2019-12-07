#include <engine/console.h>
#include <engine/storage.h>

#include "auto_map.h"
#include "editor.h"


void CTilesetMapper::Load(const json_value &rElement)
{
	for(unsigned i = 0; i < rElement.u.array.length; ++i)
	{
		if(rElement[i].u.object.length != 1)
			continue;

		// new rule set
		CRuleSet NewRuleSet;
		const char* pConfName = rElement[i].u.object.values[0].name;
		str_copy(NewRuleSet.m_aName, pConfName, sizeof(NewRuleSet.m_aName));
		const json_value &rStart = *(rElement[i].u.object.values[0].value);

		// get basetile
		const json_value &rBasetile = rStart["basetile"];
		if(rBasetile.type == json_integer)
			NewRuleSet.m_BaseTile = clamp((int)rBasetile.u.integer, 0, 255);
		else
			NewRuleSet.m_BaseTile = 1;

		// get rules
		const json_value &rRuleNode = rStart["rules"];
		for(unsigned j = 0; j < rRuleNode.u.array.length && j < MAX_RULES; j++)
		{
			// create a new rule
			CRule NewRule;

			// index
			const json_value &rIndex = rRuleNode[j]["index"];
			if(rIndex.type == json_integer)
				NewRule.m_Index = clamp((int)rIndex.u.integer, 0, 255);
			else
				NewRule.m_Index = 1;

			// random
			const json_value &rRandom = rRuleNode[j]["random"];
			if(rRandom.type == json_integer)
				NewRule.m_Random = clamp((int)rRandom.u.integer, 0, 99999);
			else
				NewRule.m_Random = 0;

			// rotate
			const json_value &rRotate = rRuleNode[j]["rotate"];
			if(rRotate.type == json_integer && (rRotate.u.integer == 90 || rRotate.u.integer == 180 || rRotate.u.integer == 270))
				NewRule.m_Rotation = rRotate.u.integer;
			else
				NewRule.m_Rotation = 0;

			// hflip
			const json_value &rHFlip = rRuleNode[j]["hflip"];
			if(rHFlip.type == json_integer)
				NewRule.m_HFlip = clamp((int)rHFlip.u.integer, 0, 1);
			else
				NewRule.m_HFlip = 0;

			// vflip
			const json_value &rVFlip = rRuleNode[j]["vflip"];
			if(rVFlip.type == json_integer)
				NewRule.m_VFlip = clamp((int)rVFlip.u.integer, 0, 1);
			else
				NewRule.m_VFlip = 0;

			// get rule's content
			const json_value &rCondition = rRuleNode[j]["condition"];
			if(rCondition.type == json_array)
			{
				for(unsigned k = 0; k < rCondition.u.array.length; k++)
				{
					CRuleCondition Condition;

					Condition.m_X = rCondition[k]["x"].u.integer;
					Condition.m_Y = rCondition[k]["y"].u.integer;
					const json_value &rValue = rCondition[k]["value"];
					if(rValue.type == json_string)
					{
						// the value is not an index, check if it's full or empty
						if(str_comp((const char *)rValue, "full") == 0)
							Condition.m_Value = CRuleCondition::FULL;
						else
							Condition.m_Value = CRuleCondition::EMPTY;
					}
					else if(rValue.type == json_integer)
						Condition.m_Value = clamp((int)rValue.u.integer, (int)CRuleCondition::EMPTY, 255);
					else
						Condition.m_Value = CRuleCondition::EMPTY;

					NewRule.m_aConditions.add(Condition);
				}
			}

			NewRuleSet.m_aRules.add(NewRule);
		}

		m_aRuleSets.add(NewRuleSet);
	}
}

const char* CTilesetMapper::GetRuleSetName(int Index) const
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CTilesetMapper::Proceed(CLayerTiles *pLayer, int ConfigID, RECTi Area)
{
	if(pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_aRuleSets.size())
		return;

	CRuleSet *pConf = &m_aRuleSets[ConfigID];

	if(!pConf->m_aRules.size())
		return;

	pLayer->Clamp(&Area);
	
	int BaseTile = pConf->m_BaseTile;

	// auto map !
	int MaxIndex = pLayer->m_Width*pLayer->m_Height;
	for(int y = Area.y; y < Area.y + Area.h; y++)
		for(int x = Area.x; x < Area.x + Area.w; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			if(pTile->m_Index == 0)
				continue;

			pTile->m_Index = BaseTile;

			for(int i = 0; i < pConf->m_aRules.size(); ++i)
			{
				bool RespectRules = true;
				for(int j = 0; j < pConf->m_aRules[i].m_aConditions.size() && RespectRules; ++j)
				{
					CRuleCondition *pCondition = &pConf->m_aRules[i].m_aConditions[j];
					int CheckIndex = clamp((y+pCondition->m_Y), 0, pLayer->m_Height-1)*pLayer->m_Width+clamp((x+pCondition->m_X), 0, pLayer->m_Width-1);

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

				if(RespectRules && (pConf->m_aRules[i].m_Random <= 1 || (int)(frandom() * pConf->m_aRules[i].m_Random) == 1))
				{
					pTile->m_Index = pConf->m_aRules[i].m_Index;
					pTile->m_Flags = 0;

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

void CDoodadsMapper::Load(const json_value &rElement)
{
	for(unsigned i = 0; i < rElement.u.array.length; ++i)
	{
		if(rElement[i].u.object.length != 1)
			continue;

		// new rule set
		CRuleSet NewRuleSet;
		const char* pConfName = rElement[i].u.object.values[0].name;
		str_copy(NewRuleSet.m_aName, pConfName, sizeof(NewRuleSet.m_aName));
		const json_value &rStart = *(rElement[i].u.object.values[0].value);

		// get rules
		const json_value &rRuleNode = rStart["rules"];
		for(unsigned j = 0; j < rRuleNode.u.array.length && j < MAX_RULES; j++)
		{
			// create a new rule
			CRule NewRule;

			// startid
			const json_value &rStartid = rRuleNode[j]["startid"];
			if(rStartid.type == json_integer)
				NewRule.m_Rect.x = clamp((int)rStartid.u.integer, 0, 255);
			else
				NewRule.m_Rect.x = 1;

			// endid
			const json_value &rEndid = rRuleNode[j]["endid"];
			if(rEndid.type == json_integer)
				NewRule.m_Rect.y = clamp((int)rEndid.u.integer, 0, 255);
			else
				NewRule.m_Rect.y = 1;

			// broken, skip
			if(NewRule.m_Rect.x > NewRule.m_Rect.y)
				continue;

			// rx
			const json_value &rRx = rRuleNode[j]["rx"];
			if(rRx.type == json_integer)
				NewRule.m_RelativePos.x = rRx.u.integer;
			else
				NewRule.m_RelativePos.x = 0;

			// ry
			const json_value &rRy = rRuleNode[j]["ry"];
			if(rRy.type == json_integer)
				NewRule.m_RelativePos.y = rRy.u.integer;
			else
				NewRule.m_RelativePos.y = 0;

			// width
			NewRule.m_Size.x = absolute(NewRule.m_Rect.x-NewRule.m_Rect.y)%16+1;
			// height
			NewRule.m_Size.y = floor((float)absolute(NewRule.m_Rect.x-NewRule.m_Rect.y)/16)+1;

			// random
			const json_value &rRandom = rRuleNode[j]["random"];
			if(rRandom.type == json_integer)
				NewRule.m_Random = clamp((int)rRandom.u.integer, 1, 99999);
			else
				NewRule.m_Random = 1;

			// hflip
			const json_value &rHFlip = rRuleNode[j]["hflip"];
			if(rHFlip.type == json_integer)
				NewRule.m_HFlip = clamp((int)rHFlip.u.integer, 0, 1);
			else
				NewRule.m_HFlip = 0;

			// vflip
			const json_value &rVFlip = rRuleNode[j]["vflip"];
			if(rVFlip.type == json_integer)
				NewRule.m_VFlip = clamp((int)rVFlip.u.integer, 0, 1);
			else
				NewRule.m_VFlip = 0;

			// location
			NewRule.m_Location = CRule::FLOOR;
			const json_value &rLocation = rRuleNode[j]["location"];
			if(rLocation.type == json_string)
			{
				if(str_comp((const char *)rLocation, "ceiling") == 0)
					NewRule.m_Location = CRule::CEILING;
				else if(str_comp((const char *)rLocation, "walls") == 0)
					NewRule.m_Location = CRule::WALLS;
			}

			NewRuleSet.m_aRules.add(NewRule);
		}

		m_aRuleSets.add(NewRuleSet);
	}

	// sort
	for(int i = 0; i < m_aRuleSets.size(); i++)
	{
		std::stable_sort(&m_aRuleSets[i].m_aRules[0], &m_aRuleSets[i].m_aRules[m_aRuleSets[i].m_aRules.size()]);
	}
}

const char* CDoodadsMapper::GetRuleSetName(int Index) const
{
	if(Index < 0 || Index >= m_aRuleSets.size())
		return "";

	return m_aRuleSets[Index].m_aName;
}

void CDoodadsMapper::AnalyzeGameLayer()
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

void CDoodadsMapper::PlaceDoodads(CLayerTiles *pLayer, CRule *pRule, array<array<int> > *pPositions, int Amount, int LeftWall)
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

void CDoodadsMapper::Proceed(CLayerTiles *pLayer, int ConfigID, int Amount)
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
