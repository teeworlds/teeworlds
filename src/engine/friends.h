/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_FRIENDS_H
#define ENGINE_FRIENDS_H

#include <engine/shared/protocol.h>

#include "kernel.h"

struct CFriendInfo
{
	char m_aName[MAX_NAME_LENGTH];
	char m_aClan[MAX_CLAN_LENGTH];
	unsigned m_NameHash;
	unsigned m_ClanHash;
};

class IFriends : public IInterface
{
	MACRO_INTERFACE("friends", 0)
public:
	enum
	{
		FRIEND_NO=0,
		FRIEND_CLAN,
		FRIEND_PLAYER,

		MAX_FRIENDS=128,
	};

	virtual void Init() = 0;

	virtual int NumFriends() const = 0;
	virtual const CFriendInfo *GetFriend(int Index) const = 0;
	virtual int GetFriendState(const char *pName, const char *pClan) const = 0;
	virtual bool IsFriend(const char *pName, const char *pClan, bool PlayersOnly) const = 0;

	virtual void AddFriend(const char *pName, const char *pClan) = 0;
	virtual void RemoveFriend(const char *pName, const char *pClan) = 0;
};

#endif
