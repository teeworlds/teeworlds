#include "component.h"

#include "gameclient.h"

class IKernel *CComponent::Kernel() const { return m_pClient->Kernel(); }
class IGraphics *CComponent::Graphics() const { return m_pClient->Graphics(); }
class ITextRender *CComponent::TextRender() const { return m_pClient->TextRender(); }
class IInput *CComponent::Input() const { return m_pClient->Input(); }
class IStorage *CComponent::Storage() const { return m_pClient->Storage(); }
class CUI *CComponent::UI() const { return m_pClient->UI(); }
class ISound *CComponent::Sound() const { return m_pClient->Sound(); }
class CRenderTools *CComponent::RenderTools() const { return m_pClient->RenderTools(); }
class CConfig *CComponent::Config() const { return m_pClient->Config(); }
class IConsole *CComponent::Console() const { return m_pClient->Console(); }
class IDemoPlayer *CComponent::DemoPlayer() const { return m_pClient->DemoPlayer(); }
class IDemoRecorder *CComponent::DemoRecorder() const { return m_pClient->DemoRecorder(); }
class IServerBrowser *CComponent::ServerBrowser() const { return m_pClient->ServerBrowser(); }
class CLayers *CComponent::Layers() const { return m_pClient->Layers(); }
class CCollision *CComponent::Collision() const { return m_pClient->Collision(); }

float CComponent::LocalTime() const
{
	return Client()->LocalTime();
}

class IClient *CComponent::Client() const
{
	return m_pClient->Client();
}
