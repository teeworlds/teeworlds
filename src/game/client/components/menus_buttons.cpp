/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/textrender.h>
#include <engine/keys.h>

#include <game/generated/client_data.h>

#include "menus.h"


float *CMenus::ButtonFade(const void *pID, float Seconds, int Checked)
{
	float *pFade = (float*)pID;
	if(UI()->HotItem() == pID || Checked)
		*pFade = Seconds;
	else if(*pFade > 0.0f)
	{
		if(m_ResetFade)
			*pFade = 0.0f;
		else
		{
			*pFade -= Client()->RenderFrameTime();
			if(*pFade < 0.0f)
				*pFade = 0.0f;
		}
	}
	return pFade;
}

int CMenus::DoButtonDefault(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners, float r, float FontFactor, vec4 ColorHot, bool TextFade)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds, Checked);
	float FadeVal = *pFade/Seconds;

	vec4 Color = mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), ColorHot, FadeVal);

	RenderTools()->DrawUIRect(pRect, Color, Corners, r);
	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	Temp.HMargin((Temp.h*FontFactor)/2.0f, &Temp);
	if(TextFade)
	{
		TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
		TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	}
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	if(TextFade)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButtonStart(const void *pID, const char *pText, const CUIRect *pRect, const char *pImageName, float r, float FontFactor)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds);
	float FadeVal = *pFade/Seconds;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, r);
	CUIRect Text, Image;
	pRect->VSplitRight(pRect->h*4.0f, &Text, &Image); // always correct ratio for image

	// render image
	const CMenuImage *pImage = FindMenuImage(pImageName);
	if(pImage)
	{
		
		Graphics()->TextureSet(pImage->m_GreyTexture);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		if(*pFade > 0.0f)
		{
			Graphics()->TextureSet(pImage->m_OrgTexture);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, *pFade/Seconds);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		
	}

	Text.HMargin(pRect->h>=20.0f?2.0f:1.0f, &Text);
	Text.HMargin((Text.h*FontFactor)/2.0f, &Text);
	Text.VSplitLeft(r, 0, &Text);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Text, pText, Text.h*ms_FontmodHeight, 0);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int CMenus::DoTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners, float r, float FontFactor, vec4 ColorHot, bool Fade)
{
	float FadeVal = 0.0f;
	if(Checked)
		FadeVal = 1.0f;

	if(Fade)
	{
		float Seconds = 0.6f; //  0.6 seconds for fade
		float *pFade = ButtonFade(pID, Seconds, Checked);
		FadeVal = *pFade/Seconds;
	}

	vec4 Color = mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), ColorHot, FadeVal);

	RenderTools()->DrawUIRect(pRect, Color, Corners, r);

	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	Temp.HMargin((Temp.h*FontFactor)/2.0f, &Temp);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButtonSprite(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, bool Draw)
{
	int Inside = UI()->MouseInside(pRect);

	float FadeVal = 0.0f;

	if(pID)
	{
		float Seconds = 0.6f; //  0.6 seconds for fade
		float *pFade = ButtonFade(pID, Seconds, 0);
		FadeVal = *pFade/Seconds;
	}
	else if(Inside)
		FadeVal = 1.0f;

	if(Draw)
		RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, 5.0f);

	CUIRect Icon = *pRect;
	//Icon.Margin(2.0f, &Icon);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f+FadeVal*0.5f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	if(pID)
		return UI()->DoButtonLogic(pID, "", false, pRect);

	int ReturnValue = 0;
	if(Inside && Input()->KeyDown(KEY_MOUSE_1))
		ReturnValue = 1;

	return ReturnValue;
}

int CMenus::DoSwitchSprite(const void *pID, int ImageID, int SpriteIDInactive, int SpriteIDActive, int Checked, const CUIRect *pRect)
{
	int Inside = UI()->MouseInside(pRect);

	float FadeVal = 0.0f;

	if(pID)
	{
		float Seconds = 0.6f; //  0.6 seconds for fade
		float *pFade = ButtonFade(pID, Seconds, 0);
		FadeVal = *pFade/Seconds;
	}
	else if(Inside)
		FadeVal = 1.0f;

	CUIRect Icon = *pRect;
	//Icon.Margin(1.0f, &Icon);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.75f+FadeVal*0.25f);
	if(Checked)
		RenderTools()->SelectSprite(SpriteIDActive);
	else
		RenderTools()->SelectSprite(SpriteIDInactive);
	IGraphics::CQuadItem QuadItem(Icon.x, Icon.y, Icon.w, Icon.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	if(pID)
		return UI()->DoButtonLogic(pID, "", Checked, pRect);

	int ReturnValue = 0;
	if(Inside && Input()->KeyDown(KEY_MOUSE_1))
		ReturnValue = 1;

	return ReturnValue;
}

int CMenus::DoButtonMouseOver(int ImageID, int SpriteID, const CUIRect *pRect)
{
	int Inside = UI()->MouseInside(pRect);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Inside ? 1.0f : 0.6f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return Inside;
}

int CMenus::DoButtonToggle(const void *pID, int Checked, const CUIRect *pRect, bool Active)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(Checked?SPRITE_GUIBUTTON_ON:SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(UI()->HotItem() == pID && Active)
	{
		RenderTools()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active ? UI()->DoButtonLogic(pID, "", Checked, pRect) : 0;
}

void CMenus::DoButtonKeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds, Checked);
	float FadeVal = *pFade/Seconds;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, 5.0f);
	CUIRect Temp;
	pRect->HMargin(1.0f, &Temp);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
}

int CMenus::DoButtonGridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
//void CMenus::ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(Checked)
	{
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);
		TextRender()->TextColor(0.0f, 0.0f, 0.0f, 1.0f);
		TextRender()->TextOutlineColor(1.0f, 1.0f, 1.0f, 0.25f);
	}

	if(pText)
	{
		CUIRect Label;
		pRect->VSplitLeft(5.0f, 0, &Label);
		Label.y+=2.0f;
		UI()->DoLabel(&Label, pText, pRect->h*ms_FontmodHeight*0.8f, 0);
	}

	if(Checked)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}

	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButtonGridHeaderIcon(const void *pID, int ImageID, int SpriteID, const CUIRect *pRect, int Corners)
{
	float Seconds = 0.6f; //  0.6 seconds for fade
	float *pFade = ButtonFade(pID, Seconds);

	RenderTools()->DrawUIRect(pRect, vec4(1.0f, 1.0f, 1.0f, 0.5f+(*pFade/Seconds)*0.25f), Corners, 5.0f);

	CUIRect Image;
	pRect->HMargin(2.0f, &Image);
	pRect->VMargin((Image.w-Image.h)/2.0f, &Image);

	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pID, "", false, pRect);
}

int CMenus::DoButtonCheckBoxCommon(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect, bool Checked)
//void CMenus::ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra)
{
	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_ALL, 5.0f);

	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);

	c.Margin(2.0f, &c);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, UI()->HotItem() == pID ? 1.0f : 0.6f);
	if(Checked)
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_ACTIVE);
	else
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_INACTIVE);
	IGraphics::CQuadItem QuadItem(c.x, c.y, c.w, c.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	t.y += 2.0f; // lame fix
	UI()->DoLabel(&c, pBoxText, pRect->h*ms_FontmodHeight*0.6f, 0);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight*0.8f, -1);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int CMenus::DoButtonCheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButtonCheckBoxCommon(pID, pText, "", pRect, Checked);
}

int CMenus::DoButtonCheckBoxNumber(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButtonCheckBoxCommon(pID, pText, aBuf, pRect);
}
