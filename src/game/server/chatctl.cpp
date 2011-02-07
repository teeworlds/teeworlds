#include "chatctl.h"

class IChatCtl *IChatCtl::ms_paRegged[MAX_CHATCTLS] = {0};
char IChatCtl::ms_CmdChar = '/';
class CGameContext *IChatCtl::ms_pGameContext = 0;

void IChatCtl::Init(class CGameContext *pGameContext)
{
	ms_pGameContext = pGameContext;
}

bool IChatCtl::Register(class IChatCtl *pChatCtl)
{
	for(int z = 0; z < MAX_CHATCTLS; ++z)
		if (!ms_paRegged[z])
			return (ms_paRegged[z] = pChatCtl);
	return false;
}

bool IChatCtl::Dispatch(class CPlayer *pPlayer, const char *pMsg)
{
	for(int z = 0; z < MAX_CHATCTLS; ++z)
		if (ms_paRegged[z] && ms_paRegged[z]->HandleChatMsg(pPlayer, pMsg))
			return true;
	return false;
}

