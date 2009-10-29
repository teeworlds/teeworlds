

class IEngine
{
public:
	virtual ~IEngine() {}
	virtual class IGraphics *Graphics() = 0;
};


class IGameClient
{
public:
	virtual ~IGameClient() {}
};

extern IGameClient *CreateGameClient(IEngine *pEngine);
