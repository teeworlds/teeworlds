#ifndef GAME_SERVER_ACCOUNT_H
#define GAME_SERVER_ACCOUNT_H

#define MAX_ACCNAME 32

#include <stdint.h>

#include <game/server/chatctl.h>
#include "acc_payload.h"

struct CAccPayloadHead
{
	uint32_t m_RegDate;

	uint32_t m_FailCount;

	uint32_t m_FailDate[2];
	uint32_t m_LastLoginDate;

	uint32_t x_Align[7];

	char m_FailIP[2][16];
	char m_LastLoginIP[16];

	char m_aPassHash[32]; //we ignore the very last char of the hash for having an unlikely-to-be-padded struct here
};

struct CAccPayload
{
	struct CAccPayloadHead m_Head;
	struct CAccGamePayload m_Body;
};

class CAccount
{
private:
	char m_aAccName[MAX_ACCNAME+1];
	struct CAccPayload m_Payload;

public:
	CAccount(const char* pAccName);
	virtual ~CAccount();

	const char *Name() const { return m_aAccName; }
	struct CAccGamePayload* Payload() { return &m_Payload.m_Body; }
	struct CAccPayloadHead* Head() { return &m_Payload.m_Head; }

	bool SetPass(const char *pPass);
	bool VerifyPass(const char* pPass) const;

	bool Write() const;
	bool Read();

	// ------- static --------

	static const char *ms_pPayloadHash;
	static class IChatCtl *ms_pChatHnd;

	static void Init(const char *pPayloadHash);
	static void HashPass(char *pDst, const char *pSrc);
	static bool IsValidAccName(const char *pName);
	static bool ParseAccline(char *pDstName, unsigned int SzName, char *pDstPass, unsigned int SzPass, const char *pLine);
	static void OverrideName(char *pDst, unsigned SzDst, class CPlayer *pPlayer, const char *pWantedName);


};

class CAccChatHandler : public IChatCtl
{
public:
	CAccChatHandler();
	virtual ~CAccChatHandler();
	virtual bool HandleChatMsg(class CPlayer *pPlayer, const char *pMsg);

};

#endif
