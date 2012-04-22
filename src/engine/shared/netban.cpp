#include <base/math.h>

#include <engine/console.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "netban.h"


bool CNetBan::StrAllnum(const char *pStr)
{
	while(*pStr)
	{
		if(!(*pStr >= '0' && *pStr <= '9'))
			return false;
		pStr++;
	}
	return true;
}


CNetBan::CNetHash::CNetHash(const NETADDR *pAddr)
{
	if(pAddr->type==NETTYPE_IPV4)
		m_Hash = (pAddr->ip[0]+pAddr->ip[1]+pAddr->ip[2]+pAddr->ip[3])&0xFF;
	else
		m_Hash = (pAddr->ip[0]+pAddr->ip[1]+pAddr->ip[2]+pAddr->ip[3]+pAddr->ip[4]+pAddr->ip[5]+pAddr->ip[6]+pAddr->ip[7]+
			pAddr->ip[8]+pAddr->ip[9]+pAddr->ip[10]+pAddr->ip[11]+pAddr->ip[12]+pAddr->ip[13]+pAddr->ip[14]+pAddr->ip[15])&0xFF;
	m_HashIndex = 0;
}

CNetBan::CNetHash::CNetHash(const CNetRange *pRange)
{
	m_Hash = 0;
	m_HashIndex = 0;
	for(int i = 0; pRange->m_LB.ip[i] == pRange->m_UB.ip[i]; ++i)
	{
		m_Hash += pRange->m_LB.ip[i];
		++m_HashIndex;
	}
	m_Hash &= 0xFF;
}

int CNetBan::CNetHash::MakeHashArray(const NETADDR *pAddr, CNetHash aHash[17])
{
	int Length = pAddr->type==NETTYPE_IPV4 ? 4 : 16;
	aHash[0].m_Hash = 0;
	aHash[0].m_HashIndex = 0;
	for(int i = 1, Sum = 0; i <= Length; ++i)
	{
		Sum += pAddr->ip[i-1];
		aHash[i].m_Hash = Sum&0xFF;
		aHash[i].m_HashIndex = i%Length;
	}
	return Length;
}


template<class T, int HashCount>
typename CNetBan::CBan<T> *CNetBan::CBanPool<T, HashCount>::Add(const T *pData, const CBanInfo *pInfo,  const CNetHash *pNetHash)
{
	if(!m_pFirstFree)
		return 0;

	// create new ban
	CBan<T> *pBan = m_pFirstFree;
	pBan->m_Data = *pData;
	pBan->m_Info = *pInfo;
	pBan->m_NetHash = *pNetHash;
	if(pBan->m_pNext)
		pBan->m_pNext->m_pPrev = pBan->m_pPrev;
	if(pBan->m_pPrev)
		pBan->m_pPrev->m_pNext = pBan->m_pNext;
	else
		m_pFirstFree = pBan->m_pNext;

	// add it to the hash list
	if(m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash])
		m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash]->m_pHashPrev = pBan;
	pBan->m_pHashPrev = 0;
	pBan->m_pHashNext = m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash];
	m_paaHashList[pNetHash->m_HashIndex][pNetHash->m_Hash] = pBan;

	// insert it into the used list
	if(m_pFirstUsed)
	{
		for(CBan<T> *p = m_pFirstUsed; ; p = p->m_pNext)
		{
			if(p->m_Info.m_Expires == CBanInfo::EXPIRES_NEVER || (pInfo->m_Expires != CBanInfo::EXPIRES_NEVER && pInfo->m_Expires <= p->m_Info.m_Expires))
			{
				// insert before
				pBan->m_pNext = p;
				pBan->m_pPrev = p->m_pPrev;
				if(p->m_pPrev)
					p->m_pPrev->m_pNext = pBan;
				else
					m_pFirstUsed = pBan;
				p->m_pPrev = pBan;
				break;
			}

			if(!p->m_pNext)
			{
				// last entry
				p->m_pNext = pBan;
				pBan->m_pPrev = p;
				pBan->m_pNext = 0;
				break;
			}
		}
	}
	else
	{
		m_pFirstUsed = pBan;
		pBan->m_pNext = pBan->m_pPrev = 0;
	}

	// update ban count
	++m_CountUsed;

	return pBan;
}

