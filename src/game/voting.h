/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VOTING_H
#define GAME_VOTING_H

enum
{
	VOTE_DESC_LENGTH=64,
	VOTE_CMD_LENGTH=512,
	VOTE_SEARCH_LENGTH=64,
	VOTE_REASON_LENGTH=16,

	MAX_VOTE_OPTIONS=128,
	MAX_VOTE_OPTION_ADD=21,

	VOTE_COOLDOWN=60,
};

struct CVoteOptionClient
{
	CVoteOptionClient *m_pNext;
	CVoteOptionClient *m_pPrev;
	char m_aDescription[VOTE_DESC_LENGTH];
};

struct CVoteOptionServer
{
	CVoteOptionServer *m_pNext;
	CVoteOptionServer *m_pPrev;
	char m_aDescription[VOTE_DESC_LENGTH];
	char m_aCommand[1];
};

#endif
