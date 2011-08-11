#ifndef GAME_SERVER_RANK_H
#define GAME_SERVER_RANK_H

#include <vector>
#include <map>

#include <game/server/chatctl.h>
#include "../account.h"

class CRank
{
	private:

	std::vector<double> ranked;
	std::map<unsigned, double> scores;

	public:

	void UpdateScore(CAccount* acc);
	int MaxRank() const;
	int GetRank(const char* name);
	int GetRank(double score) const;
	void Init();
};

class CRankHnd : public IChatCtl
{
public:
	CRankHnd();
	virtual bool HandleChatMsg(class CPlayer *pPlayer, const char *pMsg);
	static bool ParseLine(char *pDstEmote, unsigned int SzEmote, unsigned *pDstTime, const char *pLine);
};

#endif
