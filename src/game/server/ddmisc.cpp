#include <base/system.h>

#include <game/server/player.h>

#include "ddmisc.h"

CDDChatHnd::CDDChatHnd()
{
	IChatCtl::Register(this);
}

bool CDDChatHnd::HandleChatMsg(class CPlayer *pPlayer, const char *pMsg)
{
	unsigned Time;
	char aType[16];
	char aLine[256];
	str_copy(aLine, pMsg, sizeof aLine);
	if (!ParseLine(aType, sizeof aType, &Time, str_skip_to_whitespace(aLine)))
	{
		GameContext()->SendChatTarget(pPlayer->GetCID(), "usage: /emote {happy/angry/pain/blink/surprise} <seconds>");
		return true;
	}
	
	if (!pPlayer->GetCharacter())
		return true;

	int ResetTick = GameContext()->Server()->Tick() + Time * GameContext()->Server()->TickSpeed();

	if (str_comp(aType, "angry") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_ANGRY, ResetTick);
	else if (str_comp(aType, "blink") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_BLINK, ResetTick);
	else if (str_comp(aType, "happy") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_HAPPY, ResetTick);
	else if (str_comp(aType, "pain") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_PAIN, ResetTick);
	else if (str_comp(aType, "surprise") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_SURPRISE, ResetTick);
	else if (str_comp(aType, "normal") == 0)
		pPlayer->GetCharacter()->SetDefEmote(EMOTE_NORMAL, ResetTick);
	else
	{
		GameContext()->SendChatTarget(pPlayer->GetCID(), "usage: /emote {happy/angry/pain/blink/surprise} <seconds>");
		return true;
	}

	return true;
}

//from account, TODO generalize
bool CDDChatHnd::ParseLine(char *pDstEmote, unsigned int SzEmote, unsigned *pDstTime, const char *pLine)
{
	if (!pDstEmote || !SzEmote || !pDstTime || !pLine || !*pLine)
		return false;

	char aTime[16];

	pDstEmote[0] = '\0';

	char aLine[128];
	str_copy(aLine, pLine, sizeof aLine);

	char *pWork = str_skip_whitespaces(aLine);

	if (*pWork)
	{
		char *pEnd = str_skip_to_whitespace(pWork);

		str_copy(pDstEmote, pWork, SzEmote);

		if ((unsigned int)(pEnd - pWork) < SzEmote)
			pDstEmote[pEnd - pWork] = '\0';

		pWork = str_skip_whitespaces(pEnd);

		if (*pWork)
		{
			str_copy(aTime, pWork, sizeof aTime);
			pEnd = aTime + str_length(aTime) - 1;
			while(pEnd >= aTime && (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\n' || *pEnd == '\r'))
				*(pEnd--) = '\0';
		}
	}

	if (!*aTime)
		return false;

	if (str_comp(aTime, "0") == 0)
	{
		*pDstTime = 0;
		return *pDstEmote;
	}

	*pDstTime = str_toint(aTime);

	return *pDstEmote && *pDstTime;
}
