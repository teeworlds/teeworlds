/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_FRIENDS_H
#define ENGINE_FRIENDS_H

#include <engine/shared/protocol.h>

#include "kernel.h"

struct CContactInfo
{
	char m_aName[MAX_NAME_LENGTH];
	char m_aClan[MAX_CLAN_LENGTH];
	unsigned m_NameHash;
	unsigned m_ClanHash;

	enum
	{
		// for contact lists
		CONTACT_NO=0,
		CONTACT_CLAN,
		CONTACT_PLAYER,

		MAX_CONTACTS=128,
	};
};

class IFriends : public IInterface
{
	MACRO_INTERFACE("friends", 0)
public:
	virtual void Init() = 0;

	virtual int NumFriends() const = 0;
	virtual const CContactInfo *GetFriend(int Index) const = 0;
	virtual int GetFriendState(const char *pName, const char *pClan) const = 0;
	virtual bool IsFriend(const char *pName, const char *pClan, bool PlayersOnly) const = 0;

	virtual void AddFriend(const char *pName, const char *pClan) = 0;
	virtual void RemoveFriend(const char *pName, const char *pClan) = 0;
};

class IBlacklist : public IInterface
{
	MACRO_INTERFACE("blacklist", 0)
public:
	virtual void Init() = 0;

	virtual bool IsIgnored(const char *pName, const char *pClan, bool PlayersOnly) const = 0;

	virtual void AddIgnoredPlayer(const char *pName, const char *pClan) = 0;
	virtual void RemoveIgnoredPlayer(const char *pName, const char *pClan) = 0;
};

#endif
