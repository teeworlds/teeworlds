#include "rank.h"
#include <base/system.h>
#include <algorithm>
#include "../gamecontext.h"

void CRank::UpdateScore(CAccount* acc)
{
	const char* name = acc->Name();
	double newScore = acc->Payload()->blockScore;
	unsigned hash = str_quickhash(name);
	std::map<unsigned, double>::iterator it = scores.find(hash);
	unsigned int rank;
	char* nameP;
	if (it == scores.end())
	{
		rank = rankedI++;
		if (rank >= sizeof(ranked)) return;
		ranked[rank] = 0;
		rankedNames[rank] = &names[namesI];
		int len = str_length(name);
		if (namesI+len+1 >= sizeof(names)) return;
		mem_copy(&names[namesI], acc->Name(), len+1);
		nameP = rankedNames[rank];
		namesI += len + 1;
		scores[hash] = newScore;
	}
	else
	{
		rank = GetRank(it->second);
		nameP = rankedNames[rank];
		it->second = newScore;
	}
	while (rank > 0 && newScore > ranked[rank-1])
	{
		ranked[rank] = ranked[rank-1];
		rankedNames[rank] = rankedNames[rank-1];
		rank--;
	}
	while (rank < MaxRank()-1 && newScore < ranked[rank+1])
	{
		ranked[rank] = ranked[rank+1];
		rankedNames[rank] = rankedNames[rank+1];
		rank++;
	}
	ranked[rank] = newScore;
	rankedNames[rank] = nameP;
}

unsigned int CRank::MaxRank() const
{
	return rankedI;
}

unsigned int CRank::GetRank(const char* name)
{
	unsigned hash = str_quickhash(name);
	std::map<unsigned, double>::iterator it = scores.find(hash);
	if (it == scores.end())
		return -1;
	return GetRank(it->second);
}

bool comp (int i,int j) { return (i>j); }

unsigned int CRank::GetRank(double score) const
{
	return std::distance(&ranked[0], std::lower_bound(&ranked[0], &ranked[MaxRank()], score, comp));
}

double CRank::GetScore(int rank) const
{
	return ranked[rank];
}

const char* CRank::GetName(int rank) const
{
	return rankedNames[rank];
}

void Listacc(CAccount* acc, void* user)
{
	((CRank*)user)->UpdateScore(acc);
}

void CRank::Init()
{
	dbg_msg("rank", "reading all accs, may take some time");
	rankedI = 0;
	namesI = 0;
	CAccount::ListAccs(*Listacc, (void*)this);
	dbg_msg("rank","done, read %d accounts",MaxRank());
}

CRankHnd::CRankHnd()
{
	IChatCtl::Register(this);
}

bool CRankHnd::HandleChatMsg(class CPlayer *pPlayer, const char *pMsg)
{
	if (str_comp_num(pMsg, "/rank", 5) == 0)
	{
		if (!pPlayer->GetAccount())
		{
			GameContext()->SendChatTarget(pPlayer->GetCID(), "you must /register or /login to use this feature");
			return true;
		}
		char buf[200];
//		str_format(buf, sizeof(buf), "\"%s\" has %.1f score and is #%d out of %d accounts", GameContext()->Server()->ClientName(pPlayer->GetCID()), pPlayer->GetAccount()->Payload()->blockScore, GameContext()->m_Rank.GetRank(pPlayer->GetAccount()->Payload()->blockScore)+1, GameContext()->m_Rank.MaxRank());
		str_format(buf, sizeof(buf), "\"%s\" has %.1f score and is #%d out of %d accounts", pPlayer->GetAccount()->Name(), pPlayer->GetAccount()->Payload()->blockScore, GameContext()->m_Rank.GetRank(pPlayer->GetAccount()->Payload()->blockScore)+1, GameContext()->m_Rank.MaxRank());
		GameContext()->SendChat(-1, CGameContext::CHAT_ALL, buf);
		return true;
	}
	if (str_comp_num(pMsg, "/top", 4) == 0)
	{
		char buf[200];
		str_format(buf, sizeof(buf), "Top 5 players:");
		GameContext()->SendChatTarget(pPlayer->GetCID(), buf);
		int upper = 5;
		if (GameContext()->m_Rank.MaxRank() < 5) upper = GameContext()->m_Rank.MaxRank();
		for (int i=0; i<upper; i++)
		{
			str_format(buf, sizeof(buf), "%d) \"%s\" with %.1f score", i+1, GameContext()->m_Rank.GetName(i), GameContext()->m_Rank.GetScore(i));
			GameContext()->SendChatTarget(pPlayer->GetCID(), buf);
		}
		return true;
	}
	return false;
}
