#include <base/system.h>

#include <engine/external/md5/md5.h>
#include <engine/shared/config.h>
#include <game/server/player.h>

#include "account.h"

#define MAX_FILEPATH 256

//#define D(F, A...) printf("%s:%s():%d - " F "\n",__FILE__, __func__,  __LINE__,##A)

const char* CAccount::ms_pPayloadHash = 0;
class IChatCtl *CAccount::ms_pChatHnd = 0;

CAccount::CAccount(const char* pAccName)
{
	if (pAccName)
		str_copy(m_aAccName, pAccName, sizeof m_aAccName);
	else
		m_aAccName[0] = '\0';

	mem_zero(&m_Payload, sizeof m_Payload);
	m_Payload.m_Head.m_RegDate = m_Payload.m_Head.m_LastLoginDate = time_timestamp();// for new accs only
}

CAccount::~CAccount()
{
}

bool CAccount::SetPass(const char *pPass)
{
	char aHashedPass[33];
	if (!pPass || !*pPass)
		return false;

	HashPass(aHashedPass, pPass);

	str_copy(m_Payload.m_Head.m_aPassHash, aHashedPass, sizeof m_Payload.m_Head.m_aPassHash);

	return true;
}

bool CAccount::VerifyPass(const char* pPass) const
{
	if (!pPass || !*pPass)
		return false;

	char aBuf[33];
	HashPass(aBuf, pPass);

	return str_comp_num(aBuf, m_Payload.m_Head.m_aPassHash, (sizeof m_Payload.m_Head.m_aPassHash) - 1) == 0;
}

bool CAccount::Write() const
{
	// we always pad to have the size be a multiple of 32, to counter different
	// padding behaviour of different compilers.
	char aBodyChunk[31 + sizeof m_Payload.m_Body];
	bool Ret = false;
	if (!*m_aAccName || !*g_Config.m_SvAccDir)
		return false;

	// round up to the next full multiple of 32
	int SzBody = (31 + sizeof m_Payload.m_Body) & (~31);

	mem_zero(aBodyChunk, sizeof aBodyChunk);
	mem_copy(aBodyChunk, &m_Payload.m_Body, sizeof m_Payload.m_Body);

	char aBuf[MAX_FILEPATH];
	str_format(aBuf, sizeof aBuf, "%s/%s_%s.acc", g_Config.m_SvAccDir, ms_pPayloadHash, m_aAccName);

	IOHANDLE fd = io_open(aBuf, IOFLAG_WRITE);

	if (!fd)
		return false;
	if (io_write(fd, &m_Payload.m_Head, sizeof m_Payload.m_Head) == sizeof m_Payload.m_Head
			&& io_write(fd, aBodyChunk, SzBody) == (unsigned)SzBody)
		Ret = true;

	io_close(fd);
	return Ret;
}

bool CAccount::Read()
{
	// we always pad to have the size be a multiple of 32, to counter different
	// padding behaviour of different compilers.
	char aBodyChunk[31 + sizeof m_Payload.m_Body];

	bool Ret = false;
	if (!*m_aAccName || !*g_Config.m_SvAccDir)
		return false;

	// round up to the next full multiple of 32
	int SzBody = (31 + sizeof m_Payload.m_Body) & (~31);

	mem_zero(aBodyChunk, sizeof aBodyChunk);

	char aBuf[MAX_FILEPATH];
	str_format(aBuf, sizeof aBuf, "%s/%s_%s.acc", g_Config.m_SvAccDir, ms_pPayloadHash, m_aAccName);

	IOHANDLE fd = io_open(aBuf, IOFLAG_READ);

	if (!fd)
		return false;

	if (io_read(fd, &m_Payload.m_Head, sizeof m_Payload.m_Head) == sizeof m_Payload.m_Head
			&& io_read(fd, aBodyChunk, SzBody) == (unsigned)SzBody)
	{
		Ret = true;
		mem_copy(&m_Payload.m_Body, aBodyChunk, sizeof m_Payload.m_Body);
	}

	io_close(fd);
	return Ret;
}


