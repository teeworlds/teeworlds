#include <game/client/component.hpp>

class MAPIMAGES : public COMPONENT
{	
	int textures[64];
	int count;
public:
	MAPIMAGES();
	
	int get(int index) const { return textures[index]; }
	int num() const { return count; }
	
	virtual void on_reset();
};

