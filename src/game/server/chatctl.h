#ifndef GAME_SERVER_CHATCTL_H
#define GAME_SERVER_CHATCTL_H

#define MAX_CHATCTLS 32

/* certainly not the best, but a (really) simple way to do it
   classes implementing this interface are required to Register()
   themselves (or a delegate) in order to receive chat messages */
class IChatCtl
{
	private:
		static class IChatCtl *ms_paRegged[MAX_CHATCTLS];
		static class CGameContext *ms_pGameContext;
	protected:
		class CGameContext *GameContext() const { return ms_pGameContext; }

	public:
		static char ms_CmdChar;

		// return true to indicate the message got handled
		virtual bool HandleChatMsg(class CPlayer *pPlayer, const char *pMsg) = 0;

		static void Init(class CGameContext *pGameContext);
		static bool Register(class IChatCtl *pChatCtl);
		static bool Dispatch(class CPlayer *pPlayer, const char *pMsg);

};

#endif
