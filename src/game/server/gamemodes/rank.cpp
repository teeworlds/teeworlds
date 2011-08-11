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
	int rank;
	if (it == scores.end())
	{
		rank = ranked.size();
		ranked.push_back(0);
		scores[hash] = newScore;
	}
	else
	{
		rank = GetRank(it->second);
		it->second = newScore;
	}
	while (rank > 0 && newScore > ranked[rank-1])
	{
		ranked[rank] = ranked[rank-1];
		rank--;
	}
	while (rank < MaxRank()-1 && newScore < ranked[rank+1])
	{
		ranked[rank] = ranked[rank+1];
		rank++;
	}
	ranked[rank] = newScore;
}

int CRank::MaxRank() const
{
	return ranked.size();
}

int CRank::GetRank(const char* name)
{
	unsigned hash = str_quickhash(name);
	std::map<unsigned, double>::iterator it = scores.find(hash);
	if (it == scores.end())
		return -1;
	return GetRank(it->second);
}

bool comp (int i,int j) { return (i>j); }

int CRank::GetRank(double score) const
{
	return std::distance(ranked.begin(),lower_bound(ranked.begin(), ranked.end(), score, comp));
}

void Listacc(CAccount* acc, void* user)
{
	((CRank*)user)->UpdateScore(acc);
}

void CRank::Init()
{
	dbg_msg("rank", "reading all accs, may take some time");
	CAccount::ListAccs(*Listacc, (void*)this);
	dbg_msg("rank","done, read %d accounts",MaxRank());
}

CRankHnd::CRankHnd()
{
	IChatCtl::Register(this);
}

bool CRankHnd::HandleChatMsg(class CPlayer *pPlayer, const char *pMsg)
{
	if (str_comp_num(pMsg, "/rank", 5) != 0)
		return false;
	if (!pPlayer->GetAccount())
	{
		GameContext()->SendChatTarget(pPlayer->GetCID(), "you must /register or /login to use this feature");
		return true;
	}
	char buf[200];
//	str_format(buf, sizeof(buf), "\"%s\" has %.1f score and is #%d out of %d accounts", GameContext()->Server()->ClientName(pPlayer->GetCID()), pPlayer->GetAccount()->Payload()->blockScore, GameContext()->m_Rank.GetRank(pPlayer->GetAccount()->Payload()->blockScore)+1, GameContext()->m_Rank.MaxRank());
	str_format(buf, sizeof(buf), "\"%s\" has %.1f score and is #%d out of %d accounts", pPlayer->GetAccount()->Name(), pPlayer->GetAccount()->Payload()->blockScore, GameContext()->m_Rank.GetRank(pPlayer->GetAccount()->Payload()->blockScore)+1, GameContext()->m_Rank.MaxRank());
	GameContext()->SendChat(-1, CGameContext::CHAT_ALL, buf);
	return true;
}