template<class T, int HashCount>
int CNetBan::CBanPool<T, HashCount>::Remove(CBan<T> *pBan)
{
	if(pBan == 0)
		return -1;

	// remove from hash list
	if(pBan->m_pHashNext)
		pBan->m_pHashNext->m_pHashPrev = pBan->m_pHashPrev;
	if(pBan->m_pHashPrev)
		pBan->m_pHashPrev->m_pHashNext = pBan->m_pHashNext;
	else
		m_paaHashList[pBan->m_NetHash.m_HashIndex][pBan->m_NetHash.m_Hash] = pBan->m_pHashNext;
	pBan->m_pHashNext = pBan->m_pHashPrev = 0;

	// remove from used list
	if(pBan->m_pNext)
		pBan->m_pNext->m_pPrev = pBan->m_pPrev;
	if(pBan->m_pPrev)
		pBan->m_pPrev->m_pNext = pBan->m_pNext;
	else
		m_pFirstUsed = pBan->m_pNext;

	// add to recycle list
	if(m_pFirstFree)
		m_pFirstFree->m_pPrev = pBan;
	pBan->m_pPrev = 0;
	pBan->m_pNext = m_pFirstFree;
	m_pFirstFree = pBan;

	// update ban count
	--m_CountUsed;

	return 0;
}

template<class T, int HashCount>
void CNetBan::CBanPool<T, HashCount>::Update(CBan<CDataType> *pBan, const CBanInfo *pInfo)
{
	pBan->m_Info = *pInfo;

	// remove from used list
	if(pBan->m_pNext)
		pBan->m_pNext->m_pPrev = pBan->m_pPrev;
	if(pBan->m_pPrev)
		pBan->m_pPrev->m_pNext = pBan->m_pNext;
	else
		m_pFirstUsed = pBan->m_pNext;

	// insert it into the used list
	if(m_pFirstUsed)
	{
		for(CBan<T> *p = m_pFirstUsed; ; p = p->m_pNext)
		{
			if(p->m_Info.m_Expires == CBanInfo::EXPIRES_NEVER || (pInfo->m_Expires != CBanInfo::EXPIRES_NEVER && pInfo->m_Expires <= p->m_Info.m_Expires))
			{
				// insert before
				pBan->m_pNext = p;
				pBan->m_pPrev = p->m_pPrev;
				if(p->m_pPrev)
					p->m_pPrev->m_pNext = pBan;
				else
					m_pFirstUsed = pBan;
				p->m_pPrev = pBan;
				break;
			}

			if(!p->m_pNext)
			{
				// last entry
				p->m_pNext = pBan;
				pBan->m_pPrev = p;
				pBan->m_pNext = 0;
				break;
			}
		}
	}
	else
	{
		m_pFirstUsed = pBan;
		pBan->m_pNext = pBan->m_pPrev = 0;
	}
}

template<class T, int HashCount>
void CNetBan::CBanPool<T, HashCount>::Reset()
{
	mem_zero(m_paaHashList, sizeof(m_paaHashList));
	mem_zero(m_aBans, sizeof(m_aBans));
	m_pFirstUsed = 0;
	m_CountUsed = 0;

	for(int i = 1; i < MAX_BANS-1; ++i)
	{
		m_aBans[i].m_pNext = &m_aBans[i+1];
		m_aBans[i].m_pPrev = &m_aBans[i-1];
	}

	m_aBans[0].m_pNext = &m_aBans[1];
	m_aBans[MAX_BANS-1].m_pPrev = &m_aBans[MAX_BANS-2];
	m_pFirstFree = &m_aBans[0];
}

template<class T, int HashCount>
typename CNetBan::CBan<T> *CNetBan::CBanPool<T, HashCount>::Get(int Index) const
{
	if(Index < 0 || Index >= Num())
		return 0;

	for(CNetBan::CBan<T> *pBan = m_pFirstUsed; pBan; pBan = pBan->m_pNext, --Index)
	{
		if(Index == 0)
			return pBan;
	}

	return 0;
}


