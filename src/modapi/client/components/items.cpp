#include <engine/graphics.h>
#include <engine/demo.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/effects.h>

#include <engine/textrender.h>

#include "items.h"

CModAPI_Component_Items::CModAPI_Component_Items()
{
	m_LastTime = time_get();
}

void CModAPI_Component_Items::SetLayer(int Layer)
{
	m_Layer = Layer;
}

int CModAPI_Component_Items::GetLayer() const
{
	return m_Layer;
}
	
void CModAPI_Component_Items::RenderModAPISprite(const CNetObj_ModAPI_Sprite *pPrev, const CNetObj_ModAPI_Sprite *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!ModAPIGraphics()) return;

	float Angle = mix(2.0*pi*static_cast<float>(pPrev->m_Angle)/360.0f, 2.0*pi*static_cast<float>(pCurrent->m_Angle)/360.0f, Client()->IntraGameTick());
	float Size = mix(pPrev->m_Size, pCurrent->m_Size, Client()->IntraGameTick());
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	
	ModAPIGraphics()->DrawSprite(pCurrent->m_SpriteId, Pos, Size, Angle, 0);
}

void CModAPI_Component_Items::RenderModAPIAnimatedSprite(const CNetObj_ModAPI_AnimatedSprite *pPrev, const CNetObj_ModAPI_AnimatedSprite *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!ModAPIGraphics()) return;

	float Angle = mix(2.0*pi*static_cast<float>(pPrev->m_Angle)/360.0f, 2.0*pi*static_cast<float>(pCurrent->m_Angle)/360.0f, Client()->IntraGameTick());
	float Size = mix(pPrev->m_Size, pCurrent->m_Size, Client()->IntraGameTick());
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	vec2 Offset = mix(vec2(pPrev->m_OffsetX, pPrev->m_OffsetY), vec2(pCurrent->m_OffsetX, pCurrent->m_OffsetY), Client()->IntraGameTick());
	
	float Time = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	Time = (Time/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	Time = Time / (static_cast<float>(pCurrent->m_Duration)/1000.f);
	
	ModAPIGraphics()->DrawAnimatedSprite(pCurrent->m_SpriteId, Pos, Size, Angle, 0, pCurrent->m_AnimationId, Time, Offset);
}

void CModAPI_Component_Items::RenderModAPISpriteCharacter(const CNetObj_ModAPI_SpriteCharacter *pPrev, const CNetObj_ModAPI_SpriteCharacter *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!ModAPIGraphics()||!m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Active) return;
	
	float Angle = mix(2.0*pi*static_cast<float>(pPrev->m_Angle)/360.0f, 2.0*pi*static_cast<float>(pCurrent->m_Angle)/360.0f, Client()->IntraGameTick());
	float Size = mix(pPrev->m_Size, pCurrent->m_Size, Client()->IntraGameTick());
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	
	if(m_pClient->m_LocalClientID != -1 && m_pClient->m_LocalClientID == pCurrent->m_ClientId)
	{
		Pos = m_pClient->m_LocalCharacterPos + Pos;
	}
	else
	{
		CNetObj_Character PrevChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Prev;
		CNetObj_Character CurChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Cur;
		Pos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(CurChar.m_X, CurChar.m_Y), Client()->IntraGameTick()) + Pos;
	}
	
	ModAPIGraphics()->DrawSprite(pCurrent->m_SpriteId, Pos, Size, Angle, 0);
}

void CModAPI_Component_Items::RenderModAPIAnimatedSpriteCharacter(const CNetObj_ModAPI_AnimatedSpriteCharacter *pPrev, const CNetObj_ModAPI_AnimatedSpriteCharacter *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!ModAPIGraphics()||!m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Active) return;
	
	float Angle = mix(2.0*pi*static_cast<float>(pPrev->m_Angle)/360.0f, 2.0*pi*static_cast<float>(pCurrent->m_Angle)/360.0f, Client()->IntraGameTick());
	float Size = mix(pPrev->m_Size, pCurrent->m_Size, Client()->IntraGameTick());
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	vec2 Offset = mix(vec2(pPrev->m_OffsetX, pPrev->m_OffsetY), vec2(pCurrent->m_OffsetX, pCurrent->m_OffsetY), Client()->IntraGameTick());
	
	if(m_pClient->m_LocalClientID != -1 && m_pClient->m_LocalClientID == pCurrent->m_ClientId)
	{
		Pos = m_pClient->m_LocalCharacterPos + Pos;
	}
	else
	{
		CNetObj_Character PrevChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Prev;
		CNetObj_Character CurChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Cur;
		Pos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(CurChar.m_X, CurChar.m_Y), Client()->IntraGameTick()) + Pos;
	}
	
	float Time = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	Time = (Time/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	Time = Time / (static_cast<float>(pCurrent->m_Duration)/1000.f);
	
	ModAPIGraphics()->DrawAnimatedSprite(pCurrent->m_SpriteId, Pos, Size, Angle, 0, pCurrent->m_AnimationId, Time, Offset);
}

