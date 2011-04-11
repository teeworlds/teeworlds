#include "ed_auto_map.h"
#include "ed_editor.h"

#include <stdio.h>	// sscanf

#include <engine/storage.h>
#include <engine/console.h>
#include <engine/shared/linereader.h>

CAutoMapper::CAutoMapper(CEditor *pEditor)
{
	m_pEditor = pEditor;
	m_FileLoaded = false;
}

void CAutoMapper::Load(char* pTileName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "data/editor/%s.rules", pTileName);
	IOHANDLE RulesFile = m_pEditor->Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	
	if(RulesFile)
	{
		CLineReader LineReader;
		LineReader.Init(RulesFile);
		
		CConfiguration *pCurrentConf = 0;
		CIndexRule *pCurrentIndex = 0;
		
		char aBuf[256];
		
		// read each line
		while(char *pLine = LineReader.Get())
		{
			// skip blank/empty lines as well as comments
			if(str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r'
				&& pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
			{
				if(pLine[0]== '[')
				{
					// new configuration, get the name
					pLine++;
					
					CConfiguration NewConf;
					int ID = m_aConfigs.add(NewConf);
					pCurrentConf = &m_aConfigs[ID];
					
					str_copy(pCurrentConf->m_aName, pLine, str_length(pLine));
				}
				
				else
				{
					if(!str_comp_num(pLine, "Index", 5))
					{
						// new index
						int ID = 0;
						char aFlip[128] = "";
						
						sscanf(pLine, "Index %d %s", &ID, aFlip);
						
						CIndexRule NewIndexRule;
						NewIndexRule.m_ID = ID;
						NewIndexRule.m_Flag = 0;
						NewIndexRule.m_RandomValue = 0;
						NewIndexRule.m_BaseTile = false;
						
						if(str_length(aFlip) > 0)
						{
							if(!str_comp(aFlip, "XFLIP"))
								NewIndexRule.m_Flag = TILEFLAG_VFLIP;
							else if(!str_comp(aFlip, "YFLIP"))
								NewIndexRule.m_Flag = TILEFLAG_HFLIP;
						}
						
						// add the index rule object and make it current
						int ArrayID = pCurrentConf->m_aIndexRules.add(NewIndexRule);
						pCurrentIndex = &pCurrentConf->m_aIndexRules[ArrayID];
					}
					
					else if(!str_comp_num(pLine, "BaseTile", 8) && pCurrentIndex)
					{
						pCurrentIndex->m_BaseTile = true;
					}
					
					else if(!str_comp_num(pLine, "Pos", 3) && pCurrentIndex)
					{
						int x = 0, y = 0;
						char aValue[128];
						int Value = CPosRule::EMPTY;
						bool IndexValue = false;
						
						sscanf(pLine, "Pos %d %d %s", &x, &y, aValue);
						
						if(!str_comp(aValue, "EMPTY"))
							Value = CPosRule::EMPTY;
						else if(!str_comp(aValue, "FULL"))
							Value = CPosRule::FULL;
						else if(!str_comp_num(aValue, "INDEX", 5))
						{
							int UselessInteger;
							sscanf(pLine, "Pos %d %d INDEX %d", &UselessInteger, &UselessInteger, &Value);
							IndexValue = true;
						}

						CPosRule NewPosRule = { x, y, Value, IndexValue};
						pCurrentIndex->m_aRules.add(NewPosRule);
					}
					
					else if(!str_comp_num(pLine, "Random", 6) && pCurrentIndex)
					{
						sscanf(pLine, "Random %d", &pCurrentIndex->m_RandomValue);
					}
				}
			}
		}
		
		io_close(RulesFile);
		
		str_format(aBuf, sizeof(aBuf),"loaded %s", aPath);
		m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);
		
		m_FileLoaded = true;
	}
}

array<char*> CAutoMapper::GetConfigNames()
{
	array<char*> aNames;
	
	for(int i = 0; i < m_aConfigs.size(); ++i)
		aNames.add(m_aConfigs[i].m_aName);
		
	return aNames;
}

void CAutoMapper::Proceed(CLayerTiles *pLayer, int ConfigID)
{
	if(!m_FileLoaded || pLayer->m_Readonly)
		return;
	
	CConfiguration *pConf = &m_aConfigs[ConfigID];
	
	if(!pConf->m_aIndexRules.size())
		return;
	
	int BaseTile = 1;
	
	// find base tile if there is one
	for(int i = 0; i < pConf->m_aIndexRules.size(); ++i)
	{
		if(pConf->m_aIndexRules[i].m_BaseTile)
		{
			BaseTile = pConf->m_aIndexRules[i].m_ID;
			break;
		}
	}
	
	// auto map !
	for(int y = 0; y < pLayer->m_Height; y++)
	{
		for(int x = 0; x < pLayer->m_Width; x++)
		{
			CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
			
			if(pTile->m_Index > 0)
			{
				pTile->m_Index = BaseTile;
				
				for(int i = 0; i < pConf->m_aIndexRules.size(); ++i)
				{
					bool RespectRules = true;
					
					if(!pConf->m_aIndexRules[i].m_BaseTile)
					{
						for(int j = 0; j < pConf->m_aIndexRules[i].m_aRules.size(); ++j)
						{
							CPosRule *pRule = &pConf->m_aIndexRules[i].m_aRules[j];
							int CheckIndex = (y+pRule->m_Y)*pLayer->m_Width+(x+pRule->m_X);
							int MaxIndex = pLayer->m_Width*pLayer->m_Height;
							
 							if(pRule->m_IndexValue)
							{
								if(CheckIndex >= 0 && CheckIndex < MaxIndex && pLayer->m_pTiles[CheckIndex].m_Index != pRule->m_Value)
									RespectRules = false;
							}
							else
							{
								if(CheckIndex >= 0 && CheckIndex < MaxIndex && pLayer->m_pTiles[CheckIndex].m_Index > 0 && pRule->m_Value == CPosRule::EMPTY)
									RespectRules = false;
									
								if(CheckIndex >= 0 && CheckIndex < MaxIndex && pLayer->m_pTiles[CheckIndex].m_Index == 0 && pRule->m_Value == CPosRule::FULL)
									RespectRules = false;
							}
							if(CheckIndex < 0 || CheckIndex >= MaxIndex)
								RespectRules = false;
						}
					}
					
					if(RespectRules)
					{
						if(pConf->m_aIndexRules[i].m_RandomValue <= 1 || (int)((float)rand() / ((float)RAND_MAX + 1) * pConf->m_aIndexRules[i].m_RandomValue)  == 1)
						{
							pTile->m_Index = pConf->m_aIndexRules[i].m_ID;
							pTile->m_Flags = 0;
							
							if(pConf->m_aIndexRules[i].m_Flag)
								pTile->m_Flags ^= pConf->m_aIndexRules[i].m_Flag;
						}
					}
				}
				
				m_pEditor->m_Map.m_Modified = true;
			}
		}
	}
}