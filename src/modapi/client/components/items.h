#ifndef MODAPI_CLIENT_COMPONENTS_ITEMS_H
#define MODAPI_CLIENT_COMPONENTS_ITEMS_H
#include <game/client/component.h>
#include <modapi/graphics.h>

class CModAPI_Component_Items : public CComponent
{
private:
	int m_Layer;
	
private:
	void RenderModAPISprite(const CNetObj_ModAPI_Sprite *pPrev, const struct CNetObj_ModAPI_Sprite *pCurrent);
	void RenderModAPISpriteCharacter(const struct CNetObj_ModAPI_SpriteCharacter *pPrev, const struct CNetObj_ModAPI_SpriteCharacter *pCurrent);
	void RenderModAPIAnimatedSprite(const CNetObj_ModAPI_AnimatedSprite *pPrev, const struct CNetObj_ModAPI_AnimatedSprite *pCurrent);
	void RenderModAPIAnimatedSpriteCharacter(const CNetObj_ModAPI_AnimatedSpriteCharacter *pPrev, const struct CNetObj_ModAPI_AnimatedSpriteCharacter *pCurrent);
	
	void RenderModAPILine(const struct CNetObj_ModAPI_Line *pCurrent);
	
	void RenderModAPIText(const struct CNetObj_ModAPI_Text *pPrev, const struct CNetObj_ModAPI_Text *pCurrent);
	void RenderModAPITextCharacter(const struct CNetObj_ModAPI_TextCharacter *pPrev, const struct CNetObj_ModAPI_TextCharacter *pCurrent);
	void RenderModAPIAnimatedText(const struct CNetObj_ModAPI_AnimatedText *pPrev, const struct CNetObj_ModAPI_AnimatedText *pCurrent);
	void RenderModAPIAnimatedTextCharacter(const struct CNetObj_ModAPI_AnimatedTextCharacter *pPrev, const struct CNetObj_ModAPI_AnimatedTextCharacter *pCurrent);

public:
	void SetLayer(int Layer);
	int GetLayer() const;
	virtual void OnRender();
};

#endif