void CModAPI_Component_Items::RenderModAPILine(const struct CNetObj_ModAPI_Line *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!ModAPIGraphics())
		return;
	//Geometry
	vec2 StartPos = vec2(pCurrent->m_StartX, pCurrent->m_StartY);
	vec2 EndPos = vec2(pCurrent->m_EndX, pCurrent->m_EndY);
	
	float Time = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	Time = (Time/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	
	ModAPIGraphics()->DrawLine(RenderTools(),pCurrent->m_LineStyleId, StartPos, EndPos, Time);
}

void CModAPI_Component_Items::RenderModAPIText(const CNetObj_ModAPI_Text *pPrev, const CNetObj_ModAPI_Text *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!TextRender()) return;

	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	
	char aText[64];
	IntsToStr(pCurrent->m_aText, 16, &aText[0]);
	
	ModAPIGraphics()->DrawText(TextRender(), aText, Pos, ModAPI_IntToColor(pCurrent->m_Color), pCurrent->m_Size, pCurrent->m_Alignment);
}

void CModAPI_Component_Items::RenderModAPIAnimatedText(const CNetObj_ModAPI_AnimatedText *pPrev, const CNetObj_ModAPI_AnimatedText *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!TextRender()) return;

	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	vec2 Offset = mix(vec2(pPrev->m_OffsetX, pPrev->m_OffsetY), vec2(pCurrent->m_OffsetX, pCurrent->m_OffsetY), Client()->IntraGameTick());
	
	float Time = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	Time = (Time/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	Time = Time / (static_cast<float>(pCurrent->m_Duration)/1000.f);
	
	char aText[64];
	IntsToStr(pCurrent->m_aText, 16, &aText[0]);
	
	ModAPIGraphics()->DrawAnimatedText(TextRender(), aText, Pos, ModAPI_IntToColor(pCurrent->m_Color), pCurrent->m_Size, pCurrent->m_Alignment, pCurrent->m_AnimationId, Time, Offset);
}

void CModAPI_Component_Items::RenderModAPITextCharacter(const CNetObj_ModAPI_TextCharacter *pPrev, const CNetObj_ModAPI_TextCharacter *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!TextRender()||!m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Active) return;
	
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	
	if(m_pClient->m_LocalClientID != -1 && m_pClient->m_LocalClientID == pCurrent->m_ClientId)
	{
		Pos = m_pClient->m_LocalCharacterPos + Pos;
	}
	else
	{
		CNetObj_Character PrevChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Prev;
		CNetObj_Character CurChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Cur;
		Pos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(CurChar.m_X, CurChar.m_Y), Client()->IntraGameTick()) + Pos;
	}
	
	char aText[64];
	IntsToStr(pCurrent->m_aText, 16, &aText[0]);
	
	ModAPIGraphics()->DrawText(TextRender(), aText, Pos, ModAPI_IntToColor(pCurrent->m_Color), pCurrent->m_Size, pCurrent->m_Alignment);
}

void CModAPI_Component_Items::RenderModAPIAnimatedTextCharacter(const CNetObj_ModAPI_AnimatedTextCharacter *pPrev, const CNetObj_ModAPI_AnimatedTextCharacter *pCurrent)
{
	if(pCurrent->m_ItemLayer != GetLayer()) return;
	if(!TextRender()||!m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Active) return;
	
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	vec2 Offset = mix(vec2(pPrev->m_OffsetX, pPrev->m_OffsetY), vec2(pCurrent->m_OffsetX, pCurrent->m_OffsetY), Client()->IntraGameTick());
	
	if(m_pClient->m_LocalClientID != -1 && m_pClient->m_LocalClientID == pCurrent->m_ClientId)
	{
		Pos = m_pClient->m_LocalCharacterPos + Pos;
	}
	else
	{
		CNetObj_Character PrevChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Prev;
		CNetObj_Character CurChar = m_pClient->m_Snap.m_aCharacters[pCurrent->m_ClientId].m_Cur;
		Pos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(CurChar.m_X, CurChar.m_Y), Client()->IntraGameTick()) + Pos;
	}
	
	float Time = Client()->GameTick() + Client()->IntraGameTick() - pCurrent->m_StartTick;
	Time = (Time/static_cast<float>(SERVER_TICK_SPEED)) * 1000.0f;
	Time = Time / (static_cast<float>(pCurrent->m_Duration)/1000.f);
	
	char aText[64];
	IntsToStr(pCurrent->m_aText, 16, &aText[0]);
	
	ModAPIGraphics()->DrawAnimatedText(TextRender(), aText, Pos, ModAPI_IntToColor(pCurrent->m_Color), pCurrent->m_Size, pCurrent->m_Alignment, pCurrent->m_AnimationId, Time, Offset);
}