template<class T>
void CNetBan::MakeBanInfo(const CBan<T> *pBan, char *pBuf, unsigned BuffSize, int Type) const
{
	if(pBan == 0 || pBuf == 0)
	{
		if(BuffSize > 0)
			pBuf[0] = 0;
		return;
	}
	
	// build type based part
	char aBuf[256];
	if(Type == MSGTYPE_PLAYER)
		str_copy(aBuf, "You have been banned", sizeof(aBuf));
	else
	{
		char aTemp[256];
		switch(Type)
		{
		case MSGTYPE_LIST:
			str_format(aBuf, sizeof(aBuf), "%s banned", NetToString(&pBan->m_Data, aTemp, sizeof(aTemp))); break;
		case MSGTYPE_BANADD:
			str_format(aBuf, sizeof(aBuf), "banned %s", NetToString(&pBan->m_Data, aTemp, sizeof(aTemp))); break;
		case MSGTYPE_BANREM:
			str_format(aBuf, sizeof(aBuf), "unbanned %s", NetToString(&pBan->m_Data, aTemp, sizeof(aTemp))); break;
		default:
			aBuf[0] = 0;
		}
	}

	// add info part
	if(pBan->m_Info.m_Expires != CBanInfo::EXPIRES_NEVER)
	{
		int Mins = ((pBan->m_Info.m_Expires-time_timestamp()) + 59) / 60;
		if(Mins <= 1)
			str_format(pBuf, BuffSize, "%s for 1 minute (%s)", aBuf, pBan->m_Info.m_aReason);
		else
			str_format(pBuf, BuffSize, "%s for %d minutes (%s)", aBuf, Mins, pBan->m_Info.m_aReason);
	}
	else
		str_format(pBuf, BuffSize, "%s for life (%s)", aBuf, pBan->m_Info.m_aReason);
}

template<class T>
int CNetBan::Ban(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason)
{
	// do not ban localhost
	if(NetMatch(pData, &m_LocalhostIPV4) || NetMatch(pData, &m_LocalhostIPV6))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban failed (localhost)");
		return -1;
	}

	int Stamp = Seconds > 0 ? time_timestamp()+Seconds : CBanInfo::EXPIRES_NEVER;

	// set up info
	CBanInfo Info = {0};
	Info.m_Expires = Stamp;
	str_copy(Info.m_aReason, pReason, sizeof(Info.m_aReason));

	// check if it already exists
	CNetHash NetHash(pData);
	CBan<typename T::CDataType> *pBan = pBanPool->Find(pData, &NetHash);
	if(pBan)
	{
		// adjust the ban
		pBanPool->Update(pBan, &Info);
		char aBuf[128];
		MakeBanInfo(pBan, aBuf, sizeof(aBuf), MSGTYPE_LIST);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		return 1;
	}

	// add ban and print result
	pBan = pBanPool->Add(pData, &Info, &NetHash);
	if(pBan)
	{
		char aBuf[128];
		MakeBanInfo(pBan, aBuf, sizeof(aBuf), MSGTYPE_BANADD);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		return 0;
	}
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban failed (full banlist)");
	return -1;
}

template<class T>
int CNetBan::Unban(T *pBanPool, const typename T::CDataType *pData)
{
	CNetHash NetHash(pData);
	CBan<typename T::CDataType> *pBan = pBanPool->Find(pData, &NetHash);
	if(pBan)
	{
		char aBuf[256];
		MakeBanInfo(pBan, aBuf, sizeof(aBuf), MSGTYPE_BANREM);
		pBanPool->Remove(pBan);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		return 0;
	}
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "unban failed (invalid entry)");
	return -1;
}

void CNetBan::Init(IConsole *pConsole, IStorage *pStorage)
{
	m_pConsole = pConsole;
	m_pStorage = pStorage;
	m_BanAddrPool.Reset();
	m_BanRangePool.Reset();

	net_host_lookup("localhost", &m_LocalhostIPV4, NETTYPE_IPV4);
	net_host_lookup("localhost", &m_LocalhostIPV6, NETTYPE_IPV6);

	Console()->Register("ban", "s?ir", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConBan, this, "Ban ip for x minutes for any reason");
	Console()->Register("ban_range", "ss?ir", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConBanRange, this, "Ban ip range for x minutes for any reason");
	Console()->Register("unban", "s", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConUnban, this, "Unban ip/banlist entry");
	Console()->Register("unban_range", "ss", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConUnbanRange, this, "Unban ip range");
	Console()->Register("unban_all", "", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConUnbanAll, this, "Unban all entries");
	Console()->Register("bans", "", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConBans, this, "Show banlist");
	Console()->Register("bans_save", "s", CFGFLAG_SERVER|CFGFLAG_MASTER|CFGFLAG_STORE, ConBansSave, this, "Save banlist in a file");
}

