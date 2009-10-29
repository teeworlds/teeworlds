
class IEditor
{
public:
	virtual ~IEditor() {}
	virtual void Init(class IGraphics *pGraphics) = 0;
	virtual void UpdateAndRender() = 0;
};

extern IEditor *CreateEditor();
