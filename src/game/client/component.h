/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENT_H
#define GAME_CLIENT_COMPONENT_H

#include <engine/input.h>

#include <engine/client.h>
#include <engine/console.h>

class CGameClient;

class CComponent
{
protected:
	friend class CGameClient;

	CGameClient *m_pClient;

	// perhaps propagte pointers for these as well
	class IKernel *Kernel() const;
	class IGraphics *Graphics() const;
	class ITextRender *TextRender() const;
	class IClient *Client() const;
	class IInput *Input() const;
	class IStorage *Storage() const;
	class CUI *UI() const;
	class ISound *Sound() const;
	class CRenderTools *RenderTools() const;
	class CConfig *Config() const;
	class IConsole *Console() const;
	class IDemoPlayer *DemoPlayer() const;
	class IDemoRecorder *DemoRecorder() const;
	class IServerBrowser *ServerBrowser() const;
	class CLayers *Layers() const;
	class CCollision *Collision() const;

	float LocalTime() const;

public:
	virtual ~CComponent() {}

	virtual void OnStateChange(int NewState, int OldState) {}
	virtual void OnConsoleInit() {}
	virtual int GetInitAmount() const { return 0; } // Amount of progress reported by this component during OnInit
	virtual void OnInit() {}
	virtual void OnShutdown() {}
	virtual void OnReset() {}
	virtual void OnRender() {}
	virtual void OnRelease() {}
	virtual void OnMapLoad() {}
	virtual void OnMessage(int Msg, void *pRawMsg) {}
	virtual bool OnCursorMove(float x, float y, int CursorType) { return false; }
	virtual bool OnInput(IInput::CEvent e) { return false; }
};

#endif
