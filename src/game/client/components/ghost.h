/* (c) Rajh, Redix and Sushi. */

#ifndef GAME_CLIENT_COMPONENTS_GHOST_H
#define GAME_CLIENT_COMPONENTS_GHOST_H

#include <base/tl/array.h>

#include <game/client/component.h>

class CGhost : public CComponent
{
private:
	enum
	{
		MAX_ACTIVE_GHOSTS = 8,
	};

	class CGhostItem
	{
	public:
		CTeeRenderInfo m_RenderInfo;
		array<CNetObj_Character> m_lPath;
		char m_aOwner[MAX_NAME_LENGTH];
		bool m_Mirror;
		int m_Team;

		CGhostItem() { Reset(); }

		bool Empty() { return m_lPath.size() == 0; }

		void Reset()
		{
			m_lPath.clear();
			m_Mirror = false;
			m_Team = -1;
		}
	};

	class IGhostLoader *m_pGhostLoader;
	class IGhostRecorder *m_pGhostRecorder;

	CGhostItem m_aActiveGhosts[MAX_ACTIVE_GHOSTS];
	CGhostItem m_CurGhost;

	int m_TickDiff[MAX_ACTIVE_GHOSTS];

	int m_StartRenderTick;
	int m_LastRecordTick;
	int m_LastDeathTick;
	bool m_Rendering;
	bool m_Recording;

	bool m_SymmetricMap;

	void AddInfos(CNetObj_Character Char);
	int GetSlot();

	void MirrorChar(CNetObj_Character *pChar, int Middle);

	void StartRecord();
	void StopRecord(int Time = 0);
	void StartRender();
	void StopRender();
	void RenderGhostNamePlate(CNetObj_Character Prev, CNetObj_Character Player, const char *pName);
	
	void InitRenderInfos(CTeeRenderInfo *pRenderInfo, const char *pSkinName, int UseCustomColor, int ColorBody, int ColorFeet);

	static void ConGPlay(IConsole::IResult *pResult, void *pUserData);

public:
	CGhost();

	virtual void OnRender();
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnMapLoad();

	void OnGameJoin(int Team);

	int Load(const char* pFilename);
	void Unload(int Slot);

	void ToggleMirror(int Slot) { m_aActiveGhosts[Slot].m_Mirror = !m_aActiveGhosts[Slot].m_Mirror; };
	bool IsMirrored(int Slot) const { return m_aActiveGhosts[Slot].m_Mirror; };

	bool IsMapSymmetric() const { return m_SymmetricMap; }

	class IGhostLoader *GhostLoader() const { return m_pGhostLoader; }
	class IGhostRecorder *GhostRecorder() const { return m_pGhostRecorder; }
};

#endif
