#ifndef GAME_SERVER_DDMISC_H
#define GAME_SERVER_DDMISC_H

#include <game/server/chatctl.h>

class CDDChatHnd : public IChatCtl
{
public:
	CDDChatHnd();
	virtual bool HandleChatMsg(class CPlayer *pPlayer, const char *pMsg);
	static bool ParseLine(char *pDstEmote, unsigned int SzEmote, unsigned *pDstTime, const char *pLine);

};

#endif
