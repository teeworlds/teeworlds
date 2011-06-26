/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_FRIENDS_H
#define ENGINE_CLIENT_FRIENDS_H

#include <engine/friends.h>

class CFriends : public IFriends
{
	CFriendInfo m_aFriends[MAX_FRIENDS];
	int m_NumFriends;

	static void ConAddFriend(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveFriend(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

public:
	CFriends();

	void Init();

	int NumFriends() const { return m_NumFriends; }
	const CFriendInfo *GetFriend(int Index) const;
	int GetFriendState(const char *pName, const char *pClan) const;
	bool IsFriend(const char *pName, const char *pClan, bool PlayersOnly) const;

	void AddFriend(const char *pName, const char *pClan);
	void RemoveFriend(const char *pName, const char *pClan);
	void RemoveFriend(int Index);
};

#endif