void CNetBan::Update()
{
	int Now = time_timestamp();

	// remove expired bans
	char aBuf[256], aNetStr[256];
	while(m_BanAddrPool.First() && m_BanAddrPool.First()->m_Info.m_Expires != CBanInfo::EXPIRES_NEVER && m_BanAddrPool.First()->m_Info.m_Expires < Now)
	{
		str_format(aBuf, sizeof(aBuf), "ban %s expired", NetToString(&m_BanAddrPool.First()->m_Data, aNetStr, sizeof(aNetStr)));
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		m_BanAddrPool.Remove(m_BanAddrPool.First());
	}
	while(m_BanRangePool.First() && m_BanRangePool.First()->m_Info.m_Expires != CBanInfo::EXPIRES_NEVER && m_BanRangePool.First()->m_Info.m_Expires < Now)
	{
		str_format(aBuf, sizeof(aBuf), "ban %s expired", NetToString(&m_BanRangePool.First()->m_Data, aNetStr, sizeof(aNetStr)));
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		m_BanRangePool.Remove(m_BanRangePool.First());
	}
}

int CNetBan::BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason)
{
	return Ban(&m_BanAddrPool, pAddr, Seconds, pReason);
}

int CNetBan::BanRange(const CNetRange *pRange, int Seconds, const char *pReason)
{
	if(pRange->IsValid())
		return Ban(&m_BanRangePool, pRange, Seconds, pReason);

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban failed (invalid range)");
	return -1;
}

int CNetBan::UnbanByAddr(const NETADDR *pAddr)
{
	return Unban(&m_BanAddrPool, pAddr);
}

int CNetBan::UnbanByRange(const CNetRange *pRange)
{
	if(pRange->IsValid())
		return Unban(&m_BanRangePool, pRange);
	
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban failed (invalid range)");
	return -1;
}

int CNetBan::UnbanByIndex(int Index)
{
	int Result;
	char aBuf[256];
	CBanAddr *pBan = m_BanAddrPool.Get(Index);
	if(pBan)
	{
		NetToString(&pBan->m_Data, aBuf, sizeof(aBuf));
		Result = m_BanAddrPool.Remove(pBan);
	}
	else
	{
		CBanRange *pBan = m_BanRangePool.Get(Index-m_BanAddrPool.Num());
		if(pBan)
		{
			NetToString(&pBan->m_Data, aBuf, sizeof(aBuf));
			Result = m_BanRangePool.Remove(pBan);
		}
		else
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "unban failed (invalid index)");
			return -1;
		}
	}

	char aMsg[256];
	str_format(aMsg, sizeof(aMsg), "unbanned index %i (%s)", Index, aBuf);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aMsg);
	return Result;
}

bool CNetBan::IsBanned(const NETADDR *pAddr, char *pBuf, unsigned BufferSize) const
{
	CNetHash aHash[17];
	int Length = CNetHash::MakeHashArray(pAddr, aHash);

	// check ban adresses
	CBanAddr *pBan = m_BanAddrPool.Find(pAddr, &aHash[Length]);
	if(pBan)
	{
		MakeBanInfo(pBan, pBuf, BufferSize, MSGTYPE_PLAYER);
		return true;
	}

	// check ban ranges
	for(int i = Length-1; i >= 0; --i)
	{
		for(CBanRange *pBan = m_BanRangePool.First(&aHash[i]); pBan; pBan = pBan->m_pHashNext)
		{
			if(NetMatch(&pBan->m_Data, pAddr, i, Length))
			{
				MakeBanInfo(pBan, pBuf, BufferSize, MSGTYPE_PLAYER);
				return true;
			}
		}
	}
	
	return false;
}

void CNetBan::ConBan(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	const char *pStr = pResult->GetString(0);
	int Minutes = pResult->NumArguments()>1 ? clamp(pResult->GetInteger(1), 0, 44640) : 30;
	const char *pReason = pResult->NumArguments()>2 ? pResult->GetString(2) : "No reason given";

	NETADDR Addr;
	if(net_addr_from_str(&Addr, pStr) == 0)
		pThis->BanAddr(&Addr, Minutes*60, pReason);
	else
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (invalid network address)");
}