void CModAPI_Component_Items::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		switch(Item.m_Type)
		{
			case NETOBJTYPE_MODAPI_SPRITE:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPISprite((const CNetObj_ModAPI_Sprite *)pPrev, (const CNetObj_ModAPI_Sprite *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_ANIMATEDSPRITE:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPIAnimatedSprite((const CNetObj_ModAPI_AnimatedSprite *)pPrev, (const CNetObj_ModAPI_AnimatedSprite *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_SPRITECHARACTER:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPISpriteCharacter((const CNetObj_ModAPI_SpriteCharacter *)pPrev, (const CNetObj_ModAPI_SpriteCharacter *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_ANIMATEDSPRITECHARACTER:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPIAnimatedSpriteCharacter((const CNetObj_ModAPI_AnimatedSpriteCharacter *)pPrev, (const CNetObj_ModAPI_AnimatedSpriteCharacter *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_LINE:
			{
				RenderModAPILine((const CNetObj_ModAPI_Line *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_TEXT:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPIText((const CNetObj_ModAPI_Text *)pPrev, (const CNetObj_ModAPI_Text *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_ANIMATEDTEXT:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPIAnimatedText((const CNetObj_ModAPI_AnimatedText *)pPrev, (const CNetObj_ModAPI_AnimatedText *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_TEXTCHARACTER:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPITextCharacter((const CNetObj_ModAPI_TextCharacter *)pPrev, (const CNetObj_ModAPI_TextCharacter *)pData);
			}
			break;
				
			case NETOBJTYPE_MODAPI_ANIMATEDTEXTCHARACTER:
			{
				const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
				if(pPrev)
					RenderModAPIAnimatedTextCharacter((const CNetObj_ModAPI_AnimatedTextCharacter *)pPrev, (const CNetObj_ModAPI_AnimatedTextCharacter *)pData);
			}
			break;
		}
	}

	int64 CurrentTime = time_get();
	int64 DeltaTime = CurrentTime - m_LastTime;
	m_LastTime = CurrentTime;

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(!pInfo->m_Paused)
			UpdateEvents((float)((DeltaTime)/(double)time_freq())*pInfo->m_Speed);
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameData && !(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			UpdateEvents((float)((DeltaTime)/(double)time_freq()));
	}
}

void CModAPI_Component_Items::UpdateEvents(float DeltaTime)
{
	{
		std::list<CTextEventState>::iterator Iter = m_TextEvent.begin();
		while(Iter != m_TextEvent.end())
		{			
			Iter->m_Time += DeltaTime;
			float Time = Iter->m_Time / Iter->m_Duration;
			
			ModAPIGraphics()->DrawAnimatedText(
				TextRender(),
				Iter->m_aText,
				Iter->m_Pos,
				Iter->m_Color,
				Iter->m_Size,
				Iter->m_Alignment,
				Iter->m_AnimationId,
				Time,
				Iter->m_Offset);
			
			const CModAPI_Asset_Animation* pAnimation = ModAPIGraphics()->m_AnimationsCatalog.GetAsset(Iter->m_AnimationId);
			if(!pAnimation || Time > pAnimation->GetDuration())
			{
				Iter = m_TextEvent.erase(Iter);
			}
			else
			{
				Iter++;
			}
		}
	}
}
	
bool CModAPI_Component_Items::ProcessEvent(int Type, CNetEvent_Common* pEvent)
{
	switch(Type)
	{
		case NETEVENTTYPE_MODAPI_ANIMATEDTEXT:
		{			
			CNetEvent_ModAPI_AnimatedText* pTextEvent = (CNetEvent_ModAPI_AnimatedText*) pEvent;
			if(pTextEvent->m_ItemLayer != GetLayer())
				return false;
			
			CTextEventState EventState;
			EventState.m_Pos.x = static_cast<float>(pTextEvent->m_X);
			EventState.m_Pos.y = static_cast<float>(pTextEvent->m_Y);
			EventState.m_Size = static_cast<float>(pTextEvent->m_Size);
			EventState.m_Color = ModAPI_IntToColor(pTextEvent->m_Color);
			EventState.m_Alignment = pTextEvent->m_Alignment;
			IntsToStr(pTextEvent->m_aText, 16, &EventState.m_aText[0]);
			
			EventState.m_AnimationId = pTextEvent->m_AnimationId;
			EventState.m_Offset.x = static_cast<float>(pTextEvent->m_OffsetX);
			EventState.m_Offset.y = static_cast<float>(pTextEvent->m_OffsetY);
			EventState.m_Duration = static_cast<float>(pTextEvent->m_Duration)/1000.f;
			EventState.m_Time = 0.f;
			
			m_TextEvent.push_back(EventState);
		}
		return true;
	}
}
