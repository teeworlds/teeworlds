/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_UI_H
#define ENGINE_UI_H
#include "kernel.h"

class IUI : public IInterface
{
	MACRO_INTERFACE("ui", 0)

public:
	virtual void OnInit() = 0;
	virtual void OnReset() = 0;
};

extern IUI *CreateUI();
#endif
