#ifndef ENGINE_EDITOR_H
#define ENGINE_EDITOR_H
#include "kernel.h"

class IEditor : public IInterface
{
	MACRO_INTERFACE("editor", 0)
public:

	virtual ~IEditor() {}
	virtual void Init() = 0;
	virtual void UpdateAndRender() = 0;
};

extern IEditor *CreateEditor();
#endif
