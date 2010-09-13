// copyright (c) 2010 magnus auvinen, see licence.txt for more info
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
