/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENT_H
#define GAME_CLIENT_COMPONENT_H

#include <engine/input.h>
#include "gameclient.h"

class CComponent
{
protected:
	friend class CGameClient;

	CGameClient *m_pClient;

	// perhaps propagte pointers for these as well
	class IKernel *Kernel() const { return m_pClient->Kernel(); }
	class IGraphics *Graphics() const { return m_pClient->Graphics(); }
	class ITextRender *TextRender() const { return m_pClient->TextRender(); }
	class IClient *Client() const { return m_pClient->Client(); }
	class IInput *Input() const { return m_pClient->Input(); }
	class IStorage *Storage() const { return m_pClient->Storage(); }
	class CUI *UI() const { return m_pClient->UI(); }
	class ISound *Sound() const { return m_pClient->Sound(); }
	class CRenderTools *RenderTools() const { return m_pClient->RenderTools(); }
	class IConsole *Console() const { return m_pClient->Console(); }
	class IDemoPlayer *DemoPlayer() const { return m_pClient->DemoPlayer(); }
	class IDemoRecorder *DemoRecorder() const { return m_pClient->DemoRecorder(); }
	class IServerBrowser *ServerBrowser() const { return m_pClient->ServerBrowser(); }
	class CLayers *Layers() const { return m_pClient->Layers(); }
	class CCollision *Collision() const { return m_pClient->Collision(); }
public:
	virtual ~CComponent() {}

	virtual void OnStateChange(int NewState, int OldState) {};
	virtual void OnConsoleInit() {};
	virtual void OnInit() {};
	virtual void OnReset() {};
	virtual void OnRender() {};
	virtual void OnRelease() {};
	virtual void OnMapLoad() {};
	virtual void OnMessage(int Msg, void *pRawMsg) {}
	virtual bool OnMouseMove(float x, float y) { return false; }
	virtual bool OnInput(IInput::CEvent e) { return false; }
};

#endif