void CNetBan::ConBanRange(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	const char *pStr1 = pResult->GetString(0);
	const char *pStr2 = pResult->GetString(1);
	int Minutes = pResult->NumArguments()>2 ? clamp(pResult->GetInteger(2), 0, 44640) : 30;
	const char *pReason = pResult->NumArguments()>3 ? pResult->GetString(3) : "No reason given";

	CNetRange Range;
	if(net_addr_from_str(&Range.m_LB, pStr1) == 0 && net_addr_from_str(&Range.m_UB, pStr2) == 0)
		pThis->BanRange(&Range, Minutes*60, pReason);
	else
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "ban error (invalid range)");
}

void CNetBan::ConUnban(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	const char *pStr = pResult->GetString(0);
	if(StrAllnum(pStr))
		pThis->UnbanByIndex(str_toint(pStr));
	else
	{
		NETADDR Addr;
		if(net_addr_from_str(&Addr, pStr) == 0)
			pThis->UnbanByAddr(&Addr);
		else
			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "unban error (invalid network address)");
	}
}

void CNetBan::ConUnbanRange(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	const char *pStr1 = pResult->GetString(0);
	const char *pStr2 = pResult->GetString(1);

	CNetRange Range;
	if(net_addr_from_str(&Range.m_LB, pStr1) == 0 && net_addr_from_str(&Range.m_UB, pStr2) == 0)
		pThis->UnbanByRange(&Range);
	else
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "unban error (invalid range)");
}

void CNetBan::ConUnbanAll(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	pThis->UnbanAll();
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", "unbanned all entries");
}

void CNetBan::ConBans(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	int Count = 0;
	char aBuf[256], aMsg[256];
	for(CBanAddr *pBan = pThis->m_BanAddrPool.First(); pBan; pBan = pBan->m_pNext)
	{
		pThis->MakeBanInfo(pBan, aBuf, sizeof(aBuf), MSGTYPE_LIST);
		str_format(aMsg, sizeof(aMsg), "#%i %s", Count++, aBuf);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aMsg);
	}
	for(CBanRange *pBan = pThis->m_BanRangePool.First(); pBan; pBan = pBan->m_pNext)
	{
		pThis->MakeBanInfo(pBan, aBuf, sizeof(aBuf), MSGTYPE_LIST);
		str_format(aMsg, sizeof(aMsg), "#%i %s", Count++, aBuf);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aMsg);
	}
	str_format(aMsg, sizeof(aMsg), "%d %s", Count, Count==1?"ban":"bans");
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aMsg);
}

void CNetBan::ConBansSave(IConsole::IResult *pResult, void *pUser)
{
	CNetBan *pThis = static_cast<CNetBan *>(pUser);

	char aBuf[256];
	IOHANDLE File = pThis->Storage()->OpenFile(pResult->GetString(0), IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		str_format(aBuf, sizeof(aBuf), "failed to save banlist to '%s'", pResult->GetString(0));
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
		return;
	}

	int Now = time_timestamp();
	char aAddrStr1[NETADDR_MAXSTRSIZE], aAddrStr2[NETADDR_MAXSTRSIZE];
	for(CBanAddr *pBan = pThis->m_BanAddrPool.First(); pBan; pBan = pBan->m_pNext)
	{
		int Min = pBan->m_Info.m_Expires>-1 ? (pBan->m_Info.m_Expires-Now+59)/60 : -1;
		net_addr_str(&pBan->m_Data, aAddrStr1, sizeof(aAddrStr1), false);
		str_format(aBuf, sizeof(aBuf), "ban %s %i %s", aAddrStr1, Min, pBan->m_Info.m_aReason);
		io_write(File, aBuf, str_length(aBuf));
		io_write_newline(File);
	}
	for(CBanRange *pBan = pThis->m_BanRangePool.First(); pBan; pBan = pBan->m_pNext)
	{
		int Min = pBan->m_Info.m_Expires>-1 ? (pBan->m_Info.m_Expires-Now+59)/60 : -1;
		net_addr_str(&pBan->m_Data.m_LB, aAddrStr1, sizeof(aAddrStr1), false);
		net_addr_str(&pBan->m_Data.m_UB, aAddrStr2, sizeof(aAddrStr2), false);
		str_format(aBuf, sizeof(aBuf), "ban_range %s %s %i %s", aAddrStr1, aAddrStr2, Min, pBan->m_Info.m_aReason);
		io_write(File, aBuf, str_length(aBuf));
		io_write_newline(File);
	}

	io_close(File);
	str_format(aBuf, sizeof(aBuf), "saved banlist to '%s'", pResult->GetString(0));
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "net_ban", aBuf);
}
