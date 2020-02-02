/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/shared/config.h>

#include "contacts.h"

IContactList::IContactList()
{
	mem_zero(m_aContacts, sizeof(m_aContacts));
	m_NumContacts = 0;
}

const CContactInfo *IContactList::GetContact(int Index) const
{
	return &m_aContacts[max(0, Index%m_NumContacts)];
}

int IContactList::GetContactState(const char *pName, const char *pClan) const
{
	int Result = CContactInfo::CONTACT_NO;
	unsigned NameHash = str_quickhash(pName);
	unsigned ClanHash = str_quickhash(pClan);
	for(int i = 0; i < m_NumContacts; ++i)
	{
		if(m_aContacts[i].m_ClanHash == ClanHash)
		{
			if(m_aContacts[i].m_aName[0] == 0)
				Result = CContactInfo::CONTACT_CLAN;
			else if(m_aContacts[i].m_NameHash == NameHash)
			{
				Result = CContactInfo::CONTACT_PLAYER;
				break;
			}
		}
	}
	return Result;
}

bool IContactList::IsContact(const char *pName, const char *pClan, bool PlayersOnly) const
{
	unsigned NameHash = str_quickhash(pName);
	unsigned ClanHash = str_quickhash(pClan);
	for(int i = 0; i < m_NumContacts; ++i)
	{
		if(m_aContacts[i].m_ClanHash == ClanHash &&
			((!PlayersOnly && m_aContacts[i].m_aName[0] == 0) || m_aContacts[i].m_NameHash == NameHash))
			return true;
	}
	return false;
}

void IContactList::AddContact(const char *pName, const char *pClan)
{
	if(m_NumContacts == CContactInfo::MAX_CONTACTS || (pName[0] == 0 && pClan[0] == 0))
		return;

	// make sure we don't have the friend already
	unsigned NameHash = str_quickhash(pName);
	unsigned ClanHash = str_quickhash(pClan);
	for(int i = 0; i < m_NumContacts; ++i)
	{
		if(m_aContacts[i].m_NameHash == NameHash && m_aContacts[i].m_ClanHash == ClanHash)
			return;
	}

	str_copy(m_aContacts[m_NumContacts].m_aName, pName, sizeof(m_aContacts[m_NumContacts].m_aName));
	str_copy(m_aContacts[m_NumContacts].m_aClan, pClan, sizeof(m_aContacts[m_NumContacts].m_aClan));
	m_aContacts[m_NumContacts].m_NameHash = NameHash;
	m_aContacts[m_NumContacts].m_ClanHash = ClanHash;
	++m_NumContacts;
}

void IContactList::RemoveContact(const char *pName, const char *pClan)
{
	unsigned NameHash = str_quickhash(pName);
	unsigned ClanHash = str_quickhash(pClan);
	for(int i = 0; i < m_NumContacts; ++i)
	{
		if(m_aContacts[i].m_NameHash == NameHash && m_aContacts[i].m_ClanHash == ClanHash)
		{
			RemoveContact(i);
			return;
		}
	}
}

void IContactList::RemoveContact(int Index)
{
	if(Index >= 0 && Index < m_NumContacts)
	{
		mem_move(&m_aContacts[Index], &m_aContacts[Index+1], sizeof(CContactInfo)*(m_NumContacts-(Index+1)));
		--m_NumContacts;
	}
	return;
}

void IContactList::ConfigSave(IConfigManager *pConfigManager, const char* pCmdStr)
{
	char aBuf[128];
	const char *pEnd = aBuf+sizeof(aBuf)-4;
	for(int i = 0; i < this->m_NumContacts; ++i)
	{
		str_copy(aBuf, pCmdStr, sizeof(aBuf));

		const char *pSrc = this->m_aContacts[i].m_aName;
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

		pSrc = this->m_aContacts[i].m_aClan;
		*pDst++ = '"';
		while(*pSrc && pDst < pEnd)
		{
			if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
				*pDst++ = '\\';
			*pDst++ = *pSrc++;
		}
		*pDst++ = '"';
		*pDst++ = 0;

		pConfigManager->WriteLine(aBuf);
	}	
}

void CFriends::ConAddFriend(IConsole::IResult *pResult, void *pUserData)
{
	CFriends *pSelf = static_cast<CFriends *>(pUserData);
	pSelf->AddFriend(pResult->GetString(0), pResult->GetString(1));
}

void CFriends::ConRemoveFriend(IConsole::IResult *pResult, void *pUserData)
{
	CFriends *pSelf = static_cast<CFriends *>(pUserData);
	pSelf->RemoveFriend(pResult->GetString(0), pResult->GetString(1));
}

void CBlacklist::ConAddIgnore(IConsole::IResult *pResult, void *pUserData)
{
	CBlacklist *pSelf = static_cast<CBlacklist *>(pUserData);
	pSelf->AddIgnoredPlayer(pResult->GetString(0), pResult->GetString(1));
}

void CBlacklist::ConRemoveIgnore(IConsole::IResult *pResult, void *pUserData)
{
	CBlacklist *pSelf = static_cast<CBlacklist *>(pUserData);
	pSelf->RemoveIgnoredPlayer(pResult->GetString(0), pResult->GetString(1));
}

void CFriends::Init()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterCallback(ConfigSaveCallback, this);

	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();
	if(pConsole)
	{
		pConsole->Register("add_friend", "ss", CFGFLAG_CLIENT, ConAddFriend, this, "Add a friend");
		pConsole->Register("remove_friend", "ss", CFGFLAG_CLIENT, ConRemoveFriend, this, "Remove a friend");
	}
}

void CBlacklist::Init()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterCallback(ConfigSaveCallback, this);

	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();
	if(pConsole)
	{
		pConsole->Register("add_ignore", "ss", CFGFLAG_CLIENT, ConAddIgnore, this, "Ignore a player");
		pConsole->Register("remove_ignore", "ss", CFGFLAG_CLIENT, ConRemoveIgnore, this, "Stop ignoring a player");
	}
}

void CFriends::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CFriends *pSelf = (CFriends *)pUserData;
	pSelf->ConfigSave(pConfigManager, "add_friend ");
}

void CBlacklist::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CBlacklist *pSelf = (CBlacklist *)pUserData;
	pSelf->ConfigSave(pConfigManager, "add_ignore ");
}