void CAccount::Init(const char *pPayloadHash)
{
	ms_pChatHnd = new CAccChatHandler;
	IChatCtl::Register(ms_pChatHnd);
 	CAccount::ms_pPayloadHash = pPayloadHash;
	if (!*g_Config.m_SvAccDir)
		str_copy(g_Config.m_SvAccDir, ".", 2);

	if (!fs_is_dir(g_Config.m_SvAccDir) && (fs_makedir(g_Config.m_SvAccDir) != 0 || !fs_is_dir(g_Config.m_SvAccDir))) // sic
	{
		dbg_msg("acc", "failed to create account directory \"%s\", falling back to \"./\"", g_Config.m_SvAccDir);
		str_copy(g_Config.m_SvAccDir, ".", 2);
	}
}

void CAccount::HashPass(char *pDst, const char *pSrc)
{
	if (!pSrc)
		return;

	char aBuf[33];
	md5_state_t State;
	md5_byte_t aDigest[16];

	md5_init(&State);
	md5_append(&State, (const md5_byte_t *)pSrc, str_length(pSrc));
	md5_finish(&State, aDigest);

	for (int i = 0; i < 16; ++i)
		str_format(aBuf + 2*i, 3, "%02x", aDigest[i]);

	str_copy(pDst, aBuf, 33);
}

bool CAccount::IsValidAccName(const char *pSrc)
{
	if (!pSrc || !*pSrc)
		return false;

	while(*pSrc)
	{
		const char *pAC = g_Config.m_SvAccAllowedNameChars;
		bool Okay = false;
		while(*pAC)
			if (*pSrc == *(pAC++))
			{
				Okay = true;
				break;
			}

		if (!Okay)
			return false;

		pSrc++;
	}

	return true;
}

