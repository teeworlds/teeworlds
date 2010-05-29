#ifndef ENGINE_MESSAGE_H
#define ENGINE_MESSAGE_H

#include <engine/shared/packer.h>

class CMsgPacker : public CPacker
{
public:
	CMsgPacker(int Type)
	{
		Reset();
		AddInt(Type);
	}
};

#endif
