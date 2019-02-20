/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_FRIENDS_H
#define ENGINE_CLIENT_FRIENDS_H

#include <engine/friends.h>

class CFriends : public virtual IFriends
{
protected:
	CFriendInfo m_aFriends[MAX_FRIENDS];
	int m_NumFriends;

public:
	CFriends();

	void ConfigSave(IConfig *pConfig, const char* pCmdStr);

	static void ConAddFriend(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveFriend(IConsole::IResult *pResult, void *pUserData);

	virtual void Init() = 0;

	int NumFriends() const { return m_NumFriends; }
	const CFriendInfo *GetFriend(int Index) const;
	int GetFriendState(const char *pName, const char *pClan) const;
	bool IsFriend(const char *pName, const char *pClan, bool PlayersOnly) const;

	void AddFriend(const char *pName, const char *pClan);
	void RemoveFriend(const char *pName, const char *pClan);
	void RemoveFriend(int Index);
};

class CGoodFriends: public CFriends, public IGoodFriends
{
public:
	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

	void Init();
};

class CBadFriends: public CFriends, public IBadFriends
{
public:
	static void ConfigSaveCallback(IConfig *pConfig, void *pUserData);

	void Init();
};

#endif
