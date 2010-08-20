/* copyright (c) 2008 rajh and gregwar. Score stuff */

#ifndef SCORE_H_RACE
#define SCORE_H_RACE
#include <engine/server.h>
#include <engine/shared/protocol.h>
#include <string>
#include <list>


class CPlayerScore
{
public:
	char name[MAX_NAME_LENGTH];
	float m_Score;

	CPlayerScore(const char *name, float score);

	bool operator==(const CPlayerScore& other) { return (this->m_Score == other.m_Score); }
	bool operator<(const CPlayerScore& other) { return (this->m_Score < other.m_Score); }
};

class CScore
{
	class CGameContext *m_pGameServer;
	std::string SaveFile();
public:
	CScore(class CGameContext *pGameServer);
	CScore();
	void Save();
	void Load();
	CPlayerScore *SearchName(const char *name, int &pos);
	CPlayerScore *SearchName(const char *name);
	void ParsePlayer(const char *name, float score);
	std::list<std::string> Top5Draw(int id, int debut);
};

#endif
