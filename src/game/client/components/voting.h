/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_VOTING_H
#define GAME_CLIENT_COMPONENTS_VOTING_H
#include <game/client/component.h>
#include <game/client/ui.h>
#include <engine/shared/memheap.h>

class CVoting : public CComponent
{
	CHeap m_Heap;

	static void ConCallvote(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	
	int64 m_Closetime;
	char m_aDescription[512];
	char m_aCommand[512];
	int m_Voted;
	
	void ClearOptions();
	void Callvote(const char *pType, const char *pValue);
	
public:

	struct CVoteOption
	{
		CVoteOption *m_pNext;
		CVoteOption *m_pPrev;
		char m_aCommand[1];
	};
	
	CVoteOption *m_pFirst;
	CVoteOption *m_pLast;

	CVoting();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnMessage(int Msgtype, void *pRawMsg);
	virtual void OnRender();
	
	void RenderBars(CUIRect Bars, bool Text);
	
	void CallvoteKick(int ClientId, const char *pReason);
	void CallvoteOption(int Option);
	void ForcevoteKick(int ClientId, const char *pReason);
	void ForcevoteOption(int Option);
	
	void Vote(int v); // -1 = no, 1 = yes
	
	int SecondsLeft() { return (m_Closetime - time_get())/time_freq(); }
	bool IsVoting() { return m_Closetime != 0; }
	int TakenChoice() const { return m_Voted; }
	const char *VoteDescription() const { return m_aDescription; }
	const char *VoteCommand() const { return m_aCommand; }
	
	int m_Yes, m_No, m_Pass, m_Total;
};

#endif
