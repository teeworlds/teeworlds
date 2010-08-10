/* copyright (c) 2008 rajh and gregwar. Score stuff */

#include "score.h"
#include "gamecontext.h"
#include <string.h>
#include <sstream>
#include <fstream>
#include <list>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/server/server.h>
#include <engine/server.h>

CPlayerScore::CPlayerScore(const char *name, float score)
{
	str_copy(this->name, name, sizeof(this->name));
	this->m_Score = score;
}

std::list<CPlayerScore> top;

CScore::CScore(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	Load();
}

CScore::CScore()
{
Load();
}

std::string CScore::SaveFile()
{
	std::ostringstream oss;
	if(!g_Config.m_SvExternalRecords) {
		oss << g_Config.m_SvMap << "_record.dtb";
	} else {
		char buf[512];
		CServer* server = static_cast<CServer*>(m_pGameServer->Server());
		oss << server->Storage()->ApplicationSavePath() << "/records/" << g_Config.m_SvMap << "_record.dtb";
	}
	return oss.str();
}

void CScore::Save()
{
	
		std::fstream f;
		f.open(SaveFile().c_str(), std::ios::out);
		if(!f.fail()) {
			for(std::list<CPlayerScore>::iterator i=top.begin(); i!=top.end(); i++)
			{
				f << i->name << std::endl << i->m_Score << std::endl;
			}
		}
	f.close();
}

void CScore::Load()
{
	std::fstream f;
	f.open(SaveFile().c_str(), std::ios::in);
	top.clear();
	while (!f.eof() && !f.fail())
	{
		std::string tmpname, tmpscore;
		std::getline(f, tmpname);
		if(!f.eof() && tmpname != "")
		{
			std::getline(f, tmpscore);
			top.push_back(*new CPlayerScore(tmpname.c_str(), atof(tmpscore.c_str())));
		}
	}
	f.close();
}

CPlayerScore *CScore::SearchName(const char *name, int &pos)
{
	pos=0;
	for (std::list<CPlayerScore>::iterator i = top.begin(); i!=top.end(); i++)
	{
		pos++;
		if (!strcmp(i->name, name))
		{
			return & (*i);
		}
	}
	pos=-1;
	return 0;
}

CPlayerScore *CScore::SearchName(const char *name)
{
	for (std::list<CPlayerScore>::iterator i = top.begin(); i!=top.end(); i++)
	{
		if (!strcmp(i->name, name))
		{
			return & (*i);
		}
	}
	return 0;
}


void CScore::ParsePlayer(const char *name, float score)
{
	CPlayerScore *player = SearchName(name);
	if (player)
	{
		if (player->m_Score > score)
		{
			player->m_Score = score;
			top.sort();
			Save();
		}
	}
	else
	{
		top.push_back(*new CPlayerScore(name, score));
		top.sort();
		Save();
	}
}

void CScore::Top5Draw(int id, int debut)
{
	int pos = 1;
	//char buf[512];
	
	m_pGameServer->SendChatTarget(id, "----------- Top 5 -----------");
	for (std::list<CPlayerScore>::iterator i = top.begin(); i != top.end() && pos <= 5+debut; i++)
	{
		if(i->m_Score < 0)
			continue;

		if(pos >= debut)
		{
			std::ostringstream oss;
			oss << pos << ". " << i->name << "   Time: ";

			if ((int)(i->m_Score)/60 != 0)
				oss << (int)(i->m_Score)/60 << " minute(s) ";
			if (i->m_Score-((int)i->m_Score/60)*60 != 0)
				oss << i->m_Score-((int)i->m_Score/60)*60 <<" second(s)";

			m_pGameServer->SendChatTarget(id, oss.str().c_str());
		}
		pos++;
	}
	m_pGameServer->SendChatTarget(id, "-----------------------------");
}