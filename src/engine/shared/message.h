/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_MESSAGE_H
#define ENGINE_SHARED_MESSAGE_H
class CMessage
{
public:
	virtual bool Pack(void *pData, unsigned MaxDataSize);
	virtual bool Unpack(const void *pData, unsigned DataSize);
};
#endif
