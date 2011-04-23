/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include "friends.h"

CFriends::CFriends()
{
	mem_zero(m_aFriends, sizeof(m_aFriends));
}

void CFriends::ConAddFriend(IConsole::IResult *pResult, void *pUserData)
{
	CFriends *pSelf = (CFriends *)pUserData;
	pSelf->AddFriend(pResult->GetString(0), pResult->GetString(1));
}

void CFriends::ConRemoveFriend(IConsole::IResult *pResult, void *pUserData)
{
	CFriends *pSelf = (CFriends *)pUserData;
	pSelf->RemoveFriend(pResult->GetString(0), pResult->GetString(1));
}

void CFriends::Init()
{
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();
	if(pConsole)
	{
		pConsole->Register("add_friend", "ss", CFGFLAG_CLIENT, ConAddFriend, this, "Add a friend");
		pConsole->Register("remove_Friend", "ss", CFGFLAG_CLIENT, ConRemoveFriend, this, "Remove a friend");
	}
}

const CFriendInfo *CFriends::GetFriend(int Index) const
{
	return &m_aFriends[max(0, Index%m_NumFriends)];
}

bool CFriends::IsFriend(const char *pName, const char *pClan, bool PlayersOnly) const
{
	for(int i = 0; i < m_NumFriends; ++i)
	{
		if(!str_comp(m_aFriends[i].m_aClan, pClan) &&
			((!PlayersOnly && m_aFriends[i].m_aName[0] == 0) || !str_comp(m_aFriends[i].m_aName, pName)))
			return true;
	}
	return false;
}

void CFriends::AddFriend(const char *pName, const char *pClan)
{
	if(m_NumFriends == MAX_FRIENDS || (pName[0] == 0 && pClan[0] == 0))
		return;

	// make sure we don't have the friend already
	for(int i = 0; i < m_NumFriends; ++i)
	{
		if(!str_comp(m_aFriends[i].m_aName, pName) && !str_comp(m_aFriends[i].m_aClan, pClan))
			return;
	}

	str_copy(m_aFriends[m_NumFriends].m_aName, pName, sizeof(m_aFriends[m_NumFriends].m_aName));
	str_copy(m_aFriends[m_NumFriends].m_aClan, pClan, sizeof(m_aFriends[m_NumFriends].m_aClan));
	++m_NumFriends;
}

void CFriends::RemoveFriend(const char *pName, const char *pClan)
{
	for(int i = 0; i < m_NumFriends; ++i)
	{
		if(!str_comp(m_aFriends[i].m_aName, pName) && !str_comp(m_aFriends[i].m_aClan, pClan))
		{
			RemoveFriend(i);
			return;
		}
	}
}

void CFriends::RemoveFriend(int Index)
{
	if(Index >= 0 && Index < m_NumFriends)
	{
		mem_move(&m_aFriends[Index], &m_aFriends[Index+1], sizeof(CFriendInfo)*(m_NumFriends-(Index+1)));
		--m_NumFriends;
	}
	return;
}

void CFriends::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CFriends *pSelf = (CFriends *)pUserData;
	char aBuf[128];
	const char *pEnd = aBuf+sizeof(aBuf)-4;
	for(int i = 0; i < pSelf->m_NumFriends; ++i)
	{
		str_copy(aBuf, "add_friend ", sizeof(aBuf));

		const char *pSrc = pSelf->m_aFriends[i].m_aName;
		char *pDst = aBuf+str_length(aBuf);
		*pDst++ = '"';
		while(*pSrc && pDst < pEnd)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst++ = '"';
		*pDst++ = ' ';

		pSrc = pSelf->m_aFriends[i].m_aClan;
		*pDst++ = '"';
		while(*pSrc && pDst < pEnd)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst++ = '"';
		*pDst++ = 0;

		pConfig->WriteLine(aBuf);
	}
}