bool CAccount::ParseAccline(char *pDstName, unsigned int SzName, char *pDstPass, unsigned int SzPass, const char *pLine)
{
	if (!pDstName || !SzName || !pDstPass || !SzPass || !pLine || !*pLine)
		return false;

	pDstName[0] = pDstPass[0] = '\0';

	char aLine[128];
	str_copy(aLine, pLine, sizeof aLine);

	char *pWork = str_skip_whitespaces(aLine);

	if (*pWork)
	{
		char *pEnd = str_skip_to_whitespace(pWork);

		str_copy(pDstName, pWork, SzName);

		if ((unsigned int)(pEnd - pWork) < SzName)
			pDstName[pEnd - pWork] = '\0';

		pWork = str_skip_whitespaces(pEnd);

		if (*pWork)
		{
			str_copy(pDstPass, pWork, SzPass);
			pEnd = pDstPass + str_length(pDstPass) - 1;
			while(pEnd >= pDstPass && (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\n' || *pEnd == '\r'))
				*(pEnd--) = '\0';
		}
	}

	return *pDstName && *pDstPass;
}

void CAccount::OverrideName(char *pDst, unsigned SzDst, class CPlayer *pPlayer, const char *pWantedName)
{
	if (pPlayer->GetAccount() && *g_Config.m_SvAccMemberPrefix)
		str_copy(pDst, g_Config.m_SvAccMemberPrefix, SzDst);
	else if (!pPlayer->GetAccount() && *g_Config.m_SvAccGuestPrefix)
		str_copy(pDst, g_Config.m_SvAccGuestPrefix, SzDst);
	else
		pDst[0] = '\0';

	str_append(pDst, (g_Config.m_SvAccEnforceNames && pPlayer->GetAccount())?pPlayer->GetAccount()->m_aAccName:pWantedName, SzDst);
}



CAccChatHandler::CAccChatHandler()
{
}

CAccChatHandler::~CAccChatHandler()
{
}

bool CAccChatHandler::HandleChatMsg(class CPlayer *pPlayer, const char *pMsg)
{
	char aName[MAX_ACCNAME+1];
	char aPass[33];//32 significant chars in password to be hashed
	char aBuf[256];
	CAccount *pAcc = NULL;

	if (str_comp_num(pMsg, "/reg", 4) == 0)
	{
		if (!g_Config.m_SvAccEnable)
			str_copy(aBuf, "Account system is disabled.", sizeof aBuf);
		else if (pPlayer->GetAccount())
			str_copy(aBuf, "You cannot create a new account while being logged in.", sizeof aBuf);
		else if (!CAccount::ParseAccline(aName, sizeof aName, aPass, sizeof aPass, str_skip_to_whitespace((char*)pMsg)))
			str_copy(aBuf, "To create a new account, say: /reg NAME PASSWORD", sizeof aBuf);
		else if (str_find(aPass," "))
			str_copy(aBuf, "The password must not contain any spaces.", sizeof aBuf);
		else if (!CAccount::IsValidAccName(aName))
			str_format(aBuf, sizeof aBuf, "Illegal account name. Allowed characters are: %s", g_Config.m_SvAccAllowedNameChars);
		else if ((pAcc = new CAccount(aName))->Read())
			str_copy(aBuf, "An account with this name does already exist. Choose a different name.", sizeof aBuf);
		else if (!pAcc->SetPass(aPass) || !pAcc->Write())
			str_copy(aBuf, "Account creation failed. Sorry.", sizeof aBuf);
		else
		{
			str_copy(aBuf, "Account successfully created, logged in. Do not forget your password, there is no way to get it back if lost.", sizeof aBuf);
			pPlayer->SetAccount(pAcc);
			pAcc->Payload()->blockScore = 10.0; // XXX dirty
			char aName[MAX_NAME_LENGTH];
			CAccount::OverrideName(aName, sizeof aName, pPlayer, pPlayer->m_OrigName);
			GameContext()->Server()->SetClientName(pPlayer->GetCID(), aName);
			if (!g_Config.m_SvTournamentMode)
				pPlayer->SetTeam(0);
			pAcc = NULL;
		}

		if (pAcc)
			delete pAcc;

		GameContext()->SendChatTarget(pPlayer->GetCID(), aBuf);
	}
	else if (str_comp_num(pMsg, "/login", 6) == 0)
	{
		bool PassFailed = false;
		if (!g_Config.m_SvAccEnable)
			str_copy(aBuf, "Account system is disabled.", sizeof aBuf);
		else if (pPlayer->GetAccount())
			str_copy(aBuf, "You are already logged in.", sizeof aBuf);
		else if (!CAccount::ParseAccline(aName, sizeof aName, aPass, sizeof aPass, str_skip_to_whitespace((char*)pMsg)))
			str_copy(aBuf, "To login into an existing account, say: /login NAME PASSWORD", sizeof aBuf);
		else if (!CAccount::IsValidAccName(aName) || !(pAcc = new CAccount(aName))->Read() || (PassFailed = !pAcc->VerifyPass(aPass)))
		{
			str_copy(aBuf, "Failed to login, wrong accdata version, unknown account or password.", sizeof aBuf);
			if (PassFailed)
			{
				int FailInd = pAcc->Head()->m_FailCount?1:0;

				pAcc->Head()->m_FailCount++;
				char aIP[16];
				GameContext()->Server()->GetClientAddr(pPlayer->GetCID(), aIP, sizeof aIP);
				str_copy(pAcc->Head()->m_FailIP[FailInd], aIP, sizeof (pAcc->Head()->m_FailIP[FailInd]));
				pAcc->Head()->m_FailDate[FailInd] = time_timestamp();
				pAcc->Write();
			}
		}
		else
		{
			bool Already = false;
			for (int i = 0; !Already && i < MAX_CLIENTS; ++i)
			{
				CPlayer *p = ((CGameContext*)GameContext())->m_apPlayers[i];
				if (p && p->GetAccount() && str_comp(p->GetAccount()->Name(), pAcc->Name()) == 0)
					Already = true;
			}

			if (Already)
				str_copy(aBuf, "Account in use. Nice try.", sizeof aBuf);
			else
			{

				char aTmp[128];
				if (pAcc->Head()->m_FailCount > 0)
				{
					char aTime[2][32];
					str_timestamp_at(aTime[0], sizeof (aTime[0]), pAcc->Head()->m_FailDate[0]);
					if (pAcc->Head()->m_FailCount > 1)
					{
						str_timestamp_at(aTime[1], sizeof (aTime[1]), pAcc->Head()->m_FailDate[1]);
						str_format(aTmp, sizeof aTmp, "%d failed logins since last login. first: %s at %s, last: %s at %s",
								pAcc->Head()->m_FailCount, pAcc->Head()->m_FailIP[0], aTime[0], pAcc->Head()->m_FailIP[1], aTime[1]);
					}
					else
					{
						str_format(aTmp, sizeof aTmp, "One failed login since last login: %s at %s",
							pAcc->Head()->m_FailIP[0], aTime[0]);
					}
					pAcc->Head()->m_FailCount = 0;
					pAcc->Head()->m_FailIP[0][0] = pAcc->Head()->m_FailIP[1][0] = '\0';
					GameContext()->SendChatTarget(pPlayer->GetCID(), aTmp);
				}

				str_timestamp_at(aTmp, sizeof aTmp, pAcc->Head()->m_LastLoginDate);
				str_format(aBuf, sizeof aBuf, "Login successful. Last login from %s at %s.", pAcc->Head()->m_LastLoginIP, aTmp);

				pAcc->Head()->m_LastLoginDate = time_timestamp();
				GameContext()->Server()->GetClientAddr(pPlayer->GetCID(), pAcc->Head()->m_LastLoginIP, sizeof (pAcc->Head()->m_LastLoginIP));

				pAcc->Write();

				pPlayer->SetAccount(pAcc);
				char aName[MAX_NAME_LENGTH];
				CAccount::OverrideName(aName, sizeof aName, pPlayer, pPlayer->m_OrigName);
				GameContext()->Server()->SetClientName(pPlayer->GetCID(), aName);
				if (!g_Config.m_SvTournamentMode)
					pPlayer->SetTeam(0);

				pAcc = NULL;
			}
		}

		GameContext()->SendChatTarget(pPlayer->GetCID(), aBuf);
		if (pAcc)
			delete pAcc;

	}
	else if (str_comp_num(pMsg, "/pass", 5) == 0)
	{
		char aFormerPass[sizeof aPass];
		if (!g_Config.m_SvAccEnable)
			str_copy(aBuf, "Account system is disabled.", sizeof aBuf);
		else if (!(pAcc = pPlayer->GetAccount()))
			str_copy(aBuf, "Cannot change password from outside, use /login first.", sizeof aBuf);
		else if (!CAccount::ParseAccline(aFormerPass, sizeof aFormerPass, aPass, sizeof aPass, str_skip_to_whitespace((char*)pMsg)))
			str_copy(aBuf, "To change your password, say: /pass OLD_PASS NEW_PASS", sizeof aBuf);
		else if (str_find(aPass," "))
			str_copy(aBuf, "The password must not contain any spaces.", sizeof aBuf);
		else if (!pAcc->VerifyPass(aFormerPass))
			str_copy(aBuf, "Failed to change password, incorrect old password given.", sizeof aBuf);
		else
		{
			pAcc->SetPass(aPass);
			if (!pAcc->Write())
			{
				str_copy(aBuf, "Failed to save the new password, sorry.", sizeof aBuf);
				pAcc->SetPass(aFormerPass);
			}
			else
				str_copy(aBuf, "Password changed.", sizeof aBuf);
		}

		GameContext()->SendChatTarget(pPlayer->GetCID(), aBuf);
	}
	else if (str_comp(pMsg, "/save") == 0)
	{
		if (!g_Config.m_SvAccEnable)
			str_copy(aBuf, "Account system is disabled.", sizeof aBuf);
		else if (!pPlayer->GetAccount())
			str_copy(aBuf, "Cannot save while not logged in. use /login", sizeof aBuf);
		else if (!pPlayer->GetAccount()->Write())
			str_copy(aBuf, "Failed to save. Sorry.", sizeof aBuf);
		else
			str_copy(aBuf, "Saved.", sizeof aBuf);

		GameContext()->SendChatTarget(pPlayer->GetCID(), aBuf);
	}
	else if (str_comp(pMsg, "/bail") == 0) //logout w/o saving
	{
		if (!g_Config.m_SvAccAllowBail)
			str_copy(aBuf, "Cannot bail, disallowed.", sizeof aBuf);
		else if (!pPlayer->GetAccount())
			str_copy(aBuf, "Cannot bail out while not logged in.", sizeof aBuf);
		else
		{
			str_copy(aBuf, "Logged out without saving.", sizeof aBuf);
			delete pPlayer->GetAccount();
			pPlayer->SetAccount(NULL);

			char aName[MAX_NAME_LENGTH];
			CAccount::OverrideName(aName, sizeof aName, pPlayer, pPlayer->m_OrigName);
			GameContext()->Server()->SetClientName(pPlayer->GetCID(), aName);
			if (g_Config.m_SvAccEnforce)
				pPlayer->SetTeam(TEAM_SPECTATORS);
		}

		GameContext()->SendChatTarget(pPlayer->GetCID(), aBuf);
	}
	else
		return false;

	return true;
}
