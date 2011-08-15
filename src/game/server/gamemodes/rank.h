#ifndef GAME_SERVER_RANK_H
#define GAME_SERVER_RANK_H

#include <vector>
#include <map>

#include <game/server/chatctl.h>
#include "../account.h"

class CRank
{
	private:

	double ranked[20000];
	unsigned int rankedI;
	char* rankedNames[20000];
	std::map<unsigned, double> scores;
	char names[200000];
	unsigned int namesI;

	public:

	void UpdateScore(CAccount* acc);
	unsigned int MaxRank() const;
	unsigned int GetRank(const char* name);
	unsigned int GetRank(double score) const;
	double GetScore(int rank) const;
	const char* GetName(int rank) const;
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
